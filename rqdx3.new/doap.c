/*
 *  file = DOAP.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the ABORT PROGRAM command
 */

#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "pcb.h"
#include "dup.h"

extern struct $ccb _ccb;
extern struct $pcb _pcb;

/*
 *  the ABORT PROGRAM command packet
 */
struct $apc
    {
    long	p_crf;
    word	p_r1[2];
    byte	p_opcd;
    byte	p_r2;
    word	p_mod;
    };

/*
 *  the ABORT PROGRAM response packet
 */
struct $apr
    {
    long	p_crf;
    word	p_r1[2];
    byte	p_opcd;
    byte	p_r2;
    word	p_sts;
    };

#define		rs_ap		sizeof( struct $apr )

#define PKT (*pkt)
#define RSP (*(struct $apr *)&(PKT.data))
#define CCB _ccb
#define PCB _pcb

/*
 *  process an ABORT PROGRAM command
 *
 *  We merely have to make certain that a DUP program is running (either local
 *  or supplied), and then set the "abort" state in the PCB.
 */
do_ap( pkt )
register struct $pkt *pkt;
    {
#if debug>=1
    printf( "\nABORT PROGRAM" );
#endif
    /*
     *  if we are running a program, set the "abort" state (if no program is
     *  running, this is an invalid command!)
     */
    if( CCB.state & cs_dup )
	{
	PCB.state |= ps_abo;
	RSP.p_sts = st_suc;
	}
    else
	RSP.p_sts = st_cmd;
    RSP.p_opcd |= op_end;
    PKT.size = rs_ap;
    PKT.type = mt_seq;
    put_packet( pkt );
    }
