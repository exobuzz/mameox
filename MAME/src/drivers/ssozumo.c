#pragma code_seg("C501")
#pragma bss_seg("B501")
#pragma data_seg("D501")
#pragma const_seg("K501")
/***************************************************************************

Syusse Oozumou
(c) 1984 Technos Japan (Licensed by Data East)

Driver by Takahiro Nogi (nogi@kt.rim.or.jp) 1999/10/04

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "cpu/m6809/m6809.h"

extern unsigned char *ssozumo_videoram2, *ssozumo_colorram2;
extern size_t ssozumo_videoram2_size;
extern unsigned char *ssozumo_scroll;

WRITE_HANDLER( ssozumo_paletteram_w );
PALETTE_INIT( ssozumo );
VIDEO_UPDATE( ssozumo );
VIDEO_START( ssozumo );


static INTERRUPT_GEN( ssozumo_interrupt )
{
	static int coin;

	if ((readinputport(0) & 0xc0) != 0xc0)
	{
		if (coin == 0)
		{
			coin = 1;
			nmi_line_pulse();
			return;
		}
	}
	else coin = 0;

	irq0_line_hold();
}


WRITE_HANDLER( ssozumo_sh_command_w )
{
	soundlatch_w(offset, data);
	cpu_set_irq_line(1, M6502_IRQ_LINE, HOLD_LINE);
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x077f, MRA_RAM },

	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x3000, 0x31ff, MRA_RAM },

	{ 0x4000, 0x4000, input_port_0_r },
	{ 0x4010, 0x4010, input_port_1_r },
	{ 0x4020, 0x4020, input_port_2_r },
	{ 0x4030, 0x4030, input_port_3_r },

	{ 0x6000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x077f, MWA_RAM },

	{ 0x0780, 0x07ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x2000, 0x23ff, MWA_RAM, &ssozumo_videoram2, &ssozumo_videoram2_size },
	{ 0x2400, 0x27ff, MWA_RAM, &ssozumo_colorram2 },
	{ 0x3000, 0x31ff, videoram_w, &videoram, &videoram_size },
	{ 0x3200, 0x33ff, colorram_w, &colorram },
	{ 0x3400, 0x35ff, MWA_RAM },
	{ 0x3600, 0x37ff, MWA_RAM },

	{ 0x4000, 0x4000, MWA_RAM },			// fg page select?
	{ 0x4010, 0x4010, ssozumo_sh_command_w },
	{ 0x4020, 0x4020, MWA_RAM, &ssozumo_scroll },
//	{ 0x4030, 0x4030, MWA_RAM },
	{ 0x4050, 0x407f, ssozumo_paletteram_w, &paletteram },

	{ 0x6000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x01ff, MRA_RAM },
	{ 0x2007, 0x2007, soundlatch_r },
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x2000, 0x2000, AY8910_write_port_0_w },
	{ 0x2001, 0x2001, AY8910_control_port_0_w },
	{ 0x2002, 0x2002, AY8910_write_port_1_w },
	{ 0x2003, 0x2003, AY8910_control_port_1_w },
	{ 0x2004, 0x2004, DAC_0_signed_data_w },
	{ 0x2005, 0x2005, interrupt_enable_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END


INPUT_PORTS_START( ssozumo )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#if 0
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#endif
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,		/* 8*8 characters */
	1024,		/* 1024 characters */
	3,		/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every char takes 8 consecutive bytes */
};


static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 tiles */
	256,	/* 256 tiles */
	3,		/* 3 bits per pixel */
	{ 2*256*16*16, 256*16*16, 0 },	/* the bitplanes are separated */
	{ 16*8 + 0, 16*8 + 1, 16*8 + 2, 16*8 + 3, 16*8 + 4, 16*8 + 5, 16*8 + 6, 16*8 + 7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8		/* every tile takes 16 consecutive bytes */
};


