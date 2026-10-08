/*
 *  file = DOPLF.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the PORT LAST FAILURE error log routine
 */

#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "mscp.h"

extern list pkts;
extern word ha_flag;
extern struct $ccb _ccb;

extern byte *$deqf_head( );

/*
 *  the PORT LAST FAILURE error log packet
 */
struct $plfe
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
    word	l_perr;
    };

#define		es_plf		sizeof( struct $plfe )

#define PKT (*pkt)
#define ERR (*(struct $plfe *)&(PKT.data))
#define CCB _ccb

/*
 *  this routine handles PORT LAST FAILURE error log packets
 *
 *  The flow is simple:  if an error occurred during a previous incarnation,
 *  then allocate an error log packet, fill in its fields (including the
 *  previous SA error code), and send it to the host.  This is the only packet
 *  sent to the host with a packet connection identifier of "diagnostic".
 */
do_plf( error )
word error;
    {
    register struct $pkt *pkt;

    pkt = $deqf_head( &pkts );
    ERR.l_crf = 0;
    ERR.l_unit = 0;
    ERR.l_seq = 0;
    ERR.l_fmt = fm_cnt;
    ERR.l_flgs = lf_snr;
    ERR.l_evnt = st_cnt;
    ERR.l_cnti[0] = 0;
    ERR.l_cnti[1] = 0;
    ERR.l_cnti[2] = 0;
    ERR.l_cnti[3] = CCB.type;
    ERR.l_csvr = rqdx3_softv;
    ERR.l_chvr = rqdx3_hardv;
    ERR.l_perr = error;
    PKT.size = es_plf;
    PKT.type = mt_dat;
    PKT.connid = ct_diag;
    ha_flag++;
    put_packet( pkt );
    }
