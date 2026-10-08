/*
 *  file = CIBBR.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  this module contains the bad block replacement routines
 */

#include "defs.h"
#include "tcb.h"
#include "ucb.h"
#include "mscp.h"

#define		op_si1		0x05
#define		op_srp		0x40
#define		op_ft		0x60

extern byte r$cmd;
extern byte w$dat;
extern byte w$cmd;

extern byte data[];
extern byte temp[];
extern byte id_table[][4];
extern word fpl;
extern word reg_7;
extern word sd_flag;

extern word get_rbn( );
extern word read( );
extern word write( );
extern word rd_rct( );
extern word wr_rct( );
extern word rd_xbn( );
extern word wr_xbn( );

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will locate the RBN which corresponds with the given LBN
 *
 *  To do this, we must read the RCT, so of course if the "ignore media format"
 *  modifier was set when the unit was brought online, just pack up and go.
 *  Otherwise, we map the LBN to its primary RBN:  we compute how many LBNs map
 *  to each RBN (total number of LBNs divided by total number of RBNs), and
 *  divide the given LBN by this number.  This gives us the RBN which would be
 *  allocated to this LBN if the RBN were free.  Since the RBN can be in use
 *  (replacing another LBN which maps to the same RBN), the "primary" RBN may
 *  not be the right one.  We could just do a linear search from here, looking
 *  at higher-numbered RBNs, but that would waste some of the space in the
 *  current RCT block.  So, instead, we look both up and down, ping-ponging
 *  around the primary RBN, until we run out of gas totally and are forced to
 *  go to the next RCT block.  In that block, and in all others, we do a linear
 *  search, and when we reach the end of the RCT, we wrap back around to the
 *  front.  The search continues until we find the LBN we want, or we find an
 *  unused RBN.  If we find the LBN we want, we are golden.  If we find instead
 *  an unused RBN, it means that the LBN cannot be found (i.e., it has not been
 *  replaced yet and this is a new occurrence of it being bad).
 *
 *  At this point, an example may help.  Suppose that a unit has 50000 LBNs
 *  and 160 RBNs.  We want to find the RBN associated with LBN 3000.  We
 *  calculate that there are 50000/160=312 LBNs for each RBN.  This means that
 *  LBN 3000 maps into RBN 3000/312=9.  Thus RBN 9 is the "primary" RBN for LBN
 *  3000.  We read the RCT block which contains RBN 9 (there are 128 RBNs per
 *  RCT block, so we read RCT block 2, taking the two block header into
 *  account).  We look at offset 9, to see if it contains LBN 3000.  If it
 *  does, we stop, and return RBN 9 as our answer.  If it doesn't, but contains
 *  some other LBN instead, we keep looking, since we don't stop unless we
 *  match or we find an unused RBN.  Thus we look at entry 10, then 8, 11, 7,
 *  12, 6, 13, 5, 14, 4, 15, 3, 16, 2, 17, 1, 18, 0, 19, 20, 21, 22, ..., in
 *  order, until either we match our LBN or find an RBN which is unused.
 *  Notice the peculiar order; the numbers ping-pong back and forth between
 *  higher RBNs and lower RBNs, until a limit is reached (0 in this case) when
 *  the scan continues in the other direction only.  If we don't find the LBN
 *  we want in all of RCT block 2, and none of the entries indicate that that
 *  RBN is unused, we then go to RCT block 3, and look at entries 0, 1, 2, up
 *  to 127, and continue in this fashion until all possibilities have been
 *  exhausted.  Got it?
 *
 *  To simplify matters, this routine always returns both the entry which
 *  corresponds with the given LBN (i.e., the "old" RBN), and the first unused
 *  entry (i.e., the "new" RBN).
 */
