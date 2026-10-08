/*
 *  file = UCB.H
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  definition of $UCB structure
 *
 *  ucb_job		-- list of JCBs servicing this UCB (just one, though)
 *  pkts		-- list of queued PKTs
 *  tcbs		-- list of queued TCBs
 *  hostsize		-- number of available host LBNs
 *  flags		-- MSCP flags (definition in MSCP.H)
 *  media		-- media identifier (creation of Ed Gardner)
 *  volume		-- serial number of unit
 *  rctsize		-- size of unit's replacement control table
 *  rctcopies		-- number of RCT copies
 *  state		-- unit state
 *	us_onl		--     unit is online to a host
 *	us_ofl		--     unit is offline and unusable
 *	us_rd		--     type is RD (hard disk)
 *	us_rx		--     type is RX (floppy disk)
 *	us_imf		--     ignore media format error (don't use RCT)
 *	us_unk		--     unit is of unknown type
 *	us_mfe		--     unit is not formatted
 *	us_hs3		--     "head select 3" is important to this unit
 *	us_50		--     floppy is RX50
 *	us_33		--     floppy is RX33
 *	us_tsf		--     floppy uses DEC's ten sector format
 *	us_up		--     current elevator direction (up or down)
 *	us_fct		--     LBN being replaced is from FCT, so shortcut
 *  block		-- current block (for elevator seek algorithm)
 *  rs_bit		-- front panel bit for run/stop button
 *  wp_bit		-- front panel bit for write protect button
 *  pd_bit		-- bit to use for poll drive command
 *  lbnbase		-- block number of first LBN
 *  lbnsize		-- number of LBNs
 *  rbnbase		-- block number of first RBN
 *  rbnsize		-- number of RBNs
 *  pccyl		-- first cylinder for (write) precompensation
 *  rccyl		-- first cylinder for reduced (write) current
 *  mode		-- "mode" for SMC9224 (selects CRC or ECC)
 *  op_sd		-- "select drive" opcode for SMC9224
 *  pllctl		-- value to put into the phase-locked loop CSR
 *  gap0		-- gap0 value for reformatted tracks
 *  gap1		-- gap1 value for reformatted tracks
 *  gap2		-- gap2 value for reformatted tracks
 *  gap3		-- gap3 value for reformatted tracks
 *  sync		-- sync value for reformatted tracks
 *  sec			-- actual number of sectors (per track)
 *  sur			-- actual number of surfaces
 *  cyl			-- actual number of cylinders
 *  sec_interleave	-- sector-to-sector interleave factor
 *  sur_skew		-- surface-to-surface skew amount
 *  cyl_skew		-- cylinder-to-cylinder skew amount
 *  cur_cyl		-- current cylinder (used for overlapped seeks)
 *  ucb			-- UCB ownership waiting list (for linked JCB chain)
 *  spt			-- MSCP sectors per track
 *  tpg			-- MSCP tracks per group
 *  gpc			-- MSCP groups per cylinder
 *  tcb			-- current TCB (for getting status and/or aborting)
 *  type		-- unit class and model value
 */

struct $ucb
    {
    byte		*ucb_job;
    struct $pkt		*pkts;
    struct $tcb		*tcbs;
    long		hostsize;
    word		flags;
    long		media;
    long		volume;
    word		rctsize;
    word		rctcopies;

#define		us_onl		bit0
#define		us_ofl		bit1
#define		us_rd		bit2
#define		us_rx		bit3
#define		us_imf		bit4
#define		us_unk		bit5
#define		us_mfe		bit6
#define		us_hs3		bit7
#define		us_50		bit8
#define		us_33		bit9
#define		us_tsf		bit10
#define		us_up		bit14
#define		us_fct		bit15

    word		state;
    long		block;
    word		rs_bit;
    word		wp_bit;
    word		pd_bit;
    long		lbnbase;
    long		lbnsize;
    long		rbnbase;
    long		rbnsize;
    word		pccyl;
    word		rccyl;
    word		mode;
    word		op_sd;
    word		pllctl;
    word		gap0;
    word		gap1;
    word		gap2;
    word		gap3;
    word		sync;
    word		sec;
    word		sur;
    word		cyl;
    word		sec_interleave;
    word		sur_skew;
    word		cyl_skew;
    word		cur_cyl;
    list		ucb;
    word		spt;
    word		tpg;
    word		gpc;
    struct $tcb		*tcb;
    word		type;
    };
