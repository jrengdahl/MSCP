/*
 *  file = DISK.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  this module contains the disk read/write/seek routines
 */

#include "defs.h"
#include "tcb.h"
#include "ucb.h"
#include "mscp.h"

extern list dma;
extern list mem;
extern list tcbs;
extern byte *_packet;
extern byte data[];
extern byte temp[];
extern byte rx_table[];

extern byte *$deqf_head( );
extern word get_rbn( );
extern word put_rbn( );
extern word read( );
extern word st_err( );
extern word write( );
extern word rd_rct( );
extern word rd_rpl( );
extern word rd_seg( );
extern word wr_rct( );
extern word wr_rpl( );
extern word wr_seg( );

#define TCB (*tcb)
#define UCB (*ucb)
#define lswTCBcount (((unsigned *)&TCB.count)[lsw])
#define mswTCBcount (((unsigned *)&TCB.count)[msw])

/*
 *  this routine will read from a disk unit
 *
 *  We are reading either from the host LBN space or from the RCT.  If the
 *  target block is within the host LBN space, then add on the LBN base, and
 *  then convert this to a PBN set (cylinder, surface, sector).  Lock the
 *  memory buffer, and begin the transfer.  We transfer "chunks", where the
 *  size of a chunk can be from one sector to a track's worth of sectors.
 *  We continue to transfer chunks until the desired number of bytes have been
 *  read.  For READ commands, the data will automatically be copied to the
 *  host, since the "write to Q-bus" bit is set in the TCB, while for ACCESS
 *  commands, the data will be dumped.  If the target block is in the RCT,then
 *  then we convert the given block number into an RCT offset (the RCT is
 *  accessible at the high end of the host LBN space).  Again, lock the memory
 *  buffer, and begin the transfer.  Here the "chunk" is always a single
 *  block.  This time we need to copy the data to the host ourselves, since
 *  the regular RCT read routine does not do this for us.  In either case, if
 *  there is any error, the transfer is aborted.  Finally, all resources are
 *  returned.
 */
word rd_cmd( tcb )
register struct $tcb *tcb;
    {
    register word error;
    register struct $ucb *ucb;

    error = st_suc;
    ucb = TCB.ucb;
    if( TCB.type & tt_rct )
	{
	/*
	 *  the block is in the RCT
	 */
	TCB.block -= UCB.hostsize;
	$acquire( &mem );
	/*
	 *  read the desired RCT block
	 */
	if( error = rd_rct( tcb, ( word ) TCB.block, data ) )
	    goto DROP_MEM;
	/*
	 *  copy the data to the host
	 */
	$acquire( &dma );
	_packet = TCB.pkt;
	if( TCB.fatal = put_buffer( TCB.qbus, data, 512 ) )
	    error = st_err( tcb );
	goto DROP_DMA;
	}
    else
	{
	/*
	 *  the block is within the host LBN space
	 */
	TCB.block += UCB.lbnbase;
	if( UCB.state & us_rd )
	    calc_pbn( tcb );
	$acquire( &mem );
	while( TCB.count > 0 )
	    {
	    /*
	     *  set up to read up to a full track of data
	     */
	    fill_tcb( tcb, data,
		    ( ( lswTCBcount <= data_size ) && ( mswTCBcount == 0 ) )
			    ? lswTCBcount : data_size );
	    if( error = rd_seg( tcb ) )
		goto DROP_MEM;
	    }
	goto DROP_MEM;
	}
    /*
     * always release any resources we own
     */
DROP_DMA:
    $release( &dma );
DROP_MEM:
    $release( &mem );
    return( error );
    }

#define TCB (*tcb)
#define UCB (*ucb)
#define lswTCBcount (((unsigned *)&TCB.count)[lsw])
#define mswTCBcount (((unsigned *)&TCB.count)[msw])

/*
 *  this routine will write to a disk unit
 *
 *  We are writing only to the host LBN space; writing to the RCT is not
 *  allowed.  Add on the LBN base, and then convert this to a PBN set
 *  (cylinder, surface, sector).  Lock the memory buffer, and begin the
 *  transfer.  We transfer "chunks", where the size of a chunk can be from one
 *  sector to a track's worth of sectors.  We continue to transfer chunks until
 *  the desired number of bytes have been written.  For WRITE commands, the
 *  data will automatically be obtained from the host, since the "read from
 *  Q-bus" bit is set in the TCB, while for ERASE commands, the data is taken
 *  as all zeros.  If there is any error, the transfer is aborted.  Finally,
 *  all resources are returned.
 */
