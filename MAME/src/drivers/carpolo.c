/***************************************************************************

	Exidy Car Polo hardware

	driver by Zsolt Vasvari

	Games supported:
		* Car Polo

	Known issues:
		* sound

     Original Bugs:
		* if you insert a coin too fast (before the GAME OVER sign appears),
		  the cars will chase *away* from the ball

****************************************************************************/

#include "driver.h"
#include "machine/6821pia.h"
#include "carpolo.h"



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x01ff, MRA_RAM },
	{ 0x5400, 0x5403, pia_0_r },
	{ 0x5800, 0x5803, pia_1_r },
	{ 0xa000, 0xa000, carpolo_ball_screen_collision_cause_r },
	{ 0xa001, 0xa001, carpolo_car_ball_collision_x_r },
	{ 0xa002, 0xa002, carpolo_car_ball_collision_y_r },
	{ 0xa003, 0xa003, carpolo_car_car_collision_cause_r },
	{ 0xa004, 0xa004, carpolo_car_border_collision_cause_r },
	{ 0xa005, 0xa005, carpolo_car_ball_collision_cause_r },
	{ 0xa006, 0xa006, carpolo_car_goal_collision_cause_r },
	{ 0xa007, 0xa007, input_port_1_r },
	{ 0xc000, 0xc000, carpolo_interrupt_cause_r },
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x3000, 0x30ff, MWA_RAM, &carpolo_alpharam },
	{ 0x4000, 0x400f, MWA_RAM, &carpolo_spriteram },
	{ 0x5400, 0x5403, pia_0_w },
	{ 0x5800, 0x5803, pia_1_w },
	{ 0xb000, 0xb000, carpolo_ball_screen_interrupt_clear_w },
	{ 0xb001, 0xb001, carpolo_timer_interrupt_clear_w },
	{ 0xb003, 0xb003, carpolo_car_car_interrupt_clear_w },
	{ 0xb004, 0xb004, carpolo_car_border_interrupt_clear_w },
	{ 0xb005, 0xb005, carpolo_car_ball_interrupt_clear_w },
	{ 0xb006, 0xb006, carpolo_car_goal_interrupt_clear_w },
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( carpolo )
	PORT_START	/* IN0 */
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_COIN4 )
	PORT_BIT (0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* the value read from here is used directly,
	   the result is calculated by 60/value */
	PORT_START	/* IN1 */
	PORT_BIT (0x03, IP_ACTIVE_HIGH, IPT_UNUSED )	/* the lowest 2 bits of the counter */
	PORT_DIPNAME( 0xfc, 0x3c, "Game Sec/Real Sec" )
	PORT_DIPSETTING(	0x00, "0.23 (Longest)" )
	PORT_DIPSETTING(	0xfc, "0.24" )
	PORT_DIPSETTING(	0xf8, "0.24" )
	PORT_DIPSETTING(	0xf4, "0.25" )
	PORT_DIPSETTING(	0xf0, "0.25" )
	PORT_DIPSETTING(	0xec, "0.25" )
	PORT_DIPSETTING(	0xe8, "0.26" )
	PORT_DIPSETTING(	0xe4, "0.26" )
	PORT_DIPSETTING(	0xe0, "0.27" )
	PORT_DIPSETTING(	0xdc, "0.27" )
	PORT_DIPSETTING(	0xd8, "0.28" )
	PORT_DIPSETTING(	0xd4, "0.28" )
	PORT_DIPSETTING(	0xd0, "0.29" )
	PORT_DIPSETTING(	0xcc, "0.29" )
	PORT_DIPSETTING(	0xc8, "0.30" )
	PORT_DIPSETTING(	0xc4, "0.31" )
	PORT_DIPSETTING(	0xc0, "0.31" )
	PORT_DIPSETTING(	0xbc, "0.32" )
	PORT_DIPSETTING(	0xb8, "0.33" )
	PORT_DIPSETTING(	0xb4, "0.33" )
	PORT_DIPSETTING(	0xb0, "0.34" )
	PORT_DIPSETTING(	0xac, "0.35" )
	PORT_DIPSETTING(	0xa8, "0.36" )
	PORT_DIPSETTING(	0xa4, "0.37" )
	PORT_DIPSETTING(	0xa0, "0.38" )
	PORT_DIPSETTING(	0x9c, "0.38" )
	PORT_DIPSETTING(	0x98, "0.39" )
	PORT_DIPSETTING(	0x94, "0.41" )
	PORT_DIPSETTING(	0x90, "0.42" )
	PORT_DIPSETTING(	0x8c, "0.44" )
	PORT_DIPSETTING(	0x84, "0.46" )
	PORT_DIPSETTING(	0x80, "0.47" )
	PORT_DIPSETTING(	0x7c, "0.48" )
	PORT_DIPSETTING(	0x78, "0.50" )
	PORT_DIPSETTING(	0x74, "0.52" )
	PORT_DIPSETTING(	0x70, "0.54" )
	PORT_DIPSETTING(	0x6c, "0.56" )
	PORT_DIPSETTING(	0x68, "0.58" )
	PORT_DIPSETTING(	0x64, "0.60" )
	PORT_DIPSETTING(	0x60, "0.63" )
	PORT_DIPSETTING(	0x5c, "0.65" )
	PORT_DIPSETTING(	0x58, "0.68" )
	PORT_DIPSETTING(	0x54, "0.71" )
	PORT_DIPSETTING(	0x50, "0.75" )
	PORT_DIPSETTING(	0x4c, "0.79" )
	PORT_DIPSETTING(	0x48, "0.83" )
	PORT_DIPSETTING(	0x44, "0.88" )
	PORT_DIPSETTING(	0x40, "0.94" )
	PORT_DIPSETTING(	0x3c, "1.00" )
	PORT_DIPSETTING(	0x38, "1.07" )
	PORT_DIPSETTING(	0x34, "1.15" )
	PORT_DIPSETTING(	0x30, "1.25" )
	PORT_DIPSETTING(	0x2c, "1.36" )
	PORT_DIPSETTING(	0x28, "1.50" )
	PORT_DIPSETTING(	0x24, "1.67" )
	PORT_DIPSETTING(	0x20, "1.88" )
	PORT_DIPSETTING(	0x1c, "2.14" )
	PORT_DIPSETTING(	0x18, "2.50" )
	PORT_DIPSETTING(	0x14, "3.00" )
	PORT_DIPSETTING(	0x10, "3.75" )
	PORT_DIPSETTING(	0x0c, "5.00" )
	PORT_DIPSETTING(	0x08, "7.50" )
	PORT_DIPSETTING(	0x04, "15.00 (Shortest)" )

	PORT_START	/* IN2 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER1 | IPF_REVERSE, 50, 10, 0, 0 )

	PORT_START	/* IN3 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2 | IPF_REVERSE, 50, 10, 0, 0 )

	PORT_START	/* IN4 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER3 | IPF_REVERSE, 50, 10, 0, 0 )

	PORT_START	/* IN5 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER4 | IPF_REVERSE, 50, 10, 0, 0 )

	PORT_START	/* IN6 - accelerator pedals */
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER4 )

	PORT_START	/* IN7 - forward/reverse */
	PORT_BIT (0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	1,
	{ 0 },
	{ STEP4(3*0x100*8+4,1),
	  STEP4(2*0x100*8+4,1),
	  STEP4(1*0x100*8+4,1),
	  STEP4(0*0x100*8+4,1) },
	{ STEP16(0,8) },
	16*8
};

