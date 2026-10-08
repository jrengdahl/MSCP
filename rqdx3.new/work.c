/*
 *  file = WORK.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  this module contains the WORK job
 */

#include "defs.h"
#include "pkt.h"
#include "tcb.h"
#include "ucb.h"

#define TCB (*tcb)
#define UCB (*ucb)
#define PKT (*pkt)

/*
 *  this routine is run as a separate job with a separate context
 *
 *  This job tries to keep its particular UCB as busy as it can.  Thus, when
 *  it is awakened, it locks the UCB data structure, and looks for a TCB to
 *  begin processing.  If there is one (at least), it unlinks it from the list
 *  (at field UCB.tcbs) and calls the appropriate action routine, keeping
 *  track of the current block and direction.  When there are no more TCBs to
 *  process, it looks for a PKT to begin processing (this can happen if a PKT
 *  comes in which contains a sequential command which must be postponed).
 *  If there is one (at least), it unlinks it from the list (at field
 *  UCB.pkts) and calls the main MSCP dispatcher; this time around, there
 *  probably won't be any non-sequential commands outstanding, so the
 *  postponed command will finally be exectuted.  Finally, it blocks itself,
 *  to be awakened when there are again more commands to process (in either of
 *  the fields UCB.tcbs or UCB.pkts).
 */
work( ucb )
register struct $ucb *ucb;
    {
    register struct $tcb *tcb;
    register struct $pkt *pkt;
    struct $pkt *nextpkt;

    while( true )
	{
	/*
	 *  must lock the UCB data structure while fiddling with lists
	 */
	$acquire( &UCB.ucb );
	/*
	 *  loop until all of the pending TCBs are serviced
	 */
	while( ( tcb = UCB.tcb = UCB.tcbs ) != null )
	    {
#if debug>=2
	    printf( "\nTCBs for unit %d, block = %ld, direction = %s",
		    UCB.op_sd & 3, UCB.block, UCB.state & us_up ? "U" : "D" );
	    tcb = &UCB.tcbs;
	    while( ( tcb = TCB.link ) != null )
		if( TCB.modifiers & tm_exp )
		    printf( "\nblock = %ld, express request", TCB.block );
		else
		    printf( "\nblock = %ld, direction = %s", TCB.block,
			    TCB.type & tt_up ? "U" : "D" );
	    tcb = UCB.tcbs;
#endif
	    /*
	     *  we have one, so unlink it and then unlock the UCB
	     */
	    UCB.tcbs = TCB.link;
	    $release( &UCB.ucb );
	    /*
	     *  only keep this information updated for commands which are not
	     *  "express request" (or else INSERT_TCB will get confused)
	     */
	    if( !( TCB.modifiers & tm_exp ) )
		{
		UCB.block = TCB.block;
		if( TCB.type & tt_up )
		    UCB.state |= us_up;
		else
		    UCB.state &= ~us_up;
		}
	    /*
	     *  do the transfer, then relock the UCB
	     */
	    _do_rw( tcb );
	    $acquire( &UCB.ucb );
	    }
	/*
	 *  are there any sequential commands which are pending?  if so, do it
	 */
	if( ( pkt = UCB.pkts ) != null )
	    {
	    /*
	     *  we must be careful to keep our list-head separate from the
	     *  list-head at UCB.pkts, since we could otherwise get really
	     *  screwed up!  (there could be PKTs on the list which need to
	     *  be postponed AGAIN; this means adding them to the UCB.pkts
	     *  list, and we would get stuck in a loop removing and adding the
	     *  same PKT over and over again IF WE USED THAT LIST-HEAD)
	     */
	    UCB.pkts = null;
	    $release( &UCB.ucb );
	    /*
	     *  now rip through our PRIVATE list of pending sequential PKTs
	     */
	    while( pkt != null )
		{
		nextpkt = PKT.link;
		do_mscp( pkt );
		pkt = nextpkt;
		}
	    }
	else
	    $release( &UCB.ucb );
	/*
	 *  all done for a while, so take it easy
	 */
	$block( &UCB.ucb_job );
	}
    }
