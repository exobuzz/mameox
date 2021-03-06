#ifndef _8255PPI_H_
#define _8255PPI_H_

#define MAX_8255 8

typedef struct
{
	int num;							 /* number of PPIs to emulate */
	read8_handler portAread[MAX_8255];
	read8_handler portBread[MAX_8255];
	read8_handler portCread[MAX_8255];
	write8_handler portAwrite[MAX_8255];
	write8_handler portBwrite[MAX_8255];
	write8_handler portCwrite[MAX_8255];
} ppi8255_interface;


/* Init */
void ppi8255_init( ppi8255_interface *intfce);

/* Read/Write */
int ppi8255_r ( int which, int offset );
void ppi8255_w( int which, int offset, int data );

void ppi8255_set_portAread( int which, read8_handler portAread);
void ppi8255_set_portBread( int which, read8_handler portBread);
void ppi8255_set_portCread( int which, read8_handler portCread);

void ppi8255_set_portAwrite( int which, write8_handler portAwrite);
void ppi8255_set_portBwrite( int which, write8_handler portBwrite);
void ppi8255_set_portCwrite( int which, write8_handler portCwrite);

#ifdef MESS
/* Peek at the ports */
data8_t ppi8255_peek( int which, offs_t offset );
#endif

/* Helpers */
READ8_HANDLER( ppi8255_0_r );
READ8_HANDLER( ppi8255_1_r );
READ8_HANDLER( ppi8255_2_r );
READ8_HANDLER( ppi8255_3_r );
READ8_HANDLER( ppi8255_4_r );
READ8_HANDLER( ppi8255_5_r );
READ8_HANDLER( ppi8255_6_r );
READ8_HANDLER( ppi8255_7_r );
WRITE8_HANDLER( ppi8255_0_w );
WRITE8_HANDLER( ppi8255_1_w );
WRITE8_HANDLER( ppi8255_2_w );
WRITE8_HANDLER( ppi8255_3_w );
WRITE8_HANDLER( ppi8255_4_w );
WRITE8_HANDLER( ppi8255_5_w );
WRITE8_HANDLER( ppi8255_6_w );
WRITE8_HANDLER( ppi8255_7_w );
#endif
