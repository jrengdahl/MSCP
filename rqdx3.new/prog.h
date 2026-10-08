/*
 *  file = PROG.H
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  definition of $PROG structure
 *
 *  name		-- ASCII name string
 *  flags		-- program flags
 *	pf_sta		--     must run standalone
 *	pf_nol		--     no overlays required (only kind supported)
 *	pf_wol		--     overlays must be writeable
 *	pf_sdd		--     program follows standard DUP dialog
 *  program		-- starting PC of program to run
 *  version		-- program version
 *  timeout		-- host-level timeout
 */

struct $prog
    {
    byte		name[6];

#define		pf_sta		bit0
#define		pf_nol		bit1
#define		pf_wol		bit2
#define		pf_sdd		bit3

    word		flags;
    word		( *program )( );
    word		version;
    word		timeout;
    };
