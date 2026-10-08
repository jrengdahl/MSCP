/*
 *  file = DUP.H
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  definition of DUP specific constants
 *
 *  see the DUP specification for further details
 */

/*
 *  opcodes
 */
#define		op_ap		6
#define		op_elp		3
#define		op_esp		2
#define		op_gds		1
#define		op_rcv		5
#define		op_snd		4

#define		op_end		128

/*
 *  modifiers
 */
#define		md_sta		bit0

/*
 *  controller flags
 */
#define		cf_sta		bit0
#define		cf_lmp		bit1
#define		cf_nsp		bit2
#define		cf_act		bit3

/*
 *  invalid command offsets
 */
#define		i_opcd		(8<<8)
#define		i_bcnt		(12<<8)

/*
 *  status codes
 */
#define		st_msk		31
#define		st_sub		32
#define		st_suc		0
#define		st_cmd		1
#define		st_nra		2
#define		st_nrs		3
#define		st_pnk		4
#define		st_lf		5
#define		st_sta		6
#define		st_hst		9
