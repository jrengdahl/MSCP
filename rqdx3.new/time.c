/*
 *  file = TIME.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  this module contains the TIME job
 */

#include "defs.h"

extern word sd_time;
extern word mo_time;
extern word cd_time;

/*
 *  this routine is run as a separate job with a separate context
 *
 *  This is where the high-level timers reside and are taken care of (select
 *  drive timeout, motor off timeout, and command done timeout).  If any of
 *  the timers expire, the appropriate action routine is called.
 */
time( )
    {
    while( true )
	{
	/*
	 *  timers are simply counters which are either zero or non-zero:  if
	 *  zero, then the timer is effectively disabled; otherwise, the
	 *  counter is decremented, and if it is now zero, the action routine
	 *  is called, leaving the timer set to zero
	 *
	 *  thus to disable a timer, set it to zero, and to enable a timer,
	 *  set it to the number of seconds you want timed
	 */
	if( sd_time != 0 )
	    if( --sd_time == 0 )
		sd_tmo( );
	if( mo_time != 0 )
	    if( --mo_time == 0 )
		mo_tmo( );
	if( cd_time != 0 )
	    if( --cd_time == 0 )
		cd_tmo( );
	$sleep( 1000 );
	}
    }