static struct GfxLayout goallayout =
{
	8,32,
	1,
	4,
	{ 4, 5, 6, 7 },
	{ STEP8(0,8) },
	{ STEP8(192*8,8*8),
	  STEP16( 0*8,8*8),
	  STEP8(128*8,8*8) },
	0
};

static struct GfxLayout alphalayout =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout, 0,         12 },
	{ REGION_GFX2, 0, &goallayout,   12*2,      2 },
	{ REGION_GFX3, 0, &alphalayout,  12*2+2*16, 4 },
	{ -1 }
};


/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( carpolo )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,11289000/12)		/* 940.75 kHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(carpolo_timer_interrupt,1)	/* this not strictly VBLANK,
													   but it's supposed to happen 60
													   times a sec, so it's a good place */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(carpolo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 239, 0, 255)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16+1)	/* extra color for score area */
	MDRV_COLORTABLE_LENGTH(12*2+2*16+4*2)

	MDRV_PALETTE_INIT(carpolo)
	MDRV_VIDEO_START(carpolo)
	MDRV_VIDEO_UPDATE(carpolo)
	MDRV_VIDEO_EOF(carpolo)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( carpolo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "4000.6c",   0xf000, 0x0200, 0x9d2e75a5 )
	ROM_LOAD( "4001.6d",   0xf200, 0x0200, 0x69fb3768 )
	ROM_LOAD( "4002.6h",   0xf400, 0x0200, 0x5db179c7 )
	ROM_LOAD( "4003.6k",   0xf600, 0x0200, 0x0f992f09 )
	ROM_LOAD( "4004.6e",   0xf800, 0x0200, 0xbe731610 )
	ROM_LOAD( "4005.6f",   0xfa00, 0x0200, 0x7332f84f )
	ROM_LOAD( "4006.6l",   0xfc00, 0x0200, 0x8479c350 )
	ROM_LOAD( "4007a.6n",  0xfe00, 0x0200, 0xc6a619de )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "1024.10w",  0x0000, 0x0100, 0xeedacc7e )
	ROM_LOAD( "1023.10v",  0x0100, 0x0100, 0x45df6c74 )
	ROM_LOAD( "1022.10u",  0x0200, 0x0100, 0x00868768 )
	ROM_LOAD( "1021.10t",  0x0300, 0x0100, 0xa508af9c )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )	/* goal */
	ROM_LOAD( "1020.6v",   0x0000, 0x0100, 0x5e89fbcd )

	ROM_REGION( 0x0200, REGION_GFX3, ROMREGION_DISPOSE )	/* alpha */
	ROM_LOAD( "2513.4l",   0x0000, 0x0200, BADCRC(0xf80d8889) )	/* MIA - stolen from Night Driver */

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "328.5u",    0x0000, 0x0020, 0xf295e0fc )		/* color PROM */
	ROM_LOAD( "325.6t",    0x0020, 0x0020, 0xb8b44022 )		/* horizontal timing */
	ROM_LOAD( "326.6w",    0x0040, 0x0020, 0x628ae3d1 )		/* vertical timing */

	ROM_REGION( 0x0020, REGION_USER1, 0 )
	ROM_LOAD( "327.10s",   0x0000, 0x0020, 0xe047d24d )		/* sprite image map */
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

static DRIVER_INIT( carpolo )
{
	size_t i;
	UINT8 *ROM;


	/* invert gfx PROM since the bits are active LO */
	ROM = memory_region(REGION_GFX2);
	for (i = 0;i < memory_region_length(REGION_GFX2); i++)
		ROM[i] ^= 0x0f;
}

GAMEX(1977, carpolo, 0, carpolo, carpolo, carpolo, ROT0, "Exidy", "Car Polo", GAME_NO_SOUND)
