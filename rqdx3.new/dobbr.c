/*
 *  file = DOBBR.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the BAD BLOCK REPLACEMENT error log routine
 */

#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "tcb.h"
#include "ucb.h"
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
 *  the BAD BLOCK REPLACEMENT error log packet
 */
struct $bbre
    {
    long	l_crf;
    word	l_unit;
    word	l_seq;
    byte	l_fmt;
    byte	l_flgs;
    word	l_evnt;
    word	l_cnti[4];
    byte	l_csvr;
    byte	l_chvr;
    word	l_mlun;
    word	l_unti[4];
    byte	l_usvr;
    byte	l_uhvr;
    word	l_rpfl;
    word	l_vser[2];
    word	l_lbn[2];
    word	l_orbn[2];
    word	l_nrbn[2];
    word	l_rpev;
    };

#define		es_bbr		sizeof( struct $bbre )

#define PKT (*pkt)
#define ERR (*(struct $bbre *)&(PKT.data))
#define CMD (*(struct $minc *)&((*TCB.pkt).data))
#define RSP (*(struct $minr *)&((*TCB.pkt).data))
#define TCB (*tcb)
#define UCB (*ucb)
#define CCB _ccb

/*
 *  this routine handles BAD BLOCK REPLACEMENT error log packets
 *
 *  The flow is simple:  if the host cares about these error log packets, then
 *  allocate one, fill in its fields, and send it to the host.  Also, set the
 *  "error log generated" flag in the original response packet, to let the host
 *  know what to expect.
 */
do_bbr( tcb, lbn, ddm, replaced )
register struct $tcb *tcb;
long lbn;
bool ddm, replaced;
    {
    register struct $pkt *pkt;
    register struct $ucb *ucb;

    if( CCB.flags & cf_ths )
	{
	pkt = $deqf_head( &pkts );
	ucb = TCB.ucb;
	ERR.l_crf = CMD.p_crf;
	ERR.l_unit = CMD.p_unit;
	ERR.l_seq = 0;
	ERR.l_fmt = fm_rpl;
	ERR.l_flgs = lf_suc|lf_snr;
	ERR.l_evnt = st_bbr;
	ERR.l_cnti[0] = 0;
	ERR.l_cnti[1] = 0;
	ERR.l_cnti[2] = 0;
	ERR.l_cnti[3] = CCB.type;
	ERR.l_csvr = rqdx3_softv;
	ERR.l_chvr = rqdx3_hardv;
	ERR.l_mlun = CMD.p_unit;
	ERR.l_unti[0] = CMD.p_unit;
	ERR.l_unti[1] = 0;
	ERR.l_unti[2] = 0;
	ERR.l_unti[3] = UCB.type;
	ERR.l_usvr = 0;
	ERR.l_uhvr = 0;
	ERR.l_rpfl = 0;
	ERR.l_vser[0] = ( ( word * ) &UCB.volume )[lsw];
	ERR.l_vser[1] = ( ( word * ) &UCB.volume )[msw];
	ERR.l_lbn[0] = ( ( word * ) &lbn )[lsw];
	ERR.l_lbn[1] = ( ( word * ) &lbn )[msw];
	ERR.l_orbn[0] = 0;
	ERR.l_orbn[1] = 0;
	ERR.l_nrbn[0] = 0;
	ERR.l_nrbn[1] = 0;
	ERR.l_rpev = st_dat + st_sub * 7;
	if( replaced )
	    {
	    ERR.l_rpfl |= lfr_rp|lfr_te;
	    if( ddm )
		ERR.l_rpfl |= lfr_fe;
	    if( TCB.oldrbn >= 0 )
		{
		ERR.l_rpfl |= lfr_br;
		ERR.l_orbn[0] = ( ( word * ) &TCB.oldrbn )[lsw];
		ERR.l_orbn[1] = ( ( word * ) &TCB.oldrbn )[msw];
		}
	    ERR.l_nrbn[0] = ( ( word * ) &TCB.newrbn )[lsw];
	    ERR.l_nrbn[1] = ( ( word * ) &TCB.newrbn )[msw];
	    }
	RSP.p_flgs |= ef_log;
	PKT.size = es_bbr;
	PKT.type = mt_dat;
	PKT.connid = ct_mscp;
	ha_flag++;
	put_packet( pkt );
	}
    }
