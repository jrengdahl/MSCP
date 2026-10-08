/*
 *  file = DATA.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  this module contains storage and initialization code
 */

#include "defs.h"
#include "pkt.h"
#include "ccb.h"
#include "jcb.h"
#include "pcb.h"
#include "tcb.h"
#include "ucb.h"
#include "prog.h"
#include "ring.h"

/*
 *  this stuff gets saved by the self test code
 */
globaldef word		board_okay;
globaldef word		i$udc;
globaldef word		i$dma;
globaldef word		i$clk;
globaldef word		i$ipw;
globaldef word		i$ipr;
globaldef word		i$saw;
globaldef word		i$sec;
globaldef word		port_error;
globaldef word		stalling;

/*
 *  this stuff gets destroyed by the self test code
 */
globaldef list		ipr_job;
globaldef list		sec_job;
globaldef list		udc_job;
globaldef list		pd_jobs[4];
globaldef list		dma;
globaldef list		mem;
globaldef list		udc;
globaldef list		pkts;
globaldef list		tcbs;
globaldef byte		*ucbs[4];
globaldef byte		*rx_sector;
globaldef byte		id_table[18][4];
globaldef word		rx_opcode;
globaldef word		rx_number;
globaldef word		fpl;
globaldef word		fps;
globaldef word		reg_7;
globaldef word		reg_8;
globaldef word		reg_9;
globaldef word		pd_mask;
globaldef word		ha_time;
globaldef word		sd_time;
globaldef word		mo_time;
globaldef word		cd_time;
globaldef word		pd_flag;
globaldef word		ha_flag;
globaldef word		sd_flag;
globaldef word		udc_flag;
globaldef word		dma_flag;
globaldef word		clk_flag;
globaldef word		ipr_flag;
globaldef word		saw_flag;
globaldef word		sec_flag;
globaldef word		total_sectors;
globaldef word		unit_base;
globaldef word		credits;
globaldef word		vector;
globaldef word		udc$bug;
globaldef word		main_stack[main_size];
globaldef word		poll_stack[poll_size];
globaldef word		scan_stack[scan_size];
globaldef word		time_stack[time_size];
globaldef long		purge_flag;
globaldef struct $jcb	main_jcb;
globaldef struct $jcb	poll_jcb;
globaldef struct $jcb	scan_jcb;
globaldef struct $jcb	time_jcb;
globaldef struct $jcb	work_jcb[4];
globaldef struct $ccb	_ccb;
globaldef struct $pcb	_pcb;
globaldef struct $tcb	_tcb[max_packets];
globaldef struct $ucb	_ucb[4];
globaldef struct $pkt	_pkt[max_packets];
globaldef struct $ring	cmd_ring;
globaldef struct $ring	rsp_ring;

extern direct( ), format( );

dsect "c$buff";

globaldef byte		data[data_size+4];
globaldef byte		temp[512+4];
globaldef word		work_stack[4][work_size];

dsect "c$code";

globaldef byte		rx_table[50] =
    { 1, 3, 5, 7, 9, 2, 4, 6, 8, 10,
      3, 5, 7, 9, 1, 4, 6, 8, 10, 2,
      5, 7, 9, 1, 3, 6, 8, 10, 2, 4,
      7, 9, 1, 3, 5, 8, 10, 2, 4, 6,
      9, 1, 3, 5, 7, 10, 2, 4, 6, 8 };
globaldef word		_bit0[4] =
    { 1, 2, 4, 8 };
globaldef word		_bit4[4] =
    { 16, 32, 64, 128 };
globaldef struct $prog	lcl_prog[max_programs] =
    { { { 'F', 'O', 'R', 'M', 'A', 'T' }, pf_sta|pf_sdd, &format, 1, 60 } };

dsect "";
