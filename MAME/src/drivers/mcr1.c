#pragma code_seg("C305")
#pragma bss_seg("B305")
#pragma data_seg("D305")
#pragma const_seg("K305")
/***************************************************************************

	Midway MCR-1 system

    driver by Christopher Kirmse, Aaron Giles

	Games supported:
		* Solar Fox
		* Kick

	Known bugs:
		* none at this time

****************************************************************************

	Memory map

****************************************************************************

	========================================================================
	CPU #1
	========================================================================
	0000-6FFF   R     xxxxxxxx    Program ROM
	7000-77FF   R/W   xxxxxxxx    NVRAM
	F000-F1FF   R/W   xxxxxxxx    Sprite RAM
	F400-F41F     W   xxxxxxxx    Palette RAM blue/green
	F800-F81F     W   xxxxxxxx    Palette RAM red
	FC00-FFFF   R/W   xxxxxxxx    Background video RAM
	========================================================================
	0000        R     x-xxxxxx    Input ports
	            R     x-------    Service switch (active low)
	            R     --x-----    Tilt
	            R     ---xxx--    External inputs
	            R     ------x-    Right coin
	            R     -------x    Left coin
	0000        W     xxxxxxxx    Data latch OP0 (coin meters, 2 led's and cocktail 'flip')
	0001        R     xxxxxxxx    External inputs
	0002        R     xxxxxxxx    External inputs
	0003        R     xxxxxxxx    DIP switches
	0004        R     xxxxxxxx    External inputs
	0004        W     xxxxxxxx    Data latch OP4 (comm. with external hardware)
	0007        R     xxxxxxxx    Audio status
	001C-001F   W     xxxxxxxx    Audio latches 1-4
	00E0        W     --------    Watchdog reset
	00E8        W     xxxxxxxx    Unknown (written at initialization time)
	00F0-00F3   W     xxxxxxxx    CTC communications
	========================================================================
	Interrupts:
		NMI ???
		INT generated by CTC
	========================================================================


	========================================================================
	CPU #2 (Super Sound I/O)
	========================================================================
	0000-3FFF   R     xxxxxxxx    Program ROM
	8000-83FF   R/W   xxxxxxxx    Program RAM
	9000-9003   R     xxxxxxxx    Audio latches 1-4
	A000          W   xxxxxxxx    AY-8910 #1 control
	A001        R     xxxxxxxx    AY-8910 #1 status
	A002          W   xxxxxxxx    AY-8910 #1 data
	B000          W   xxxxxxxx    AY-8910 #2 control
	B001        R     xxxxxxxx    AY-8910 #2 status
	B002          W   xxxxxxxx    AY-8910 #2 data
	C000          W   xxxxxxxx    Audio status
	E000          W   xxxxxxxx    Unknown
	F000        R     xxxxxxxx    Audio board switches
	========================================================================
	Interrupts:
		NMI ???
		INT generated by external circuitry 780 times/second
	========================================================================

***************************************************************************/


#include "driver.h"
#include "machine/z80fmly.h"
#include "sndhrdw/mcr.h"
#include "vidhrdw/generic.h"
#include "mcr.h"


static const UINT8 *nvram_init;


/*************************************
 *
 *	Kick input ports
 *
 *************************************/

static READ_HANDLER( kick_dial_r )
{
	return (readinputport(1) & 0x0f) | ((readinputport(6) << 4) & 0xf0);
}



/*************************************
 *
 *	Solar Fox input ports
 *
 *************************************/

static READ_HANDLER( solarfox_input_0_r )
{
	/* This is a kludge; according to the wiring diagram, the player 2 */
	/* controls are hooked up as documented below. If you go into test */
	/* mode, they will respond. However, if you try it in a 2-player   */
	/* game in cocktail mode, they don't work at all. So we fake-mux   */
	/* the controls through player 1's ports */
	if (mcr_cocktail_flip)
		return readinputport(0) | 0x08;
	else
		return ((readinputport(0) & ~0x14) | 0x08) | ((readinputport(0) & 0x08) >> 1) | ((readinputport(2) & 0x01) << 4);
}


static READ_HANDLER( solarfox_input_1_r )
{
	/*  same deal as above */
	if (mcr_cocktail_flip)
		return readinputport(1) | 0xf0;
	else
		return (readinputport(1) >> 4) | 0xf0;
}



/*************************************
 *
 *	NVRAM save/load
 *
 *************************************/