word wr_cmd( tcb )
register struct $tcb *tcb;
    {
    register word error;
    register struct $ucb *ucb;

    error = st_suc;
    ucb = TCB.ucb;
    /*
     *  the block is within the host LBN space
     */
    TCB.block += UCB.lbnbase;
    if( UCB.state & us_rd )
	calc_pbn( tcb );
    $acquire( &mem );
    /*
     *  we distinguish WRITEs from ERASEs by the "read from Q-bus" bit; an
     *  ERASE requires us to zero the data buffer before beginning the write
     *  operation (we need to zero only a single track's worth, at most)
     */
    if( !( TCB.type & tt_rfq ) )
	zero( tcb );
    while( TCB.count > 0 )
	{
	/*
	 *  set up to write up to a full track of data
	 */
	fill_tcb( tcb, data,
		( ( lswTCBcount <= data_size ) && ( mswTCBcount == 0 ) )
			? lswTCBcount : data_size );
	if( error = wr_seg( tcb ) )
	    goto DROP_MEM;
	}
    /*
     * always release any resources we own
     */
DROP_MEM:
    $release( &mem );
    return( error );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will compare with a disk unit
 *
 *  We are reading either from the host LBN space or from the RCT.  If the
 *  target block is within the host LBN space, then add on the LBN base, and
 *  then convert this to a PBN set (cylinder, surface, sector).  Lock the
 *  memory buffer, and begin the transfer.  We transfer "chunks", where the
 *  size of a chunk can be from one sector to a track's worth of sectors.
 *  We continue to transfer chunks until the desired number of bytes have been
 *  read.  Since this is a COMPARE command, the data is first read from the
 *  disk into the memory buffer, and then it is read from the host, a block at
 *  a time, and compared byte-by-byte with the disk data.  Any discrepancy is
 *  reported to the host.  If there is any error, the transfer is aborted.  If
 *  the target block is in the RCT, then we just return success without doing
 *  anything.  Finally, all resources are returned.
 */
word cmp_cmd( tcb )
register struct $tcb *tcb;
    {
    register word *data_ptr, *temp_ptr;
    word i, j, k, error;
    struct $ucb *ucb;

    error = st_suc;
    ucb = TCB.ucb;
    if( TCB.type & tt_rct )
	{
	/*
	 *  the block is in the RCT
	 */
	return( st_suc );
	}
    else
	{
	/*
	 *  the block is within the host LBN space
	 */
	TCB.block += UCB.lbnbase;
	if( UCB.state & us_rd )
	    calc_pbn( tcb );
	$acquire( &mem );
	while( TCB.count > 0 )
	    {
	    /*
	     *  set up to read (not a full track of) data for RD devices or
	     *  a single block of data for RX devices (there was a reason for
	     *  this, but it escapes me right now)
	     */
	    i = ( UCB.state & us_rx ) ? 1*512 : 12*512;
	    fill_tcb( tcb, data, i <= TCB.count ? i : ( word ) TCB.count );
	    if( error = rd_seg( tcb ) )
		goto DROP_MEM;
	    /*
	     *  we have read the data with no error; now, a block at a time,
	     *  get the corresponding data from the host and do the compare
	     */
	    for( i = 0; i < TCB.segment; i += 512 )
		{
		/*
		 *  k is the size of the chunk to do (it is the minumum of 512
		 *  and however many bytes we have left to do)
		 */
		k = TCB.segment - i;
		if( k > 512 )
		    k = 512;
		/*
		 *  get a chunk of that size from the host into the temp area
		 */
		$acquire( &dma );
		_packet = TCB.pkt;
		if( TCB.fatal = get_buffer( TCB.qbus, temp, k ) )
		    {
		    error = st_err( tcb );
		    goto DROP_DMA;
		    }
		$release( &dma );
		TCB.qbus += k;
		/*
		 *  compare the data area with the temp area
		 */
		data_ptr = &data[i];
		temp_ptr = &temp[0];
		for( j = ( unsigned ) k / 2; --j >= 0; )
		    if( *data_ptr++ != *temp_ptr++ )
			{
			/*
			 *  found an error at this byte!
			 */
			TCB.count += TCB.segment - i;
			error = st_cmp;
			goto DROP_MEM;
			}
		}
	    }
	goto DROP_MEM;
	}
    /*
     * always release any resources we own
     */
DROP_DMA:
    $release( &dma );
DROP_MEM:
    $release( &mem );
    return( error );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will read a contiguous segment of a disk unit
 *
 *  Here we read up to TCB.number sectors (where TCB.number varies from one to
 *  the size of a track).  If we get either a header compare error or an ECC
 *  error on an RD device, then we call the "replace" routine.  This guy checks
 *  to see if the block in question has already been replaced (in which case
 *  it just reads the data from the replacement block) or if it needs to be
 *  replaced (in which case it does so and THEN does the read from the newly
 *  assigned replacement block).  If we read a deleted data mark, we return
 *  a status of "forced error".
 */
word rd_seg( tcb )
register struct $tcb *tcb;
    {
    register struct $ucb *ucb;
    word error, rpl_error;

    ucb = TCB.ucb;
    /*
     *  read until all sectors have been read
     */
    while( TCB.number > 0 )
	{
	/*
	 *  read some stuff and update the TCB fields
	 */
	if( !( error = read( tcb ) ) && TCB.warning )
	    st_err( tcb );
	if( TCB.result > 0 )
	    update_tcb( tcb, TCB.result );
	/*
	 *  if there was any error, do the recovery procedure outline above
	 */
	if( error )
	    if( ( error & ( er_hce|er_ecc ) ) && ( UCB.state & us_rd ) )
		if( rpl_error = rd_rpl( tcb ) )
		    if( rpl_error & er_ddm )
			return( st_dat );
		    else
			return( st_err( tcb ) );
		else
		    update_tcb( tcb, 1 );
	    else
		return( st_err( tcb ) );
	}
    return( st_suc );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will write a contiguous segment of a disk unit
 *
 *  Here we write up to TCB.number sectors (where TCB.number varies from one to
 *  the size of a track).  If we get a header compare error on an RD device,
 *  then we call the "replace" routine.  This guy checks to see if the block in
 *  question has already been replaced (in which case it just writes the data
 *  to the replacement block) or if it needs to be replaced (in which case it
 *  does so and THEN does the write to the newly assigned replacement block).
 */
word wr_seg( tcb )
register struct $tcb *tcb;
    {
    register struct $ucb *ucb;
    word error;

    ucb = TCB.ucb;
    while( TCB.number > 0 )
	{
	if( !( error = write( tcb ) ) && TCB.warning )
	    st_err( tcb );
	if( TCB.result > 0 )
	    update_tcb( tcb, TCB.result );
	if( error )
	    if( ( error & er_hce ) && ( UCB.state & us_rd ) )
		if( wr_rpl( tcb ) )
		    return( st_err( tcb ) );
		else
		    {
		    update_tcb( tcb, 1 );
		    if( !( TCB.type & tt_rfq ) )
			zero( tcb );
		    }
	    else
		return( st_err( tcb ) );
	}
    return( st_suc );
    }

#define TCB (*tcb)
#define UCB (*ucb)
#define newTCB (*newtcb)

/*
 *  this routine will revector or replace a block being read
 *
 *  We are called if we get an error reading a block.  We allocate a new TCB,
 *  fill in some fields, and attempt to find the RBN associated with the given
 *  LBN.  If we find one, the read is done from that RBN, else we call the
 *  routine which does an actual replacement operation, so that now there IS
 *  an RBN for this LBN.  Of course, reading from the RBN may provoke the same
 *  kinds of errors which got us here in the first place (header compare error
 *  or ECC error), so we loop!
 */
word rd_rpl( tcb )
register struct $tcb *tcb;
    {
    register struct $ucb *ucb;
    register struct $tcb *newtcb;
    word error;
    long lbn;

    /*
     *  get a new TCB so we don't destroy good information in the current TCB
     */
    newtcb = $deqf_head( &tcbs );
    ucb = TCB.ucb;
    lbn = TCB.block - UCB.lbnbase;
    newTCB.ucb = TCB.ucb;
    newTCB.pkt = TCB.pkt;
    /*
     *  see if this LBN currently has an RBN (TCB.oldrbn is valid)
     */
    if( error = get_rbn( newtcb, lbn ) )
	goto EXIT;
    if( newTCB.oldrbn < 0 )
BAD_RBN:
	/*
	 *  no current RBN, so make one
	 */
	if( error = put_rbn( newtcb, lbn, false ) )
	    goto EXIT;
	else if( newTCB.oldrbn < 0 )
	    newTCB.oldrbn = TCB.block - UCB.rbnbase;
#if debug>=1
    if( newTCB.oldrbn >= 0 )
	printf( "\nrevectoring LBN %ld to RBN %ld", lbn, newTCB.oldrbn );
#endif
    /*
     *  now do the read from the RBN, rather than the LBN
     */
    newTCB.block = newTCB.oldrbn + UCB.rbnbase;
    newTCB.count = TCB.count < 512 ? TCB.count : 512;
    newTCB.type = TCB.type | tt_new;
    newTCB.modifiers = TCB.modifiers;
    newTCB.qbus = TCB.qbus;
    fill_tcb( newtcb, TCB.buffer, ( word ) newTCB.count );
    /*
     *  an error now causes the RBN to be replaced
     */
    if( ( error = read( newtcb ) ) & ( er_hce|er_ecc ) )
	goto BAD_RBN;
EXIT:
    $enq_head( &tcbs, newtcb );
    return( error );
    }

#define TCB (*tcb)
#define UCB (*ucb)
#define newTCB (*newtcb)

/*
 *  this routine will revector or replace a block being written
 *
 *  We are called if we get an error writing a block.  We allocate a new TCB,
 *  fill in some fields, and attempt to find the RBN associated with the given
 *  LBN.  If we find one, the write is done to that RBN, else we call the
 *  routine which does an actual replacement operation, so that now there IS
 *  an RBN for this LBN.  Of course, writing to the RBN may provoke the same
 *  kinds of errors which got us here in the first place (header compare error
 *  only, it seems) so we loop!
 */
word wr_rpl( tcb )
register struct $tcb *tcb;
    {
    register struct $ucb *ucb;
    register struct $tcb *newtcb;
    word error;
    long lbn;

    /*
     *  get a new TCB so we don't destroy good information in the current TCB
     */
    newtcb = $deqf_head( &tcbs );
    ucb = TCB.ucb;
    lbn = TCB.block - UCB.lbnbase;
    newTCB.ucb = TCB.ucb;
    newTCB.pkt = TCB.pkt;
    /*
     *  see if this LBN currently has an RBN (TCB.oldrbn is valid)
     */
    if( error = get_rbn( newtcb, lbn ) )
	goto EXIT;
    if( newTCB.oldrbn < 0 )
BAD_RBN:
	/*
	 *  no current RBN, so make one
	 */
	if( error = put_rbn( newtcb, lbn, false ) )
	    goto EXIT;
	else if( newTCB.oldrbn < 0 )
	    newTCB.oldrbn = TCB.block - UCB.rbnbase;
#if debug>=1
    if( newTCB.oldrbn >= 0 )
	printf( "\nrevectoring LBN %ld to RBN %ld", lbn, newTCB.oldrbn );
#endif
    /*
     *  now do the write to the RBN, rather than the LBN
     */
    newTCB.block = newTCB.oldrbn + UCB.rbnbase;
    newTCB.count = TCB.count < 512 ? TCB.count : 512;
    newTCB.type = TCB.type | tt_new;
    newTCB.modifiers = TCB.modifiers;
    newTCB.qbus = TCB.qbus;
    fill_tcb( newtcb, TCB.buffer, ( word ) newTCB.count );
    /*
     *  an error now causes the RBN to be replaced
     */
    if( ( error = write( newtcb ) ) & er_hce )
	goto BAD_RBN;
EXIT:
    $enq_head( &tcbs, newtcb );
    return( error );
    }

#if debug>=100

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will print the contents of the RCT
 *
 *  This is a debugging routine only.  It does what it says it does.
 */
print_rct( tcb )
register struct $tcb *tcb;
    {
    register struct $ucb *ucb;
    word rctblock, rctoffset;
    long lbn, rbn;

    ucb = TCB.ucb;
    if( rd_rct( tcb, 0, rct0 ) )
	{
	printf( "\nerror reading RCT sector 0" );
	return( failure );
	}
    else
	{
	if( rct0[4] & bit15 )
	    printf( "\nController Initiated BBR is in phase 1" );
	if( rct0[4] & bit14 )
	    printf( "\nController Initiated BBR is in phase 2" );
	if( rct0[4] & bit13 )
	    printf( "\nA bad RBN is being replaced" );
	if( rct0[4] & bit7 )
	    printf( "\nLBN being replaced has a Forced Error" );
        if( rct0[4] & ( bit15|bit14 ) )
	    {
	    ( ( word * ) &lbn )[lsw] = rct0[6];
	    ( ( word * ) &lbn )[msw] = rct0[7];
	    printf( "\nLBN = %ld", lbn );
	    }
	if( rct0[4] & bit14 )
	    {
	    ( ( word * ) &rbn )[lsw] = rct0[8];
	    ( ( word * ) &rbn )[msw] = rct0[9];
	    printf( "\nRBN = %ld", rbn );
	    }
	if( rct0[4] & bit13 )
	    {
	    ( ( word * ) &rbn )[lsw] = rct0[10];
	    ( ( word * ) &rbn )[msw] = rct0[11];
	    printf( "\nbad RBN = %ld", rbn );
	    }
	}
    rbn = 0;
    for( rctblock = 2; rctblock < UCB.rctsize; rctblock++ )
	{
	if( rd_rct( tcb, rctblock, rct2 ) )
	    {
	    printf( "\nerror reading RCT sector %d", rctblock );
	    return( failure );
	    }
	else
	    for( rctoffset = 0; rctoffset < 256; rctoffset += 2, rbn++ )
		{
		if( ( rct2[rctoffset+1] & 0xF000 ) != 0x0000 )
		    {
		    ( ( word * ) &lbn )[lsw] = rct2[rctoffset];
		    ( ( word * ) &lbn )[msw] = rct2[rctoffset+1] & 0x0FFF;
		    switch( rct2[rctoffset+1] & 0xF000 )
			{
			case 0x3000:
			    printf( "\nRBN %ld replaces LBN %ld", rbn, lbn );
			    break;
			case 0x8000:
			    printf( "\nend of RCT at RBN %ld", rbn );
			    return( success );
			default:
			    printf( "\nRBN %ld == %04X%04X (LBN %ld)", rbn,
				    rct2[rctoffset+1], rct2[rctoffset], lbn );
			    break;
			}
		    }
		}
	}
    return( success );
    }

#endif

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine implements the RCT multi-read algorithm
 *
 *  The algorithm consists of reading each of the RCT copies in turn, using a
 *  compare operation.  If the compare fails, the next copy is tried, until
 *  either we succeed or we run out of copies.
 */
word rd_rct( tcb, i, buffer )
register struct $tcb *tcb;
word i;
byte *buffer;
    {
    register word *data_ptr, *temp_ptr;
    word j, k;
    struct $ucb *ucb;

#if debug>=2
    printf( "\nreading RCT block %d", i );
#endif
    ucb = TCB.ucb;
    /*
     *  set up the TCB fields that control the read
     */
    TCB.modifiers = 0;
    TCB.block = UCB.hostsize + i + UCB.lbnbase;
    TCB.type = tt_new;
    /*
     *  do this until we succeed or until there are no more copies
     */
    for( j = UCB.rctcopies; --j >= 0; )
	{
	fill_tcb( tcb, buffer, 512 );
	if( !read( tcb ) )
	    {
	    TCB.buffer = temp;
	    if( !read( tcb ) )
		{
		/*
		 *  we have read twice in a row now with no errors; make sure
		 *  the same data was read both times by comparing them
		 */
		data_ptr = buffer;
		temp_ptr = &temp[0];
		for( k = 256; --k >= 0; )
		    if( *data_ptr++ != *temp_ptr++ )
			break;
		if( k < 0 )
		    return( st_suc );
		}
	    }
	/*
	 *  this copy is not good, try the next one
	 */
	TCB.buffer = buffer;
	TCB.block += UCB.rctsize;
	TCB.type = tt_new;
	}
    return( st_mfe );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine implements the RCT multi-write algorithm
 *
 *  The algorithm consists of writing each of the RCT copies in turn, using a
 *  compare operation.  If the compare fails, the block is rewritten with the
 *  "force error" flag set (so that future reads of it will fail).  Each copy
 *  is done this way, and the operation succeeds if any write/compare worked
 *  correctly.
 */
word wr_rct( tcb, i, buffer )
register struct $tcb *tcb;
word i;
byte *buffer;
    {
    register word *data_ptr, *temp_ptr;
    bool good;
    word j, k;
    struct $ucb *ucb;

#if debug>=2
    printf( "\nwriting RCT block %d", i );
#endif
    good = false;
    ucb = TCB.ucb;
    /*
     *  set up the TCB fields that control the write
     */
    TCB.modifiers = 0;
    TCB.block = UCB.hostsize + i + UCB.lbnbase;
    TCB.type = tt_new;
    /*
     *  do this until we succeed or until there are no more copies
     */
    for( j = UCB.rctcopies; --j >= 0; )
	{
	fill_tcb( tcb, buffer, 512 );
	if( !write( tcb ) )
	    {
	    TCB.buffer = temp;
	    if( !read( tcb ) )
		{
		/*
		 *  we have written and then read the data now with no errors;
		 *  make sure we read back the same data that we wrote
		 */
		data_ptr = buffer;
		temp_ptr = &temp[0];
		for( k = 256; --k >= 0; )
		    if( *data_ptr++ != *temp_ptr++ )
			break;
		if( k < 0 )
		    good = true;
		else
		    {
		    /*
		     *  turn on the "force error" flag
		     */
		    TCB.modifiers = tm_err;
		    write( tcb );
		    TCB.modifiers = 0;
		    }
		}
	    }
	/*
	 *  this copy is done, do the next one
	 */
	TCB.buffer = buffer;
	TCB.block += UCB.rctsize;
	TCB.type = tt_new;
	}
    return( good ? st_suc : st_mfe );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will fill the current TCB with parameters
 *
 *  We want to figure out the cylinder, surface, and sector to begin the
 *  transfer on, and also the size of the transfer.  The size is the minimum
 *  of the requested byte count, the track size, and the data buffer size.
 *  If the TCB type is not "new", then some of the calculations can be skipped
 *  since they were done before (and perhaps updated by UPDATE_TCB).
 */
fill_tcb( tcb, buffer, count )
register struct $tcb *tcb;
byte *buffer;
word count;
    {
    register struct $ucb *ucb;
    register word block;

    ucb = TCB.ucb;
    /*
     *  the calculations are different for RD and RX devices
     */
    if( UCB.state & us_rd )
	{
	if( TCB.type & tt_new )
	    {
	    /*
	     *  compute the physical quantities to hand to the SMC9224 chip
	     */
	    TCB.type &= ~tt_new;
	    calc_pbn( tcb );
	    }
	/*
	 *  this is how many sectors we can do on this track
	 */
	TCB.number = UCB.sec - TCB.sector;
	}
    else
	{
	/*
	 *  compute the physical quantities to hand to the SMC9224 chip
	 */
	block = ( word ) TCB.block;
	if( UCB.state & us_tsf )
	    {
	    /*
	     *  this is DEC's ten sector format
	     */
	    TCB.cylinder = block / 10;
	    TCB.surface = 0;
	    TCB.sector = &rx_table[block % 50];
	    /*
	     *  we are required to map track 80 into track 0
	     */
	    if( ++TCB.cylinder > 79 )
		TCB.cylinder = 0;
	    /*
	     *  this is how many sectors we can do on this track
	     */
	    TCB.number = 10 - block % 10;
	    }
	else
	    {
	    /*
	     *  this isn't DEC's ten sector format
	     */
	    TCB.cylinder = block / 30;
	    block -= TCB.cylinder * 30;
	    if( ++block <= 15 )
		{
		TCB.surface = 0;
		TCB.sector = block;
		}
	    else
		{
		TCB.surface = 1;
		TCB.sector = block - 15;
		}
	    /*
	     *  this is how many sectors we can do on this track
	     */
	    TCB.number = 16 - TCB.sector;
	    }
	}
    /*
     *  the segment size is the minimum of the maximum permitted transfer size
     *  (passed in variable "count") and the number of bytes left on this track
     */
    TCB.segment = ( unsigned ) TCB.number * 512;
    if( count < TCB.segment )
	{
	TCB.number = ( ( unsigned ) count + 511 ) / 512;
	TCB.segment = count;
	}
    TCB.buffer = buffer;
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will update the current TCB
 *
 *  We need to reflect the fact that the transfer is partially complete.  We
 *  are given the number of sectors successfully transferred, so we update all
 *  counters and pointers accordingly.
 */
update_tcb( tcb, number )
register struct $tcb *tcb;
register word number;
    {
    register struct $ucb *ucb;
    word segment;

    ucb = TCB.ucb;
    /*
     *  force RX devices to call FILL_TCB each time (we cannot reliably update)
     */
    if( UCB.state & us_rx )
	{
	TCB.number = 0;
	TCB.sector = 0;
	}
    else
	TCB.number -= number;
    segment = ( unsigned ) number * 512;
    /*
     *  we reduce TCB.count; we increase TCB.block, TCB.buffer, and TCB.qbus;
     *  and we also compute the physical values (cylinder, surface, and sector)
     *  of the next block to transfer
     */
    if( ( TCB.count -= segment ) > 0 )
	{
	/*
	 *  don't bother doing anything else if the transfer is complete
	 *  (i.e., if TCB.count <= 0)
	 */
	TCB.block += number;
	if( ( TCB.sector += number ) >= UCB.sec )
	    {
	    TCB.sector = 0;
	    if( ++TCB.surface >= UCB.sur )
		{
		TCB.surface = 0;
		++TCB.cylinder;
		}
	    }
	TCB.buffer += segment;
	if( TCB.type & ( tt_wtq|tt_rfq ) )
	    {
	    TCB.qbus += segment;
	    TCB.segment -= segment;
	    }
	}
    else
	TCB.count = 0;
    }

#define TCB (*tcb)
#define UCB (*ucb)
#define thisTCB (*thistcb)
#define lastTCB (*lasttcb)
#define thisTCBup (thisTCB.type&tt_up)
#define nextTCB (lasttcb=thistcb,thistcb=lastTCB.link)

/*
 *  this routine will insert the current transfer control block into a queue
 *
 *  The queue of TCBs for each UCB looks like the following:
 *
 *	UCB.tcbs -> TCB_0 -> TCB_1 -> ... -> TCB_n -> 0 ;
 *
 *  if the queue is empty, then this reduces to
 *
 *	UCB.tcbs -> 0 .
 *
 *  The queue of TCBs is kept in priority order; all TCBs which have the
 *  "express request" modifier set are at the front of the queue, and
 *  are inserted in FIFO order; next come all TCBs which can be serviced while
 *  the disk head continues to move in its current direction (either up or
 *  down); finally come all TCBs which must wait until the disk head reverses
 *  direction to be serviced.  The current state of the unit (direction and
 *  block number) are stored in the UCB, and all comparisons are made using it
 *  as a reference.
 *
 *  Variables "thistcb" and "lasttcb" are used in the algorithm.  thistcb
 *  is a pointer to the TCB currently being compared against, while lasttcb is
 *  a pointer to the previously considered TCB.  This backpointer is necessary
 *  since the UCB.tcbs queue is a singly-linked list, and not a doubly-linked
 *  list (as would be preferred).
 *
 *  The algorithm works as follows:
 *
 *	1.  If the queue is empty, simply insert the new TCB.  By default we
 *	    have found the best spot.
 *
 *	2.  If there are any express TCBs, skip past them.
 *
 *	3.  If this is an express TCB, insert the TCB here.  This maintains
 *	    the FIFO nature of express requests.
 *
 *	4.  Compare the desired LBN with the current LBN.  This will determine
 *	    whether the TCB should be serviced with the disk head moving up or
 *	    down (desired LBN > current LBN implies up, desired LBN < current
 *	    LBN implies down, desired LBN = current LBN implies that, because
 *	    of fairness considerations, this TCB should not be serviced before
 *	    the disk head reverses its current direction).
 *
 *	5.  Four cases will arise in step 4 above:
 *
 *		a.  current direction = up, desired direction = up
 *		b.  current direction = up, desired direction = down
 *		c.  current direction = down, desired direction = down
 *		d.  current direction = down, desired direction = up
 *
 *	    Cases a and c are handled by scanning through the remaining TCBs
 *	    looking for either a TCB going in the opposite direction (in which
 *	    case the new TCB is inserted before the direction-reversing TCB),
 *	    or a TCB whose LBN is further away from the current LBN than the
 *	    new TCB's LBN is (in which case the new TCB is inserted before the
 *	    further-away TCB).  Cases b and d are handled similarly, with the
 *	    added necessity of first skipping those TCBs whose direction is
 *	    opposite to the desired direction.
 */
insert_tcb( tcb )
register struct $tcb *tcb;
    {
    register struct $ucb *ucb;
    register struct $tcb *thistcb;
    struct $tcb *lasttcb;

    ucb = TCB.ucb;
    /*
     *  if the queue is empty, insert the new TCB here
     */
    thistcb = &UCB.tcbs;
    if( nextTCB == null )
	goto EXIT_1;
    /*
     *  skip all express TCBs
     */
    while( thisTCB.modifiers & tm_exp )
	if( nextTCB == null )
	    goto EXIT_1;
    /*
     *  if this is an express TCB, insert it here
     */
    if( TCB.modifiers & tm_exp )
	goto EXIT_1;
    /*
     *  check for the four cases as described above
     */
    if( UCB.state & us_up )
	{
	if( TCB.block > UCB.block )
	    {
	    /*
	     *  case a:
	     *
	     *	    1)  set TCB direction to up
	     *	    2)  insert when lastTCB.block < TCB.block < thisTCB.block
	     *		or when the first TCB whose direction is not also up
	     *		is reached
	     */
	    TCB.type |= tt_up;
	    while( thisTCBup && !( TCB.block < thisTCB.block ) )
		if( nextTCB == null )
		    goto EXIT_2;
	    goto EXIT_2;
	    }
	else
	    {
	    /*
	     *  case b:
	     *
	     *	    1)  set TCB direction to down (the default)
	     *	    2)  skip all TCBs whose direction is up
	     *	    3)  insert when lastTCB.block > TCB.block > thisTCB.block
	     */
	    while( thisTCBup || !( TCB.block > thisTCB.block ) )
		if( nextTCB == null )
		    goto EXIT_2;
	    goto EXIT_2;
 	    }
	}
    else
	{
	if( TCB.block < UCB.block )
	    {
	    /*
	     *  case c:
	     *
	     *	    1)  set TCB direction to down (the default)
	     *	    2)  insert when lastTCB.block > TCB.block > thisTCB.block
	     *		or when the first TCB whose direction is not also down
	     *		is reached
	     */
	    while( !thisTCBup && !( TCB.block > thisTCB.block ) )
		if( nextTCB == null )
		    goto EXIT_2;
	    goto EXIT_2;
	    }
	else
	    {
	    /*
	     *  case d:
	     *
	     *	    1)  set TCB direction to up
	     *	    2)  skip all TCBs whose direction is down
	     *	    3)  insert when lastTCB.block < TCB.block < thisTCB.block
	     */
	    TCB.type |= tt_up;
	    while( !thisTCBup || !( TCB.block < thisTCB.block ) )
		if( nextTCB == null )
		    goto EXIT_2;
	    goto EXIT_2;
	    }
	}
EXIT_1:
    /*
     *  set the direction bit properly
     */
    if( UCB.state & us_up )
	{
	if( TCB.block > UCB.block )
	    TCB.type |= tt_up;
	}
    else
	{
	if( TCB.block >= UCB.block )
	    TCB.type |= tt_up;
	}
EXIT_2:
    /*
     *  fix up queue pointers and exit
     */
    lastTCB.link = tcb;
    TCB.link = thistcb;
    }

#define TCB (*tcb)

/*
 *  this routine translates an error code from er_xxx to st_xxx
 *
 *  The controller-specific "er_xxx" error codes are translated into generic
 *  MSCP "st_xxx" error codes.  If appropriate, the error is logged to the
 *  host.  Fatal errors (if any) override warning errors.
 */
word st_err( tcb )
register struct $tcb *tcb;
    {
    register word error;

    /*
     *  get the fatal error, or the warning error, or success
     */
    if( !( error = TCB.fatal ) )
	if( !( error = TCB.warning ) )
	    return( st_suc );
    /*
     *  these errors are not severe, and do not get reported to the error log
     */
    if( error & er_cnt )
	return( st_cnt + st_sub * 0 );
    if( error & er_ddm )
	return( st_dat + st_sub * 0 );
    if( error & er_dnr )
	return( st_ofl + st_sub * 1 );
    /*
     *  these errors are severe, and get reported to the error log
     */
    if( error & er_nem )
	{
	error = st_hst + st_sub * 3;
	do_hbe( tcb, error );
	return( error );
	}
    if( error & er_mpe )
	{
	error = st_hst + st_sub * 4;
	do_hbe( tcb, error );
	return( error );
	}
    if( error & er_pte )
	{
	error = st_hst + st_sub * 5;
	do_hbe( tcb, error );
	return( error );
	}
    if( error & er_hce )
	{
	error = st_dat + st_sub * 2;
	do_dte( tcb, error );
	return( error );
	}
    if( error & er_ecc )
	{
	error = st_dat + st_sub * 7;
	do_dte( tcb, error );
	return( error );
	}
    if( error & er_cnw )
	{
	error = st_drv + st_sub * 7;
	do_dte( tcb, error );
	return( error );
	}
    /*
     *  if none of this makes sense, just report a vague "drive error"
     */
    error = st_drv + st_sub * 0;
    do_dte( tcb, error );
    return( error );
    }

#define TCB (*tcb)

/*
 *  this routine will zero the data buffer for ERASE commands
 */
zero( tcb )
register struct $tcb *tcb;
    {
    register word *data_ptr;
    register word i;

    if( TCB.count < data_size )
	i = ( ( ( unsigned ) TCB.count + 511 ) & ~511 ) / 2;
    else
	i = data_size / 2;
    data_ptr = &data[0];
    for( ; --i >= 0; )
	*data_ptr++ = 0;
    }
