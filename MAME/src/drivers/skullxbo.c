#pragma code_seg("C575")
#pragma data_seg("D575")
#pragma bss_seg("B575")
#pragma const_seg("K575")
#pragma comment(linker, "/merge:D575=575")
#pragma comment(linker, "/merge:C575=575")
#pragma comment(linker, "/merge:B575=575")
#pragma comment(linker, "/merge:K575=575")
/***************************************************************************

	Atari Skull & Crossbones hardware

	driver by Aaron Giles

	Games supported:
		* Skull & Crossbones (1989) [2 sets]

	Known bugs:
		* none at this time

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"
#include "skullxbo.h"



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_scanline_int_state)
		newstate = 1;
	if (atarigen_video_int_state)
		newstate = 2;
	if (atarigen_sound_int_state)
		newstate = 4;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void irq_gen(int param)
{
	(void)param;
	atarigen_scanline_int_gen();
}


static void alpha_row_update(int scanline)
{
	data16_t *check = &atarigen_alpha[(scanline / 8) * 64 + 42];

	/* check for interrupts in the alpha ram */
	/* the interrupt occurs on the HBLANK of the 6th scanline following */
	if (check < &atarigen_alpha[0x7c0] && (*check & 0x8000))
		timer_set(cpu_getscanlineperiod() * 6.9, 0, irq_gen);

	/* update the playfield and motion objects */
	skullxbo_scanline_update(scanline);
}


static MACHINE_INIT( skullxbo )
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(alpha_row_update, 8);
	atarijsa_reset();
}



/*************************************
 *
 *	I/O read dispatch.
 *
 *************************************/

static READ16_HANDLER( special_port1_r )
{
	int temp = readinputport(1);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x0040;
	if (atarigen_get_hblank()) temp ^= 0x0010;
	return temp;
}



/*************************************
 *
 *	Who knows what this is?
 *
 *************************************/

static WRITE16_HANDLER( skullxbo_mobwr_w )
{
	logerror("MOBWR[%02X] = %04X\n", offset, data);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0xff2000, 0xff2fff, MRA16_RAM },
	{ 0xff5000, 0xff5001, atarigen_sound_r },
	{ 0xff5800, 0xff5801, input_port_0_word_r },
	{ 0xff5802, 0xff5803, special_port1_r },
	{ 0xff6000, 0xff6fff, atarigen_eeprom_r },
	{ 0xff8000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0xff0000, 0xff07ff, skullxbo_mobmsb_w },
	{ 0xff0800, 0xff0bff, atarigen_halt_until_hblank_0_w },
	{ 0xff0c00, 0xff0fff, atarigen_eeprom_enable_w },
	{ 0xff1000, 0xff13ff, atarigen_video_int_ack_w },
	{ 0xff1400, 0xff17ff, atarigen_sound_w },
	{ 0xff1800, 0xff1bff, atarigen_sound_reset_w },
	{ 0xff1c00, 0xff1c7f, skullxbo_playfieldlatch_w },
	{ 0xff1c80, 0xff1cff, skullxbo_xscroll_w, &atarigen_xscroll },
	{ 0xff1d00, 0xff1d7f, atarigen_scanline_int_ack_w },
	{ 0xff1d80, 0xff1dff, watchdog_reset16_w },
	{ 0xff1e00, 0xff1e7f, skullxbo_playfieldlatch_w },
	{ 0xff1e80, 0xff1eff, skullxbo_xscroll_w },
	{ 0xff1f00, 0xff1f7f, atarigen_scanline_int_ack_w },
	{ 0xff1f80, 0xff1fff, watchdog_reset16_w },
	{ 0xff2000, 0xff2fff, atarigen_666_paletteram_w, &paletteram16 },
	{ 0xff4000, 0xff47ff, skullxbo_yscroll_w, &atarigen_yscroll },
	{ 0xff4800, 0xff4fff, skullxbo_mobwr_w },
	{ 0xff6000, 0xff6fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xff8000, 0xff9fff, atarigen_playfield_latched_lsb_w, &atarigen_playfield },
	{ 0xffa000, 0xffbfff, atarigen_playfield_upper_w, &atarigen_playfield_upper },
	{ 0xffc000, 0xffcf7f, atarigen_alpha_w, &atarigen_alpha },
	{ 0xffcf80, 0xffcfff, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0xffd000, 0xffdfff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0xffe000, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( skullxbo )
	PORT_START      /* ff5800 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START      /* ff5802 */
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNUSED )	/* HBLANK */
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )	/* /AUDBUSY */
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )

	JSA_II_PORT		/* audio board port */
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout pflayout =
{
	16,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4,RGN_FRAC(1,2)+4, 0,0, 4,4,
	  RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+12,RGN_FRAC(1,2)+12, 8,8, 12,12 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};