static NVRAM_HANDLER( mcr1 )
{
	unsigned char *ram = memory_region(REGION_CPU1);

	if (read_or_write)
		mame_fwrite(file, &ram[0x7000], 0x800);
	else if (file)
		mame_fread(file, &ram[0x7000], 0x800);
	else if (nvram_init)
		memcpy(&ram[0x7000], nvram_init, 16);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0x7000, 0x77ff, MRA_RAM },
	{ 0xf000, 0xf1ff, MRA_RAM },
	{ 0xfc00, 0xffff, MRA_RAM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x6fff, MWA_ROM },
	{ 0x7000, 0x77ff, MWA_RAM },
	{ 0xf000, 0xf1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf400, 0xf41f, paletteram_xxxxRRRRBBBBGGGG_split1_w, &paletteram },
	{ 0xf800, 0xf81f, paletteram_xxxxRRRRBBBBGGGG_split2_w, &paletteram_2 },
	{ 0xfc00, 0xffff, mcr1_videoram_w, &videoram, &videoram_size },
MEMORY_END


static PORT_READ_START( readport )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ 0x04, 0x04, input_port_4_r },
	{ 0x07, 0x07, ssio_status_r },
	{ 0x10, 0x10, input_port_0_r },
	{ 0xf0, 0xf3, z80ctc_0_r },
PORT_END


static PORT_WRITE_START( writeport )
	{ 0x1c, 0x1f, ssio_data_w },
	{ 0xe0, 0xe0, watchdog_reset_w },
	{ 0xe8, 0xe8, MWA_NOP },
	{ 0xf0, 0xf3, z80ctc_0_w },
