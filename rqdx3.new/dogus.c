/*
 *  file = DOGUS.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the GET UNIT STATUS command
 */

#include "defs.h"
#include "pkt.h"
#include "ucb.h"
#include "mscp.h"

extern word unit_base;

extern byte *get_ucb( );

/*
 *  the GET UNIT STATUS command packet
 */
struct $gusc
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_r2;
    word	p_mod;
    };

/*
 *  the GET UNIT STATUS response packet
 */
struct $gusr
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
    word	p_trk;
    word	p_grp;
    word	p_cyl;
    byte	p_usvr;
    byte	p_uhvr;
    word	p_rcts;
    byte	p_rbns;
    byte	p_rctc;
    };

#define		rs_gus		sizeof( struct $gusr )

#define PKT (*pkt)
#define CMD (*(struct $gusc *)&(PKT.data))
#define RSP (*(struct $gusr *)&(PKT.data))
#define UCB (*ucb)

/*
 *  process a GET UNIT STATUS command
 *
 *  This is a sequential command, but since it changes no state, and no non-
 *  sequential commands change state either, we can treat it as if it were
 *  not.  If the "next unit" modifier is set, then what is being inquired
 *  about is either this unit number or the next higher unit number which
 *  exists (and if no more unit numbers exist, wrap the unit number around to
 *  zero).  The command status returned can be either "offline" (for units
 *  which don't exist) "offline, no volume mounted" (for units which have the
 *  run/stop button set to stop or which have no media present), "available"
 *  (for units which could be online but aren't), or "success" (for units
 *  which are currently online to a host).
 */
do_gus( pkt )
register struct $pkt *pkt;
    {
    register word unit;
    register struct $ucb *ucb;

    unit = CMD.p_unit;
#if debug>=1
    printf( "\nGET UNIT STATUS, unit = %d", unit );
#endif
    RSP.p_flgs = 0;
    /*
     *  if the "next unit" modifier is set, then find a unit number which is
     *  between unit_base and unit_base+3 AND which corresponds to a unit
     *  which can potentially be brought online (the UCB for it exists); if
     *  this can't be done, then report on unit 0 instead
     */
    if( CMD.p_mod & md_nxu )
	{
	if( unit < unit_base )
	    unit = unit_base;
	if( ( unit > unit_base+3 ) || ( get_ucb( unit ) == null ) )
	    unit = 0;
#if debug>=1
	if( unit != CMD.p_unit )
	    printf( " (changed to unit = %d)", unit );
#endif
	RSP.p_unit = unit;
	}
    /*
     *  get the corresponding UCB; a failure here gets an "offline" status
     */
    if( ( ucb = get_ucb( unit ) ) == null )
	RSP.p_sts = st_ofl;
    else
	{
	/*
	 *  return the appropriate status (as discussed above)
	 */
	if( UCB.state & us_ofl )
	    RSP.p_sts = st_ofl + st_sub * 1;
	else if( !( UCB.state & us_onl ) )
	    RSP.p_sts = st_avl;
	else
	    RSP.p_sts = st_suc;
	/*
	 *  fill in all of the various response packet fields (these fields
	 *  are required to be valid only if the unit exists)
	 */
	RSP.p_mlun = unit;
	RSP.p_unfl = UCB.flags;
	RSP.p_unti[0] = unit;
	RSP.p_unti[1] = 0;
	RSP.p_unti[2] = 0;
	RSP.p_unti[3] = UCB.type;
	RSP.p_medi[0] = ( ( word * ) &UCB.media )[lsw];
	RSP.p_medi[1] = ( ( word * ) &UCB.media )[msw];
	RSP.p_trk = UCB.spt;
	RSP.p_grp = UCB.tpg;
	RSP.p_cyl = UCB.gpc;
	RSP.p_usvr = 0;
	RSP.p_uhvr = 0;
	RSP.p_rcts = UCB.rctsize;
	/*
	 *  RX devices have no RCT, and RD devices have a single copy RCT
	 *  (this is required of controllers which implement BBR on their own)
	 */
	if( UCB.state & us_rx )
	    {
	    RSP.p_rbns = 0;
	    RSP.p_rctc = 0;
	    }
	else
	    {
	    RSP.p_rbns = 1;
	    RSP.p_rctc = 1;
	    }
	}
    /*
     *  these fields are required to be ALWAYS valid, no matter what the unit
     *  state is (or isn't)
     */
    RSP.p_shun = unit;
    RSP.p_shst = 0;
    RSP.p_opcd |= op_end;
    PKT.size = rs_gus;
    PKT.type = mt_seq;
    put_packet( pkt );
    }
