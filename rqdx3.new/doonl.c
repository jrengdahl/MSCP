/*
 *  file = DOONL.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the ONLINE command
 */

#include "defs.h"
#include "pkt.h"
#include "tcb.h"
#include "ucb.h"
#include "mscp.h"

extern list mem;
extern list tcbs;
extern word fpl;

extern byte *$deqf_head( );
extern byte *get_ucb( );
extern word put_rbn( );
extern word size_media( );

/*
 *  the ONLINE command packet
 */
struct $onlc
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
 *  the ONLINE response packet
 */
struct $onlr
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

#define		rs_onl		sizeof( struct $onlr )

#define PKT (*pkt)
#define CMD (*(struct $onlc *)&(PKT.data))
#define RSP (*(struct $onlr *)&(PKT.data))
#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  process an ONLINE command
 *
 *  This is a sequential command, so if there are any non-sequential commands
 *  outstanding, we must hold this command pending until they are complete; if
 *  not (the UCB.tcbs list is empty), then we can attempt to actually bring
 *  the unit online.  The unit must actually exist (and be either an RD or an
 *  RX), and it must not be offline (i.e., if an RD, the run/stop button must
 *  not be pressed; if an RX, there must be media inserted and the door must
 *  be closed).  If the unit is an RD, then unless the "ignore media format"
 *  modifier is set an integrity check of the RCT is performed (see the file
 *  CIBBR.C for details); if this check fails, a status of "media format
 *  error" is returned.  If the unit is an RX, then an attempt is made to
 *  determine the density of the media inserted; if this fails, a status of
 *  "media format error" is returned.  Host settable flags are set, including
 *  software write protect (if enabled), and the unit is marked online.  This
 *  command returns certain media-dependent information to the host as its
 *  final step.
 */
do_onl( pkt )
register struct $pkt *pkt;
    {
    register struct $tcb *tcb;
    register struct $ucb *ucb;
    word unit, mod, error;

    unit = CMD.p_unit;
    mod = CMD.p_mod;
#if debug>=1
    printf( "\nONLINE, unit = %d", unit );
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
	if( UCB.state & us_mfe )
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
	    /*
	     *  grab a TCB, and die if none available
	     */
	    tcb = $deqf_head( &tcbs );
	    TCB.ucb = ucb;
	    TCB.pkt = pkt;
	    /*
	     *  acquire exclusive use of the memory buffer area
	     */
	    $acquire( &mem );
	    /*
	     *  for RD devices, do the sanity check on the RCT unless we are
	     *  explicitly asked not to; return an error if appropriate
	     */
	    if( ( UCB.state & us_rd ) && !( mod & md_imf )
		    && ( error = put_rbn( tcb, ( long ) -1, false ) ) )
		RSP.p_sts = error;
	    /*
	     *  for RX devices, size the media present
	     */
	    else if( ( UCB.state & us_rx ) && ( error = size_media( ucb ) ) )
		RSP.p_sts = error;
	    else
		{
		RSP.p_sts = st_suc;
		/*
		 *  if the unit is already online, say so; otherwise, update
		 *  the unit flags and set the unit state to online
		 */
		if( UCB.state & us_onl )
		    RSP.p_sts |= st_sub * 8;
		else
		    {
		    UCB.flags &= ( uf_wph|uf_rpl|uf_rmv );
		    UCB.flags |= CMD.p_unfl & uf_msk;
		    /*
		     *  the host has to say pretty please to set the software
		     *  write protect flag; see if the host did
		     */
		    if( ( mod & md_swp ) && ( CMD.p_unfl & uf_wps ) )
			{
			UCB.flags |= uf_wps;
			fpl |= UCB.wp_bit;
			}
		    UCB.state |= us_onl;
		    /*
		     *  if the host set the "ignore media format" modifier,
		     *  remember it (it has dire consequences since it turns
		     *  off revectoring and replacement as well)
		     */
		    if( mod & md_imf )
			UCB.state |= us_imf;
		    }
		}
	    /*
	     *  release those precious resources
	     */
	    $release( &mem );
	    $enq_head( &tcbs, tcb );
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
    PKT.size = rs_onl;
    PKT.type = mt_seq;
    put_packet( pkt );
    }