PORT_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( solarfox )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x03, 0x03, "Bonus" )
	PORT_DIPSETTING(    0x02, "None" )
	PORT_DIPSETTING(    0x03, "After 10 racks" )
	PORT_DIPSETTING(    0x01, "After 20 racks" )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x40, 0x40, "Ignore Hardware Failure" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ))
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ))
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ))

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( kick )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 -- this is the Kick spinner input.  */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 3, 50, 0, 0 )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x01, 0x00, "Music" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (fake) player 2 dial */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( kicka )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 -- this is the Kick spinner input.  */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 3, 50, 0, 0 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x01, 0x00, "Music" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_BIT( 0x3e, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ))
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ))
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (fake) player 2 dial */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_COCKTAIL, 3, 50, 0, 0 )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &mcr_bg_layout,     0, 1 },	/* colors 0-15 */
	{ REGION_GFX2, 0, &mcr_sprite_layout, 16, 1 },	/* colors 16-31 */
	{ -1 } /* end of array */
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( mcr1 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, MAIN_OSC_MCR_I/8)
	MDRV_CPU_CONFIG(mcr_daisy_chain)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(mcr_interrupt,2)

	MDRV_FRAMES_PER_SECOND(30)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_30HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(mcr)
	MDRV_NVRAM_HANDLER(mcr1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 30*16)
	MDRV_VISIBLE_AREA(0*16, 32*16-1, 0*16, 30*16-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	
	MDRV_VIDEO_START(mcr1)
	MDRV_VIDEO_UPDATE(mcr1)

	/* sound hardware */
	MDRV_IMPORT_FROM(mcr_ssio)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( solarfox )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sfcpu.3b",     0x0000, 0x1000, 0x8c40f6eb )
	ROM_LOAD( "sfcpu.4b",     0x1000, 0x1000, 0x4d47bd7e )
	ROM_LOAD( "sfcpu.5b",     0x2000, 0x1000, 0xb52c3bd5 )
	ROM_LOAD( "sfcpu.4d",     0x3000, 0x1000, 0xbd5d25ba )
	ROM_LOAD( "sfcpu.5d",     0x4000, 0x1000, 0xdd57d817 )
	ROM_LOAD( "sfcpu.6d",     0x5000, 0x1000, 0xbd993cd9 )
	ROM_LOAD( "sfcpu.7d",     0x6000, 0x1000, 0x8ad8731d )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "sfsnd.7a",     0x0000, 0x1000, 0xcdecf83a )
	ROM_LOAD( "sfsnd.8a",     0x1000, 0x1000, 0xcb7788cb )
	ROM_LOAD( "sfsnd.9a",     0x2000, 0x1000, 0x304896ce )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sfcpu.4g",     0x0000, 0x1000, 0xba019a60 )
	ROM_LOAD( "sfcpu.5g",     0x1000, 0x1000, 0x7ff0364e )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sfvid.1a",     0x0000, 0x2000, 0x9d9b5d7e )
	ROM_LOAD( "sfvid.1b",     0x2000, 0x2000, 0x78801e83 )
	ROM_LOAD( "sfvid.1d",     0x4000, 0x2000, 0x4d8445cf )
	ROM_LOAD( "sfvid.1e",     0x6000, 0x2000, 0x3da25495 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.12d",   0x0000, 0x0020, 0xe1281ee9 )	/* from shollow, assuming it's the same */
ROM_END


ROM_START( kick )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1200a-v2.b3",  0x0000, 0x1000, 0x65924917 )
	ROM_LOAD( "1300b-v2.b4",  0x1000, 0x1000, 0x27929f52 )
	ROM_LOAD( "1400c-v2.b5",  0x2000, 0x1000, 0x69107ce6 )
	ROM_LOAD( "1500d-v2.d4",  0x3000, 0x1000, 0x04a23aa1 )
	ROM_LOAD( "1600e-v2.d5",  0x4000, 0x1000, 0x1d2834c0 )
	ROM_LOAD( "1700f-v2.d6",  0x5000, 0x1000, 0xddf84ce1 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "4200-a.a7",    0x0000, 0x1000, 0x9e35c02e )
	ROM_LOAD( "4300-b.a8",    0x1000, 0x1000, 0xca2b7c28 )
	ROM_LOAD( "4400-c.a9",    0x2000, 0x1000, 0xd1901551 )
	ROM_LOAD( "4500-d.a10",   0x3000, 0x1000, 0xd36ddcdc )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1800g-v2.g4",  0x0000, 0x1000, 0xb4d120f3 )
	ROM_LOAD( "1900h-v2.g5",  0x1000, 0x1000, 0xc3ba4893 )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2600a-v2.1e",  0x0000, 0x2000, 0x2c5d6b55 )
	ROM_LOAD( "2700b-v2.1d",  0x2000, 0x2000, 0x565ea97d )
	ROM_LOAD( "2800c-v2.1b",  0x4000, 0x2000, 0xf3be56a1 )
	ROM_LOAD( "2900d-v2.1a",  0x6000, 0x2000, 0x77da795e )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.12d",   0x0000, 0x0020, 0xe1281ee9 )	/* from shollow, assuming it's the same */
ROM_END


ROM_START( kicka )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1200-a.b3",    0x0000, 0x1000, 0x22fa42ed )
	ROM_LOAD( "1300-b.b4",    0x1000, 0x1000, 0xafaca819 )
	ROM_LOAD( "1400-c.b5",    0x2000, 0x1000, 0x6054ee56 )
	ROM_LOAD( "1500-d.d4",    0x3000, 0x1000, 0x263af0f3 )
	ROM_LOAD( "1600-e.d5",    0x4000, 0x1000, 0xeaaa78a7 )
	ROM_LOAD( "1700-f.d6",    0x5000, 0x1000, 0xc06c880f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "4200-a.a7",    0x0000, 0x1000, 0x9e35c02e )
	ROM_LOAD( "4300-b.a8",    0x1000, 0x1000, 0xca2b7c28 )
	ROM_LOAD( "4400-c.a9",    0x2000, 0x1000, 0xd1901551 )
	ROM_LOAD( "4500-d.a10",   0x3000, 0x1000, 0xd36ddcdc )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1000-g.g4",    0x0000, 0x1000, 0xacdae4f6 )
	ROM_LOAD( "1100-h.g5",    0x1000, 0x1000, 0xdbb18c96 )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2600-a.1e",    0x0000, 0x2000, 0x74b409d7 )
	ROM_LOAD( "2700-b.1d",    0x2000, 0x2000, 0x78eda36c )
	ROM_LOAD( "2800-c.1b",    0x4000, 0x2000, 0xc93e0170 )
	ROM_LOAD( "2900-d.1a",    0x6000, 0x2000, 0x91e59383 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.12d",   0x0000, 0x0020, 0xe1281ee9 )	/* from shollow, assuming it's the same */
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( solarfox )
{
	static const UINT8 hiscore_init[] = { 0,0,1,1,1,1,1,3,3,3,7 };
	nvram_init = hiscore_init;

	MCR_CONFIGURE_SOUND(MCR_SSIO);
	install_port_read_handler(0, 0x00, 0x00, solarfox_input_0_r);
	install_port_read_handler(0, 0x01, 0x01, solarfox_input_1_r);
	install_port_write_handler(0, 0x01, 0x01, mcr_control_port_w);

	mcr12_sprite_xoffs = 16;
	mcr12_sprite_xoffs_flip = 0;
}


static DRIVER_INIT( kick )
{
	nvram_init = NULL;

	MCR_CONFIGURE_SOUND(MCR_SSIO);
	install_port_read_handler(0, 0x01, 0x01, kick_dial_r);
	install_port_write_handler(0, 0x03, 0x03, mcr_control_port_w);

	mcr12_sprite_xoffs = 0;
	mcr12_sprite_xoffs_flip = 16;
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1981, solarfox, 0,    mcr1, solarfox, solarfox, ROT90 ^ ORIENTATION_FLIP_Y, "Bally Midway", "Solar Fox" )
GAME( 1981, kick,     0,    mcr1, kick,     kick,     ORIENTATION_SWAP_XY,        "Midway", "Kick (upright)" )
GAME( 1981, kicka,    kick, mcr1, kicka,    kick,     ROT90,                      "Midway", "Kick (cocktail)" )
#pragma data_seg()
#pragma code_seg()
#pragma bss_seg()
#pragma const_seg()