static struct GfxLayout spritelayout =
{
	16,16,		/* 16*16 sprites */
	1280,		/* 1280 sprites */
	3,		/* 3 bits per pixel */
	{ 2*1280*16*16, 1280*16*16, 0 },	/* the bitplanes are separated */
	{ 16*8 + 0, 16*8 + 1, 16*8 + 2, 16*8 + 3, 16*8 + 4, 16*8 + 5, 16*8 + 6, 16*8 + 7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8		/* every sprite takes 16 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 4 },
	{ REGION_GFX2, 0, &tilelayout,   4*8, 4 },
	{ REGION_GFX3, 0, &spritelayout, 8*8, 2 },
	{ -1 }		/* end of array */
};


static struct AY8910interface ay8910_interface =
{
	2,		/* 2 chips */
	1500000,	/* 1.5 MHz?????? */
	{ 30, 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static struct DACinterface dac_interface =
{
	1,
	{ 30 }
};


static MACHINE_DRIVER_START( ssozumo )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 1200000)	/* 1.2 MHz ???? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(ssozumo_interrupt,1)

	MDRV_CPU_ADD(M6502, 975000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) 		/* 975 kHz ?? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,16)	/* IRQs are triggered by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8 - 1, 1*8, 31*8 - 1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64 + 16)
	MDRV_COLORTABLE_LENGTH(64 + 16)

	MDRV_PALETTE_INIT(ssozumo)
	MDRV_VIDEO_START(ssozumo)
	MDRV_VIDEO_UPDATE(ssozumo)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



ROM_START( ssozumo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	/* Main Program ROMs */
	ROM_LOAD( "ic61.g01",	0x06000, 0x2000, 0x86968f46 )	// m1
	ROM_LOAD( "ic60.g11",	0x08000, 0x2000, 0x1a5143dd )	// m2
	ROM_LOAD( "ic59.g21",	0x0a000, 0x2000, 0xd3df04d7 )	// m3
	ROM_LOAD( "ic58.g31",	0x0c000, 0x2000, 0x0ee43a78 )	// m4
	ROM_LOAD( "ic57.g41",	0x0e000, 0x2000, 0xac77aa4c )	// m5

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	/* Sound Program & Voice Sample ROMs*/
	ROM_LOAD( "ic47.g50",	0x04000, 0x2000, 0xb64ec829 )	// a1
	ROM_LOAD( "ic46.g60",	0x06000, 0x2000, 0x630d7380 )	// a2
	ROM_LOAD( "ic45.g70",	0x08000, 0x2000, 0x1854b657 )	// a3
	ROM_LOAD( "ic44.g80",	0x0a000, 0x2000, 0x40b9a0da )	// a4
	ROM_LOAD( "ic43.g90",	0x0c000, 0x2000, 0x20262064 )	// a5
	ROM_LOAD( "ic42.ga0",	0x0e000, 0x2000, 0x98d7e998 )	// a6

	ROM_REGION( 0x06000, REGION_GFX1, ROMREGION_DISPOSE )
	/* Character ROMs */
	ROM_LOAD( "ic22.gq0",	0x00000, 0x2000, 0xb4c7e612 )	// c1
	ROM_LOAD( "ic23.gr0",	0x02000, 0x2000, 0x90bb9fda )	// c2
	ROM_LOAD( "ic21.gs0",	0x04000, 0x2000, 0xd8cd5c78 )	// c3

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	/* tile set ROMs */
	ROM_LOAD( "ic69.gt0",	0x00000, 0x2000, 0x771116ca )	// t1
	ROM_LOAD( "ic59.gu0",	0x02000, 0x2000, 0x68035bfd )	// t2
	ROM_LOAD( "ic81.gv0",	0x04000, 0x2000, 0xcdda1f9f )	// t3

	ROM_REGION( 0x1e000, REGION_GFX3, ROMREGION_DISPOSE )
	/* sprites ROMs */
	ROM_LOAD( "ic06.gg0",	0x00000, 0x2000, 0xd2342c50 )	// s1a
	ROM_LOAD( "ic05.gh0",	0x02000, 0x2000, 0x14a3cb10 )	// s1b
	ROM_LOAD( "ic04.gi0",	0x04000, 0x2000, 0x169276c1 )	// s1c
	ROM_LOAD( "ic03.gj0",	0x06000, 0x2000, 0xe71b9f28 )	// s1d
	ROM_LOAD( "ic02.gk0",	0x08000, 0x2000, 0x6e94773c )	// s1e
	ROM_LOAD( "ic29.gl0",	0x0a000, 0x2000, 0x40f67cc4 )	// s2a
	ROM_LOAD( "ic28.gm0",	0x0c000, 0x2000, 0x8c97b1a2 )	// s2b
	ROM_LOAD( "ic27.gn0",	0x0e000, 0x2000, 0xbe8bb3dd )	// s2c
	ROM_LOAD( "ic26.go0",	0x10000, 0x2000, 0x9c098a2c )	// s2d
	ROM_LOAD( "ic25.gp0",	0x12000, 0x2000, 0xf73f8a76 )	// s2e
	ROM_LOAD( "ic44.gb0",	0x14000, 0x2000, 0xcdd7f2eb )	// s3a
	ROM_LOAD( "ic43.gc0",	0x16000, 0x2000, 0x7b4c632e )	// s3b
	ROM_LOAD( "ic42.gd0",	0x18000, 0x2000, 0xcd1c8fe6 )	// s3c
	ROM_LOAD( "ic41.ge0",	0x1a000, 0x2000, 0x935578d0 )	// s3d
	ROM_LOAD( "ic40.gf0",	0x1c000, 0x2000, 0x5a3bf1ba )	// s3e

	ROM_REGION( 0x0080, REGION_PROMS, 0 )
	ROM_LOAD( "ic33.gz0",	0x00000, 0x0020, 0x523d29ad )	/* char palette red and green components */
	ROM_LOAD( "ic30.gz2",	0x00020, 0x0020, 0x0de202e1 )	/* tile palette red and green components */
	ROM_LOAD( "ic32.gz1",	0x00040, 0x0020, 0x6fbff4d2 )	/* char palette blue component */
	ROM_LOAD( "ic31.gz3",	0x00060, 0x0020, 0x18e7fe63 )	/* tile palette blue component */
ROM_END



GAMEX( 1984, ssozumo, 0, ssozumo, ssozumo, 0, ROT270, "Technos", "Syusse Oozumou (Japan)", GAME_NO_COCKTAIL )
#pragma data_seg()
#pragma code_seg()
#pragma bss_seg()
#pragma const_seg()
