/*
 *  this module contains rqdx3 test routines
 */

#include "defs.h"
#include "jcb.h"
#include "ucb.h"

#define		op_rd			0x03
#define		op_srp			0x40
#define		op_sri			0x57

byte *_packet;

extern byte r$cmd;
extern byte r$dat;
extern byte w$cmd;
extern byte w$dat;
extern word w$fpl;
extern word r$sawr;
extern word w$sard;

extern byte data[];
extern word reg_8;
extern word main_stack[main_size];
extern struct $jcb main_jcb;

extern byte *get_ucb( );
extern void main( );

start( )
    {
    $create( &main_jcb, &main, 0, &main_stack[main_size] );
    }

#define UCB (*ucb)

main( )
    {
    register struct $ucb *ucb;

    clear( );
    setup( );
    ucb = get_ucb( 0 );
    select( ucb );
    while( true )
	{
	w$cmd = op_srp + 4;
	w$dat = ( r$sawr >> 4 ) & 0x70;
	w$dat = r$sawr & 0xFF;
	w$cmd = op_srp + 8;
	w$dat = reg_8;
	w$fpl = 0;
	put_udc( op_sri );
	w$cmd = op_srp + 8;
	w$dat = reg_8 + ( ( r$sawr >> 12 ) & 0x07 );
	w$fpl = UCB.wp_bit|UCB.rs_bit;
	put_udc( op_rd );
	while( w$cmd = op_srp + 9, !( ( w$sard = r$dat ) & bit4 ) )
	    $sleep( 1 );
	}
    deselect( ucb );
    while( true )
	$sleep( 1000 );
    }
