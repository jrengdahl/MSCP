/*
 *  file = CCB.H
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  definition of $CCB structure
 *
 *  flags		-- MSCP flags (definition in MSCP.H)
 *  state		-- controller state
 *	cs_ini		--     currently being initialized
 *	cs_act		--     "active" (online to a host now)
 *	cs_dup		--     running a DUP program
 *	cs_abo		--     the DUP program is being aborted
 *  timeout		-- host timeout value
 *  type		-- controller class and model value
 */

struct $ccb
    {
    word		flags;

#define		cs_ini		bit0
#define		cs_act		bit1
#define		cs_dup		bit2
#define		cs_abo		bit3

    word		state;
    word		timeout;
    word		type;
    };
