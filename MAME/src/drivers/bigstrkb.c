#pragma code_seg("C61")
#pragma bss_seg("B61")
#pragma data_seg("D61")
#pragma const_seg("K61")
/* Jaleco's Big Striker (bootleg version)

 Driver by David Haywood
 Inputs by Stephh

 maybe it could be merged with megasys1.c, could be messy

 todo:

 complete sound (YM2151 like megasys1?)
 sprite lag (buffers spriteram?)

*/

#include "driver.h"

/*

68k interrupts
lev 1 : 0x64 : 0000 0406 - ?
lev 2 : 0x68 : 0000 0434 - ?
lev 3 : 0x6c : 0000 05be - xxx
lev 4 : 0x70 : 0000 04d2 - ?
lev 5 : 0x74 : 0000 05be - xxx
lev 6 : 0x78 : 0004 0000 - vblank?
lev 7 : 0x7c : 0000 05be - xxx

*/

data16_t *bsb_videoram, *bsb_videoram2, *bsb_videoram3;
data16_t *bsb_vidreg1, *bsb_vidreg2;
data16_t *bigstrkb_spriteram;

WRITE16_HANDLER( bsb_videoram_w );
WRITE16_HANDLER( bsb_videoram2_w );
WRITE16_HANDLER( bsb_videoram3_w );
VIDEO_START(bigstrkb);
VIDEO_UPDATE(bigstrkb);


/* Memory Maps */

/* some regions might be too large */

static MEMORY_READ16_START( bigstrkb_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	/* most region sizes are unknown */
//	{ 0x0c0000, 0x0cffff, megasys1_vregs_C_r },
	{ 0x0D0000, 0x0dffff, MRA16_RAM },
	{ 0x0E0000, 0x0E3fff, MRA16_RAM },
	{ 0x0e8000, 0x0ebfff, MRA16_RAM },
	{ 0x0ec000, 0x0effff, MRA16_RAM },
	{ 0x0f0000, 0x0fffff, MRA16_RAM },
	{ 0x1f0000, 0x1fffff, MRA16_RAM },
	{ 0x700000, 0x700001, input_port_0_word_r }, /* Dip 1 */
	{ 0x700002, 0x700003, input_port_1_word_r }, /* Dip 2 */
	{ 0x700004, 0x700005, input_port_2_word_r }, /* System */
	{ 0x70000a, 0x70000b, input_port_4_word_r }, /* Player 1 */
	{ 0x70000c, 0x70000d, input_port_3_word_r }, /* Player 2 */

	{ 0xE00000, 0xE00001, OKIM6295_status_0_lsb_r },
	{ 0xE00002, 0xE00003, OKIM6295_status_1_lsb_r },

	{ 0xF00000, 0xFFFFFF, MRA16_RAM },	// main RAM
MEMORY_END


static MEMORY_WRITE16_START( bigstrkb_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
//	{ 0x0c0000, 0x0cffff, megasys1_vregs_C_w, &megasys1_vregs },

	{ 0x0C2004, 0x0C2005, MWA16_NOP },
	{ 0x0C200C, 0x0C200d, MWA16_NOP },
	{ 0x0C2104, 0x0C2105, MWA16_NOP },
	{ 0x0C2108, 0x0C2109, MWA16_NOP },
	{ 0x0C2200, 0x0C2201, MWA16_NOP },
	{ 0x0C2208, 0x0C2209, MWA16_NOP },
	{ 0x0c2308, 0x0c2309, MWA16_NOP },	// bit 0 of DSW1 (flip screen) - use vregs

	{ 0x0D0000, 0x0dffff, MWA16_RAM },  // 0xd2000 - 0xd3fff?   0xd8000?

	{ 0x0e0000, 0x0e3fff, bsb_videoram2_w, &bsb_videoram2 },
	{ 0x0e8000, 0x0ebfff, bsb_videoram3_w, &bsb_videoram3 },
	{ 0x0ec000, 0x0effff, bsb_videoram_w, &bsb_videoram },

	{ 0x0f0000, 0x0f7fff, MWA16_RAM },
	{ 0x0f8000, 0x0f87ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x0f8800, 0x0fffff, MWA16_RAM },

	{ 0x1f0000, 0x1f7fff, MWA16_RAM },
	{ 0x1f8000, 0x1f87ff, MWA16_RAM, &bigstrkb_spriteram },
	{ 0x1f8800, 0x1fffff, MWA16_RAM },

	{ 0x700020, 0x700027, MWA16_RAM, &bsb_vidreg1 },
	{ 0x700030, 0x700037, MWA16_RAM, &bsb_vidreg2 },

	{ 0xB00000, 0xB00001, MWA16_NOP },

	{ 0xE00000, 0xE00001, OKIM6295_data_0_lsb_w },
	{ 0xE00002, 0xE00003, OKIM6295_data_1_lsb_w },

	{ 0xE00008, 0xE00009, MWA16_NOP },
	{ 0xE0000c, 0xE0000d, MWA16_NOP },

	{ 0xF00000, 0xFFFFFF, MWA16_RAM },
MEMORY_END

#define BIGSTRKB_PLAYER_INPUT( player, start ) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | player ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | player ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | player ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | player ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | player ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | player ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | player ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, start )

INPUT_PORTS_START( bigstrkb )
	PORT_START	/* DSW0 (0x700000.w) */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )	// also set "Coin B" to "Free Play"
	/* 0x01 to 0x05 gives 2C_3C */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )	// also set "Coin A" to "Free Play"
	/* 0x10 to 0x50 gives 2C_3C */

	PORT_START	/* DSW1 (0x700002.w) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )	// Check code at 0x00097c (flip screen)
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x06, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x18, 0x18, "Timer Speed" )
	PORT_DIPSETTING(    0x08, "Slow" )				// 65
	PORT_DIPSETTING(    0x18, "Normal" )			// 50
	PORT_DIPSETTING(    0x10, "Fast" )				// 35
	PORT_DIPSETTING(    0x00, "Fastest" )			// 25
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Players Game" )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x40, "2 Credits" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )		// Check code at 0x000c50 (test mode ?)
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* System inputs (0x700004.w) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Player 1 controls (0x70000c.w) */
	BIGSTRKB_PLAYER_INPUT( IPF_PLAYER1, IPT_START1 )

	PORT_START	/* Player 2 controls (0x70000a.w) */
	BIGSTRKB_PLAYER_INPUT( IPF_PLAYER2, IPT_START2 )