word get_rbn( tcb, lbn )
register struct $tcb *tcb;
long lbn;
    {
    register word *rct2ptr;
    register struct $ucb *ucb;
    word i, j, k, lbnlo, lbnhi, rctblock, rbn, rctoffset, error;

    ucb = TCB.ucb;
    /*
     *  be pessimistic and assume failure
     */
    TCB.oldrbn = -1;
    TCB.newrbn = -1;
    if( UCB.state & us_imf )
	return( st_mfe );
    /*
     *  get what the entry we want should look like (high and low parts)
     */
    lbnlo = ( ( word * ) &lbn )[lsw];
    lbnhi = ( ( ( word * ) &lbn )[msw] & 0x0FFF ) | 0x3000;
    /*
     *  compute the "primary" RBN of this LBN, and where to find it
     */
    rbn = lbn * UCB.rbnsize / UCB.lbnsize;
    rctblock = ( rbn >> 7 ) + 2;
    rctoffset = ( rbn & 127 ) << 1;
#if debug>=2
    printf( "\nlbnlo = %o, lbnhi = %o, rctblock = %d, rctoffset = %d",
	    lbnlo, lbnhi, rctblock, rctoffset );
#endif
    /*
     *  read the RCT block which contains the "primary" RBN's entry
     */
    if( error = rd_rct( tcb, rctblock, rct2 ) )
	return( error );
    /*
     *  point to the "primary" entry first
     */
    rct2ptr = &rct2[rctoffset];
#if debug>=2
    printf( "\nrct2ptr = %o, rct2ptr[0] = %o, rct2ptr[1] = %o",
	    rct2ptr, rct2ptr[0], rct2ptr[1] );
#endif
    /*
     *  if the "primary" entry is unused, we are done (there was no "old" RBN)
     */
    if( rct2ptr[1] == 0x0000 )
	{
	TCB.newrbn = rbn;
	return( st_suc );
	}
    /*
     *  if the "primary" entry is the one we want, we have found the "old" RBN
     *  and now want to look for a "new" RBN
     */
    if( ( rct2ptr[0] == lbnlo ) && ( rct2ptr[1] == lbnhi ) )
	TCB.oldrbn = rbn;
    /*
     *  begin the ping-pong search
     */
    for( i = 2; i < 256; i += 2 )
	{
	j = rctoffset + i;
	/*
	 *  don't check any entry past the limit
	 */
	if( j <= 254 )
	    {
	    rct2ptr = &rct2[j];
#if debug>=2
	    printf( "\nrct2ptr = %o, rct2ptr[0] = %o, rct2ptr[1] = %o",
		    rct2ptr, rct2ptr[0], rct2ptr[1] );
#endif
	    /*
	     *  if this entry is unused, we are done
	     */
	    if( rct2ptr[1] == 0x0000 )
		{
		TCB.newrbn = rbn + ( i >> 1 );
		return( st_suc );
		}
	    /*
	     *  if we hit the end of the RCT, wrap around to the beginning
	     */
	    if( rct2ptr[1] == 0x8000 )
		{
		k = 2;
		goto RCT_LOOP;
		}
	    /*
	     *  if we find what we want, remember it
	     */
	    if( ( rct2ptr[0] == lbnlo ) && ( rct2ptr[1] == lbnhi ) )
		TCB.oldrbn = rbn + ( i >> 1 );
	    }
	j = rctoffset - i;
	/*
	 *  don't check any entry past the limit
	 */
	if( j >= 0 )
	    {
	    rct2ptr = &rct2[j];
#if debug>=2
	    printf( "\nrct2ptr = %o, rct2ptr[0] = %o, rct2ptr[1] = %o",
		    rct2ptr, rct2ptr[0], rct2ptr[1] );
#endif
	    /*
	     *  if this entry is unused, we are done
	     */
	    if( rct2ptr[1] == 0x0000 )
		{
		TCB.newrbn = rbn - ( i >> 1 );
		return( st_suc );
		}
	    /*
	     *  if we hit the end of the RCT, wrap around to the beginning
	     */
	    if( rct2ptr[1] == 0x8000 )
		{
		k = 2;
		goto RCT_LOOP;
		}
	    /*
	     *  if we find what we want, remember it
	     */
	    if( ( rct2ptr[0] == lbnlo ) && ( rct2ptr[1] == lbnhi ) )
		TCB.oldrbn = rbn - ( i >> 1 );
	    }
	}
    /*
     *  we aren't done yet, but have exhausted the first block we looked at;
     *  try the next block, and continue until it feels right
     */
    k = rctblock + 1;
RCT_LOOP:
    /*
     *  there are two ways to get out of this loop:  either find an unused
     *  RBN, or else discover that finding an unused RBN is impossible
     */
    for( i = k; ; i++ )
	{
	/*
	 *  read the next RCT block
	 */
	if( error = rd_rct( tcb, i, rct2 ) )
	    return( error );
	else
	    {
	    /*
	     *  don't ping-pong, but go straight through this block
	     */
	    for( j = 0; j < 256; j += 2 )
		{
		/*
		 *  if we return to the original block and offset, then we
		 *  must conclude that the RCT is full
		 */
		if( ( i == rctblock ) && ( j == rctoffset ) )
		    return( st_mfe );
		rct2ptr = &rct2[j];
#if debug>=2
	    printf( "\nrct2ptr = %o, rct2ptr[0] = %o, rct2ptr[1] = %o",
		    rct2ptr, rct2ptr[0], rct2ptr[1] );
#endif
		/*
		 *  if this entry is unused, we are done
		 */
		if( rct2ptr[1] == 0x0000 )
		    {
		    TCB.newrbn = ( ( ( i - 2 ) << 8 ) + j ) >> 1;
		    return( st_suc );
		    }
		/*
		 *  if we hit the end of the RCT, wrap around to the beginning
		 */
		if( rct2ptr[1] == 0x8000 )
		    {
		    k = 2;
		    goto RCT_LOOP;
		    }
		/*
		 *  if we find what we want, remember it
		 */
		if( ( rct2ptr[0] == lbnlo ) && ( rct2ptr[1] == lbnhi ) )
		    TCB.oldrbn = ( ( ( i - 2 ) << 8 ) + j ) >> 1;
		}
	    }
	}
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will replace a bad LBN with a good RBN
 */
word put_rbn( tcb, lbn, flag )
register struct $tcb *tcb;
long lbn;
bool flag;
    {
    register word *temp_ptr;
    register struct $ucb *ucb;
    word i, ddm, error, oldrbn, oldrctblock, oldrctoffset, newrbn,
	    newrctblock, newrctoffset, *data_ptr;

    /*
     *  STEP 1 -- If we are asked to replace a bad block, check whether or not
     *    the volume is write protected.  If it is, it implies that the
     *    replacement cannot succeed, so just exit by branching to step 18.
     *    If it isn't, then the replacement can proceed, so go to step 4.
     *    If we are not replacing a valid bad block, then we must be bringing
     *    the volume online, so go to step 3.
     */
    oldrbn = -1;
    newrbn = -1;
    ucb = TCB.ucb;
    if( UCB.state & us_imf )
	return( st_suc );
    if( lbn >= 0 )
	if( UCB.flags & ( uf_wph|uf_wps ) )
	    goto STEP_18B;
	else
	    goto STEP_4;
    /*
     *  STEP 3 -- If we are recovering from a failure or loss of context that
     *    occurred during phase 1 of bad block replacement, then go to step 7.
     *    If we are recovering from a failure or loss of context that occurred
     *    during phase 2 of bad block replacement, then go to step 11.
     *    Otherwise, exit with success.  If any errors occur, we cannot
     *    proceed at all.
     */
    if( error = rd_rct( tcb, 0, rct0 ) )
	goto STEP_18A;
    if( UCB.flags & ( uf_wph|uf_wps ) )
	goto STEP_14;
    if( error = wr_rct( tcb, 0, rct0 ) )
	goto STEP_18A;
    rd_rct( tcb, 1, rct1 );
    wr_rct( tcb, 1, rct1 );
    ddm = rct0[4] & bit7;
    ( ( word * ) &lbn )[lsw] = rct0[6];
    ( ( word * ) &lbn )[msw] = rct0[7];
    newrbn = rct0[8];
    oldrbn = rct0[10];
    TCB.newrbn = newrbn;
    TCB.oldrbn = oldrbn;
    if( rct0[4] & bit15 )
	if( error = rd_rct( tcb, 1, rct1 ) )
	    goto STEP_18A;
	else
	    goto STEP_7;
    if( rct0[4] & bit14 )
	if( error = rd_rct( tcb, 1, rct1 ) )
	    goto STEP_18A;
	else
	    goto STEP_11;
    goto STEP_14;
    /*
     *  STEP 4 -- We clear a sector-sized buffer, then read the current
     *    contents of the bad block into the buffer.  The buffer is cleared
     *    first for the rare case when no data can be transferred.  The read
     *    is performed with error recovery and error correction enabled.
     *    In addition to saving the data, we remember whether or not the
     *    read succeeded.  The saved data is considered valid if the read
     *    succeeded, or invalid if it did not.
     */
STEP_4:
    for( i = 0; i < 512; i++ )
	rct1[i] = 0;
    TCB.modifiers = 0;
    TCB.block = lbn + UCB.lbnbase;
    TCB.type = tt_new;
    fill_tcb( tcb, rct1, 512 );
    ddm = read( tcb );
    /*
     *  STEP 5 -- Record the data obtained when the bad block was read during
     *    step 4 in sector 1 of each RCT copy.  If the data cannot be
     *    successfully recorded in at least one copy of the RCT, report the
     *    error to the error log and go to step 18.
     */
    if( error = wr_rct( tcb, 1, rct1 ) )
	goto STEP_18A;
    /*
     *  STEP 6 -- Record the bad block's LBN, whether or not the saved data is
     *    valid, and the fact that we are now in phase 1 of replacement in
     *    sector 0 of each RCT copy.  This means that we must read sector 0,
     *    modify it, and write it back to each RCT copy.  If we cannot read
     *    any sector 0 successfully, we go to step 18.  If we cannot
     *    successfully write at least one copy of the RCT, we go to step 17.
     */
    if( error = rd_rct( tcb, 0, rct0 ) )
	goto STEP_17;
    rct0[4] &= ~( bit15|bit14|bit13|bit7 );
    rct0[4] |= bit15;
    if( ddm )
	rct0[4] |= bit7;
    rct0[6] = ( ( word * ) &lbn )[lsw];
    rct0[7] = ( ( word * ) &lbn )[msw];
    if( error = wr_rct( tcb, 0, rct0 ) )
	goto STEP_18A;
    /*
     *  STEP 7 -- Write and read test patterns on the suspected bad block to
     *    determine whether or not it is in fact a bad block.  Go to step 9
     *    if the test patterns fail, indicating that the block is indeed bad.
     *    Continue with step 8 if the test patterns succeed, indicating that
     *    the block may be good.  The patterns fail if either the block is
     *    again reported as a bad block or if they cannot be written and read
     *    back correctly.
     */
STEP_7:
    if( flag )
	goto STEP_9;
    TCB.modifiers = ( tm_ser|tm_sec );
    TCB.block = lbn + UCB.lbnbase;
    TCB.type = tt_new;
    fill_tcb( tcb, temp, 512 );
    temp_ptr = &temp[0];
    for( i = 256/2; --i >= 0; )
	{
	*temp_ptr++ = 0xB6DB;
	*temp_ptr++ = 0xEB6D;
	}
    if( error = write( tcb ) )
	goto STEP_9;
    if( error = read( tcb ) )
	goto STEP_9;
    temp_ptr = &temp[0];
    for( i = 256/2; --i >= 0; )
	{
	if( *temp_ptr++ != 0xB6DB )
	    goto STEP_9;
	if( *temp_ptr++ != 0xEB6D )
	    goto STEP_9;
	}
    temp_ptr = &temp[0];
    for( i = 256/2; --i >= 0; )
	{
	*temp_ptr++ = 0x4924;
	*temp_ptr++ = 0x1492;
	}
    if( error = write( tcb ) )
	goto STEP_9;
    if( error = read( tcb ) )
	goto STEP_9;
    temp_ptr = &temp[0];
    for( i = 256/2; --i >= 0; )
	{
	if( *temp_ptr++ != 0x4924 )
	    goto STEP_9;
	if( *temp_ptr++ != 0x1492 )
	    goto STEP_9;
	}
    /*
     *  STEP 8 -- We write the saved data back out to the bad block using a
     *    write-compare operation.  The write is performed with the "force
     *    error" modifier set if and only if the saved data is invalid.  Go to
     *    step 13 if the write succeeds and the block is no longer reported
     *    as bad, since the original problem was transient.  The write
     *    succeeds if no error is detected and the saved data is valid or if
     *    only a forced error is detected and the saved data is invalid.
     */
    TCB.modifiers = ( ddm ? ( tm_err|tm_ser|tm_sec ) : ( tm_ser|tm_sec ) );
    TCB.block = lbn + UCB.lbnbase;
    TCB.type = tt_new;
    fill_tcb( tcb, rct1, 512 );
    if( error = write( tcb ) )
	goto STEP_9;
    fill_tcb( tcb, temp, 512 );
    if( error = read( tcb ) )
	if( !( error & er_ddm ) || !ddm )
	    goto STEP_9;
    data_ptr = &rct1[0];
    temp_ptr = &temp[0];
    for( i = 256; --i >= 0; )
	if( *data_ptr++ != *temp_ptr++ )
	    goto STEP_9;
    goto STEP_13;
    /*
     *  STEP 9 -- We scan the RCT and determine what new RBN the bad block
     *    should be replaced with, whether or not the bad block as been
     *    previously replaced, and (if appropriate) the bad block's old RBN.
     *    The RCT is not updated at this time.  If the RCT scan fails, we
     *    report the error to the error log and go to step 16.
     */
STEP_9:
    if( error = get_rbn( tcb, lbn ) )
	goto STEP_16;
    newrbn = TCB.newrbn;
    oldrbn = TCB.oldrbn;
    /*
     *  STEP 10 -- Record the new RBN, whether or not the bad block has been
     *    previously replaced, (if appropriate) the bad block's old RBN, and
     *    the fact that we are in phase 2 of bad block replacement in sector 0
     *    of each RCT copy.  The RCT must be updated without reading, instead
     *    using the copy of sector 0 last read from or written to the RCT.
     *    If the RCT cannot be updated, report the error to the error log and
     *    go to step 16.
     */
    rct0[4] &= ~( bit15|bit14|bit13 );
    rct0[4] |= bit14;
    if( oldrbn >= 0 )
	rct0[4] |= bit13;
    rct0[8] = newrbn;
    rct0[9] = 0;
    rct0[10] = oldrbn;
    rct0[11] = 0;
    if( error = wr_rct( tcb, 0, rct0 ) )
	goto STEP_16;
    /*
     *  STEP 11 -- We update the RCT to indicate that the bad block has been
     *    replaced with the new RBN, and that the old RBN (if any) is
     *    unusable.  If this requires updating two blocks in the RCT, then
     *    both blocks must be read before either is written.  If a block
     *    cannot be read successfully, report the error to the error log and
     *    go to step 16.  If a block cannot be written successfully, report
     *    the error to the error log and go to step 15.
     */
STEP_11:
    newrctblock = ( newrbn >> 7 ) + 2;
    newrctoffset = ( newrbn & 127 ) << 1;
    if( error = rd_rct( tcb, newrctblock, rct2 ) )
	goto STEP_16;
    rct2[newrctoffset] = ( ( word * ) &lbn )[lsw];
    rct2[newrctoffset+1] = ( ( ( word * ) &lbn )[msw] & 0x0FFF ) | 0x3000;
    if( oldrbn >= 0 )
	{
	oldrctblock = ( oldrbn >> 7 ) + 2;
	oldrctoffset = ( oldrbn & 127 ) << 1;
	if( oldrctblock != newrctblock )
	    {
	    if( error = rd_rct( tcb, oldrctblock, rct3 ) )
		goto STEP_16;
	    rct3[oldrctoffset] = 0x0000;
	    rct3[oldrctoffset+1] = 0x4000;
	    if( error = wr_rct( tcb, oldrctblock, rct3 ) )
		goto STEP_15A;
	    }
	else
	    {
	    rct2[oldrctoffset] = 0x0000;
	    rct2[oldrctoffset+1] = 0x4000;
	    }
	}
    if( error = wr_rct( tcb, newrctblock, rct2 ) )
	goto STEP_15B;
    /*
     *  STEP 12 -- We write the contents of the old bad block to the new RBN.
     *    If the saved data is invalid, the "force error" modifier is set.  If
     *    the write fails, go to step 9 to rescan the RCT for another RBN
     *    (note that the current new RBN will become the old RBN for this next
     *    pass).  The write command succeeds if no error is detected and the
     *    saved data is valid or if only a forced error is detected and the
     *    saved data is invalid.
     */
    TCB.modifiers = ( ddm ? tm_err : 0 );
    TCB.block = newrbn + UCB.rbnbase;
    TCB.type = tt_new;
    fill_tcb( tcb, rct1, 512 );
    if( error = write( tcb ) )
	goto STEP_9;
    fill_tcb( tcb, temp, 512 );
    if( error = read( tcb ) )
	if( !( error & er_ddm ) || !ddm )
	    goto STEP_9;
    data_ptr = &rct1[0];
    temp_ptr = &temp[0];
    for( i = 256; --i >= 0; )
	if( *data_ptr++ != *temp_ptr++ )
	    goto STEP_9;
    replace( tcb, lbn );
    /*
     *  STEP 13 -- We update sector 0 of each copy of the RCT to indicate that
     *    we are no longer in the middle of replacing a bad block.  The RCT
     *    must be updated without reading sector 0, instead using the copy of
     *    sector 0 last read from or written to the RCT.  If the RCT cannot be
     *    updated, report the error to the error log and go to step 17.
     */
STEP_13:
    rct0[4] &= ~( bit15|bit14|bit13|bit7 );
    rct0[6] = 0;
    rct0[7] = 0;
    rct0[8] = 0;
    rct0[9] = 0;
    rct0[10] = 0;
    rct0[11] = 0;
    if( error = wr_rct( tcb, 0, rct0 ) )
	goto STEP_17;
    /*
     *  STEP 14 -- Exit successfully.
     */
#if debug>=1
    printf( "\nreplacing LBN %ld with RBN %d", lbn, newrbn );
#endif
    do_bbr( tcb, lbn, ddm, newrbn >= 0 );
STEP_14:
    TCB.oldrbn = newrbn;
    return( st_suc );
    /*
     *  STEP 15 -- We restore the RCT to indicate that the new RBN is
     *    unallocated and usable, and that the bad block is either not
     *    replaced or is revectored to the old RBN, whichever was its original
     *    status.  The RCT must be updated without reading any blocks from it,
     *    instead using the copies of the relevant blocks which were read in
     *    step 11.  Any errors are reported to the error log but are otherwise
     *    ignored.
     */
STEP_15A:
    rct3[oldrctoffset] = ( ( word * ) &lbn )[lsw];
    rct3[oldrctoffset+1] = ( ( ( word * ) &lbn )[msw] & 0x0FFF ) | 0x3000;
    wr_rct( tcb, oldrctblock, rct3 );
    goto STEP_16;
STEP_15B:
    rct2[newrctoffset] = 0x0000;
    rct2[newrctoffset+1] = 0x0000;
    wr_rct( tcb, newrctblock, rct2 );
    if( oldrbn >= 0 )
	if( oldrctblock != newrctblock )
	    goto STEP_15A;
    /*
     *  STEP 16 -- Again we try to write the saved data to the suspected bad
     *    block.  The "force error" modifier bit is set if and only if the
     *    data is invalid.  Any errors are reported to the error log but are
     *    otherwise ignored.
     */
STEP_16:
    TCB.modifiers = ( ddm ? tm_err : 0 );
    TCB.block = lbn + UCB.lbnbase;
    TCB.type = tt_new;
    fill_tcb( tcb, rct1, 512 );
    write( tcb );
    /*
     *  STEP 17 -- We update sector 0 of each of the RCT copies to indicate
     *    that it is no longer in the middle of replacing a bad block.  The
     *    RCT must be updated without reading sector 0, instead using the copy
     *    of sector 0 last read from or written to the RCT.  Any errors are
     *    reported to the error log but are otherwise ignored.
     */
STEP_17:
    rct0[4] &= ~( bit15|bit14|bit13|bit7 );
    rct0[6] = 0;
    rct0[7] = 0;
    rct0[8] = 0;
    rct0[9] = 0;
    rct0[10] = 0;
    rct0[11] = 0;
    wr_rct( tcb, 0, rct0 );
    /*
     *  STEP 18 -- The replacement has failed.  Return a status indicating
     *    this fact.  Force the unit to become "Unit-Available".
     */
STEP_18A:
    UCB.state &= ~( us_onl|us_imf );
    UCB.flags &= ( uf_wph|uf_rpl|uf_rmv );
    fpl &= ~UCB.wp_bit;
    if( !( UCB.state & us_ofl ) )
	do_una( ucb );
STEP_18B:
    return( st_mfe );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will replace a bad block by reformatting the track which the
 *  block is on and marking its header in such a way that the block cannot be
 *  found any more
 */
replace( tcb, lbn )
register struct $tcb *tcb;
long lbn;
    {
    register struct $ucb *ucb;
    word i, j, sector, surface, cylinder;
    long block;

#if debug>=1
    printf( "\nreformatting near LBN %ld", lbn );
#endif
    ucb = TCB.ucb;
    TCB.block = lbn + UCB.lbnbase;
    TCB.type = tt_new;
    fill_tcb( tcb, rct1, 512 );
    fill_id( tcb );
    /*
     *  remember the physical location of the defective block
     */
    sector = TCB.sector;
    surface = TCB.surface;
    cylinder = TCB.cylinder;
    block = lbn - TCB.sector;
    for( i = 0; i < UCB.sec; i++ )
	{
	/*
	 *  any block on this track which has an entry in the RCT is known
	 *  to be bad, and its header will be marked so that attempts to
	 *  access it will fail
	 */
	if( ( !get_rbn( tcb, block + i ) ) && ( TCB.oldrbn >= 0 ) )
	    {
	    for( j = 0; id_table[j][2] != i; j++ )
		;
	    id_table[j][2] = -1;
	    }
	/*
	 *  copy the data in each block to one of the XBNs, unless we are
	 *  still formatting the unit
	 */
	if( !( UCB.state & us_fct ) )
	    {
	    TCB.modifiers = 0;
	    TCB.block = block + i + UCB.lbnbase;
	    TCB.type = tt_new;
	    fill_tcb( tcb, rct1, 512 );
	    read( tcb );
	    wr_xbn( tcb, i + 3, rct1 );
	    }
	}
    /*
     *  do the actual reformatting
     */
    TCB.surface = surface;
    TCB.cylinder = cylinder;
    reformat( tcb );
    /*
     *  restore the data from the XBNs to each block on this track, unless we
     *  are still formatting the unit
     */
    if( !( UCB.state & us_fct ) )
	for( i = 0; i < UCB.sec; i++ )
	    {
	    rd_xbn( tcb, i + 3, rct1 );
	    TCB.modifiers = 0;
	    TCB.block = block + i + UCB.lbnbase;
	    TCB.type = tt_new;
	    fill_tcb( tcb, rct1, 512 );
	    write( tcb );
	    }
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine implements the XBN multi-read algorithm
 *
 *  The algorithm consists of reading each of the XBN copies in turn, using a
 *  compare operation.  If the compare fails, the next copy is tried, until
 *  either we succeed or we run out of copies.
 */
word rd_xbn( tcb, i, buffer )
register struct $tcb *tcb;
word i;
byte *buffer;
    {
    register word *data_ptr, *temp_ptr;
    word j, k;
    struct $ucb *ucb;

#if debug>=2
    printf( "\nreading XBN block %d", i );
#endif
    ucb = TCB.ucb;
    TCB.modifiers = 0;
    TCB.block = i;
    TCB.type = tt_new;
    for( j = 3; --j >= 0; )
	{
	fill_tcb( tcb, buffer, 512 );
	if( !read( tcb ) )
	    {
	    TCB.buffer = temp;
	    if( !read( tcb ) )
		{
		data_ptr = buffer;
		temp_ptr = &temp[0];
		for( k = 256; --k >= 0; )
		    if( *data_ptr++ != *temp_ptr++ )
			break;
		if( k < 0 )
		    return( st_suc );
		}
	    }
	TCB.buffer = buffer;
	TCB.block += UCB.sec;
	TCB.type = tt_new;
	}
    return( st_mfe );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine implements the XBN multi-write algorithm
 *
 *  The algorithm consists of writing each of the XBN copies in turn, using a
 *  compare operation.  If the compare fails, the block is rewritten with the
 *  "force error" flag set (so that future reads of it will fail).  Each copy
 *  is done this way, and the operation succeeds if any write/compare worked
 *  correctly.
 */
word wr_xbn( tcb, i, buffer )
register struct $tcb *tcb;
word i;
byte *buffer;
    {
    register word *data_ptr, *temp_ptr;
    bool good;
    word j, k;
    struct $ucb *ucb;

#if debug>=2
    printf( "\nwriting XBN block %d", i );
#endif
    good = false;
    ucb = TCB.ucb;
    TCB.modifiers = 0;
    TCB.block = i;
    TCB.type = tt_new;
    for( j = 3; --j >= 0; )
	{
	fill_tcb( tcb, buffer, 512 );
	if( !write( tcb ) )
	    {
	    TCB.buffer = temp;
	    if( !read( tcb ) )
		{
		data_ptr = buffer;
		temp_ptr = &temp[0];
		for( k = 256; --k >= 0; )
		    if( *data_ptr++ != *temp_ptr++ )
			break;
		if( k < 0 )
		    good = true;
		else
		    {
		    TCB.modifiers = tm_err;
		    write( tcb );
		    TCB.modifiers = 0;
		    }
		}
	    }
	TCB.buffer = buffer;
	TCB.block += UCB.sec;
	TCB.type = tt_new;
	}
    return( good ? st_suc : st_mfe );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will fill the id table for formatting purposes
 *
 *  This is a convoluted routine which takes sector-to-sector interleave,
 *  surface-to-surface skew, and cylinder-to-cylinder skew into account.  Take
 *  my word for it that it works, and don't touch the code.
 */
fill_id( tcb )
register struct $tcb *tcb;
    {
    register struct $ucb *ucb;
    word i, j, skew;

    ucb = TCB.ucb;
    skew = ( ( long ) TCB.surface * UCB.sur_skew +
	    ( long ) TCB.cylinder * UCB.cyl_skew ) % UCB.sec;
    for( j = 0; j < UCB.sec; j++ )
	id_table[j][3] = 0;
    for( i = 0; i < UCB.sec; i++ )
	{
	j = ( skew + i * UCB.sec_interleave ) % UCB.sec;
	while( id_table[j][3] != 0 )
	    j = ( j + 1 ) % UCB.sec;
	id_table[j][0] = TCB.cylinder;
	id_table[j][1] = TCB.surface + ( ( TCB.cylinder >> 4 ) & 0xF0 );
	id_table[j][2] = i;
	id_table[j][3] = 2;
	}
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will format a given cylinder and surface
 *
 *  This provides the low-level code necessary to actually alter the headers
 *  which need to be marked bad.  Again, touch this code at your own risk.
 */
reformat( tcb )
register struct $tcb *tcb;
    {
    register struct $ucb *ucb;
    word i, opcode;

    ucb = TCB.ucb;
    select( ucb );
    restore( ucb );
    for( i = 0; i < TCB.cylinder; i++ )
	put_udc( op_si1 );
    opcode = op_ft;
    if( TCB.cylinder >= UCB.pccyl )
	opcode |= 2;
    i = &id_table;
    w$cmd = op_srp + 0;
    w$dat = ( ( byte * ) &i )[lsb];
    w$dat = ( ( byte * ) &i )[msb];
    w$dat = 0;
    put_udc( sd_flag );
    w$cmd = op_srp + 0;
    w$dat = -UCB.gap0;
    w$dat = -UCB.gap1;
    w$dat = -UCB.gap2;
    w$dat = -UCB.gap3;
    w$dat = TCB.surface;
    w$dat = ~UCB.sync;
    w$dat = ~UCB.sec;
    w$dat = ~4;
    for( i = 0; i < rqdx3_retry; i++ )
	{
	put_udc( opcode );
	if( !( r$cmd & ( bit4|bit3 ) ) )
	    break;
	}
    w$cmd = op_srp + 7;
    w$dat = reg_7;
    deselect( ucb );
    }
