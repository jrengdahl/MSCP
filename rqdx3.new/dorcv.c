/*
 *  file = DORCV.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the RECEIVE DATA command
 */

#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "pcb.h"
#include "dup.h"

extern list dma;
extern byte *_packet;
extern struct $ccb _ccb;
extern struct $pcb _pcb;

/*
 *  the RECEIVE DATA command packet
 */
struct $rcvc
    {
    long	p_crf;
    word	p_r1[2];
    byte	p_opcd;
    byte	p_r2;
    word	p_mod;
    word	p_bcnt[2];
    word	p_buff[6];
    };

/*
 *  the RECEIVE DATA response packet
 */
struct $rcvr
    {
    long	p_crf;
    word	p_r1[2];
    byte	p_opcd;
    byte	p_r2;
    word	p_sts;
    word	p_bcnt[2];
    };

#define		rs_rcv		sizeof( struct $rcvr )

#define PKT (*pkt)
#define CMD (*(struct $rcvc *)&(PKT.data))
#define RSP (*(struct $rcvr *)&(PKT.data))
#define CCB _ccb
#define PCB _pcb

/*
 *  process a RECEIVE DATA command
 *
 *  This command sends data to the host; it is half of the required means of
 *  communication.  A DUP program must be running, else this is an invalid
 *  command.  The DUP program signals that it is ready to send data by placing
 *  the address of a buffer into the PCB and setting the "receive data" state;
 *  if there is no address there, then this command is held for later and
 *  control is returned.  If an address is present, then we copy some bytes
 *  (the number of bytes copied is the smaller of the two byte counts).
 */
do_rcv( pkt )
register struct $pkt *pkt;
    {
    long count, qbus;

#if debug>=1
    printf( "\nRECEIVE DATA" );
#endif
    /*
     *  we must be running a DUP program
     */
    if( CCB.state & cs_dup )
	{
	/*
	 *  is that program ready to send data?  if not, merely queue this PKT
	 */
	if( PCB.buffer == null )
	    {
	    $enq_tail( &PCB.pkts, pkt );
	    return;
	    }
	/*
	 *  get the size and location of the host buffer, and minimize the
	 *  size with the size of the DUP program's buffer
	 */
	( ( word * ) &count )[lsw] = CMD.p_bcnt[0];
	( ( word * ) &count )[msw] = CMD.p_bcnt[1];
	( ( word * ) &qbus )[lsw] = CMD.p_buff[0];
	( ( word * ) &qbus )[msw] = CMD.p_buff[1];
	if( count < 2 )
	    RSP.p_sts = st_cmd + i_bcnt;
	/*
	 *  if the DUP program isn't in the "receive data" state, complain
	 */
	else if( !( PCB.state & ps_rcv ) )
	    RSP.p_sts = st_cmd + i_opcd;
	else
	    {
	    if( count < PCB.count )
		PCB.count = count;
	    /*
	     *  lock the DMA engine, copy the bytes, and then release it
	     */
	    $acquire( &dma );
	    _packet = pkt;
	    put_buffer( qbus, PCB.buffer, PCB.count );
	    $release( &dma );
	    /*
	     *  clear the "receive data" state, and zap the buffer address
	     */
	    PCB.state &= ~ps_rcv;
	    PCB.buffer = null;
	    RSP.p_bcnt[0] = PCB.count;
	    RSP.p_bcnt[1] = 0;
	    RSP.p_sts = st_suc;
	    }
	}
    else
	RSP.p_sts = st_cmd;
    RSP.p_opcd |= op_end;
    PKT.size = rs_rcv;
    PKT.type = mt_seq;
    put_packet( pkt );
    }
