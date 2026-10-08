/*
 *  file = PCB.H
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  definition of $PCB structure
 *
 *  count		-- size of buffer of data to send or receive
 *  buffer		-- address of aforementioned buffer
 *  program		-- starting PC of DUP program to run
 *  work		-- progress counter
 *  state		-- program state
 *	ps_beg		--     program is just beginning
 *	ps_abo		--     an abort has been requested
 *	ps_snd		--     SEND DATA in progress
 *	ps_rcv		--     RECEIVE DATA in progress
 *  timeout		-- host-level timeout
 *  pkts		-- list of queued DUP packets (should be one at most)
 */

struct $pcb
    {
    word		count;
    byte		*buffer;
    word		( *program )( );
    long		work;

#define		ps_beg		bit0
#define		ps_abo		bit1
#define		ps_snd		bit2
#define		ps_rcv		bit3

    word		state;
    word		timeout;
    byte		*pkts;
    };
