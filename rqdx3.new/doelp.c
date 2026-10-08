/*
 *  file = DOELP.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  the EXECUTE LOCAL PROGRAM command
 */
#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "pcb.h"
#include "prog.h"
#include "dup.h"

extern list mem;
extern struct $ccb _ccb;
extern struct $pcb _pcb;
extern struct $prog lcl_prog[max_programs];

/*
 *  the EXECUTE LOCAL PROGRAM command packet
 */
struct $elpc
    {
    long	p_crf;
    word	p_r1[2];
    byte	p_opcd;
    byte	p_r2;
    word	p_mod;
    byte	p_name[6];
    };

/*
 *  the EXECUTE LOCAL PROGRAM response packet
 */
struct $elpr
    {
    long	p_crf;
    word	p_r1[2];
    byte	p_opcd;
    byte	p_r2;
    word	p_sts;
    word	p_vrsn;
    byte	p_tmo;
    byte	p_flgs;
    };

#define		rs_elp		sizeof( struct $elpr )

#define PKT (*pkt)
#define CMD (*(struct $elpc *)&(PKT.data))
#define RSP (*(struct $elpr *)&(PKT.data))
#define CCB _ccb
#define PCB _pcb
#define PROG (*prog)
#define PROGname ((word *)&PROG.name)
#define CMDp_name ((word *)&CMD.p_name)

/*
 *  process an EXECUTE LOCAL PROGRAM command
 *
 *  When we get this command, we must not already be running a DUP program; if
 *  we are, then this command is invalid.  Else, we compare the ASCII name of
 *  the desired program with a list of valid ASCII local program names.  If we
 *  find a match, then the PCB is filled in with enough information to begin
 *  running that program, and we set the state to "beginning".  This will cause
 *  the program to be invoked when the controller is next idle (see the idle
 *  loop in MAIN.C for more information).
 */
do_elp( pkt )
register struct $pkt *pkt;
    {
    register word i;
    register struct $prog *prog;

#if debug>=1
    printf( "\nEXECUTE LOCAL PROGRAM, name = %0.6s", CMD.p_name );
#endif
    /*
     *  we can't already be running a program!
     */
    if( !( CCB.state & cs_dup ) )
	{
	/*
	 *  loop through the known local programs looking for a name match
	 */
	prog = &lcl_prog[0];
	for( i = max_programs; --i >= 0; prog++ )
	    {
	    if( PROGname[0] == CMDp_name[0] )
		if( PROGname[1] == CMDp_name[1] )
		    if( PROGname[2] == CMDp_name[2] )
			{
			/*
			 *  found a match, so check if the flags are okay; if
			 *  so, copy what information we need from this PROG
			 *  to the PCB
			 *
			 *  we set the controller state to show that a DUP
			 *  program is running, and we set the program state
			 *  to show that it is ready to begin
			 */
			if( ( PROG.flags & pf_sta ) && !( CMD.p_mod & md_sta ) )
			    {
			    RSP.p_sts = st_sta;
			    break;
			    }
			else
			    {
			    $acquire( &mem );
			    RSP.p_sts = st_suc;
			    RSP.p_vrsn = PROG.version;
			    RSP.p_tmo = PROG.timeout;
			    RSP.p_flgs = PROG.flags;
			    CCB.state |= cs_dup;
			    PCB.program = PROG.program;
			    PCB.work = 0;
			    PCB.state = ps_beg;
			    PCB.timeout = PROG.timeout;
			    break;
			    }
			}
	    }
	/*
	 *  if we didn't find a name which matches, complain to the host
	 */
	if( !( CCB.state & cs_dup ) )
	    RSP.p_sts = st_pnk;
	}
    else
	RSP.p_sts = st_cmd;
    RSP.p_opcd |= op_end;
    PKT.size = rs_elp;
    PKT.type = mt_seq;
    put_packet( pkt );
    }
