/*
 *  file = DOBBR.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the DISK TRANSFER ERROR error log routine
 */

#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "tcb.h"
#include "ucb.h"
#include "mscp.h"

extern list pkts;
extern word udc$bug;
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
 *  the DISK TRANSFER ERROR error log packet
 */
struct $dtee
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
    word	l_scyl;
    word	l_vser[2];
    byte	l_stat;
    byte	l_chip;
    byte	l_disk;
    byte	l_csec;
    byte	l_csur;
    byte	l_ccyl;
    };

#define		es_dte		sizeof( struct $dtee )

#define PKT (*pkt)
#define ERR (*(struct $dtee *)&(PKT.data))
#define CMD (*(struct $minc *)&((*TCB.pkt).data))
#define RSP (*(struct $minr *)&((*TCB.pkt).data))
#define TCB (*tcb)
#define UCB (*ucb)
#define CCB _ccb

/*
 *  this routine handles DISK TRANSFER ERROR error log packets
 *
 *  The flow is simple:  if the host cares about these error log packets, then
 *  allocate one, fill in its fields, and send it to the host.  Also, set the
 *  "error log generated" flag in the original response packet, to let the host
 *  know what to expect.
 */
do_dte( tcb, error )
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
	ERR.l_unit = CMD.p_unit;
	ERR.l_seq = 0;
	ERR.l_fmt = fm_sde;
	ERR.l_flgs = ( TCB.fatal ? lf_snr : ( lf_suc|lf_snr ) );
	ERR.l_evnt = error;
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
	ERR.l_scyl = TCB.cylinder;
	ERR.l_vser[0] = ( ( word * ) &UCB.volume )[lsw];
	ERR.l_vser[1] = ( ( word * ) &UCB.volume )[msw];
	ERR.l_stat = TCB.stat;
	ERR.l_chip = TCB.chip;
	ERR.l_disk = TCB.disk + ( udc$bug << 3 );
	ERR.l_csec = TCB.csec;
	ERR.l_csur = TCB.csur;
	ERR.l_ccyl = TCB.ccyl;
	RSP.p_flgs |= ef_log;
	PKT.size = es_dte;
	PKT.type = mt_dat;
	PKT.connid = ct_mscp;
	ha_flag++;
	put_packet( pkt );
	}
    }
