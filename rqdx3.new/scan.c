/*
 *  file = SCAN.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  this module contains the SCAN job
 */

#include "defs.h"

extern word unit_base;

extern byte *get_ucb( );

/*
 *  this routine is run as a separate job with a separate context
 *
 *  This job will scan for changes in unit state.  Once per second (if the
 *  system is pretty quiet) each unit will be looked at to see if any state
 *  changes have occurred.  For RD devices, this means that the run/stop
 *  button has been toggled or the write protect button has been toggled.  For
 *  RX devices, this means that the floppy door has been opened or closed only
 *  (since you can't change the write protect except by physically altering
 *  the media, and to do that you must open and then close the door).  This
 *  tracking is done so that UNIT NOW AVAILABLE attention messages can be sent
 *  to the host in a timely fashion.
 */
scan( )
    {
    register word unit;

    while( true )
	{
	for( unit = 0; unit < 4; unit++ )
	    get_ucb( unit + unit_base );
	$sleep( 1000 );
	}
    }
