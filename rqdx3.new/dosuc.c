/*
 *  file = DOSUC.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the SET UNIT CHARACTERISTICS command
 */

#include "defs.h"
#include "pkt.h"
#include "ucb.h"
#include "mscp.h"

extern word fpl;

extern byte *get_ucb( );

/*
 *  the SET UNIT CHARACTERISTICS command packet
 */
struct $succ
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_r2;
    word	p_mod;
    word	p_r3;
    word	p_unfl;
    word	p_r4[6];
    word	p_dvpm[2];
    word	p_shun;
    word	p_cpsp;
    };

/*
 *  the SET UNIT CHARACTERISTICS response packet
 */
struct $sucr
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_flgs;
    word	p_sts;
    word	p_mlun;
    word	p_unfl;
    word	p_r2[2];
    word	p_unti[4];
    word	p_medi[2];
    word	p_shun;
    word	p_shst;
    word	p_unsz[2];
    word	p_vser[2];
    };

#define		rs_suc		sizeof( struct $sucr )

#define PKT (*pkt)
#define CMD (*(struct $succ *)&(PKT.data))
#define RSP (*(struct $sucr *)&(PKT.data))
#define UCB (*ucb)

/*
 *  process a SET UNIT CHARACTERISTICS command
 *
 *  This is a sequential command, so if there are any non-sequential commands
 *  outstanding, we must hold this command pending until they are complete; if
 *  not (the UCB.tcbs list is empty), and if the unit actually exists and is
 *  not offline, then new values for the unit flags are set.  Like the ONLINE
 *  command, this command returns certain media-dependent information to the
 *  host as its final step.
 */
do_suc( pkt )
register struct $pkt *pkt;
    {
    register word unit;
    register struct $ucb *ucb;
    word mod;

    unit = CMD.p_unit;
    mod = CMD.p_mod;
#if debug>=1
    printf( "\nSET UNIT CHARACTERISTICS, unit = %d", unit );
#endif
    RSP.p_flgs = 0;
    /*
     *  convert the unit number into a UCB; if anything is bogus, crap out
     */
    if( ( ucb = get_ucb( unit ) ) == null )
	RSP.p_sts = st_ofl;
    else
	{
	/*
	 *  lock the UCB data structure for our personal use
	 */
	$acquire( &UCB.ucb );
	/*
	 *  return a status of "media format error, unit not formatted" or
	 *  "offline, no volume mounted" if either is appropriate
	 */
	if( !( UCB.state & ( us_rd|us_rx ) ) )
	    RSP.p_sts = st_mfe + st_sub * 6;
	else if( UCB.state & us_ofl )
	    RSP.p_sts = st_ofl + st_sub * 1;
	else
	    {
	    /*
	     *  any non-sequential commands in progress?  if so, simply add
	     *  this PKT to the end of the pending sequential PKTs list, and
	     *  we will get back to it eventually
	     */
	    if( ( UCB.tcb != null ) || ( UCB.tcbs != null ) )
		{
		$enq_tail( &UCB.pkts, pkt );
		$release( &UCB.ucb );
		return;
		}
	    RSP.p_sts = st_suc;
	    /*
	     *  update the unit flags
	     */
	    UCB.flags &= ( uf_wph|uf_rpl|uf_rmv );
	    UCB.flags |= CMD.p_unfl & uf_msk;
	    if( ( mod & md_swp ) && ( CMD.p_unfl & uf_wps ) )
		{
		UCB.flags |= uf_wps;
		fpl |= UCB.wp_bit;
		}
	    }
	/*
	 *  fill in all of those silly fields in the response packet
	 */
	RSP.p_mlun = unit;
	RSP.p_unfl = UCB.flags;
	RSP.p_unti[0] = unit;
	RSP.p_unti[1] = 0;
	RSP.p_unti[2] = 0;
	RSP.p_unti[3] = UCB.type;
	RSP.p_medi[0] = ( ( word * ) &UCB.media )[lsw];
	RSP.p_medi[1] = ( ( word * ) &UCB.media )[msw];
	RSP.p_unsz[0] = ( ( word * ) &UCB.hostsize )[lsw];
	RSP.p_unsz[1] = ( ( word * ) &UCB.hostsize )[msw];
	RSP.p_vser[0] = ( ( word * ) &UCB.volume )[lsw];
	RSP.p_vser[1] = ( ( word * ) &UCB.volume )[msw];
	/*
	 *  unlock the UCB data structure so someone else can use it
	 */
	$release( &UCB.ucb );
	}
    /*
     *  no matter what, make these fields valid
     */
    RSP.p_shun = unit;
    RSP.p_shst = 0;
    RSP.p_opcd |= op_end;
    PKT.size = rs_suc;
    PKT.type = mt_seq;
    put_packet( pkt );
    }
