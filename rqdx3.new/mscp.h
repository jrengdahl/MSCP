/*
 *  file = MSCP.H
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  definition of MSCP specific constants
 *
 *  see the MSCP specification for further details
 */

/*
 *  opcodes
 */
#define		op_abo		1
#define 	op_acc		16
#define 	op_avl		8
#define 	op_ccd		17
#define 	op_cmp		32
#define 	op_dap		11
#define 	op_ers		18
#define 	op_flu		19
#define		op_fmt		47
#define 	op_gcs		2
#define 	op_gus		3
#define 	op_onl		9
#define 	op_rd		33
#define 	op_scc		4
#define 	op_suc		10
#define 	op_wr		34

#define 	op_end		128
#define 	op_sex		7

#define 	op_ava		64

/*
 *  modifiers
 */
#define 	md_cse		bit13
#define 	md_cmp		bit14
#define 	md_exp		bit15
#define 	md_err		bit12
#define 	md_sch		bit11
#define 	md_scl		bit10
#define 	md_sec		bit9
#define 	md_ser		bit8

#define 	md_nxu		bit0

#define 	md_rip		bit0
#define 	md_imf		bit1

#define 	md_swp		bit2
#define 	md_shd		bit4

/*
 *  end flags
 */
#define 	ef_log		bit5
#define 	ef_sex		bit4

/*
 *  controller flags
 */
#define 	cf_atn		bit7
#define 	cf_msc		bit6
#define 	cf_oth		bit5
#define 	cf_ths		bit4
#define 	cf_rpl		bit15
#define		cf_msk		(cf_atn|cf_msc|cf_oth|cf_ths)

/*
 *  unit flags
 */
#define 	uf_cmr		bit0
#define 	uf_cmw		bit1
#define 	uf_rpl		bit15
#define 	uf_rmv		bit7
#define		uf_wpd		bit8
#define 	uf_wph		bit13
#define 	uf_wps		bit12
#define		uf_msk		(uf_cmr|uf_cmw)

/*
 *  format codes
 */
#define		fm_cnt		0
#define		fm_bad		1
#define		fm_dsk		2
#define		fm_sdi		3
#define		fm_sde		4
#define		fm_rpl		9

/*
 *  log flags
 */
#define		lf_suc		bit7
#define		lf_con		bit6
#define		lf_snr		bit0
#define		lf_bbr		bit5
#define		lf_rct		bit4

/*
 *  additional log flags for bad block replacement
 */
#define		lfr_rp		bit15
#define		lfr_fe		bit14
#define		lfr_te		bit13
#define		lfr_rf		bit12
#define		lfr_ri		bit11
#define		lfr_br		bit10

/*
 *  invalid command offsets
 */
#define		i_opcd		(8<<8)
#define		i_bcnt		(12<<8)
#define		i_lbn		(28<<8)
#define		i_vrsn		(12<<8)
#define		i_fmti		(28<<8)

/*
 *  status codes
 */
#define		st_msk		31
#define		st_sub		32
#define		st_suc		0
#define		st_cmd		1
#define		st_abo		2
#define		st_ofl		3
#define		st_avl		4
#define		st_mfe		5
#define		st_wpr		6
#define		st_cmp		7
#define		st_dat		8
#define		st_hst		9
#define		st_cnt		10
#define		st_drv		11
#define		st_bbr		20
#define		st_dia		31

/*
 *  check for consistent definitions
 */
#ifdef tm_err
#if md_err != tm_err
#undef md_err
#endif
#endif

#ifdef tm_exp
#if md_exp != tm_exp
#undef md_exp
#endif
#endif

#ifdef tm_sec
#if md_sec != tm_sec
#undef md_sec
#endif
#endif

#ifdef tm_ser
#if md_ser != tm_ser
#undef md_ser
#endif
#endif