static struct GfxLayout anlayout =
{
	16,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 1 },
	{ 0,0, 2,2, 4,4, 6,6, 8,8, 10,10, 12,12, 14,14 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};


static struct GfxLayout molayout =
{
	16,8,
	RGN_FRAC(1,5),
	5,
	{ RGN_FRAC(4,5), RGN_FRAC(3,5), RGN_FRAC(2,5), RGN_FRAC(1,5), RGN_FRAC(0,5) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &molayout, 0x000, 32 },
	{ REGION_GFX2, 0, &pflayout, 0x200, 16 },
	{ REGION_GFX3, 0, &anlayout, 0x300, 16 },
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( skullxbo )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz/2)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	
	MDRV_MACHINE_INIT(skullxbo)
	MDRV_NVRAM_HANDLER(atarigen)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(42*16, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*16-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)
	
	MDRV_VIDEO_START(skullxbo)
	MDRV_VIDEO_UPDATE(skullxbo)
	
	/* sound hardware */
	MDRV_IMPORT_FROM(jsa_ii_mono)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( skullxbo )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "5150", 0x00000, 0x10000, CRC(9546d88b) SHA1(4b02c1c8a57377e651a719a0fe2a3532594f3e71) )
	ROM_LOAD16_BYTE( "5151", 0x00001, 0x10000, CRC(b9ed8bd4) SHA1(784a2fc8f5901d9e462966e3f7226ce3c6db980a) )
	ROM_LOAD16_BYTE( "5152", 0x20000, 0x10000, CRC(c07e44fc) SHA1(0aacb77be59c398c9eb5f01108957bbb17c9e5cd) )
	ROM_LOAD16_BYTE( "5153", 0x20001, 0x10000, CRC(fef8297f) SHA1(f62f378a957599ea38579a29df1f9e11335e8fb3) )
	ROM_LOAD16_BYTE( "5154", 0x40000, 0x10000, CRC(de4101a3) SHA1(21cf656fc0695a0ef31cfa6e686069d7d4965cce) )
	ROM_LOAD16_BYTE( "5155", 0x40001, 0x10000, CRC(78c0f6ad) SHA1(21ce2a83cd3350cd7ff53627a7e27599b9754a12) )
	ROM_LOAD16_BYTE( "5156", 0x70000, 0x08000, CRC(cde16b55) SHA1(bf5059f0f73a8819551fb3ded3cb6a3123841481) )
	ROM_LOAD16_BYTE( "5157", 0x70001, 0x08000, CRC(31c77376) SHA1(19eb54d73edb25fda6803df896e182d05c5bad7e) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1149",      0x10000, 0x4000, CRC(8d730e7a) SHA1(b89fb9cadcf813ea5cba55f1efcdcdd2517944c7) )
	ROM_CONTINUE(          0x04000, 0xc000 )

	ROM_REGION( 0x190000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1102",     0x000000, 0x10000, CRC(90becdfa) SHA1(aa5aaeda70e137518a9e58906daed66fa563b27e) ) /* MO */
	ROM_LOAD( "1104",     0x010000, 0x10000, CRC(33609071) SHA1(6d9671a9edbdd28c1e360017253dab5dd858dbe7) ) /* MO */
	ROM_LOAD( "1106",     0x020000, 0x10000, CRC(71962e9f) SHA1(4e6017ede5ce2fec7f6e25eadfc4070f3296ff2f) ) /* MO */
	ROM_LOAD( "1101",     0x030000, 0x10000, CRC(4d41701e) SHA1(b34b392ba00e580cb719be2c1a40cfc0d2f177e3) ) /* MO */
	ROM_LOAD( "1103",     0x040000, 0x10000, CRC(3011da3b) SHA1(e7118b111e0de9b9a2dfe5a165f2140e90c919e7) ) /* MO */

	ROM_LOAD( "1108",     0x050000, 0x10000, CRC(386c7edc) SHA1(a2f61c7f8fb822433957565f373ab5cc8e0a2ba0) ) /* MO */
	ROM_LOAD( "1110",     0x060000, 0x10000, CRC(a54d16e6) SHA1(ad10623f0a87e5a92b8e8c611c2d374a8fd7047e) ) /* MO */
	ROM_LOAD( "1112",     0x070000, 0x10000, CRC(669411f6) SHA1(a0c678a75076b466f4a27c881642c44d47c9dea0) ) /* MO */
	ROM_LOAD( "1107",     0x080000, 0x10000, CRC(caaeb57a) SHA1(e20050d10ee18f52ac36616003de241aa9951eab) ) /* MO */
	ROM_LOAD( "1109",     0x090000, 0x10000, CRC(61cb4e28) SHA1(6d0cb4409fa4c9c696abd612395d5f6ddede6779) ) /* MO */

	ROM_LOAD( "1114",     0x0a0000, 0x10000, CRC(e340d5a1) SHA1(29a23ad2b6c8302508a8b49cfbc064fe86a8b908) ) /* MO */
	ROM_LOAD( "1116",     0x0b0000, 0x10000, CRC(f25b8aca) SHA1(c8c6d0951098c32e32b87d9717cc14bb91ac2017) ) /* MO */
	ROM_LOAD( "1118",     0x0c0000, 0x10000, CRC(8cf73585) SHA1(b1f3e44f6cd434ecfe4d88463b7e2e7b48d2bf1f) ) /* MO */
	ROM_LOAD( "1113",     0x0d0000, 0x10000, CRC(899b59af) SHA1(c7344e4bf57024415463cb50c788631fbad07132) ) /* MO */
	ROM_LOAD( "1115",     0x0e0000, 0x10000, CRC(cf4fd19a) SHA1(731fc7bb1dacacc6e2e4db1e096d07d5fe3d8b19) ) /* MO */

	ROM_LOAD( "1120",     0x0f0000, 0x10000, CRC(fde7c03d) SHA1(ec336af0f3314af134fd1a64c478be06249a550a) ) /* MO */
	ROM_LOAD( "1122",     0x100000, 0x10000, CRC(6ff6a9f2) SHA1(af8fda010c15e13e58e0f235b7b5a12d5a21fc0c) ) /* MO */
	ROM_LOAD( "1124",     0x110000, 0x10000, CRC(f11909f1) SHA1(2dbd5c92e8329f5b5033b3633bd56618eb7da875) ) /* MO */
	ROM_LOAD( "1119",     0x120000, 0x10000, CRC(6f8003a1) SHA1(df8494ce56213dddd11f1365c03bb77ebf3e6eba) ) /* MO */
	ROM_LOAD( "1121",     0x130000, 0x10000, CRC(8ff0a1ec) SHA1(8df33657a035316a1e4ce7d7b33af6e51b990c48) ) /* MO */

	ROM_LOAD( "1125",     0x140000, 0x10000, CRC(3aa7c756) SHA1(c1585733cef28fdf031e335503364846cfd0384a) ) /* MO */
	ROM_LOAD( "1126",     0x150000, 0x10000, CRC(cb82c9aa) SHA1(1469d2825e6d366a8e3f74294b0a64f2a63384aa) ) /* MO */
	ROM_LOAD( "1128",     0x160000, 0x10000, CRC(dce32863) SHA1(b0476de8d54dcf163a723b1fc805da4e1ca11c27) ) /* MO */
	/* 170000-18ffff empty */

	ROM_REGION( 0x0a0000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "2129",     0x000000, 0x10000, CRC(36b1a578) SHA1(ded9cccd1009e517662387353333f20031abddd5) ) /* playfield */
	ROM_LOAD( "2131",     0x010000, 0x10000, CRC(7b7c04a1) SHA1(b57f3f35f39ecf912daf2611919f31a92005f07b) ) /* playfield */
	ROM_LOAD( "2133",     0x020000, 0x10000, CRC(e03fe4d9) SHA1(d9a9174a2d2e2d83f7c32177f0dbab74f3875d2e) ) /* playfield */
	ROM_LOAD( "2135",     0x030000, 0x10000, CRC(7d497110) SHA1(4d5ce6673a112486e9dff77c901d90105aa0a210) ) /* playfield */
	ROM_LOAD( "2137",     0x040000, 0x10000, CRC(f91e7872) SHA1(690715a92e8ca1b1d22fff85f9ed3f1e02edca99) ) /* playfield */
	ROM_LOAD( "2130",     0x050000, 0x10000, CRC(b25368cc) SHA1(110e6882399e200b3a4cdd823cc5b0565183cfeb) ) /* playfield */
	ROM_LOAD( "2132",     0x060000, 0x10000, CRC(112f2d20) SHA1(3acd43cf73f1be10c17a717c8990f5c656935b3a) ) /* playfield */
	ROM_LOAD( "2134",     0x070000, 0x10000, CRC(84884ed6) SHA1(6129f090a4e5b8f65895086e1731b13ee72b6079) ) /* playfield */
	ROM_LOAD( "2136",     0x080000, 0x10000, CRC(bc028690) SHA1(e75a961febf1f1a6c4981301b73bab82c3d19785) ) /* playfield */
	ROM_LOAD( "2138",     0x090000, 0x10000, CRC(60cec955) SHA1(d184746589130a8f10fcc6c79484578bd08757f0) ) /* playfield */

	ROM_REGION( 0x008000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2141",     0x000000, 0x08000, CRC(60d6d6df) SHA1(a8d56092f014a0a93742c701effaec86c75772e1) ) /* alphanumerics */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "1145",      0x00000, 0x10000, CRC(d9475d58) SHA1(5a4a0094c83f5d0e26f0c35feb0b1f15a5f5c3f9) )
	ROM_LOAD( "1146",      0x10000, 0x10000, CRC(133e6aef) SHA1(e393d0b246889779029443a19e3859d45cb900db) )
	ROM_LOAD( "1147",      0x20000, 0x10000, CRC(ba4d556e) SHA1(af4364f2c456abc20f1742c945a3acfeb5e192c4) )
	ROM_LOAD( "1148",      0x30000, 0x10000, CRC(c48df49a) SHA1(c92fde9be1a1ab09453c57eefb0dcfe49e538d07) )
ROM_END


ROM_START( skullxb2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "sku0h.bin", 0x00000, 0x10000, CRC(47083d59) SHA1(a713231c22a3c2de09af65aa2bae17ea41f10cf0) )
	ROM_LOAD16_BYTE( "sku0l.bin", 0x00001, 0x10000, CRC(2c03feaf) SHA1(e7ad1568e3008386f520ed3ba90aefbfc9417a64) )
	ROM_LOAD16_BYTE( "sku1h.bin", 0x20000, 0x10000, CRC(aa0471de) SHA1(c0bf12304b9d64595f2fc40c2fb67f6ccb9d3b8f) )
	ROM_LOAD16_BYTE( "sku1l.bin", 0x20001, 0x10000, CRC(a65386f9) SHA1(3bc7d0bc844cd1f9efa1a5f6abccfbe3ab7c2bef) )
	ROM_LOAD16_BYTE( "5154",      0x40000, 0x10000, CRC(de4101a3) SHA1(21cf656fc0695a0ef31cfa6e686069d7d4965cce) )
	ROM_LOAD16_BYTE( "5155",      0x40001, 0x10000, CRC(78c0f6ad) SHA1(21ce2a83cd3350cd7ff53627a7e27599b9754a12) )
	ROM_LOAD16_BYTE( "5156",      0x70000, 0x08000, CRC(cde16b55) SHA1(bf5059f0f73a8819551fb3ded3cb6a3123841481) )
	ROM_LOAD16_BYTE( "5157",      0x70001, 0x08000, CRC(31c77376) SHA1(19eb54d73edb25fda6803df896e182d05c5bad7e) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1149",      0x10000, 0x4000, CRC(8d730e7a) SHA1(b89fb9cadcf813ea5cba55f1efcdcdd2517944c7) )
	ROM_CONTINUE(          0x04000, 0xc000 )

	ROM_REGION( 0x190000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1102",     0x000000, 0x10000, CRC(90becdfa) SHA1(aa5aaeda70e137518a9e58906daed66fa563b27e) ) /* MO */
	ROM_LOAD( "1104",     0x010000, 0x10000, CRC(33609071) SHA1(6d9671a9edbdd28c1e360017253dab5dd858dbe7) ) /* MO */
	ROM_LOAD( "1106",     0x020000, 0x10000, CRC(71962e9f) SHA1(4e6017ede5ce2fec7f6e25eadfc4070f3296ff2f) ) /* MO */
	ROM_LOAD( "1101",     0x030000, 0x10000, CRC(4d41701e) SHA1(b34b392ba00e580cb719be2c1a40cfc0d2f177e3) ) /* MO */
	ROM_LOAD( "1103",     0x040000, 0x10000, CRC(3011da3b) SHA1(e7118b111e0de9b9a2dfe5a165f2140e90c919e7) ) /* MO */

	ROM_LOAD( "1108",     0x050000, 0x10000, CRC(386c7edc) SHA1(a2f61c7f8fb822433957565f373ab5cc8e0a2ba0) ) /* MO */
	ROM_LOAD( "1110",     0x060000, 0x10000, CRC(a54d16e6) SHA1(ad10623f0a87e5a92b8e8c611c2d374a8fd7047e) ) /* MO */
	ROM_LOAD( "1112",     0x070000, 0x10000, CRC(669411f6) SHA1(a0c678a75076b466f4a27c881642c44d47c9dea0) ) /* MO */
	ROM_LOAD( "1107",     0x080000, 0x10000, CRC(caaeb57a) SHA1(e20050d10ee18f52ac36616003de241aa9951eab) ) /* MO */
	ROM_LOAD( "1109",     0x090000, 0x10000, CRC(61cb4e28) SHA1(6d0cb4409fa4c9c696abd612395d5f6ddede6779) ) /* MO */

	ROM_LOAD( "1114",     0x0a0000, 0x10000, CRC(e340d5a1) SHA1(29a23ad2b6c8302508a8b49cfbc064fe86a8b908) ) /* MO */
	ROM_LOAD( "1116",     0x0b0000, 0x10000, CRC(f25b8aca) SHA1(c8c6d0951098c32e32b87d9717cc14bb91ac2017) ) /* MO */
	ROM_LOAD( "1118",     0x0c0000, 0x10000, CRC(8cf73585) SHA1(b1f3e44f6cd434ecfe4d88463b7e2e7b48d2bf1f) ) /* MO */
	ROM_LOAD( "1113",     0x0d0000, 0x10000, CRC(899b59af) SHA1(c7344e4bf57024415463cb50c788631fbad07132) ) /* MO */
	ROM_LOAD( "1115",     0x0e0000, 0x10000, CRC(cf4fd19a) SHA1(731fc7bb1dacacc6e2e4db1e096d07d5fe3d8b19) ) /* MO */

	ROM_LOAD( "1120",     0x0f0000, 0x10000, CRC(fde7c03d) SHA1(ec336af0f3314af134fd1a64c478be06249a550a) ) /* MO */
	ROM_LOAD( "1122",     0x100000, 0x10000, CRC(6ff6a9f2) SHA1(af8fda010c15e13e58e0f235b7b5a12d5a21fc0c) ) /* MO */
	ROM_LOAD( "1124",     0x110000, 0x10000, CRC(f11909f1) SHA1(2dbd5c92e8329f5b5033b3633bd56618eb7da875) ) /* MO */
	ROM_LOAD( "1119",     0x120000, 0x10000, CRC(6f8003a1) SHA1(df8494ce56213dddd11f1365c03bb77ebf3e6eba) ) /* MO */
	ROM_LOAD( "1121",     0x130000, 0x10000, CRC(8ff0a1ec) SHA1(8df33657a035316a1e4ce7d7b33af6e51b990c48) ) /* MO */

	ROM_LOAD( "1125",     0x140000, 0x10000, CRC(3aa7c756) SHA1(c1585733cef28fdf031e335503364846cfd0384a) ) /* MO */
	ROM_LOAD( "1126",     0x150000, 0x10000, CRC(cb82c9aa) SHA1(1469d2825e6d366a8e3f74294b0a64f2a63384aa) ) /* MO */
	ROM_LOAD( "1128",     0x160000, 0x10000, CRC(dce32863) SHA1(b0476de8d54dcf163a723b1fc805da4e1ca11c27) ) /* MO */
	/* 170000-18ffff empty */

	ROM_REGION( 0x0a0000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "2129",     0x000000, 0x10000, CRC(36b1a578) SHA1(ded9cccd1009e517662387353333f20031abddd5) ) /* playfield */
	ROM_LOAD( "2131",     0x010000, 0x10000, CRC(7b7c04a1) SHA1(b57f3f35f39ecf912daf2611919f31a92005f07b) ) /* playfield */
	ROM_LOAD( "2133",     0x020000, 0x10000, CRC(e03fe4d9) SHA1(d9a9174a2d2e2d83f7c32177f0dbab74f3875d2e) ) /* playfield */
	ROM_LOAD( "2135",     0x030000, 0x10000, CRC(7d497110) SHA1(4d5ce6673a112486e9dff77c901d90105aa0a210) ) /* playfield */
	ROM_LOAD( "2137",     0x040000, 0x10000, CRC(f91e7872) SHA1(690715a92e8ca1b1d22fff85f9ed3f1e02edca99) ) /* playfield */
	ROM_LOAD( "2130",     0x050000, 0x10000, CRC(b25368cc) SHA1(110e6882399e200b3a4cdd823cc5b0565183cfeb) ) /* playfield */
	ROM_LOAD( "2132",     0x060000, 0x10000, CRC(112f2d20) SHA1(3acd43cf73f1be10c17a717c8990f5c656935b3a) ) /* playfield */
	ROM_LOAD( "2134",     0x070000, 0x10000, CRC(84884ed6) SHA1(6129f090a4e5b8f65895086e1731b13ee72b6079) ) /* playfield */
	ROM_LOAD( "2136",     0x080000, 0x10000, CRC(bc028690) SHA1(e75a961febf1f1a6c4981301b73bab82c3d19785) ) /* playfield */
	ROM_LOAD( "2138",     0x090000, 0x10000, CRC(60cec955) SHA1(d184746589130a8f10fcc6c79484578bd08757f0) ) /* playfield */

	ROM_REGION( 0x008000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2141",     0x000000, 0x08000, CRC(60d6d6df) SHA1(a8d56092f014a0a93742c701effaec86c75772e1) ) /* alphanumerics */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "1145",      0x00000, 0x10000, CRC(d9475d58) SHA1(5a4a0094c83f5d0e26f0c35feb0b1f15a5f5c3f9) )
	ROM_LOAD( "1146",      0x10000, 0x10000, CRC(133e6aef) SHA1(e393d0b246889779029443a19e3859d45cb900db) )
	ROM_LOAD( "1147",      0x20000, 0x10000, CRC(ba4d556e) SHA1(af4364f2c456abc20f1742c945a3acfeb5e192c4) )
	ROM_LOAD( "1148",      0x30000, 0x10000, CRC(c48df49a) SHA1(c92fde9be1a1ab09453c57eefb0dcfe49e538d07) )
ROM_END



/*************************************
 *
 *	ROM decoding
 *
 *************************************/

static DRIVER_INIT( skullxbo )
{
	atarigen_eeprom_default = NULL;
	atarijsa_init(1, 2, 1, 0x0080);
	atarigen_init_6502_speedup(1, 0x4159, 0x4171);
	memset(memory_region(REGION_GFX1) + 0x170000, 0, 0x20000);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1989, skullxbo, 0,        skullxbo, skullxbo, skullxbo, ROT0, "Atari Games", "Skull & Crossbones (set 1)" )
GAME( 1989, skullxb2, skullxbo, skullxbo, skullxbo, skullxbo, ROT0, "Atari Games", "Skull & Crossbones (set 2)" )
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
