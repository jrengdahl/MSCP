/*
 *  file = UQSSP.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  this module contains the UQSSP routines
 */

#include "defs.h"
#include "ccb.h"
#include "ring.h"
#include "uqssp.h"

extern word rw$ptr;
extern word rw$csr;
extern word r$sawr;
extern word w$q22l;
extern word w$q22h;
extern word w$wcnt;
extern word w$sard;
extern word w$vec;
extern word w$req;

extern list dma;
extern list mem;
extern list udc;
extern byte *_packet;
extern byte data[];
extern word ipr_flag;
extern word saw_flag;
extern word credits;
extern word port_error;
extern word vector;
extern long purge_flag;
extern struct $ccb _ccb;
extern struct $ring cmd_ring;
extern struct $ring rsp_ring;

extern word sa_read( );
extern word t$port( );

#define CCB _ccb

/*
 *  this routine is called when the IP register is written by the host
 *
 *  It causes the four-step initialization sequence to be entered.  The
 *  controller state is set to "available", the initialization is performed,
 *  the controller state is set to "online", and if a previous error occurred,
 *  an error log packet summarizing it is sent
 */
init( )
    {
    word s1, s2, s3, s4, error;
    long address;

    /*
     *  set controller state to "Controller-Available"
     */
    $acquire( &mem );
    CCB.state |= cs_ini;
    CCB.state &= ~cs_act;
#if debug>=1
    printf( "\ncontroller state = CS.INI" );
#endif
    /*
     * 	controller initialization step one
     *
     *  send our step one prompt, and wait for the host step one response
     */
    while( saw_flag == 0 )
	write_sa( pi_s1|pi_s1qb|pi_s1di|pi_s1mp );
    while( !( ( s1 = sa_read( ) ) & pi_err ) )
	write_sa( pi_s1|pi_s1qb|pi_s1di|pi_s1mp );
    if( s1 & pi_s1wr )
	while( true )
	    {
	    write_sa( s1 );
	    s1 = sa_read( );
	    }
    /*
     *  the host sends vector and ring size information
     */
    vector = ( ( s1 >> pi_s1ivp ) & ( ( 1 << pi_s1ivs ) - 1 ) ) << 2;
    cmd_ring.size = 1 << ( ( s1 >> pi_s1crp ) & ( ( 1 << pi_s1crs ) - 1 ) );
    cmd_ring.mask = ~( ( cmd_ring.size - 1 ) << 2 );
    rsp_ring.size = 1 << ( ( s1 >> pi_s1rrp ) & ( ( 1 << pi_s1rrs ) - 1 ) );
    rsp_ring.mask = ~( ( rsp_ring.size - 1 ) << 2 );
    w$vec = vector;
    /*
     *  now run a quick self-test; if it errors, just die
     */
    $acquire( &udc );
    if( error = t$port( &data[1000], &data[2000] ) )
	fatal_error( error );
    else
	init_udc( );
    $release( &udc );
    /*
     * 	controller initialization step two
     *
     *  send our step two prompt, and wait for the host step two response
     */
    write_sa( pi_s2|pi_s2ptk|( ( s1 >> 8 ) & 0x00FF ) );
    if( ( s1 & pi_s1ie ) && ( vector != 0 ) )
	request_interrupt;
    s2 = sa_read( );
    ( ( word * ) &address )[lsw] = s2 & ~pi_s2pi;
    /*
     * 	controller initialization step three
     *
     *  send our step three prompt, and wait for the host step three response
     */
    write_sa( pi_s3|( s1 & 0x00FF ) );
    if( ( s1 & pi_s1ie ) && ( vector != 0 ) )
	request_interrupt;
    s3 = sa_read( );
    ( ( word * ) &address )[msw] = s3 & ~pi_s3pp;
    /*
     *  do the purge and poll tests if required
     */
    if( s3 & pi_s3pp )
	{
	ipr_flag = 0;
	write_sa( 0 );
	if( sa_read( ) != 0 )
	    fatal_error( pe_ppf );
	while( ipr_flag == 0 )
	    ;
	}
    /*
     *  test and zero the host communications area
     */
    rsp_ring.base = address;
    cmd_ring.base = address + rsp_ring.size * 4;
    rsp_ring.flag = address - 2;
    cmd_ring.flag = address - 4;
    if( s2 & pi_s2pi )
	{
	purge_flag = address - 6;
	test_memory( address - 8, rsp_ring.size + cmd_ring.size + 2 );
	}
    else
	{
	purge_flag = null;
	test_memory( address - 4, rsp_ring.size + cmd_ring.size + 1 );
	}
    rsp_ring.index = cmd_ring.index = 0;
    /*
     * 	controller initialization step four
     *
     *  send our step four prompt, and wait for the host step four response
     */
    write_sa( pi_s4|pi_s4cmk|pi_s4mvk );
    if( ( s1 & pi_s1ie ) && ( vector != 0 ) )
	request_interrupt;
    while( !( ( s4 = sa_read( ) ) & pi_s4go ) );
    /*
     *  queue up last failure packet if desired
     */
    if( ( s4 & pi_s4lf ) && ( port_error != 0 ) )
	do_plf( port_error );
    port_error = 0;
    /*
     *  set controller state to "Controller-Online"
     */
    CCB.state |= cs_act;
    CCB.state &= ~cs_ini;
#if debug>=1
    printf( "\ncontroller state = CS.ACT" );
#endif
    $release( &mem );
    /*
     *  to detect the illegal interrupt bug in the gate array, stick the
     *  vector the host gave us into the SA and leave it there; a real IP
     *  write will clear the SA, while a bogus IP write will NOT clear it
     */
    if( ( s1 = vector ) == 0 )
	s1++;
    w$sard = s1;
    }

/*
 *  this routine tests (well, zeros) a section of memory in the host
 */
test_memory( qbus, len )
long qbus;
word len;
    {
    register word i, *data_ptr;

    /*
     *  leave area zeroed
     */
    data_ptr = &data[0];
    for( i = len * 2; --i >= 0; )
	*data_ptr++ = 0;
    /*
     *  grab the DMA engine, poke the registers to start the transfer, wait
     *  for completion, and then release the DMA engine; an error causes the
     *  initialization sequence to be aborted
     */
    $acquire( &dma );
    rw$csr = 18;
    rw$ptr = data;
    w$q22l = ( ( word * ) &qbus )[lsw];
    w$q22h = ( ( word * ) &qbus )[msw];
    w$wcnt = len * 2;
    rw$csr++;
    while( rw$csr & 1 )
	;
    i = rw$csr;
    $release( &dma );
    if( i & ( bit7|bit6 ) )
	fatal_error( pe_qwe );
    }

/*
 *  read the SA register (we can tell that the host put new information into
 *  the SA because it causes an interrupt, which in turn increments the flag)
 */
word sa_read( )
    {
    while( saw_flag == 0 )
	read_sa;
    --saw_flag;
    return( r$sawr );
    }
