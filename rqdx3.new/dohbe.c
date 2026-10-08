/*
 *  file = DOHBE.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the HOST BUFFER ERROR error log routine
 */

#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "tcb.h"
#include "mscp.h"

extern list pkts;
extern word ha_flag;
extern struct $ccb _ccb;

extern byte *$deqf_head( );

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

/*
 *  the HOST BUFFER ERROR error log packet
 */
struct $hbee
    {
    long	l_crf;
    word	l_r1;
    word	l_seq;
    byte	l_fmt;
    byte	l_flgs;
    word	l_evnt;
    word	l_cnti[4];
    byte	l_csvr;
    byte	l_chvr;
    word	l_r2;
    word	l_badr[2];
    };

#define		es_hbe		sizeof( struct $hbee )

#define PKT (*pkt)
#define ERR (*(struct $hbee *)&(PKT.data))
#define CMD (*(struct $minc *)&((*TCB.pkt).data))
#define RSP (*(struct $minr *)&((*TCB.pkt).data))
#define TCB (*tcb)
#define CCB _ccb

/*
 *  this routine handles HOST BUFFER ERROR error log packets
 *
 *  The flow is simple:  if the host cares about these error log packets, then
 *  allocate one, fill in its fields, and send it to the host.  Also, set the
 *  "error log generated" flag in the original response packet, to let the host
 *  know what to expect.
 */
do_hbe( tcb, error )
register struct $tcb *tcb;
word error;
    {
    register struct $pkt *pkt;
    register struct $ucb *ucb;

    if( CCB.flags & cf_ths )
	{
	pkt = $deqf_head( &pkts );
	ucb = TCB.ucb;
	ERR.l_crf = CMD.p_crf;
	ERR.l_seq = 0;
	ERR.l_fmt = fm_bad;
	ERR.l_flgs = lf_snr;
	ERR.l_evnt = error;
	ERR.l_cnti[0] = 0;
	ERR.l_cnti[1] = 0;
	ERR.l_cnti[2] = 0;
	ERR.l_cnti[3] = CCB.type;
	ERR.l_csvr = rqdx3_softv;
	ERR.l_chvr = rqdx3_hardv;
	ERR.l_badr[0] = ( ( word * ) &TCB.qbus )[lsw];
	ERR.l_badr[1] = ( ( word * ) &TCB.qbus )[msw];
	RSP.p_flgs |= ef_log;
	PKT.size = es_hbe;
	PKT.type = mt_dat;
	PKT.connid = ct_mscp;
	ha_flag++;
	put_packet( pkt );
	}
    }
