/*
 *  file = SUBS.C
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  miscellaneous routines
 */

#include "defs.h"

/*
 *  find the quotient and remainder of a divided by b; put the quotient in c
 *  and return the remainder to the caller
 *
 *  this routine should be recoded in MACRO to speed it up!
 */
word quo_rem( a, b, c )
long a;
word b;
long *c;
    {
    *c = a / b;
    return( a % b );
    }
