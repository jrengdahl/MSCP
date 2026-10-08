/*
 *  file = DOUNA.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the UNIT NOW AVAILABLE attention message routine
 */

#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "ucb.h"
#include "mscp.h"

extern list pkts;
extern byte *ucbs[];
extern word unit_base;
extern word ha_flag;
extern struct $ccb _ccb;

extern byte *$deqf_head( );

/*
 *  the UNIT NOW AVAILABLE attention message packet
 */
struct $unae
    {
    long	l_crf;
    word	l_unit;
    word	l_r2;
    byte	l_opcd;
    byte	l_r3;
    word	l_r4;
    word	l_mlun;
    word	l_unfl;
    word	l_r5[2];
    word	l_unti[4];
    word	l_medi[2];
    };

#define		es_una		sizeof( struct $unae )

#define PKT (*pkt)
#define ERR (*(struct $unae *)&(PKT.data))
#define UCB (*ucb)
#define CCB _ccb

/*
 *  this routine handles UNIT NOW AVAILABLE attention message packets
 *
 *  The flow is simple:  if the host cares about these attention message
 *  packets, then allocate one, fill in its fields, and send it to the host.
 *  The only difficult part is in finding the unit number of the UCB in
 *  question, since there is no back-pointer; rather, a linear search must
 *  be done of the table of all known UCBs looking for a match.
 */
do_una( ucb )
register struct $ucb *ucb;
    {
    register word unit;
    register struct $pkt *pkt;

    if( CCB.flags & cf_atn )
	{
	for( unit = 0; ucb != ucbs[unit]; unit++ )
	    ;
	unit += unit_base;
	pkt = $deqf_head( &pkts );
	ERR.l_crf = 0;
	ERR.l_unit = unit;
	ERR.l_opcd = op_ava;
	ERR.l_mlun = unit;
	ERR.l_unfl = UCB.flags;
	ERR.l_unti[0] = unit;
	ERR.l_unti[1] = 0;
	ERR.l_unti[2] = 0;
	ERR.l_unti[3] = UCB.type;
	ERR.l_medi[0] = ( ( word * ) &UCB.media )[lsw];
	ERR.l_medi[1] = ( ( word * ) &UCB.media )[msw];
	PKT.size = es_una;
	PKT.type = mt_seq;
	PKT.connid = ct_mscp;
	ha_flag++;
	put_packet( pkt );
	}
    }
