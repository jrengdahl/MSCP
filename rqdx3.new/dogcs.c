/*
 *  file = DOGCS.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the GET COMMAND STATUS command
 */

#include "defs.h"
#include "pkt.h"
#include "tcb.h"
#include "ucb.h"
#include "mscp.h"

extern byte *get_ucb( );

/*
 *  the GET COMMAND STATUS command packet
 */
struct $gcsc
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
 *  the GET COMMAND STATUS response packet
 */
struct $gcsr
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_flgs;
    word	p_sts;
    long	p_otrf;
    word	p_cmst[2];
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

#define		rs_gcs		sizeof( struct $gcsr )

#define PKT (*pkt)
#define CMD (*(struct $gcsc *)&(PKT.data))
#define RSP (*(struct $gcsr *)&(PKT.data))

/*
 *  process a GET COMMAND STATUS command
 *
 *  The current command (tcb = UCB.tcb, pkt = TCB.pkt) is the only command
 *  which will have a status worth looking at, since for each unit, only one
 *  command is "active" at a time.  Thus, if the command reference number
 *  given does not match that of the current command, then the returned status
 *  is zero, else it is the remaining byte count.
 */
do_gcs( pkt )
register struct $pkt *pkt;
    {
    RSP.p_flgs = 0;
    RSP.p_sts = st_suc;
    _do_gcs( CMD.p_unit, CMD.p_otrf, RSP.p_cmst );
    RSP.p_opcd |= op_end;
    PKT.size = rs_gcs;
    PKT.type = mt_seq;
    put_packet( pkt );
    }

#define PKT (*pkt)
#define CMD (*(struct $minc *)&(PKT.data))
#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  actually process a GET COMMAND STATUS command
 */
_do_gcs( unit, outref, status )
word unit;
long outref;
word status[2];
    {
    register struct $pkt *pkt;
    register struct $tcb *tcb;
    register struct $ucb *ucb;

    /*
     *  assume we won't find anything useful to the host
     */
    status[0] = 0;
    status[1] = 0;
    /*
     *  get the UCB corresponding to the given unit number; a NULL means that
     *  there is none, but it's no big deal, we just pretend it never happened
     */
    if( ( ucb = get_ucb( unit ) ) != null )
	{
	/*
	 *  lock the UCB data structure for our personal use
	 */
	$acquire( &UCB.ucb );
	/*
	 *  if there is a current TCB and its PKT contains the right command
	 *  reference number, then extract and return the remaining byte count
	 */
	if( ( tcb = UCB.tcb ) != null )
	    {
	    pkt = TCB.pkt;
	    if( outref == CMD.p_crf )
		{
		status[0] = ( ( word * ) &TCB.count )[lsw];
		status[1] = ( ( word * ) &TCB.count )[msw];
		}
	    }
	/*
	 *  be a good boy and unlock the UCB data structure
	 */
	$release( &UCB.ucb );
	}
    }
