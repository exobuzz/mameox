#pragma code_seg("C435")
#pragma data_seg("D435")
#pragma bss_seg("B435")
#pragma const_seg("K435")
#pragma comment(linker, "/merge:D435=435")
#pragma comment(linker, "/merge:C435=435")
#pragma comment(linker, "/merge:B435=435")
#pragma comment(linker, "/merge:K435=435")
/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



/* palette colors (see drivers/8080bw.c) */
enum { BLACK, WHITE };


static unsigned char mask = 0;

WRITE8_HANDLER( ramtek_mask_w )
{
	mask = data;
}

WRITE8_HANDLER( ramtek_videoram_w )
{
	data = data & ~mask;

	if (videoram[offset] != data)
	{
		int i,x,y;

		videoram[offset] = data;

		y = offset / 32;
		x = 8 * (offset % 32);

		for (i = 0; i < 8; i++)
		{
			plot_pixel(tmpbitmap, x, y, Machine->pens[(data & 0x80) ? WHITE : BLACK]);

			x++;
			data <<= 1;
		}
	}
}
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
