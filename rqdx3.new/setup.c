/*
 *  file = SETUP.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  this module contains the device setup routines
 */

#include "defs.h"
#include "ucb.h"
#include "mscp.h"

#define		op_si1c		0x05
#define		op_so1c		0x07
#define		op_sd_rd	0x24
#define		op_sd_rx	0x2C
#define		op_srp		0x40

extern word rw$pll;
extern word r$fps;
extern byte r$dat;
extern byte w$dat;
extern byte w$cmd;

extern list udc;
extern byte *ucbs[];
extern word _bit0[];
extern word _bit4[];
extern struct $ucb _ucb[];

#define UCB (*ucb)

/*
 *  this routine will find and categorize each attached disk unit
 *
 *  We start with Drive Select 2, then proceed to Drive Select 3, then back to
 *  Drive Select 0, then finally to Drive Select 1.  If any device responds
 *  to a given drive select signal, then it is assigned the next available
 *  logical unit number (the first logical unit number is 0).  A device can
 *  respond in one of two ways:  RD devices respond by asserting Drive Select
 *  Acknowledge, and if we get one of these, then we read the geometry
 *  information from the XBNs and we're all done;  RX devices don't respond
 *  with the same acknowledge, but instead they will assert Track 0 if they
 *  are stepped appropriately.  Further, we have to distinguish between RX50
 *  drives and RX33 drives; the former will not respond with Track 0 if head 1
 *  is selected, while the latter will.
 */
word setup( )
    {
    word i, ui, uj;
    register word unit;
    register struct $ucb *ucb;

    /*
     *  just for the hell of it, grab control of the SMC9224
     */
    $acquire( &udc );
    /*
     *  logical unit numbers start at 0, and drive selects start at 2
     */
    ui = 0;
    ucb = &_ucb[0];
    for( uj = 2; uj < 6; uj++, ucb++ )
	{
	unit = uj & 3;
	/*
	 *  make all defaults appear
	 */
	rw$pll = 0x0008;
	w$cmd = op_srp + 4;
	w$dat = 0;
	w$cmd = op_srp + 7;
	w$dat = 0xF2;
	w$dat = 0x82;
	w$dat = 0xB9;
	/*
	 *  do a preliminary drive select, assuming it is an RD device
	 */
	put_udc( op_sd_rd + unit );
	/*
	 *  if we get the acknowledge (it's in the front panel status
	 *  register) then we know we have a hard disk attached, and we
	 *  update the UCB accordingly
	 */
	if( r$fps & ( bit15|bit14 ) )
	    {
	    ucbs[ui++] = ucb;
	    rd_drive( unit, ucb );
	    }
	else
	    {
	    /*
	     *  now we must assume that it is an RX device, so turn the motor
	     *  on and force ready to be true
	     */
	    rw$pll = 0x0009;
	    w$cmd = op_srp + 7;
	    w$dat = 0xF7;
	    put_udc( op_sd_rx + unit );
	    /*
	     *  because the RX50 can be positioned BEHIND track 0, we must
	     *  look for track 0 by stepping out for a while, then back in,
	     *  and finally out again, all the while checking for track 0 --
	     *  this is to take care of the cases where we START OUT behind
	     *  track 0 as well as the normal cases
	     */
	    for( i = 0; i < 160; i++ )
		{
		w$cmd = op_srp + 9;
		if( r$dat & bit4 )
		    break;
		if( ( i >= 80 ) && ( i <= 130 ) )
		    put_udc( op_si1c );
		else
		    put_udc( op_so1c );
		}
	    w$cmd = op_srp + 9;
	    /*
	     *  if we get track 0, then we know we have a floppy disk
	     *  attached, and we update the UCB accordingly
	     */
	    if( r$dat & bit4 )
		{
		ucbs[ui++] = ucb;
		/*
		 *  see if it's an RX33, and not an RX50
		 */
		w$cmd = op_srp + 4;
		w$dat = 1;
		put_udc( op_sd_rx + unit );
		w$cmd = op_srp + 9;
		/*
		 *  if track 0 is still true, it's the RX33 speaking
		 */
		if( r$dat & bit4 )
		    rx33_drive( unit, ucb );
		else
		    rx50_drive( unit, ucb );
		}
	    }
	}
    /*
     *  take care of any left hooks that we have thrown to the SMC9224, and
     *  then release it
     */
    init_udc( );
    $release( &udc );
    }

#define UCB (*ucb)

/*
 *  this routine will fill a unit control block with RD drive parameters
 */
rd_drive( unit, ucb )
register word unit;
register struct $ucb *ucb;
    {
    UCB.pd_bit = _bit0[unit];
    UCB.wp_bit = _bit0[unit];
    UCB.rs_bit = _bit4[unit];
    UCB.flags = uf_rpl;
    UCB.mode = 0x00C0;
    UCB.op_sd = op_sd_rd + unit;
    UCB.pllctl = ( r$fps & bit14 ) ? 0x0008 : 0x000A;
    UCB.state = us_rd|us_unk|us_hs3;
    UCB.media = 0x25644033L;
    UCB.type = ( 2<<8 ) + 6;
    }

#define UCB (*ucb)

/*
 *  this routine will fill a unit control block with RX33 drive parameters
 */
rx33_drive( unit, ucb )
register word unit;
register struct $ucb *ucb;
    {
    UCB.flags = uf_rpl|uf_rmv;
    UCB.mode = 0x0082;
    UCB.op_sd = op_sd_rx + unit;
    UCB.pllctl = 0x0009;
    UCB.state = us_rx|us_33;
    UCB.media = 0x25658021L;
    UCB.type = ( 2<<8 ) + 10;
    }

#define UCB (*ucb)

/*
 *  this routine will fill a unit control block with RX50 drive parameters
 */
rx50_drive( unit, ucb )
register word unit;
register struct $ucb *ucb;
    {
    UCB.flags = uf_rpl|uf_rmv;
    UCB.mode = 0x0082;
    UCB.op_sd = op_sd_rx + unit;
    UCB.pllctl = 0x0009;
    UCB.state = us_rx|us_50;
    UCB.media = 0x25658032L;
    UCB.type = ( 2<<8 ) + 7;
    }
