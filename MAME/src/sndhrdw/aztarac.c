#pragma code_seg("C131")
#pragma data_seg("D131")
#pragma bss_seg("B131")
#pragma const_seg("K131")
#pragma comment(linker, "/merge:D131=131")
#pragma comment(linker, "/merge:C131=131")
#pragma comment(linker, "/merge:B131=131")
#pragma comment(linker, "/merge:K131=131")
/***************************************************************************

	Centuri Aztarac hardware
	
***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "aztarac.h"

static int sound_status;

READ16_HANDLER( aztarac_sound_r )
{
    if (Machine->sample_rate)
        return sound_status & 0x01;
    else
        return 1;
}

WRITE16_HANDLER( aztarac_sound_w )
{
	if (ACCESSING_LSB)
	{
		data &= 0xff;
		soundlatch_w(offset, data);
		sound_status ^= 0x21;
		if (sound_status & 0x20)
			cpu_set_irq_line(1, 0, HOLD_LINE);
	}
}

READ_HANDLER( aztarac_snd_command_r )
{
    sound_status |= 0x01;
    sound_status &= ~0x20;
    return soundlatch_r(offset);
}

READ_HANDLER( aztarac_snd_status_r )
{
    return sound_status & ~0x01;
}

WRITE_HANDLER( aztarac_snd_status_w )
{
    sound_status &= ~0x10;
}

INTERRUPT_GEN( aztarac_snd_timed_irq )
{
    sound_status ^= 0x10;

    if (sound_status & 0x10)
        cpu_set_irq_line(1,0,HOLD_LINE);
}


#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
