/*
 *  file = 9224.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  this module contains the low-level disk I/O routines
 */

#include "defs.h"
#include "tcb.h"
#include "ucb.h"

#define		op_res		0x00
#define		op_dd		0x01
#define		op_si1		0x04
#define		op_srp		0x40
#define		op_sri		0x54
#define		op_rsl		0x5D
#define		op_wsl		0xA0

extern word rw$pll;
extern byte r$dat;
extern byte r$cmd;
extern byte w$dat;
extern byte w$cmd;

extern list sec_job;
extern list udc_job;
extern list dma;
extern list udc;
extern byte *_packet;
extern byte *rx_sector;
extern byte *_src;
extern byte *_dst;
extern word rx_opcode;
extern word rx_number;
extern word _len;
extern word _hs3;
extern word reg_7;
extern word reg_8;
extern word reg_9;
extern word mo_time;
extern word cd_time;
extern word pd_flag;
extern word sd_flag;
extern word sec_flag;
extern word total_sectors;
extern word unit_base;
extern long _qbus;

extern word get_udc( );

/*
 *  this routine will initialize the SMC9224 chip
 *
 *  The internal registers of the SMC9224 are set to default values, and all
 *  timers are turned off.
 */
init_udc( )
    {
    pd_flag = 0;
    sd_flag = 0;
    w$cmd = op_res;
    reg_7 = 0xF2;
    reg_8 = 0x82;
    reg_9 = 0xB9;
    w$cmd = op_srp + 7;
    w$dat = reg_7;
    w$dat = reg_8;
    w$dat = reg_9;
    /*
     *  force the "user programmable output bits" to be flushed
     */
    put_udc( op_dd );
    }

/*
 *  this routine will write the command register of the SMC9224 and wait
 *
 *  There is a bug in the SMC9224 which can destroy DMA activity unless the
 *  register pointer is left pointing to 10.
 */
