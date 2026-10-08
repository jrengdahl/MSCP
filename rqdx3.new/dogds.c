/*
 *  file = DOGDS.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the GET DUP STATUS command
 */

#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "pcb.h"
#include "dup.h"

extern struct $ccb _ccb;
extern struct $pcb _pcb;

/*
 *  the GET DUP STATUS command packet
 */
struct $gdsc
    {
    long	p_crf;
    word	p_r1[2];
    byte	p_opcd;
    byte	p_r2;
    word	p_mod;
    };

/*
 *  the GET DUP STATUS response packet
 */
struct $gdsr
    {
    long	p_crf;
    word	p_r1[2];
    byte	p_opcd;
    byte	p_r2;
    word	p_sts;
    byte	p_ext[3];
    byte	p_flgs;
    word	p_work[2];
    word	p_tmo;
    };

#define		rs_gds		sizeof( struct $gdsr )

#define PKT (*pkt)
#define RSP (*(struct $gdsr *)&(PKT.data))
#define CCB _ccb
#define PCB _pcb

/*
 *  process a GET DUP STATUS command
 *
 *  There isn't much to this command.  A response packet with the required
 *  information is built and sent to the host.  Included in this is a field
 *  which contains a measure of the "work" done by any running DUP program;
 *  the host checks this occasionally to make sure that nothing has gone wrong.
 */
do_gds( pkt )
register struct $pkt *pkt;
    {
#if debug>=1
    printf( "\nGET DUP STATUS" );
#endif
    RSP.p_sts = st_suc;
    RSP.p_ext[0] = 'R';
    RSP.p_ext[1] = 'Q';
    RSP.p_ext[2] = '3';
    RSP.p_flgs = cf_sta|cf_lmp;
    /*
     *  if we are running a DUP program, show that the state is "active"
     */
    if( CCB.state & cs_dup )
	RSP.p_flgs |= cf_act;
    /*
     *  here is that "work" value (which should always increase)
     */
    RSP.p_work[0] = ( ( word * ) &PCB.work )[lsw];
    RSP.p_work[1] = ( ( word * ) &PCB.work )[msw];
    RSP.p_tmo = PCB.timeout;
    RSP.p_opcd |= op_end;
    PKT.size = rs_gds;
    PKT.type = mt_seq;
    put_packet( pkt );
    }
