#pragma code_seg("C628")
#pragma data_seg("D628")
#pragma bss_seg("B628")
#pragma const_seg("K628")
#pragma comment(linker, "/merge:D628=628")
#pragma comment(linker, "/merge:C628=628")
#pragma comment(linker, "/merge:B628=628")
#pragma comment(linker, "/merge:K628=628")
/* Space Bugger - Video Hardware */

#include "driver.h"

extern data8_t* sbugger_videoram, *sbugger_videoram_attr;

static struct tilemap *sbugger_tilemap;

static void get_sbugger_tile_info(int tile_index)
{
	int tileno, color;

	tileno = sbugger_videoram[tile_index];
	color = sbugger_videoram_attr[tile_index];

	SET_TILE_INFO(0,tileno,color,0)
}

WRITE8_HANDLER( sbugger_videoram_w )
{
	sbugger_videoram[offset] = data;
	tilemap_mark_tile_dirty(sbugger_tilemap,offset);
}

WRITE8_HANDLER( sbugger_videoram_attr_w )
{
	sbugger_videoram_attr[offset] = data;
	tilemap_mark_tile_dirty(sbugger_tilemap,offset);
}

VIDEO_START(sbugger)
{

	sbugger_tilemap = tilemap_create(get_sbugger_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 8, 16,64,16);

	if (!sbugger_tilemap)
		return 1;

	return 0;
}

VIDEO_UPDATE(sbugger)
{
	tilemap_draw(bitmap,cliprect,sbugger_tilemap,0,0);
}

/* not right but so we can see things ok */
PALETTE_INIT(sbugger)
{
	/* just some random colours for now */
	int i;

	for (i = 0;i < 256;i++)
	{
		int r = rand()|0x80;
		int g = rand()|0x80;
		int b = rand()|0x80;
		if (i == 0) r = g = b = 0;

		palette_set_color(i*2+1,r,g,b);
		palette_set_color(i*2,0,0,0);

	}

}
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
