/*
 *  file = MAIN.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  this module contains general control routines
 */

#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "jcb.h"
#include "pcb.h"

extern word ha_time;
extern word unit_base;
extern word main_stack[main_size];
extern word poll_stack[poll_size];
extern word scan_stack[scan_size];
extern word time_stack[time_size];
extern word work_stack[4][work_size];
extern struct $jcb main_jcb;
extern struct $jcb poll_jcb;
extern struct $jcb scan_jcb;
extern struct $jcb time_jcb;
extern struct $jcb work_jcb[4];
extern struct $ccb _ccb;
extern struct $pcb _pcb;

extern byte *get_ucb( );
extern void main( );
extern void poll( );
extern void scan( );
extern void time( );
extern void work( );

/*
 *  this is the first routine to be called after power-up -- just start the
 *  MAIN job running in its own context
 */
start( )
    {
    $create( &main_jcb, &main, 0, &main_stack[main_size] );
    }

#define CCB _ccb
#define PCB _pcb
#define UCB (*ucb)

/*
 *  this routine is run as a separate job with a separate context
 *
 *  This job initializes much of the state of the controller, and then creates
 *  the other jobs (SCAN, TIME, POLL, WORK).  It then falls into an idle loop,
 *  constantly looking for something to do.  When a DUP program (either local
 *  or supplied) is run, it is essentially called as a subroutine from this
 *  job, with this job's context.
 */
main( )
    {
    register word unit, disk;
    register struct $ucb *ucb;

    /*
     *  initialize state
     */
    clear( );
    init( );
    setup( );
    /*
     *  create the other jobs
     */
    $create( &poll_jcb, &poll, 4, &poll_stack[poll_size] );
    for( unit = 0; ( ucb = get_ucb( unit + unit_base ) ) != null; unit++ )
	$create( &work_jcb[unit], &work, 3,
		&work_stack[unit][work_size], ucb );
    $create( &time_jcb, &time, 2, &time_stack[time_size] );
    $create( &scan_jcb, &scan, 1, &scan_stack[scan_size] );
    /*
     *  now, forever, look for either a DUP program to run, or set a timer to
     *  expire in a little while
     */
    while( true )
	if( ( CCB.state & cs_dup ) && ( PCB.state & ps_beg ) )
	    {
	    /*
	     *  found a DUP program to run, so run it
	     */
	    PCB.state &= ~ps_beg;
	    ( *PCB.program )( );
	    }
	else
	    {
	    /*
	     *  nothing else to do, so post a wake-up call for later
	     */
	    $sleep( 1000 );
	    /*
	     *  let the host access timer run
	     */
	    if( ha_time != 0 )
		if( --ha_time == 0 )
		    fatal_error( pe_hat );
	    }
    }
