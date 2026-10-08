/*
 *  file = DOFMT.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the FORMAT command
 */

#include "defs.h"
#include "pkt.h"
#include "ucb.h"
#include "mscp.h"

#define		gap0		80
#define		gap1		50
#define		gap2		22
#define		gap3		84
#define		sync		12

#define		op_si1		0x05
#define		op_srp		0x40
#define		op_ft		0x64

extern byte r$dat;
extern byte r$cmd;
extern byte w$dat;
extern byte w$cmd;

extern word reg_7;
extern word fpl;
extern word cd_time;
extern word mo_time;
extern word sd_flag;
extern word clk_flag;

extern byte *get_ucb( );

/*
 *  the FORMAT command packet
 */
struct $fmtc
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_r2;
    word	p_mod;
    word	p_bcnt[2];
    word	p_buff[6];
    word	p_fmti[2];
    };

/*
 *  the FORMAT response packet
 */
struct $fmtr
    {
    long	p_crf;
    word	p_unit;
    word	p_r1;
    byte	p_opcd;
    byte	p_flgs;
    word	p_sts;
    };

#define		rs_fmt		sizeof( struct $fmtr )

#define PKT (*pkt)
#define CMD (*(struct $fmtc *)&(PKT.data))
#define RSP (*(struct $fmtr *)&(PKT.data))
#define UCB (*ucb)

/*
 *  process a FORMAT command
 *
 *  This is a sequential command, so if there are any non-sequential commands
 *  outstanding, we must hold this command pending until they complete; if not
 *  (the UCB.tcbs list is empty), then we can proceed with our validity
 *  checks.  First, the unit must not be online to the host; this command can
 *  change the unit size (by reformatting from low density to high density)
 *  and this is not allowed of units which are online.  If the unit is online,
 *  return a status of "available, already in use".  Next, the unit must be
 *  an RX33, since that is the only floppy drive which can format its own 
 *  media, and only floppy drives are supported by this command; if not, a
 *  status of "invalid command, illegal opcode" is returned.  Next, the sign
 *  bit of the "format information" field must be set, or a status of "invalid
 *  command, illegal format information" is given.  Finally, the unit must
 *  have media inserted, or else a status of "offline, no volume mounted" is
 *  given, and the media must not be write protected, or else a status of
 *  "write protected, by hardware" is returned.  If all of these things are
 *  okay, then we can format the media, in high density RX33 mode.  
 */
do_fmt( pkt )
register struct $pkt *pkt;
    {
    register struct $ucb *ucb;

#if debug>=1
    printf( "\nFORMAT, unit = %d", CMD.p_unit );
#endif
    RSP.p_flgs = 0;
    /*
     *  get the UCB corresponding to the given unit number; a NULL means that
     *  there is none, which gets an "offline" status
     */
    if( ( ucb = get_ucb( CMD.p_unit ) ) == null )
	RSP.p_sts = st_ofl;
    else
	{
	/*
	 *  lock the UCB data structure for our personal use
	 */
	$acquire( &UCB.ucb );
	/*
	 *  any non-sequential commands in progress?  if so, simply add this
	 *  PKT to the end of the pending sequential PKTs list, and we will
	 *  get back to it eventually
	 */
	if( ( UCB.tcb != null ) || ( UCB.tcbs != null ) )
	    {
	    $enq_tail( &UCB.pkts, pkt );
	    $release( &UCB.ucb );
	    return;
	    }
	/*
	 *  return a status of "available, already in use", "invalid command,
	 *  illegal opcode", or possibly "invalid command, illegal format
	 *  information" if any of these are appropriate; otherwise, do the 
	 *  actual formatting and return its status to the host
	 */
	if( UCB.state & us_onl )
	    {
	    RSP.p_sts = st_avl + st_sub * 32;
	    /*
	     *  make the unit transition to the "available" state, as required
	     */
	    UCB.state &= ~( us_onl|us_imf );
	    UCB.flags &= ( uf_wph|uf_rpl|uf_rmv );
	    fpl &= ~UCB.wp_bit;
	    }
	else if( !( UCB.state & us_33 ) )
	    RSP.p_sts = st_cmd + i_opcd;
	else if( !( CMD.p_fmti[1] & bit15 ) )
	    RSP.p_sts = st_cmd + i_fmti;
	else
	    RSP.p_sts = _do_fmt( ucb );
	/*
	 *  since we're all done, unlock the UCB data structure
	 */
	$release( &UCB.ucb );
	}
    RSP.p_opcd |= op_end;
    PKT.size = rs_fmt;
    PKT.type = mt_seq;
    put_packet( pkt );
    }

