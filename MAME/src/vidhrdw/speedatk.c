#pragma code_seg("C761")
#pragma data_seg("D761")
#pragma bss_seg("B761")
#pragma const_seg("K761")
#pragma comment(linker, "/merge:D761=761")
#pragma comment(linker, "/merge:C761=761")
#pragma comment(linker, "/merge:B761=761")
#pragma comment(linker, "/merge:K761=761")
/*****************************************************************************************

 Speed Attack video hardware emulation

*****************************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *tilemap;

/*
Color prom dump(only 0x00-0x10 range has valid data)
0:---- ---- 0x00 Black
1:---- -x-- 0x04
2:---- -xxx 0x07
3:x-x- -xxx 0xa7
4:--x- x--- 0x28
5:xxxx x--- 0xf8
6:--xx xxxx 0x3f
7:xxxx xxxx 0xff White
8:x--- -x-- 0x84
9:x-x- xx-x 0xad
a:--x- -x-x 0x25
b:-xxx xxx- 0x7e
c:--x- xxxx 0x2f
d:xx-- ---- 0xc0
e:--xx -xx- 0x36
f:xxx- x--- 0xe8
*/

PALETTE_INIT( speedatk )
{
	int i;
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	for (i = 0;i < 0x10;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
	}

	color_prom += 0x10;

	/* Colortable entry */
}

WRITE_HANDLER( speedatk_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(tilemap,offset);
}

WRITE_HANDLER( speedatk_colorram_w )
{
	colorram[offset] = data;
	tilemap_mark_tile_dirty(tilemap,offset);
}

WRITE_HANDLER( speedatk_flip_screen_w )
{
	flip_screen_set(data);
}

static void get_tile_info(int tile_index)
{
	int code, color, region;

	code = videoram[tile_index] + ((colorram[tile_index] & 0xe0) << 3);
	color = colorram[tile_index] & 0x0f;
	region = (colorram[tile_index] & 0x10) >> 4;

	SET_TILE_INFO(region, code, color, 0)
}

VIDEO_START( speedatk )
{
	tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,34,32);

	if(!tilemap)
		return 1;

	return 0;
}

VIDEO_UPDATE( speedatk )
{
	tilemap_draw(bitmap,cliprect,tilemap,0,0);
}
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
