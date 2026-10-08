/*
 *  file = CLEAR.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  this module contains initialization code
 */

#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "pcb.h"
#include "tcb.h"
#include "mscp.h"

extern word r$fps;

extern list pkts;
extern list tcbs;
extern word ha_time;
extern word unit_base;
extern word credits;
extern word udc$bug;
extern struct $ccb _ccb;
extern struct $pcb _pcb;
extern struct $tcb _tcb[];
extern struct $pkt _pkt[];

/*
 *  this routine is called to initialize most internal data structures (since
 *  RAM is already clear, we only need to initialize non-zero stuff)
 */
clear( )
    {
    register word i;
    register struct $pkt *pkt;
    register struct $tcb *tcb;

    /*
     *  initialize the controller control block (CCB)
     */
    _ccb.timeout = 60;
    _ccb.flags = cf_rpl;
    _ccb.type = ( rqdx3_class<<8 ) + rqdx3_model;
    /*
     *  initialize the program control block (PCB)
     */
    _pcb.timeout = 60;
    /*
     *  initialize the SMC9224 chip
     */
    init_udc( );
    /*
     *  initialize random stuff
     */
    unit_base = ( r$fps & 0x3F00 ) >> 6;
    if( udc$bug )
	unit_base &= 0x007C;
    credits = max_commands - 1;
    ha_time = 60 + 1;
    /*
     *  initialize the queues
     */
    pkt = &_pkt[0];
    tcb = &_tcb[0];
    for( i = max_packets; --i >= 0; pkt++, tcb++ )
	{
	$enq_head( &pkts, pkt );
	$enq_head( &tcbs, tcb );
	}
    }
