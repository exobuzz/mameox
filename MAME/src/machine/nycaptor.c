#pragma code_seg("C505")
#pragma data_seg("D505")
#pragma bss_seg("B505")
#pragma const_seg("K505")
#pragma comment(linker, "/merge:D505=505")
#pragma comment(linker, "/merge:C505=505")
#pragma comment(linker, "/merge:B505=505")
#pragma comment(linker, "/merge:K505=505")
/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

static unsigned char from_main,from_mcu;
static int mcu_sent = 0,main_sent = 0;


static unsigned char portA_in,portA_out,ddrA;

READ_HANDLER( nycaptor_68705_portA_r )
{

	return (portA_out & ddrA) | (portA_in & ~ddrA);
}

WRITE_HANDLER( nycaptor_68705_portA_w )
{

	portA_out = data;
}

WRITE_HANDLER( nycaptor_68705_ddrA_w )
{
	ddrA = data;
}

/*
 *  Port B connections:
 *
 *  all bits are logical 1 when read (+5V pullup)
 *
 *  1   W  when 1->0, enables latch which brings the command from main CPU (read from port A)
 *  2   W  when 0->1, copies port A to the latch for the main CPU
 */

static unsigned char portB_in,portB_out,ddrB;

READ_HANDLER( nycaptor_68705_portB_r )
{
	return (portB_out & ddrB) | (portB_in & ~ddrB);
}

WRITE_HANDLER( nycaptor_68705_portB_w )
{


	if ((ddrB & 0x02) && (~data & 0x02) && (portB_out & 0x02))
	{
		portA_in = from_main;
		if (main_sent) cpu_set_irq_line(3,0,CLEAR_LINE);
		main_sent = 0;

	}
	if ((ddrB & 0x04) && (data & 0x04) && (~portB_out & 0x04))
	{

		from_mcu = portA_out;
		mcu_sent = 1;
	}

	portB_out = data;
}

WRITE_HANDLER( nycaptor_68705_ddrB_w )
{
	ddrB = data;
}


static unsigned char portC_in,portC_out,ddrC;

READ_HANDLER( nycaptor_68705_portC_r )
{
	portC_in = 0;
	if (main_sent) portC_in |= 0x01;
	if (!mcu_sent) portC_in |= 0x02;

	return (portC_out & ddrC) | (portC_in & ~ddrC);
}

WRITE_HANDLER( nycaptor_68705_portC_w )
{

	portC_out = data;
}

WRITE_HANDLER( nycaptor_68705_ddrC_w )
{
	ddrC = data;
}

WRITE_HANDLER( nycaptor_mcu_w )
{

	from_main = data;
	main_sent = 1;
	cpu_set_irq_line(3,0,ASSERT_LINE);
}

READ_HANDLER( nycaptor_mcu_r )
{

	mcu_sent = 0;
	return from_mcu;
}

READ_HANDLER( nycaptor_mcu_status_r1 )
{
	/* bit 1 = when 1, mcu has sent data to the main cpu */

	return mcu_sent?2:0;
}

READ_HANDLER( nycaptor_mcu_status_r2 )
{
	/* bit 0 = when 1, mcu is ready to receive data from main cpu */
  return main_sent?0:1;

}
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