put_udc( opcode )
register word opcode;
    {
    udc_job = null;
    w$cmd = op_srp + 10;
    w$cmd = opcode;
    $block( &udc_job );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will report the status of the last operation
 *
 *  We decode the bits in the SMC9224 interrupt status, chip status, and disk
 *  status registers in order to figure out what happened.  We further save
 *  these values (and a few others) for later use in an error log message, if
 *  required.
 */
word get_udc( type, tcb )
word type;
register struct $tcb *tcb;
    {
    word stat;
    struct $ucb *ucb;
    register word chip, disk;

    /*
     *  if there really WAS no error, just return
     */
    if( !( stat = r$cmd & ( bit4|bit3 ) ) )
	return( 0 );
    /*
     *  get interrupt status, chip status, and disk status
     */
    TCB.stat = r$cmd;
    w$cmd = op_srp + 8;
    chip = TCB.chip = r$dat;
    disk = TCB.disk = r$dat;
    w$cmd = op_srp + 3;
    /*
     *  might as well do these too
     */
    TCB.csec = r$dat;
    TCB.csur = r$dat;
    TCB.ccyl = r$dat;
    /*
     *  fix state of ready bit for RX33
     */
    ucb = TCB.ucb;
    if( UCB.state & us_33 )
	{
	w$cmd = op_srp + 7;
	w$dat = reg_7 & ~bit2;
	put_udc( sd_flag );
	w$cmd = op_srp + 9;
	if( r$dat & bit1 )
	    disk &= ~bit1;
	}
#if debug>=1
    if( !( disk & bit1 ) )
	printf( "\nDRIVE NOT READY error" );
    if( ( type & tt_wr ) && ( disk & bit0 ) )
	printf( "\nWRITE FAULT error" );
    if( !( stat & bit4 ) )
	{
	printf( "\nREAD ID error" );
	if( chip & bit5 )
	    printf( ", CRC/ECC ERROR" );
	if( chip & bit3 )
	    printf( ", SYNC ERROR" );
	}
    else if( !( stat & bit3 ) )
	{
	printf( "\nSEEK error" );
	if( chip & bit5 )
	    printf( ", CRC/ECC ERROR" );
	if( ( type & tt_rd ) && ( chip & bit4 ) )
	    printf( ", DELETED DATA MARK" );
	if( chip & bit3 )
	    printf( ", SYNC ERROR" );
	if( chip & bit2 )
	    printf( ", COMPARE ERROR" );
	}
    else
	{
	printf( "\nDATA error" );
	if( chip & bit5 )
	    printf( ", CRC/ECC ERROR" );
	if( ( type & tt_rd ) && ( chip & bit4 ) )
	    printf( ", DELETED DATA MARK" );
	if( chip & bit3 )
	    printf( ", SYNC ERROR" );
	if( disk & bit0 )
	    printf( ", WRITE FAULT" );
	}
#endif
    /*
     *  perform deselect/select to clear possible error condition
     */
    put_udc( op_dd );
    w$cmd = op_srp + 7;
    w$dat = reg_7;
    w$dat = reg_8;
    put_udc( sd_flag );
    /*
     *  dispatch on error bits
     */
    if( !( disk & bit1 ) )
	return( er_dnr + er_fat );
    if( ( type & tt_wr ) && ( disk & bit0 ) )
	return( er_cnw );
    if( !( stat & bit4 ) )
	{
	if( chip & ( bit5|bit3 ) )
	    return( er_rf + er_hce );
	}
    else if( !( stat & bit3 ) )
	{
	if( chip & ( bit5|bit3|bit2 ) )
	    return( er_sf + er_hce );
	if( ( type & tt_rd ) && ( chip & bit4 ) )
	    return( er_sf + er_ddm + er_fat );
	}
    else
	{
	if( chip & bit5 )
	    return( er_df + er_ecc );
	if( chip & bit3 )
	    return( er_df + er_hce );
	if( ( type & tt_rd ) && ( chip & bit4 ) )
	    return( er_df + er_ddm + er_fat );
	}
    return( er_rf + er_sf + er_df );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will read from a floppy disk unit
 *
 *  We try the read several times unless the "suppress error recovery"
 *  modifier was specified.  An operation consists of loading the SMC9224
 *  chip with the desired values, dropping in an opcode, and then waiting for
 *  an interrupt from the SMC9224.  There are two types of interrupt:  the
 *  "sector done" interrupt, which is used to trigger DMA activity to the host
 *  (since another sector has just been finished), and the "command done"
 *  interrupt, which is the final interrupt we will get (any expected "sector
 *  done" interrupts will not come after this one).
 */
word rx_read( tcb )
register struct $tcb *tcb;
    {
    register struct $ucb *ucb;
    word i, error, foo, bar, gag;

    ucb = TCB.ucb;
    /*
     *  here is the opcode to use
     */
    rx_opcode = op_rsl;
    /*
     *  set the "motor on" bit, and in addition set or clear the "reduced
     *  write current" bit based on the cylinder being read from; if these
     *  bits were not already set correctly, then force them out
     */
    mo_time = 0;
    i = reg_7;
    reg_7 |= bit0;
    if( UCB.state & us_33 )
	if( TCB.cylinder >= UCB.rccyl )
	    reg_7 |= bit3;
	else
	    reg_7 &= ~bit3;
    if( reg_7 != i )
	{
	w$cmd = op_srp + 7;
	w$dat = reg_7;
	put_udc( sd_flag );
	}
    _hs3 = 0;
AGAIN:
    for( i = ( TCB.modifiers & tm_ser ) ? 2 : rqdx3_retry+1; --i >= 0; )
	{
	/*
	 *  load all parameters
	 */
	rx_sector = TCB.sector;
	if( UCB.state & us_tsf )
	    {
	    foo = *rx_sector++;
	    bar = 1;
	    gag = TCB.number;
	    }
	else
	    {
	    foo = TCB.sector;
	    bar = TCB.number;
	    gag = 0;
	    }
	w$cmd = op_srp + 0;
	w$dat = ( ( byte * ) &TCB.buffer )[lsb];
	w$dat = ( ( byte * ) &TCB.buffer )[msb];
	w$dat = 0;
	w$dat = foo;
	w$dat = TCB.surface + ( ( TCB.cylinder >> 4 ) & 0xF0 );
	w$dat = TCB.cylinder;
	w$dat = bar;
	rx_number = gag;
	/*
	 *  if we are to copy the data to the Q-bus as it is being read, then
	 *  set up all counters which will be needed as we go; if not, then
	 *  the operation is greatly simplified
	 *
	 *  because of the bug in the gate array, we may get spurious "sector
	 *  done" interrupts and thus copy data to the Q-bus too early; we
	 *  must keep a count of how many we get, and if we get too many, one
	 *  of the ones we got was spurious -- but since we cannot tell WHICH
	 *  one was spurious, we just retry the whole operation
	 */
	if( TCB.type & tt_wtq )
	    {
	    sec_flag = 0;
	    total_sectors = 0;
	    sec_job = null;
	    udc_job = null;
	    w$cmd = op_srp + 10;
	    cd_time = 4;
	    w$cmd = rx_opcode;
	    _src = TCB.buffer;
	    _qbus = TCB.qbus;
	    _len = TCB.segment;
LOOP:
	    /*
	     *  we wait for something to happen, then lock access to the DMA
	     *  engine and send over our data, keeping track of how many
	     *  sectors we do this way
	     */
	    $block( &sec_job );
	    if( cd_time == 0 )
		{
		TCB.result = 0;
		return( er_dnr + er_fat );
		}
	    $acquire( &dma );
	    _packet = TCB.pkt;
	    while( sec_flag > 0 )
		{
		--sec_flag;
		total_sectors++;
		/*
		 *  the remaining byte count must be positive, or else we have
		 *  had at least one spurious "sector done" interrupt
		 */
		if( _len > 0 )
		    TCB.fatal |= put_buffer( _qbus, _src,
			    _len < 512 ? _len : 512 );
		_src += 512;
		_qbus += 512;
		_len -= 512;
		}
	    /*
	     *  unlock the DMA engine (so someone else can use it) and loop if
	     *  either the count of "sector done" interrupts is greater than
	     *  zero, or if we have not received the final "command done"
	     *  interrupt from the SMC9224
	     */
	    $release( &dma );
	    if( ( udc_job == null ) || ( sec_flag > 0 ) )
		goto LOOP;
	    }
	else
	    {
	    /*
	     *  just start the operation and wait for it to finish
	     */
	    sec_flag = 0;
	    cd_time = 4;
	    put_udc( rx_opcode );
	    if( cd_time == 0 )
		{
		TCB.result = 0;
		return( er_dnr + er_fat );
		}
	    total_sectors = sec_flag;
	    }
	cd_time = 0;
	/*
	 *  see how well we did
	 */
	if( r$cmd & ( bit4|bit3 ) )
	    {
	    error = get_udc( tt_rd, tcb );
	    if( UCB.state & us_tsf )
		{
		TCB.result = TCB.number + rx_number;
		}
	    else
		{
		w$cmd = op_srp + 3;
		TCB.result = r$dat - TCB.sector;
		}
	    if( error & er_fat )
		break;
	    }
	else
	    {
	    error = 0;
	    TCB.result = TCB.number;
	    break;
	    }
	/*
	 *  see if the error is the bogus CRC error of the SMC9224
	 */
	if( ( error & er_ecc ) && !( error = apply_ecc( tcb ) ) )
	    {
	    total_sectors++;
	    break;
	    }
	/*
	 *  a failure at this point may be merely the head bouncing
	 */
	$sleep( 36 );
	}
    /*
     *  this is weird:  a "seek error" coupled with "deleted data mark" means
     *  that the SMC9224 has incorrectly incremented the sector counter, so
     *  we must subtract one to fix it
     */
    if( ( error & er_sf ) && ( error & er_ddm ) )
	--TCB.result;
    /*
     *  any transfer which ends with a "deleted data mark" will have delivered
     *  an extra "sector done" interrupt, so account for it
     */
    if( error & er_ddm )
	--total_sectors;
    /*
     *  see if the gate array bug has bitten us
     */
    if( total_sectors != TCB.result )
	{
#if debug>=1
	printf( "\nerror in read, tot_sec = %d, TCB.result = %d",
		total_sectors, TCB.result );
#endif
	goto AGAIN;
	}
    return( error );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will write to a floppy disk unit
 *
 *  We try the write several times unless the "suppress error recovery"
 *  modifier was specified.  An operation consists of loading the SMC9224 chip
 *  with the desired values, dropping in an opcode, and then waiting for an
 *  interrupt from the SMC9224.  There are two types of interrupt:  the
 *  "sector done" interrupt, which is used to trigger DMA activity to the host
 *  (since another sector has just been finished), and the "command done"
 *  interrupt, which is the final interrupt we will get (any expected "sector
 *  done" interrupts will not come after this one).
 */
word rx_write( tcb )
register struct $tcb *tcb;
    {
    register word *data_ptr;
    register struct $ucb *ucb;
    word i, j, k, error, foo, bar, gag;

    ucb = TCB.ucb;
    /*
     *  here is the opcode to use; we set the amount of precompensation needed
     *  based on the cylinder being written to
     */
    rx_opcode = op_wsl;
    if( TCB.cylinder >= UCB.pccyl )
	rx_opcode |= 4;
    else
	rx_opcode |= 1;
    /*
     *  set the "motor on" bit, and in addition set or clear the "reduced
     *  write current" bit based on the cylinder being written to; if these
     *  bits were not already set correctly, then force them out and wait
     *  for half a second or so for everything to settle down
     */
    mo_time = 0;
    i = reg_7;
    reg_7 |= bit0;
    if( TCB.cylinder >= UCB.rccyl )
	reg_7 |= bit3;
    else
	{
	reg_7 &= ~bit3;
	if( ( _hs3 < 0 ) && ( _hs3 & bit3 ) )
	    i |= bit3;
	}
    if( reg_7 != i )
	{
	w$cmd = op_srp + 7;
	w$dat = reg_7;
	put_udc( sd_flag );
	i ^= reg_7;
	i &= ( UCB.state & us_33 ) ? ~( bit3|bit0 ) : ~bit0;
	if( i )
	    $sleep( 500 );
	}
    _hs3 = 0;
AGAIN:
    for( i = ( TCB.modifiers & tm_ser ) ? 2 : rqdx3_retry+1; --i >= 0; )
	{
	/*
	 *  load all parameters
	 */
	rx_sector = TCB.sector;
	if( UCB.state & us_tsf )
	    {
	    foo = *rx_sector++;
	    bar = 1;
	    gag = TCB.number;
	    }
	else
	    {
	    foo = TCB.sector;
	    bar = TCB.number;
	    gag = 0;
	    }
	w$cmd = op_srp + 0;
	w$dat = ( ( byte * ) &TCB.buffer )[lsb];
	w$dat = ( ( byte * ) &TCB.buffer )[msb];
	w$dat = 0;
	w$dat = foo;
	w$dat = TCB.surface + ( ( TCB.cylinder >> 4 ) & 0xF0 );
	w$dat = TCB.cylinder;
	w$dat = bar;
	/*
	 *  do explicit seeks since we cannot wait for the head to settle any
	 *  other way; only if we can do the seek successfully will we attempt
	 *  to do the write
	 */
	for( j = 4; --j >= 0; )
	    {
	    cd_time = 4;
	    put_udc( op_sri );
	    if( cd_time == 0 )
		{
		TCB.result = 0;
		return( er_dnr + er_fat );
		}
	    cd_time = 0;
	    $sleep( 36 );
	    if( !( error = get_udc( tt_wr, tcb ) ) || ( error & er_fat ) )
		break;
	    }
	if( !error )
	    {
	    rx_number = gag;
	    /*
	     *  if we are to copy the data from the Q-bus as it is being
	     *  written, then set up all counters which will be needed as we
	     *  go; if not, then the operation is greatly simplified
	     *
	     *  because of the bug in the gate array, we may get spurious
	     *  "sector done" interrupts and thus we may think that the disk
	     *  write operation is getting ahead of the Q-bus read operation;
	     *  since we must plan for this happening anyway, the bug causes
	     *  no additional problems -- if we find that the disk writing has
	     *  caught up to or passed the Q-bus reading, that is a fatal
	     *  error which causes us to retry the whole operation
	     */
	    if( TCB.type & tt_rfq )
		{
		sec_flag = -1;
		udc_job = null;
		_dst = TCB.buffer;
		_qbus = TCB.qbus;
		_len = TCB.segment;
		/*
	 	 *  we lock access to the DMA engine and begin copying data
		 *  from the Q-bus, one block at a time; as each block is
		 *  read from the Q-bus, we decrement our counter and start
		 *  copying another one; as each block is written to the disk,
		 *  we increment our counter -- and if the counter is ever
		 *  positive the disk has caught up to the DMA engine
		 */
		$acquire( &dma );
		_packet = TCB.pkt;
		TCB.fatal |= get_buffer( _qbus, _dst,
			_len < 512 ? _len : 512 );
		_dst += 512;
		_qbus += 512;
		_len -= 512;
		/*
		 *  if the last block is a partial one, zero the end of it
		 */
		if( TCB.segment & 511 )
		    {
		    k = ( unsigned ) ( 512 - ( TCB.segment & 511 ) ) / 2;
		    data_ptr = TCB.buffer + TCB.segment;
		    for( ; --k >= 0; )
			*data_ptr++ = 0;
		    }
		/*
		 *  start the disk writing, and start a loop of reading from
		 *  the Q-bus
		 */
		w$cmd = op_srp + 10;
		cd_time = 4;
		w$cmd = rx_opcode;
		while( _len > 0 )
		    {
		    TCB.fatal |= get_buffer( _qbus, _dst,
			    _len < 512 ? _len : 512 );
		    _dst += 512;
		    _qbus += 512;
		    _len -= 512;
		    /*
		     *  if the disk has caught up, wait for all activity to
		     *  finish and then restart everything
		     */
		    if( sec_flag >= 0 )
			{
#if debug>=1
			printf( "\nerror in write, sec_flag = %d", sec_flag );
#endif
			$release( &dma );
			$block( &udc_job );
			goto AGAIN;
			}
		    /*
		     *  count the block read from the Q-bus
		     */
		    --sec_flag;
		    }
		/*
		 *  all done with our Q-bus stuff, so now just wait for the
		 *  disk to finish writing
		 */
		$release( &dma );
		$block( &udc_job );
		if( cd_time == 0 )
		    {
		    TCB.result = 0;
		    return( er_dnr + er_fat );
		    }
		}
	    else
		/*
		 *  just start the operation and wait for it to finish
		 */
		{
		cd_time = 4;
		put_udc( rx_opcode );
		if( cd_time == 0 )
		    {
		    TCB.result = 0;
		    return( er_dnr + er_fat );
		    }
		}
	    cd_time = 0;
	    /*
	     *  wait for the specified tunnel erase time
	     */
	    $sleep( 3 );
	    /*
	     *  see how well we did
	     */
	    if( r$cmd & ( bit4|bit3 ) )
		{
		error = get_udc( tt_wr, tcb );
		if( UCB.state & us_tsf )
		    {
		    TCB.result = TCB.number + rx_number;
		    }
		else
		    {
		    w$cmd = op_srp + 3;
		    TCB.result = r$dat - TCB.sector;
		    }
		if( error & er_fat )
		    break;
		}
	    else
		{
		error = 0;
		TCB.result = TCB.number;
		break;
		}
	    }
	else if( error & er_fat )
	    break;
	}
    return( error );
    }

/*
 *  this is the "select drive" timeout action routine
 *
 *  After a few seconds of no activity, any currently selected drive should
 *  be deselected.  So, grab control of the SMC9224, wait for any pending poll
 *  drive command to finish, and send over a deselect command.  Then let go
 *  of the SMC9224 and return.
 */
sd_tmo( )
    {
    $acquire( &udc );
    while( pd_flag != 0 )
	$sleep( 1 );
    put_udc( op_dd );
    sd_flag = 0;
    $release( &udc );
    }

/*
 *  this is the "motor on" timeout action routine
 *
 *  After a few seconds of no activity, the floppy disk motor should be turned
 *  off and any active drive should be deselected.  So, grab control of the
 *  SMC9224, wait for any pending poll drive command to finish, turn off
 *  the "motor on" bit, and send over a deselect command.  Then let go of the
 *  SMC9224 and return.
 */
mo_tmo( )
    {
    $acquire( &udc );
    while( pd_flag != 0 )
	$sleep( 1 );
    reg_7 &= ~bit0;
    w$cmd = op_srp + 7;
    w$dat = reg_7;
    put_udc( op_dd );
    sd_flag = 0;
    $release( &udc );
    }

/*
 *  this is the "command done" timeout action routine
 *
 *  If a command has not completed within the given timeout interval, then
 *  it is aborted and the chip is reinitialized.  This usually means that the
 *  drive is not delivering any data (so that no clock can be recovered to
 *  give to the SMC9224).
 */
cd_tmo( )
    {
    w$cmd = op_res;
    w$cmd = op_srp + 7;
    w$dat = reg_7;
    w$dat = reg_8;
    w$dat = reg_9;
    w$cmd = sd_flag;
    }
