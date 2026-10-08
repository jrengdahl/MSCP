/*
 *  file = DOESP.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the EXECUTE SUPPLIED PROGRAM command
 */

#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "pcb.h"
#include "prog.h"
#include "dup.h"

extern list dma;
extern list mem;
extern byte *_packet;
extern byte data[];
extern struct $ccb _ccb;
extern struct $pcb _pcb;

/*
 *  the EXECUTE SUPPLIED PROGRAM command packet
 */
struct $espc
    {
    long	p_crf;
    word	p_r1[2];
    byte	p_opcd;
    byte	p_r2;
    word	p_mod;
    word	p_bcnt[2];
    word	p_buff[6];
    word	p_olay[6];
    };

/*
 *  the EXECUTE SUPPLIED PROGRAM response packet
 */
struct $espr
    {
    long	p_crf;
    word	p_r1[2];
    byte	p_opcd;
    byte	p_r2;
    word	p_sts;
    };

#define		rs_esp		sizeof( struct $espr )

#define PKT (*pkt)
#define CMD (*(struct $espc *)&(PKT.data))
#define RSP (*(struct $espr *)&(PKT.data))
#define CCB _ccb
#define PCB _pcb

/*
 *  process an EXECUTE SUPPLIED PROGRAM command
 *
 *  When we get this command, we must not already be running a DUP program; if
 *  we are, then this command is invalid.  Else, we copy the buffer specified
 *  from the Q-bus to local memory.  This buffer contains the instructions and
 *  data required by the DUP program.  The PCB is then filled in with enough
 *  information to begin running this program, and we set the state to
 *  "beginning".  This will cause the program to be invoked when the controller
 *  is next idle (see the idle loop in MAIN.C for more information).
 */
do_esp( pkt )
register struct $pkt *pkt;
    {
    long count, qbus;

#if debug>=1
    printf( "\nEXECUTE SUPPLIED PROGRAM" );
#endif
    /*
     *  we can't already be running a program!
     */
    if( !( CCB.state & cs_dup ) )
	{
	/*
	 *  get the size and location of the specified buffer; it can't be
	 *  either too small or too large
	 */
	( ( word * ) &count )[lsw] = CMD.p_bcnt[0];
	( ( word * ) &count )[msw] = CMD.p_bcnt[1];
	( ( word * ) &qbus )[lsw] = CMD.p_buff[0];
	( ( word * ) &qbus )[msw] = CMD.p_buff[1];
	if( ( count < 0 ) || ( count >= data_size ) )
	    RSP.p_sts = st_cmd + i_bcnt;
	else
	    {
	    /*
	     *  copy the buffer specified to local memory; remember to grab
	     *  total control of the DMA engine while the copy is in progress
	     *
	     *  see the DUP specification for a description of the buffer
	     */
	    $acquire( &mem );
	    $acquire( &dma );
	    _packet = pkt;
	    get_buffer( qbus, data, ( word ) count );
	    $release( &dma );
	    /*
	     *  a "standalone" program must be invoked with the "standalone"
	     *  modifier
	     */
	    if( ( data[16] & pf_sta ) && !( CMD.p_mod & md_sta ) )
		{
		$release( &mem );
		RSP.p_sts = st_sta;
		}
	    else
		{
		/*
		 *  we set the controller state to show that a DUP program
		 *  is running, and we set the program state to show that it
		 *  is ready to begin
		 */
		RSP.p_sts = st_suc;
#if debug>=1
		printf( ", name = %0.6s", &data[8] );
#endif
		CCB.state |= cs_dup;
		PCB.program = &data[18];
		PCB.work = 0;
		PCB.state = ps_beg;
		PCB.timeout = data[17];
		}
	    }
	}
    else
	RSP.p_sts = st_cmd;
    RSP.p_opcd |= op_end;
    PKT.size = rs_esp;
    PKT.type = mt_seq;
    put_packet( pkt );
    }
