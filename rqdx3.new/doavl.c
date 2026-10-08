/*
 *  file = DOAVL.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the AVAILABLE command
 */

#include "defs.h"
#include "pkt.h"
#include "ucb.h"
#include "mscp.h"

extern word fpl;

extern byte *get_ucb( );

/*
 *  the AVAILABLE command packet
 */
struct $avlc
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_r2;
    word	p_mod;
    };

/*
 *  the AVAILABLE response packet
 */
struct $avlr
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_flgs;
    word	p_sts;
    };

#define		rs_avl		sizeof( struct $avlr )

#define PKT (*pkt)
#define CMD (*(struct $avlc *)&(PKT.data))
#define RSP (*(struct $avlr *)&(PKT.data))
#define UCB (*ucb)

/*
 *  process an AVAILABLE command
 *
 *  First of all, the unit specified must be currently online to the host; if
 *  not, the command really makes no sense.  This is a sequential command, so
 *  if there are any non-sequential commands outstanding, we must hold this
 *  command pending until they complete; if not (the UCB.tcbs list is empty),
 *  then clear any dynamic state and flags (part of the state is the online
 *  bit!), turn off the write protect light if it was on, and return success to
 *  the host.
 */
do_avl( pkt )
register struct $pkt *pkt;
    {
    register struct $ucb *ucb;

#if debug>=1
    printf( "\nAVAILABLE, unit = %d", CMD.p_unit );
#endif
    RSP.p_flgs = 0;
    /*
     *  get the UCB corresponding to the given unit number; a NULL means that
     *  there is none, which gets an "offline" status
     */
    if( ( ucb = get_ucb( CMD.p_unit ) ) == null )
	RSP.p_sts = st_ofl;
    else
	{
	/*
	 *  lock the UCB data structure for our personal use
	 */
	$acquire( &UCB.ucb );
	/*
	 *  any non-sequential commands in progress?  if so, simply add this
	 *  PKT to the end of the pending sequential PKTs list, and we will
	 *  get back to it eventually
	 */
	if( ( UCB.tcb != null ) || ( UCB.tcbs != null ) )
	    {
	    $enq_tail( &UCB.pkts, pkt );
	    $release( &UCB.ucb );
	    return;
	    }
	/*
	 *  all's clear, so shut down (clear all "settable" bits)
	 */
	UCB.state &= ~( us_onl|us_imf );
	UCB.flags &= ( uf_wph|uf_rpl|uf_rmv );
	fpl &= ~UCB.wp_bit;
	RSP.p_sts = st_suc;
	/*
	 *  since we're all done, unlock the UCB data structure
	 */
	$release( &UCB.ucb );
	}
    RSP.p_opcd |= op_end;
    PKT.size = rs_avl;
    PKT.type = mt_seq;
    put_packet( pkt );
    }
