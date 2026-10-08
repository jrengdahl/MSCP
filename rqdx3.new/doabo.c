/*
 *  file = DOABO.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the ABORT command
 */

#include "defs.h"
#include "pkt.h"
#include "tcb.h"
#include "ucb.h"
#include "mscp.h"

extern list tcbs;

extern byte *get_ucb( );

/*
 *  the ABORT command packet
 */
struct $aboc
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_r2;
    word	p_mod;
    long	p_otrf;
    };

/*
 *  the ABORT response packet
 */
struct $abor
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_flgs;
    word	p_sts;
    long	p_otrf;
    };

/*
 *  the minimum command packet
 */
struct $minc
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_r2;
    word	p_mod;
    };

/*
 *  the minimum response packet
 */
struct $minr
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_flgs;
    word	p_sts;
    };

#define		rs_abo		sizeof( struct $abor )
#define		rs_min		sizeof( struct $minr )

#define PKT (*pkt)
#define CMD (*(struct $aboc *)&(PKT.data))
#define RSP (*(struct $abor *)&(PKT.data))

/*
 *  process an ABORT command
 *
 *  The command to abort can be found in one of three places:  it can be the
 *  current command (tcb = UCB.tcb, pkt = TCB.pkt); it can be on the list of
 *  pending sequential commands (pkt is on list UCB.pkts); or it can be on the
 *  list of pending non-sequential commands (tcb is on list UCB.tcbs, pkt =
 *  TCB.pkt).  If the command is found in any of these places, a response
 *  packet is constructed and returned immediately to the host, the aborted
 *  command is removed from its hiding place, and all resouces allocated to it
 *  (the TCB in particular if one exists) are released.  We assume here that
 *  command reference numbers are assigned uniquely by the host, so that upon
 *  the first match we can stop looking.
 */
do_abo( pkt )
register struct $pkt *pkt;
    {
#if debug>=1
    printf( "\nABORT, unit = %d", CMD.p_unit );
#endif
    RSP.p_flgs = 0;
    RSP.p_sts = st_suc;
    _do_abo( CMD.p_unit, CMD.p_otrf );
    RSP.p_opcd |= op_end;
    PKT.size = rs_abo;
    PKT.type = mt_seq;
    put_packet( pkt );
    }

#define PKT (*pkt)
#define CMD (*(struct $minc *)&(PKT.data))
#define RSP (*(struct $minr *)&(PKT.data))
#define TCB (*tcb)
#define UCB (*ucb)
#define lastPKT (*lastpkt)
#define lastTCB (*lasttcb)

/*
 *  actually process an ABORT command
 */
_do_abo( unit, outref )
word unit;
long outref;
    {
    register struct $pkt *pkt;
    register struct $tcb *tcb;
    register struct $ucb *ucb;
    struct $pkt *lastpkt;
    struct $tcb *lasttcb;

    /*
     *  get the UCB corresponding to the given unit number; a NULL means that
     *  there is none, but we don't get upset at all, we just return quietly
     */
    if( ( ucb = get_ucb( unit ) ) != null )
	{
	/*
	 *  lock the UCB data structure for our personal use
	 */
	$acquire( &UCB.ucb );
	/*
	 *  see if we are aborting the active command
	 */
	tcb = UCB.tcb;
	pkt = TCB.pkt;
	if( outref == CMD.p_crf )
	    {
	    /*
	     *  we got it!  say that it is being aborted
	     */
	    TCB.type |= tt_abo;
	    TCB.type &= ~( tt_cmr|tt_cmw );
	    TCB.count = 0;
	    goto EXIT;
	    }
	/*
	 *  point to the sequential packet list-head, and walk down the list;
	 *  for each PKT, see if the command reference number is the one we
	 *  want, and if so, build the required response packet for it, mail
	 *  it off to the host, and then quit
	 */
	pkt = &UCB.pkts;
	while( ( lastpkt = pkt, pkt = PKT.link ) != null )
	    {
	    if( outref == CMD.p_crf )
		{
		/*
		 *  we got it!  remember to unlink the PKT from the list
		 */
		lastPKT.link = PKT.link;
		RSP.p_flgs = 0;
		RSP.p_sts = st_abo;
		RSP.p_opcd |= op_end;
		PKT.size = rs_min;
		PKT.type = mt_seq;
		put_packet( pkt );
		goto EXIT;
		}
	    }
	/*
	 *  now point to the non-sequential packet list-head, and again walk
	 *  down the list; for each PKT (get this from the TCB), see if the
	 *  command reference number is the one we want, and if so, just like
	 *  before, build the required response packet for it, mail it off to
	 *  the host, and then quit
	 */
	tcb = &UCB.tcbs;
	while( ( lasttcb = tcb, tcb = TCB.link ) != null )
	    {
	    pkt = TCB.pkt;
	    if( outref == CMD.p_crf )
		{
		/*
		 *  we got it!  remember to unlink the TCB from the list
		 */
		lastTCB.link = TCB.link;
		RSP.p_flgs = 0;
		RSP.p_sts = st_abo;
		RSP.p_opcd |= op_end;
		PKT.size = rs_min;
		PKT.type = mt_seq;
		put_packet( pkt );
		/*
		 *  also remember to return this borrowed resource
		 */
		$enq_head( &tcbs, tcb );
		goto EXIT;
		}
	    }
EXIT:
	/*
	 *  since we're all done, unlock the UCB data structure
	 */
	$release( &UCB.ucb );
	}
    }
