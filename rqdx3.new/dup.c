/*
 *  file = DUP.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the DUP dispatcher
 */

#include "defs.h"
#include "pkt.h"
#include "dup.h"

/*
 *  the minimum command packet
 */
struct $minc
    {
    long	p_crf;
    word	p_r1[2];
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
    word	p_r1[2];
    byte	p_opcd;
    byte	p_r2;
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
 *  packet is received which has a connection identifier of "DUP", and from
 *  the SEND DATA and RECEIVE DATA routines when running down a list of PKTs
 *  (from the PCB.pkts field) which needs to be reparsed after having been
 *  initially deferred.  If a matching opcode is found, then the associated
 *  action routine is invoked, else an "invalid command" status is returned.
 */
do_dup( pkt )
register struct $pkt *pkt;
    {
    switch( CMD.p_opcd )
	{
	case op_ap:
	    do_ap( pkt );
	    return;
	case op_elp:
	    do_elp( pkt );
	    return;
	case op_esp:
	    do_esp( pkt );
	    return;
	case op_gds:
	    do_gds( pkt );
	    return;
	case op_rcv:
	    do_rcv( pkt );
	    return;
	case op_snd:
	    do_snd( pkt );
	    return;
	default:
	    RSP.p_sts = st_cmd + i_opcd;
	    break;
	}
    RSP.p_opcd = op_end;
    PKT.size = rs_min;
    PKT.type = mt_seq;
    put_packet( pkt );
    }
