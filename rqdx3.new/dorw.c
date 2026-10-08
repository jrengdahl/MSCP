/*
 *  file = DORW.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the READ, WRITE, COMPARE, ACCESS, and ERASE commands
 */

#include "defs.h"
#include "pkt.h"
#include "tcb.h"
#include "ucb.h"
#include "mscp.h"

extern list tcbs;

extern byte *$deqf_head( );
extern byte *get_ucb( );
extern word ok_rd( );
extern word ok_wr( );
extern word rd_cmd( );
extern word wr_cmd( );
extern word cmp_cmd( );

/*
 *  the READ, WRITE, COMPARE, ACCESS, or ERASE command packet
 */
struct $rwc
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_r2;
    word	p_mod;
    word	p_bcnt[2];
    word	p_buff[6];
    word	p_lbn[2];
    };

/*
 *  the READ, WRITE, COMPARE, ACCESS, or ERASE response packet
 */
struct $rwr
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_flgs;
    word	p_sts;
    word	p_bcnt[2];
    word	p_r2[6];
    word	p_fbbk[2];
    };

#define		rs_rw		sizeof( struct $rwr )

#define PKT (*pkt)
#define CMD (*(struct $rwc *)&(PKT.data))
#define RSP (*(struct $rwr *)&(PKT.data))
#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  process a READ, WRITE, COMPARE, ACCESS, or ERASE command
 *
 *  This is a non-sequential command, so if there are any sequential commands
 *  outstanding, we must hold this command pending until they complete; if not
 *  (the UCB.pkts list is empty), then we can attempt to insert this transfer
 *  request onto the UCB.tcbs list, in elevator-seek-type order (unless the
 *  "express request" modifier is set, of course).  The unit must exist and it
 *  must have media mounted.  We acquire a TCB and copy the appropriate fields
 *  from the PKT to it.  Note that this command routine is the only one which
 *  processes more than a single command opcode (this routine handles ALL of
 *  the transfer opcodes).  The TCB is entered onto the list of active TCBs,
 *  to be removed later and processed then.  Further, it may be necessary to
 *  process any given TCB twice, if a read or write pass needs to be followed
 *  by a compare pass.
 */
