#pragma code_seg("C665")
#pragma bss_seg("B665")
#pragma data_seg("D665")
#pragma const_seg("K665")
/**
 * @file vidhrdw/djboy.c
 *
 * video hardware for DJ Boy
 */
#include "driver.h"
#include "vidhrdw/generic.h"

static data8_t djboy_videoreg, djboy_scrollx, djboy_scrolly;
static struct tilemap *background;

void djboy_set_videoreg( data8_t data )
{
	djboy_videoreg = data;
}

WRITE_HANDLER( djboy_scrollx_w )
{
	djboy_scrollx = data;
}

WRITE_HANDLER( djboy_scrolly_w )
{
	djboy_scrolly = data;
}

static void get_bg_tile_info(int tile_index)
{
	unsigned char attr;
	attr = videoram[tile_index + 0x400];
	SET_TILE_INFO(
			2,
			videoram[tile_index] + ((attr & 0x0f) << 8),
			(attr >> 4),
			0)
}

WRITE_HANDLER( djboy_videoram_w )
{
	if( videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty( background, offset & 0x7ff);
	}
}

VIDEO_START( djboy )
{
	background = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,16,16,64,32);
	if( background )
	{
		return 0;
	}
	return -1;
}

static void
draw_sprites( struct mame_bitmap *bitmap,const struct rectangle *cliprect )
{
	int page;
	for( page=0; page<2; page++ )
	{
		const data8_t *pSource = &spriteram[page*0x800];
		int sx = 0;
		int sy = 0;
		int offs;
		for ( offs = 0 ; offs < 0x100 ; offs++)
		{
			int attr	=	pSource[offs + 0x300];
			int x		=	pSource[offs + 0x400] - ((attr << 8) & 0x100);
			int y		=	pSource[offs + 0x500] - ((attr << 7) & 0x100);
			int gfx		=	pSource[offs + 0x700];
			int code	=	pSource[offs + 0x600] + ((gfx & 0x3f) << 8);
			int flipx	=	gfx & 0x80;
			int flipy	=	gfx & 0x40;
			if( attr & 0x04 )
			{
				sx += x;
				sy += y;
			}
			else
			{
				sx  = x;
				sy  = y;
			}
			drawgfx(
				bitmap,Machine->gfx[1],
				code,
				attr >> 4,
				flipx, flipy,
				sx,sy,
				cliprect,TRANSPARENCY_PEN,0);
		} /* next tile */
	} /* next page */
} /* draw_sprites */

WRITE_HANDLER( djboy_paletteram_w )
{
	int r,g,b;
	int val;

	paletteram[offset] = data;
	offset &= ~1;
	val = (paletteram[offset]<<8) | paletteram[offset+1];

	r = (val >> 8) & 0xf;
	g = (val >> 4) & 0xf;
	b = (val >> 0) & 0xf;

	palette_set_color(
		offset/2,
		(r * 0xff) / 0xf,
		(g * 0xff) / 0xf,
		(b * 0xff) / 0xf );
}

VIDEO_UPDATE( djboy )
{
	/**
	 * xx------ msb x
	 * --x----- msb y
	 * ---x---- flipscreen
	 * ----xxxx ROM bank
	 */
	int flip = djboy_videoreg&0x10;
	int scroll;

	tilemap_set_flip( ALL_TILEMAPS, flip?(TILEMAP_FLIPX|TILEMAP_FLIPY):0 );

	scroll = djboy_scrollx | ((djboy_videoreg&0xc0)<<2);
	tilemap_set_scrollx( background, 0, scroll );

	scroll = djboy_scrolly | ((djboy_videoreg&0x20)<<3);
	tilemap_set_scrolly( background, 0, scroll );	

	tilemap_draw( bitmap, cliprect,background,0,0 );
	draw_sprites( bitmap, cliprect );
}
#pragma data_seg()
#pragma code_seg()
#pragma bss_seg()
#pragma const_seg()