/*
 *  actually process a FORMAT command
 */
#define UCB (*ucb)

word _do_fmt( ucb )
register struct $ucb *ucb;
    {
    byte id_table[15][4];
    word i, j, k, error;

    /*
     *  if the unit has no media present or is write protected, return the
     *  appropriate status
     */
    if( UCB.state & us_ofl )
	return( st_ofl + st_sub * 1 );
    if( UCB.flags & uf_wph )
	return( st_wpr + st_sub * 256 );
    /*
     *  set up for high density RX33 media, and select the unit
     */
    rx33_media( ucb );
    sd_flag = 0;
    select( ucb );
    /*
     *  make sure we are at track 0
     */
    restore( ucb );
    /*
     *  set the "motor on" bit and clear the "reduced write current" bit (this
     *  is the setting for high speed), then force these bits out and wait
     *  for half a second or so for everything to settle down
     */
    mo_time = 0;
    reg_7 |= bit0;
    reg_7 &= ~bit3;
    w$cmd = op_srp + 7;
    w$dat = reg_7;
    put_udc( sd_flag );
    $sleep( 500 );
    /*
     *  make sure that media is actually present, by looking for an index
     *  pulse to occur within a few seconds
     */
    error = st_suc;
    clk_flag = 0;
    while( w$cmd = op_srp + 9, !( r$dat & bit6 ) )
	if( clk_flag >= 2000 )
	    {
	    error = st_ofl + st_sub * 1;
	    goto EXIT;
	    }
    /*
     *  step through the cylinders and surfaces, formatting sectors as we go
     */
    for( i = 0; i < UCB.cyl; i++ )
	{
	for( j = 0; j < UCB.sur; j++ )
	    {
	    /*
	     *  build the id table
	     */
	    for( k = 0; k < UCB.sec; k++ )
		{
		id_table[k][0] = i;
		id_table[k][1] = j;
		id_table[k][2] = k+1;
		id_table[k][3] = 2;
		}
	    /*
	     *  let the SMC9224 know the table's address
	     */
	    k = &id_table;
	    w$cmd = op_srp + 0;
	    w$dat = ( ( byte * ) &k )[lsb];
	    w$dat = ( ( byte * ) &k )[msb];
	    w$dat = 0;
	    put_udc( sd_flag );
	    /*
	     *  load in the formatting parameters
	     */
	    w$cmd = op_srp + 0;
	    w$dat = -gap0;
	    w$dat = -gap1;
	    w$dat = -gap2;
	    w$dat = -gap3;
	    w$dat = j;
	    w$dat = ~sync;
	    w$dat = ~UCB.sec;
	    w$dat = ~4;
	    /*
	     *  format a track; if the operation fails, return a status of
	     *  "drive error" to the host
	     */
	    cd_time = 4;
	    put_udc( op_ft );
	    if( ( r$cmd & ( bit4|bit3 ) ) || ( cd_time == 0 ) )
		error = st_drv;
	    cd_time = 0;
	    /*
	     *  restore the user-programmable output bits
	     */
	    w$cmd = op_srp + 7;
	    w$dat = reg_7;
	    if( error )
		goto EXIT;
	    }
	/*
	 *  we are done with this cylinder, so step to the next one
	 */
	put_udc( op_si1 );
	}
EXIT:
    /*
     *  all done, so clean up and return
     */
    restore( ucb );
    deselect( ucb );
    return( error );
    }
