#pragma code_seg("C270")
#pragma data_seg("D270")
#pragma bss_seg("B270")
#pragma const_seg("K270")
#pragma comment(linker, "/merge:D270=270")
#pragma comment(linker, "/merge:C270=270")
#pragma comment(linker, "/merge:B270=270")
#pragma comment(linker, "/merge:K270=270")
/***************************************************************************

	Atari Food Fight hardware

	driver by Aaron Giles

	Games supported:
		* Food Fight

	Known bugs:
		* none at this time

****************************************************************************

	Memory map

****************************************************************************

	========================================================================
	MAIN CPU
	========================================================================
	000000-00FFFF   R     xxxxxxxx xxxxxxxx   Program ROM
	014000-01BFFF   R/W   xxxxxxxx xxxxxxxx   Program RAM
	01C000-01CFFF   R/W   xxxxxxxx xxxxxxxx   Motion object RAM (1024 entries x 2 words)
	                R/W   x------- --------      (0: Horizontal flip)
	                R/W   -x------ --------      (0: Vertical flip)
	                R/W   ---xxxxx --------      (0: Palette select)
	                R/W   -------- xxxxxxxx      (0: Tile index)
	                R/W   xxxxxxxx --------      (1: X position)
	                R/W   -------- xxxxxxxx      (1: Y position)
	800000-8007FF   R/W   xxxxxxxx xxxxxxxx   Playfield RAM (32x32 tiles)
	                R/W   x------- --------      (Tile index MSB)
	                R/W   --xxxxxx --------      (Palette select)
	                R/W   -------- xxxxxxxx      (Tile index LSBs)
	900000-9001FF   R/W   -------- ----xxxx   NVRAM
	940000-940007   R     -------- xxxxxxxx   Analog input read
	944000-944007     W   -------- --------   Analog input select
	948000          R     -------- xxxxxxxx   Digital inputs
	                R     -------- x-------      (Self test)
	                R     -------- -x------      (Player 2 throw)
	                R     -------- --x-----      (Player 1 throw)
	                R     -------- ---x----      (Aux coin)
	                R     -------- ----x---      (2 player start)
	                R     -------- -----x--      (1 player start)
	                R     -------- ------x-      (Right coin)
	                R     -------- -------x      (Left coin)
	948000            W   -------- xxxxxxxx   Digital outputs
	                  W   -------- x-------      (Right coin counter)
	                  W   -------- -x------      (Left coin counter)
	                  W   -------- --x-----      (LED 2)
	                  W   -------- ---x----      (LED 1)
	                  W   -------- ----x---      (INT2 reset)
	                  W   -------- -----x--      (INT1 reset)
	                  W   -------- ------x-      (Update)
	                  W   -------- -------x      (Playfield flip)
	94C000            W   -------- --------   Unknown
	950000-9501FF     W   -------- xxxxxxxx   Palette RAM (256 entries)
	                  W   -------- xx------      (Blue)
	                  W   -------- --xxx---      (Green)
	                  W   -------- -----xxx      (Red)
	954000            W   -------- --------   NVRAM recall
	958000            W   -------- --------   Watchdog
	A40000-A4001F   R/W   -------- xxxxxxxx   POKEY 2
	A80000-A8001F   R/W   -------- xxxxxxxx   POKEY 1
	AC0000-AC001F   R/W   -------- xxxxxxxx   POKEY 3
	========================================================================
	Interrupts:
		IRQ1 = 32V
		IRQ2 = VBLANK
	========================================================================


***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"
#include "foodf.h"



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 whichport = 0;



/*************************************
 *
 *	NVRAM
 *
 *************************************/

static READ16_HANDLER( nvram_r )
{
	return ((data16_t *)generic_nvram)[offset] | 0xfff0;
}



/*************************************
 *
 *	Interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_scanline_int_state)
		newstate |= 1;
	if (atarigen_video_int_state)
		newstate |= 2;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void scanline_update(int scanline)
{
	/* INT 1 is on 32V */
	if (scanline & 32)
		atarigen_scanline_int_gen();
}


static MACHINE_INIT( foodf )
{
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(scanline_update, 32);
}



/*************************************
 *
 *	Digital outputs
 *
 *************************************/

static WRITE16_HANDLER( digital_w )
{
	if (ACCESSING_LSB)
	{
		foodf_set_flip(data & 0x01);

		if (!(data & 0x04))
			atarigen_scanline_int_ack_w(0,0,0);
		if (!(data & 0x08))
			atarigen_video_int_ack_w(0,0,0);

		coin_counter_w(0, (data >> 6) & 1);
		coin_counter_w(1, (data >> 7) & 1);
	}
}



/*************************************
 *
 *	Analog inputs
 *
 *************************************/

static READ16_HANDLER( analog_r )
{
	return readinputport(whichport);
}


static WRITE16_HANDLER( analog_w )
{
	whichport = offset ^ 3;
}



/*************************************
 *
 *	POKEY I/O
 *
 *************************************/

static READ16_HANDLER( pokey1_word_r ) { return pokey1_r(offset); }
static READ16_HANDLER( pokey2_word_r ) { return pokey2_r(offset); }
static READ16_HANDLER( pokey3_word_r ) { return pokey3_r(offset); }

