/*
 *  file = PROG.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  this module contains the DUP local program support routines
 */

#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "pcb.h"
#include "prog.h"

extern list mem;
extern struct $ccb _ccb;
extern struct $pcb _pcb;
extern struct $prog lcl_prog[max_programs];

extern byte *$deq_head( );
extern word put_host( );

#define PCB _pcb

/*
 *  this routine will get a buffer from the host
 */
word get_host( buffer, count )
byte *buffer;
word count;
    {
    register struct $pkt *pkt;

    /*
     *  if the program has been aborted, just quit
     */
    if( PCB.state & ps_abo )
	return( failure );
    /*
     *  say we are waiting for a SEND DATA command
     */
    PCB.state |= ps_snd;
    PCB.count = count;
    PCB.buffer = buffer;
    /*
     *  if there are PKTs stacked up, execute the first one (it should be the
     *  SEND DATA command that we are waiting for)
     */
    if( ( pkt = $deq_head( &PCB.pkts ) ) != null )
	do_dup( pkt );
    /*
     *  wait for completion or abortion
     */
    while( PCB.buffer != null )
	if( PCB.state & ps_abo )
	    return( failure );
	else
	    $sleep( 1 );
    /*
     *  say we have done some work and return
     */
    ++PCB.work;
    return( success );
    }

#define PCB _pcb

/*
 *  this routine will put a buffer to the host
 */
word put_host( buffer, count )
byte *buffer;
word count;
    {
    register struct $pkt *pkt;

    /*
     *  if the program has been aborted, just quit
     */
    if( PCB.state & ps_abo )
	return( failure );
    /*
     *  say we are waiting for a RECEIVE DATA command
     */
    PCB.state |= ps_rcv;
    PCB.count = count;
    PCB.buffer = buffer;
    /*
     *  if there are PKTs stacked up, execute the first one (it should be the
     *  RECEIVE DATA command that we are waiting for)
     */
    if( ( pkt = $deq_head( &PCB.pkts ) ) != null )
	do_dup( pkt );
    /*
     *  wait for completion or abortion
     */
    while( PCB.buffer != null )
	if( PCB.state & ps_abo )
	    return( failure );
	else
	    $sleep( 1 );
    /*
     *  say we have done some work and return
     */
    ++PCB.work;
    return( success );
    }

#define PCB _pcb

/*
 *  this routine will update the progress counter
 */
word progress( )
    {
    /*
     *  if the program has been aborted, just quit
     */
    if( PCB.state & ps_abo )
	return( failure );
    /*
     *  say we have done some work and return
     */
    ++PCB.work;
    return( success );
    }

#define CCB _ccb
#define PCB _pcb

/*
 *  this routine will clean up the DUP program state so it can quit
 */
terminate( )
    {
    register struct $pkt *pkt;

    PCB.program = null;
    PCB.work = 0;
    PCB.state = 0;
    PCB.timeout = 60;
    /*
     *  if any PKTs have been deferred, process them now
     */
    while( ( pkt = $deq_head( &PCB.pkts ) ) != null )
	do_dup( pkt );
    /*
     *  say that the program is done
     */
    CCB.state &= ~cs_dup;
    $release( &mem );
    }
