/*
 *  file = DEFS.H
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  handy and useful definitions
 */

/*
 *  constants always used
 */
#ifndef	debug
#define		debug		0
#endif

#define		rqdx3_class	1
#define		rqdx3_model	19
#define		rqdx3_softv	2
#define		rqdx3_hardv	1
#define		rqdx3_retry	8
#define		false		0
#define		true		1
#define		success		0
#define		failure		1
#define		null		0
#define		bool		short
#define		list		int *
#define		void		int
#define		byte		char
#define		word		short

#ifdef	vax11c
#define		extern		globalref
#define		read_sa			_read_sa( )
#define		write_sa(v)		_write_sa( v )
#define		request_interrupt	port_interrupt( )
#define		lsb		0
#define		msb		1
#define		lsw		0
#define		msw		1
#else
#define		globaldef
#define		read_sa
#define		write_sa		w$sard =
#define		request_interrupt	w$req = 0
#define		lsb		0
#define		msb		1
#define		lsw		1
#define		msw		0
#endif

#define		main_size	200
#define		poll_size	200
#define		scan_size	100
#define		time_size	100
#define		work_size	200

#define		max_programs	1
#define		max_commands	16
#define		max_errors	4
#define		max_packets	max_commands+max_errors

#define		data_size	(18*512)
#define		rct0		((word *)&data[12*512+0])
#define		rct1		((byte *)&data[13*512+4])
#define		rct2		((word *)&data[14*512+8])
#define		rct3		((word *)&data[15*512+12])

#define		bit0		0x0001
#define		bit1		0x0002
#define		bit2		0x0004
#define		bit3		0x0008
#define		bit4		0x0010
#define		bit5		0x0020
#define		bit6		0x0040
#define		bit7		0x0080
#define		bit8		0x0100
#define		bit9		0x0200
#define		bit10		0x0400
#define		bit11		0x0800
#define		bit12		0x1000
#define		bit13		0x2000
#define		bit14		0x4000
#define		bit15		0x8000

/*
 *  port errors
 */
#define		pe_pre		1
#define		pe_pwe		2
#define		pe_rom		5
#define		pe_qre		6
#define		pe_qwe		7
#define		pe_hat		9
#define		pe_ici		14
#define		pe_pie		20
#define		pe_ppf		21
#define		pe_mre		22
#define		pe_t11		475
#define		pe_snd		476
#define		pe_rcv		477
#define		pe_nsr		478

/*
 *  message types
 */
#define		mt_seq		(0<<4)
#define		mt_dat		(1<<4)
#define		mt_cre		(2<<4)
#define		mt_mai		(15<<4)

/*
 *  connection types
 */
#define		ct_mscp		0
#define		ct_dup		2
#define		ct_diag		255

/*
 *  error types
 */
#define		er_rf		bit15
#define		er_sf		bit14
#define		er_df		bit13
#define		er_nem		bit12
#define		er_mpe		bit11
#define		er_pte		bit10
#define		er_fat		bit6
#define		er_cnt		bit5
#define		er_hce		bit4
#define		er_ecc		bit3
#define		er_cnw		bit2
#define		er_ddm		bit1
#define		er_dnr		bit0