do_rw( pkt )
register struct $pkt *pkt;
    {
    register struct $tcb *tcb;
    register struct $ucb *ucb;
    word ( *ok_routine )( );

    RSP.p_flgs = 0;
    /*
     *  get the UCB corresponding to the given unit number; a NULL means that
     *  there is none, which gets an "offline" status (note that it is not
     *  necessary to check here to see if we have an RD or an RX device, as
     *  some other command routines must do, since this check has already been
     *  done before bringing the unit online)
     */
    if( ( ucb = get_ucb( CMD.p_unit ) ) == null )
	RSP.p_sts = st_ofl;
    else
	{
	/*
	 *  lock the UCB data structure to prevent double updating
	 */
	$acquire( &UCB.ucb );
	/*
	 *  any sequential commands in progress?  if so, simply add this PKT
	 *  to the end of the pending sequential PKTs list (even though this is
	 *  a non-sequential command), and we will get back to it eventually
	 */
	if( UCB.pkts != null )
	    {
	    $enq_tail( &UCB.pkts, pkt );
	    $release( &UCB.ucb );
	    return;
	    }
	/*
	 *  grab a TCB, and die if none available
	 */
	tcb = $deqf_head( &tcbs );
	TCB.ucb = ucb;
	TCB.modifiers = CMD.p_mod;
	/*
	 *  a READ command uses the read routines, and writes the resulting
	 *  data to the Q-bus; also turn on compare if it's wanted
	 */
	if( CMD.p_opcd == op_rd )
	    {
	    ok_routine = &ok_rd;
	    TCB.routine = &rd_cmd;
	    TCB.type = ( tt_rd|tt_wtq );
	    if( ( CMD.p_mod & md_cmp ) || ( UCB.flags & uf_cmr ) )
		TCB.type |= tt_cmr;
	    }
	/*
	 *  a WRITE command uses the write routines, and reads the original
	 *  data from the Q-bus; also turn on compare if it's wanted
	 */
	else if( CMD.p_opcd == op_wr )
	    {
	    ok_routine = &ok_wr;
	    TCB.routine = &wr_cmd;
	    TCB.type = ( tt_wr|tt_rfq );
	    if( ( CMD.p_mod & md_cmp ) || ( UCB.flags & uf_cmw ) )
		TCB.type |= tt_cmw;
	    }
	/*
	 *  an ACCESS command uses the read routines, but drops all data
	 *  onto the floor (it just checks for read errors, really)
	 */
	else if( CMD.p_opcd == op_acc )
	    {
	    ok_routine = &ok_rd;
	    TCB.routine = &rd_cmd;
	    TCB.type = tt_rd;
	    }
	/*
	 *  an ERASE command uses the write routines, but fills the data buffer
	 *  with zeros first (it just mashes information, really)
	 */
	else if( CMD.p_opcd == op_ers )
	    {
	    ok_routine = &ok_wr;
	    TCB.routine = &wr_cmd;
	    TCB.type = tt_wr;
	    }
	/*
	 *  the only thing left is the COMPARE command -- it uses the read
	 *  routines, with compare turned on (further, it does not copy the
	 *  resulting data to the Q-bus)
	 */
	else
	    {
	    ok_routine = &ok_rd;
	    TCB.routine = &cmp_cmd;
	    TCB.type = tt_cmp;
	    }
	/*
	 *  these fields are the guts of the operation
	 */
	( ( word * ) &TCB.count )[lsw] = CMD.p_bcnt[0];
	( ( word * ) &TCB.count )[msw] = CMD.p_bcnt[1];
	( ( word * ) &TCB.block )[lsw] = CMD.p_lbn[0];
	( ( word * ) &TCB.block )[msw] = CMD.p_lbn[1];
	( ( word * ) &TCB.qbus )[lsw] = CMD.p_buff[0];
	( ( word * ) &TCB.qbus )[msw] = CMD.p_buff[1];
	TCB.pkt = pkt;
#if debug>=1
	switch( CMD.p_opcd )
	    {
	    case op_acc:
		printf( "\nACCESS" );
		break;
	    case op_cmp:
		printf( "\nCOMPARE" );
		break;
	    case op_ers:
		printf( "\nERASE" );
		break;
	    case op_rd:
		printf( "\nREAD" );
		break;
	    case op_wr:
		printf( "\nWRITE" );
		break;
	    }
	if( TCB.type & ( tt_cmr|tt_cmw ) )
	    printf( " and COMPARE" );
	printf( ", unit = %d, block = %ld, count = %ld",
		CMD.p_unit, TCB.block, TCB.count );
#endif
	/*
	 *  do a validity check on the TCB fields; if it succeeds, then add
	 *  this TCB to the list of TCBs which are active (this is where the
	 *  elevator seek algorithm is called into play)
	 *
	 *  further, we must unlock the UCB data structure, and then wake up
	 *  the job which is servicing this UCB's transfer requests
	 */
	if( ( RSP.p_sts = ( *ok_routine )( tcb ) ) == st_suc )
	    {
	    insert_tcb( tcb );
	    $release( &UCB.ucb );
	    $unblock( &UCB.ucb_job );
	    return;
	    }
	/*
	 *  if the validity check fails, just release our resources
	 */
	$enq_head( &tcbs, tcb );
	$release( &UCB.ucb );
	}
    /*
     *  this path is only taken for failed ("illegal") commands
     */
    RSP.p_bcnt[0] = 0;
    RSP.p_bcnt[1] = 0;
    RSP.p_opcd |= op_end;
    PKT.size = rs_rw;
    PKT.type = mt_seq;
    put_packet( pkt );
    }

#define PKT (*pkt)
#define CMD (*(struct $rwc *)&(PKT.data))
#define RSP (*(struct $rwr *)&(PKT.data))
#define TCB (*tcb)

/*
 *  actually process a READ, WRITE, COMPARE, ACCESS, or ERASE command
 */