INPUT_PORTS_END

/* GFX Decode */

static struct GfxLayout bigstrkb_charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout bigstrkb_char16layout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8, 9,10,11,12,13,14,15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	16*16
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &bigstrkb_charlayout,   0x200, 16 },
	{ REGION_GFX2, 0, &bigstrkb_char16layout,   0, 32 },
	{ REGION_GFX3, 0, &bigstrkb_char16layout,   0x300, 16 },
	{ -1 } /* end of array */
};


/* Machine Driver */

static struct OKIM6295interface okim6295_interface =
{
	2,
	{ 4000000/132, 4000000/132 },
	{ REGION_SOUND1, REGION_SOUND2 },
	{ MIXER(30,MIXER_PAN_LEFT), MIXER(30,MIXER_PAN_RIGHT) }
};

static MACHINE_DRIVER_START( bigstrkb )
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(bigstrkb_readmem,bigstrkb_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)


	MDRV_PALETTE_LENGTH(0x400)

	MDRV_VIDEO_START(bigstrkb)
	MDRV_VIDEO_UPDATE(bigstrkb)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
//	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/* Rom Loading */

ROM_START( bigstrkb )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "footgaa.015", 0x00001, 0x40000, 0x33b1d7f3 )
	ROM_LOAD16_BYTE( "footgaa.016", 0x00000, 0x40000, 0x1c6b8709 )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE  ) /* 8x8x4 FG Tiles */
	ROM_LOAD( "footgaa.005", 0x00000, 0x10000, 0xd97c9bfe ) // FIRST AND SECOND HALF IDENTICAL
	ROM_LOAD( "footgaa.006", 0x10000, 0x10000, 0x1ae56e8b ) // FIRST AND SECOND HALF IDENTICAL
	ROM_LOAD( "footgaa.007", 0x20000, 0x10000, 0xa45fa6b6 ) // FIRST AND SECOND HALF IDENTICAL
	ROM_LOAD( "footgaa.008", 0x30000, 0x10000, 0x2700888c )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_INVERT | ROMREGION_DISPOSE  ) /* 16x16x4 BG Tiles */
	ROM_LOAD( "footgaa.001", 0x000000, 0x80000, 0x0e440841 ) // x1xxxxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD( "footgaa.002", 0x080000, 0x80000, 0x92a15164 ) // x1xxxxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD( "footgaa.003", 0x100000, 0x80000, 0xda127b89 ) // x1xxxxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD( "footgaa.004", 0x180000, 0x80000, 0x3e6b0d92 ) // x1xxxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_INVERT | ROMREGION_DISPOSE ) /* 16x16x4 Sprites */
	ROM_LOAD( "footgaa.011", 0x000000, 0x20000, 0xc3924fea )
	ROM_LOAD( "footgaa.012", 0x020000, 0x20000, 0xa581e9d7 )
	ROM_LOAD( "footgaa.013", 0x040000, 0x20000, 0x26ce4b7f )
	ROM_LOAD( "footgaa.014", 0x060000, 0x20000, 0xc3cfc500 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* Samples? */
	ROM_LOAD( "footgaa.010", 0x00000, 0x40000, 0x53014576 )

	ROM_REGION( 0x40000, REGION_SOUND2, 0 ) /* Samples? */
	ROM_LOAD( "footgaa.009", 0x00000, 0x40000, 0x19bf0896 )
ROM_END

/* GAME drivers */

GAMEX( 1992, bigstrkb, bigstrik, bigstrkb, bigstrkb, 0, ROT0, "bootleg", "Big Striker (bootleg)", GAME_IMPERFECT_SOUND | GAME_NO_COCKTAIL )
#pragma data_seg()
#pragma code_seg()
#pragma bss_seg()
#pragma const_seg()
