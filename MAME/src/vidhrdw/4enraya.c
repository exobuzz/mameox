#pragma code_seg("C103")
#pragma data_seg("D103")
#pragma bss_seg("B103")
#pragma const_seg("K103")
#pragma comment(linker, "/merge:D103=103")
#pragma comment(linker, "/merge:C103=103")
#pragma comment(linker, "/merge:B103=103")
#pragma comment(linker, "/merge:K103=103")
/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *tilemap;

WRITE8_HANDLER( fenraya_videoram_w )
{
	videoram[(offset&0x3ff)*2]=data;
	videoram[(offset&0x3ff)*2+1]=(offset&0xc00)>>10;
	tilemap_mark_tile_dirty(tilemap,offset&0x3ff);
}

static void get_tile_info(int tile_index)
{
	int code = videoram[tile_index*2]+(videoram[tile_index*2+1]<<8);
	SET_TILE_INFO(
		0,
		code,
		0,
		0)
}

VIDEO_START( 4enraya )
{
	tilemap = tilemap_create( get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32 );
	return video_start_generic();
}

VIDEO_UPDATE( 4enraya)
{
	tilemap_draw(bitmap,cliprect,tilemap, 0,0);
}
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
