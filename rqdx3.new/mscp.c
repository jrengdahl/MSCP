/*
 *  file = MSCP.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the MSCP dispatcher
 */

#include "defs.h"
#include "pkt.h"
#include "mscp.h"

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

#define		rs_min		sizeof( struct $minr )

#define PKT (*pkt)
#define CMD (*(struct $minc *)&(PKT.data))
#define RSP (*(struct $minr *)&(PKT.data))

/*
 *  this routine will process active packets
 *
 *  This routine is called in two places:  from the POLL routine whenever a
 *  packet is received which has a connection identifier of "MSCP", and from
 *  a UCB's WORK routine when running down a list of PKTs (from the UCB.pkts
 *  field) which needs to be reparsed after having been initially deferred.
 *  If a matching opcode is found, then the associated action routine is
 *  invoked, else an "invalid command" status is returned.  Note that there are
 *  some routines which require no controller action (they are NOPs) and thus
 *  they automatically get a status of "success".  Further, the transfer
 *  opcodes all invoke the same action routine, and all parsing is done there.
 */
do_mscp( pkt )
register struct $pkt *pkt;
    {
    switch( CMD.p_opcd )
	{
	case op_abo:
	    do_abo( pkt );
	    return;
	case op_avl:
	    do_avl( pkt );
	    return;
	case op_fmt:
	    do_fmt( pkt );
	    return;
	case op_gcs:
	    do_gcs( pkt );
	    return;
	case op_gus:
	    do_gus( pkt );
	    return;
	case op_onl:
	    do_onl( pkt );
	    return;
	case op_scc:
	    do_scc( pkt );
	    return;
	case op_suc:
	    do_suc( pkt );
	    return;
	case op_acc:
	case op_cmp:
	case op_ers:
	case op_rd:
	case op_wr:
	    do_rw( pkt );
	    return;
	case op_ccd:
	case op_dap:
	case op_flu:
	    RSP.p_opcd |= op_end;
	    RSP.p_flgs = 0;
	    RSP.p_sts = st_suc;
	    break;
	default:
	    RSP.p_opcd = op_end;
	    RSP.p_flgs = 0;
	    RSP.p_sts = st_cmd + i_opcd;
	    break;
	}
    PKT.size = rs_min;
    PKT.type = mt_seq;
    put_packet( pkt );
    }
