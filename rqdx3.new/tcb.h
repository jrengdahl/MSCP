/*
 *  file = TCB.H
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  defintion of $TCB structure
 *
 *  link		-- link to next TCB in chain
 *  ucb			-- associated UCB
 *  pkt			-- associated PKT
 *  block		-- the current block being read or written
 *  count		-- remaining byte count of transfer
 *  qbus		-- current Q-bus address to DMA to or from
 *  routine		-- the PC of the transfer routine (read/write/compare)
 *  type		-- transfer type
 *	tt_rd		--     a READ request
 *	tt_wr		--     a WRITE request
 *	tt_cmp		--     a COMPARE request
 *	tt_wtq		--     data will be written to Q-bus
 *	tt_rfq		--     data will be read from Q-bus
 *	tt_rct		--     access is to the RCT
 *	tt_cmr		--     the READ should be COMPAREd
 *	tt_cmw		--     the WRITE should be COMPAREd
 *	tt_abo		--     the command is being aborted
 *	tt_up		--     the elevator needs to go up (or down)
 *	tt_new		--     a new block number (must do LBN to PBN stuff)
 *  modifiers		-- the original MSCP modifiers
 *	tm_ser		--     "suppress error recovery"
 *	tm_sec		--     "suppress error correction" (reads only)
 *	tm_err		--     "force error" (writes only)
 *	tm_exp		--     "express request" (ignore elevator algorithm)
 *  cylinder		-- cylinder which LBN maps to
 *  surface		-- surface which LBN maps to
 *  sector		-- sector which LBN maps to
 *  number		-- maximum number of sectors to read or write
 *  result		-- how many sectors were read or written
 *  segment		-- size in bytes of current transfer segment
 *  buffer		-- address of buffer to use
 *  oldrbn		-- during BBR, the "old" RBN
 *  newrbn		-- during BBR, the "new" RBN
 *  warning		-- latest "warning" error status (soft error)
 *  fatal		-- latest "fatal" error status (hard error)
 *  stat		-- SMC9224 interrupt status register (for error log)
 *  chip		-- SMC9224 chip status register (for error log)
 *  disk		-- SMC9224 disk status register (for error log)
 *  csec		-- SMC9224 current sector register (for error log)
 *  csur		-- SMC9224 current surface register (for error log)
 *  ccyl		-- SMC9224 current cylinder register (for error log)
 */

struct $tcb
    {
    struct $tcb		*link;
    struct $ucb		*ucb;
    struct $pkt		*pkt;
    long		block;
    long		count;
    long		qbus;
    word		( *routine )( );

#define		tt_rd		bit0
#define		tt_wr		bit1
#define		tt_cmp		bit2
#define		tt_wtq		bit3
#define		tt_rfq		bit4
#define		tt_rct		bit5
#define		tt_cmr		bit6
#define		tt_cmw		bit7
#define		tt_abo		bit8
#define		tt_up		bit14
#define		tt_new		bit15

    word		type;

#define		tm_ser		bit8
#define		tm_sec		bit9
#define		tm_err		bit12
#define		tm_exp		bit15

    word		modifiers;
    word		cylinder;
    word		surface;
    word		sector;
    word		number;
    word		result;
    word		segment;
    byte		*buffer;
    long		oldrbn;
    long		newrbn;
    word		warning;
    word		fatal;
    byte		stat;
    byte		chip;
    byte		disk;
    byte		csec;
    byte		csur;
    byte		ccyl;
    };
