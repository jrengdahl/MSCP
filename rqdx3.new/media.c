/*
 *  file = MEDIA.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  this module contains the device media routines
 */

#include "defs.h"
#include "tcb.h"
#include "ucb.h"
#include "uib.h"
#include "mscp.h"

#define		op_srp		0x40
#define		op_rt		0x5A

extern byte w$dat;
extern byte w$cmd;

extern list mem;
extern list tcbs;
extern byte data[];
extern word stalling;
extern word reg_7;
extern word sd_flag;

extern byte *$deqf_head( );
extern word get_disk( );
extern word rd_uib( );

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will fill a unit control block with RD media parameters
 */
rd_media( ucb )
register struct $ucb *ucb;
    {
    register word error;
    register struct $tcb *tcb;

    tcb = $deqf_head( &tcbs );
    TCB.ucb = ucb;
    $acquire( &mem );
    if( !( error = rd_uib( tcb ) ) )
	{
	uib_to_ucb( TCB.buffer, ucb );
	UCB.state &= ~us_unk;
	select( ucb );
	w$cmd = op_srp + 7;
	w$dat = reg_7 | bit3;
	put_udc( sd_flag );
	w$cmd = op_srp + 0;
	w$dat = ( ( byte * ) &TCB.buffer )[lsb];
	w$dat = ( ( byte * ) &TCB.buffer )[msb];
	w$dat = 0;
	put_udc( op_rt );
	if( ( data[1] & 0x0F ) == 0 )
	    UCB.state &= ~us_hs3;
	w$cmd = op_srp + 7;
	w$dat = reg_7;
	put_udc( sd_flag );
	deselect( ucb );
	}
    else if( error == st_mfe )
	{
	UCB.state |= us_mfe;
	UCB.state &= ~us_unk;
	}
    $release( &mem );
    $enq_head( &tcbs, tcb );
    }

#define UCB (*ucb)

/*
 *  this routine will fill a unit control block with RX33 media parameters
 */
rx33_media( ucb )
register struct $ucb *ucb;
    {
    UCB.pllctl |= bit8;
    UCB.op_sd &= ~4;
    UCB.mode = 0x0082;
    UCB.pccyl = 0;
    UCB.rccyl = 80;
    UCB.sec = 15;
    UCB.sur = 2;
    UCB.cyl = 80;
    UCB.hostsize = 2400;
    UCB.spt = 15;
    UCB.tpg = 2;
    UCB.gpc = 1;
    UCB.state &= ~us_tsf;
    }

#define UCB (*ucb)

/*
 *  this routine will fill a unit control block with RX50 media parameters
 */
rx50_media( ucb )
register struct $ucb *ucb;
    {
    UCB.pllctl &= ~bit8;
    UCB.op_sd |= 4;
    if( UCB.state & us_50 )
	{
	UCB.mode = 0x0082;
	UCB.pccyl = 56;
	UCB.rccyl = 44;
	}
    else
	{
	UCB.mode = 0x0081;
	UCB.pccyl = 80;
	UCB.rccyl = 0;
	}
    UCB.sec = 10;
    UCB.sur = 1;
    UCB.cyl = 80;
    UCB.hostsize = 800;
    UCB.spt = 10;
    UCB.tpg = 5;
    UCB.gpc = 16;
    UCB.state |= us_tsf;
    }

#define UCB (*ucb)
#define UIB (*uib)

/*
 *  this routine will fill a unit control block with unit information block
 *  parameters (read from XBN 0 )
 */
uib_to_ucb( uib, ucb )
register struct $uib *uib;
register struct $ucb *ucb;
    {
    register word n;
    long xbn, dbn;

    ( ( word * ) &xbn )[lsw] = UIB.xbnsize[0];
    ( ( word * ) &xbn )[msw] = UIB.xbnsize[1];
    ( ( word * ) &dbn )[lsw] = UIB.dbnsize[0];
    ( ( word * ) &dbn )[msw] = UIB.dbnsize[1];
    ( ( word * ) &UCB.lbnsize )[lsw] = UIB.lbnsize[0];
    ( ( word * ) &UCB.lbnsize )[msw] = UIB.lbnsize[1];
    ( ( word * ) &UCB.rbnsize )[lsw] = UIB.rbnsize[0];
    ( ( word * ) &UCB.rbnsize )[msw] = UIB.rbnsize[1];
    UCB.lbnbase = xbn + dbn;
    UCB.rbnbase = UCB.lbnbase + UCB.lbnsize;
    ( ( word * ) &UCB.media )[lsw] = UIB.media[0];
    ( ( word * ) &UCB.media )[msw] = UIB.media[1];
    ( ( word * ) &UCB.volume )[lsw] = UIB.volume[0];
    ( ( word * ) &UCB.volume )[msw] = UIB.volume[1];
    UCB.sec = UIB.sec;
    UCB.sur = UIB.sur;
    UCB.cyl = UIB.cyl;
    UCB.pccyl = UIB.pccyl;
    UCB.rctsize = UIB.rctsize;
    UCB.rctcopies = UIB.rctcopies;
    UCB.hostsize = UCB.lbnsize - UCB.rctsize * UCB.rctcopies;
    UCB.gap0 = UIB.gap0;
    UCB.gap1 = UIB.gap1;
    UCB.gap2 = UIB.gap2;
    UCB.gap3 = UIB.gap3;
    UCB.sync = UIB.sync;
    UCB.sec_interleave = UIB.sec_interleave;
    UCB.sur_skew = UIB.sur_skew % UCB.sec;
    UCB.cyl_skew = ( UIB.cyl_skew + UCB.sur_skew * UCB.sur ) % UCB.sec;
    UCB.spt = UCB.sec;
    UCB.tpg = UCB.sur;
    UCB.gpc = 1;
    /*
     *  if the "type" field in the UIB is invalid, we have to guess at the
     *  right value (version 1 of the RQDX3 code did not have this field)
     */
    if( ( ( ( byte * ) &UIB.type )[lsb] != 0 )
	    && ( ( ( byte * ) &UIB.type )[msb] == 2 ) )
	UCB.type = UIB.type;
    else
	UCB.type = ( 2<<8 ) + ( n = UIB.media[0] & 127,
		( n == 31 ) ? 12 :
		( n == 52 ) ? 8 :
		( n == 53 ) ? 9 :
		( n == 54 ) ? 13 : 6 );
    }

#define TCB (*tcb)
#define UCB (*ucb)

/*
 *  this routine will read the UIB from XBN 0 (or a copy)
 */
word rd_uib( tcb )
register struct $tcb *tcb;
    {
    register struct $ucb *ucb;
    register word *data_ptr;
    word i, sum;

    ucb = TCB.ucb;
    while( !( get_disk( ucb ) & bit1 ) )
	if( stalling )
	    $sleep( 10 );
	else
	    return( st_ofl );
    TCB.type = 0;
    TCB.modifiers = 0;
    TCB.cylinder = 0;
    TCB.surface = 0;
    TCB.number = 1;
    TCB.buffer = data;
    for( TCB.sector = 0; TCB.sector < 3; ++TCB.sector )
	if( !read( tcb ) )
	    {
	    for( i = 0; i < 9; i++ )
		if( data[i] != 0x00 )
		    goto ERROR;
	    if( data[9] != 0x36 )
		goto ERROR;
	    sum = 0;
	    data_ptr = &data[0];
	    for( i = 255; --i >= 0; )
		sum += *data_ptr++;
	    if( *data_ptr++ == sum )
		return( st_suc );
ERROR:
	    TCB.number = 1;
	    }
    return( st_mfe );
    }