static WRITE16_HANDLER( pokey1_word_w ) { if (ACCESSING_LSB) pokey1_w(offset, data & 0xff); }
static WRITE16_HANDLER( pokey2_word_w ) { if (ACCESSING_LSB) pokey2_w(offset, data & 0xff); }
static WRITE16_HANDLER( pokey3_word_w ) { if (ACCESSING_LSB) pokey3_w(offset, data & 0xff); }



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x00ffff, MRA16_ROM },
	{ 0x014000, 0x01cfff, MRA16_RAM },
	{ 0x800000, 0x8007ff, MRA16_RAM },
	{ 0x900000, 0x9001ff, nvram_r },
	{ 0x940000, 0x940007, analog_r },
	{ 0x948000, 0x948001, input_port_4_word_r },
	{ 0x94c000, 0x94c001, MRA16_NOP }, /* Used from PC 0x776E */
	{ 0x958000, 0x958001, watchdog_reset16_r },
	{ 0xa40000, 0xa4001f, pokey2_word_r },
	{ 0xa80000, 0xa8001f, pokey1_word_r },
	{ 0xac0000, 0xac001f, pokey3_word_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x00ffff, MWA16_ROM },
	{ 0x014000, 0x01bfff, MWA16_RAM },
	{ 0x01c000, 0x01cfff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x800000, 0x8007ff, foodf_playfieldram_w, &videoram16, &videoram_size },
	{ 0x900000, 0x9001ff, MWA16_RAM, (data16_t **)&generic_nvram, &generic_nvram_size },
	{ 0x944000, 0x944007, analog_w },
	{ 0x948000, 0x948001, digital_w },
	{ 0x950000, 0x9501ff, foodf_paletteram_w, &paletteram16 },
	{ 0x954000, 0x954001, MWA16_NOP },
	{ 0x958000, 0x958001, watchdog_reset16_w },
	{ 0xa40000, 0xa4001f, pokey2_word_w },
	{ 0xa80000, 0xa8001f, pokey1_word_w },
	{ 0xac0000, 0xac001f, pokey3_word_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( foodf )
	PORT_START	/* IN0 */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_PLAYER1 | IPF_REVERSE, 100, 10, 0, 255 )

	PORT_START	/* IN1 */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_PLAYER2 | IPF_REVERSE | IPF_COCKTAIL, 100, 10, 0, 255 )

	PORT_START	/* IN2 */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_Y | IPF_PLAYER1 | IPF_REVERSE, 100, 10, 0, 255 )

	PORT_START	/* IN3 */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_REVERSE | IPF_COCKTAIL, 100, 10, 0, 255 )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* SW1 */
	PORT_DIPNAME( 0x07, 0x00, "Bonus Coins" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x05, "1 for every 2" )
	PORT_DIPSETTING(    0x02, "1 for every 4" )
	PORT_DIPSETTING(    0x01, "1 for every 5" )
	PORT_DIPSETTING(    0x06, "2 for every 4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Coin_A ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_B ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ))
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_6C ))
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x40, DEF_STR( Free_Play ))
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*16
};


static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(1,2), 0 },
	{ 8*16+0, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*32
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 64 },	/* characters 8x8 */
	{ REGION_GFX2, 0, &spritelayout, 0, 64 },	/* sprites & playfield */
	{ -1 }
};



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static READ_HANDLER( pot_r )
{
	return (readinputport(5) >> offset) << 7;
}

static struct POKEYinterface pokey_interface =
{
	3,	/* 3 chips */
	600000,	/* .6 MHz */
	{ 33, 33, 33 },
	/* The 8 pot handlers */
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	/* The allpot handler */
	{ 0, 0, 0 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( foodf )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 6000000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(foodf)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(foodf)

	/* sound hardware */
	MDRV_SOUND_ADD(POKEY, pokey_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( foodf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for 68000 code */
	ROM_LOAD16_BYTE( "foodf.9c",     0x00000, 0x02000, CRC(ef92dc5c) SHA1(eb41291615165f549a68ebc6d4664edef1a04ac5) )
	ROM_LOAD16_BYTE( "foodf.8c",     0x00001, 0x02000, CRC(dfc3d5a8) SHA1(7abe5e9c27098bd8c93cc06f1b9e3db0744019e9) )
	ROM_LOAD16_BYTE( "foodf.9d",     0x04000, 0x02000, CRC(ea596480) SHA1(752aa33a8e8045650dd32ec7c7026e00d7896e0f) )
	ROM_LOAD16_BYTE( "foodf.8d",     0x04001, 0x02000, CRC(64b93076) SHA1(efa4090d96aa0ffd4192a045f174ac5960810bca) )
	ROM_LOAD16_BYTE( "foodf.9e",     0x08000, 0x02000, CRC(95159a3e) SHA1(f180126671776f62242ec9fd4a82a581c551ffce) )
	ROM_LOAD16_BYTE( "foodf.8e",     0x08001, 0x02000, CRC(e6cff1b1) SHA1(7c7ad2dcdff60fc092e8a825c5a6de6b506523de) )
	ROM_LOAD16_BYTE( "foodf.9f",     0x0c000, 0x02000, CRC(608690c9) SHA1(419020c69ce6fded0d9af44ead8ec4727468d58b) )
	ROM_LOAD16_BYTE( "foodf.8f",     0x0c001, 0x02000, CRC(17828dbb) SHA1(9d8e29a5e56a8a9c5db8561e4c20ff22f69b46ca) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "foodf.6lm",    0x0000, 0x2000, CRC(c13c90eb) SHA1(ebd2bbbdd7e184851d1ab4b5648481d966c78cc2) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "foodf.4d",     0x0000, 0x2000, CRC(8870e3d6) SHA1(702007d3d543f872b5bf5d00b49f6e05b46d6600) )
	ROM_LOAD( "foodf.4e",     0x2000, 0x2000, CRC(84372edf) SHA1(9beef3ff3b28405c45d691adfbc233921073be47) )
ROM_END



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1982, foodf, 0, foodf, foodf, 0, ROT0, "Atari", "Food Fight" )
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
