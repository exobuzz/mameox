#pragma code_seg("C713")
#pragma data_seg("D713")
#pragma bss_seg("B713")
#pragma const_seg("K713")
#pragma comment(linker, "/merge:D713=713")
#pragma comment(linker, "/merge:C713=713")
#pragma comment(linker, "/merge:B713=713")
#pragma comment(linker, "/merge:K713=713")
#include "vidhrdw/generic.h"

/* globals */
data8_t *timelimt_bg_videoram;
size_t timelimt_bg_videoram_size;

/* locals */
static int scrollx, scrolly;

static struct tilemap *bg_tilemap, *fg_tilemap;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Time Limit has two 32 bytes palette PROM, connected to the RGB output this
  way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/

PALETTE_INIT( timelimt ) {
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (*color_prom >> 6) & 0x01;
		bit1 = (*color_prom >> 7) & 0x01;
		b = 0x4f * bit0 + 0xa8 * bit1;

		palette_set_color(i,r,g,b);
		color_prom++;
	}
}

/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

static void get_bg_tile_info(int tile_index)
{
	SET_TILE_INFO(1, timelimt_bg_videoram[tile_index], 0, 0);
}

static void get_fg_tile_info(int tile_index)
{
	SET_TILE_INFO(0, videoram[tile_index], 0, 0);
}

VIDEO_START( timelimt )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 64, 32);

	if (!bg_tilemap)
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, 
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if (!fg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 0);

	return 0;
}

/***************************************************************************/

WRITE_HANDLER( timelimt_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( timelimt_bg_videoram_w )
{
	if (timelimt_bg_videoram[offset] != data)
	{
		timelimt_bg_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( timelimt_scroll_x_lsb_w )
{
	scrollx &= 0x100;
	scrollx |= data & 0xff;
}

WRITE_HANDLER( timelimt_scroll_x_msb_w )
{
	scrollx &= 0xff;
	scrollx |= ( data & 1 ) << 8;
}

WRITE_HANDLER( timelimt_scroll_y_w )
{
	scrolly = data;
}

/***************************************************************************

	Draw the sprites

***************************************************************************/
static void drawsprites( struct mame_bitmap *bitmap )
{
	int offs;

	for( offs = spriteram_size; offs >= 0; offs -= 4 )
	{
		int sy = 240 - spriteram[offs];
		int sx = spriteram[offs+3];
		int code = spriteram[offs+1] & 0x3f;
		int attr = spriteram[offs+2];
		int flipy = spriteram[offs+1] & 0x80;
		int flipx = spriteram[offs+1] & 0x40;

		code += ( attr & 0x80 ) ? 0x40 : 0x00;
		code += ( attr & 0x40 ) ? 0x80 : 0x00;

		drawgfx( bitmap, Machine->gfx[2],
				code,
				attr & 7,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

VIDEO_UPDATE( timelimt )
{
	tilemap_set_scrollx(bg_tilemap, 0, scrollx);
	tilemap_set_scrolly(bg_tilemap, 0, scrolly);
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);

	drawsprites( bitmap );

	tilemap_draw(bitmap, &Machine->visible_area, fg_tilemap, 0, 0);
}
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