_do_rw( tcb )
register struct $tcb *tcb;
    {
    register struct $pkt *pkt;

    pkt = TCB.pkt;
    /*
     *  do the transfer request by calling the read, write, or compare routine;
     *  failure means that we must fix up the byte count in the response packet
     *  to reflect that the number of bytes transferred is fewer than expected
     */
    if( ( RSP.p_sts = ( *TCB.routine )( tcb ) ) != st_suc )
	fix_count( tcb, pkt );
    /*
     *  if a compare pass is required, reinitialize those TCB fields which are
     *  altered by the read and write routines; failure here also means that we
     *  must fix up the byte count in the response packet
     */
    else if( TCB.type & ( tt_cmr|tt_cmw ) )
	{
	TCB.type &= ~( tt_wtq|tt_rfq );
	( ( word * ) &TCB.count )[lsw] = CMD.p_bcnt[0];
	( ( word * ) &TCB.count )[msw] = CMD.p_bcnt[1];
	( ( word * ) &TCB.block )[lsw] = CMD.p_lbn[0];
	( ( word * ) &TCB.block )[msw] = CMD.p_lbn[1];
	( ( word * ) &TCB.qbus )[lsw] = CMD.p_buff[0];
	( ( word * ) &TCB.qbus )[msw] = CMD.p_buff[1];
	if( ( RSP.p_sts = cmp_cmd( tcb ) ) != st_suc )
	    fix_count( tcb, pkt );
	}
    /*
     *  if the command is being aborted, return the appropriate values in the
     *  response packet
     */
    if( TCB.type & tt_abo )
	{
	RSP.p_sts = st_abo;
	RSP.p_bcnt[0] = 0;
	RSP.p_bcnt[1] = 0;
	}
    /*
     *  whether there was an error or not, we end up here and now return the
     *  TCB to the pool and then mail off a response to the host
     */
    $enq_head( &tcbs, tcb );
    RSP.p_opcd |= op_end;
    PKT.size = rs_rw;
    PKT.type = mt_seq;
    put_packet( pkt );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  make sure disk parameters for a read are within range
 *
 *  This routine is straightforward, and performs the checks that are listed
 *  below.  If any of the checks fail, an appropriate error status is
 *  returned, and the entire transfer command should not be attempted.
 */
word ok_rd( tcb )
register struct $tcb *tcb;
    {
    register struct $ucb *ucb;

    ucb = TCB.ucb;
    /*
     *  unit must have media mounted
     */
    if( UCB.state & us_ofl )
	return( st_ofl + st_sub * 1 );
    /*
     *  unit must be online
     */
    if( !( UCB.state & us_onl ) )
	return( st_avl );
    /*
     *  for Q-bus transfers, buffer address must be even
     */
    if( ( TCB.type & ( tt_rfq|tt_wtq|tt_cmp ) )
	    && ( ( ( word * ) &TCB.qbus )[lsw] & 1 ) )
	return( st_hst + st_sub * 1 );
    /*
     *  byte count must be reasonable
     */
    if( ( ( word * ) &TCB.count )[msw] & 0xF000 )
	return( st_cmd + i_bcnt );
    /*
     *  block number must be reasonable
     */
    if( ( ( word * ) &TCB.block )[msw] & 0xF000 )
	return( st_cmd + i_lbn );
    /*
     *  byte count must be even
     */
    if( ( ( word * ) &TCB.count )[lsw] & 1 )
	return( st_hst + st_sub * 2 );
    /*
     *  block number must be in host area
     */
    if( TCB.block >= UCB.hostsize )
	{
	/*
	 *  we are attempting to read the RCT
	 */
	TCB.type |= tt_rct;
	/*
	 *  fail of not reading first copy of RCT
	 */
	if( TCB.block >= UCB.hostsize + UCB.rctsize )
	    return( st_cmd + i_lbn );
	/*
	 *  access to the RCT must be single block
	 */
	if( ( ( ( word * ) &TCB.count )[lsw] != 512 )
		|| ( ( ( word * ) &TCB.count )[msw] != 0 ) )
	    return( st_cmd + i_bcnt );
	/*
	 *  all tests passed
	 */
	return( st_suc );
	}
    /*
     *  don't spiral into RCT
     */
    if( ( ( ( word * ) &TCB.count )[lsw] != 512 )
	    || ( ( ( word * ) &TCB.count )[msw] != 0 ) )
	if( TCB.block + ( ( TCB.count + 511 ) >> 9 ) > UCB.hostsize )
	    return( st_cmd + i_bcnt );
    /*
     *  all tests passed
     */
    return( st_suc );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  make sure disk parameters for a write are within range
 *
 *  This routine is straightforward, and performs the checks that are listed
 *  below.  If any of the checks fail, an appropriate error status is returned,
 *  and the entire transfer command should not be attempted.  To avoid code
 *  duplication, the checks which are common to both read and write are done
 *  in the routine above, and only those peculiar to writes are done here.
 */
word ok_wr( tcb )
register struct $tcb *tcb;
    {
    register struct $ucb *ucb;
    word status;

    ucb = TCB.ucb;
    /*
     *  do the read checks first
     */
    if( ( status = ok_rd( tcb ) ) != st_suc )
	return( status );
    /*
     *  disallow write access to the RCT
     */
    if( TCB.type & tt_rct )
	return( st_cmd + i_lbn );
    /*
     *  check for write protect (hard and soft)
     */
    if( UCB.flags & uf_wps )
	return( st_wpr + st_sub * 128 );
    if( UCB.flags & uf_wph )
	return( st_wpr + st_sub * 256 );
    /*
     *  all tests passed
     */
    return( st_suc );
    }

#define PKT (*pkt)
#define CMD (*(struct $rwc *)&(PKT.data))
#define RSP (*(struct $rwr *)&(PKT.data))
#define TCB (*tcb)

/*
 *  fix up the true byte count for this packet
 *
 *  Modify the byte count in the response packet to reflect the fact that the
 *  entire transfer was not done.  The value in TCB.count is how many bytes
 *  remain to be transferred; subtracting this from the requested byte count
 *  gives the actual number transferred before the error occurred.
 */
fix_count( tcb, pkt )
register struct $tcb *tcb;
register struct $pkt *pkt;
    {
    long count;

    ( ( word * ) &count )[lsw] = CMD.p_bcnt[0];
    ( ( word * ) &count )[msw] = CMD.p_bcnt[1];
    count -= TCB.count;
    RSP.p_bcnt[0] = ( ( word * ) &count )[lsw];
    RSP.p_bcnt[1] = ( ( word * ) &count )[msw];
    }
