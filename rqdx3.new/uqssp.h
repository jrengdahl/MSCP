/*
 *  file = UQSSP.H
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  definition of UQSSP specific constants
 *
 *  see the UQSSP specification for further details
 */

/*
 *  generic port constants
 */
#define		pi_err		bit15

/*
 *  step 1 constants
 */
#define		pi_s1		bit11
#define		pi_s1nv		bit10
#define		pi_s1qb		bit9
#define		pi_s1di		bit8
#define		pi_s1od		bit7
#define		pi_s1mp		bit6
#define		pi_s1wr		bit14
#define		pi_s1crp	11
#define		pi_s1crs	3
#define		pi_s1rrp	8
#define		pi_s1rrs	3
#define		pi_s1ie		bit7
#define		pi_s1ivp	0
#define		pi_s1ivs	7

/*
 *  step 2 constants
 */
#define		pi_s2		bit12
#define		pi_s2ptp	8
#define		pi_s2pts	3
#define		pi_s2ptk	((0&((1<<pi_s2pts)-1))<<pi_s2ptp)
#define		pi_s2pi		bit0

/*
 *  step 3 constants
 */
#define		pi_s3		bit13
#define		pi_s3pp		bit15

/*
 *  step 4 constants
 */
#define		pi_s4		bit14
#define		pi_s4cmp	4
#define		pi_s4cms	7
#define		pi_s4cmk	((rqdx3_model&((1<<pi_s4cms)-1))<<pi_s4cmp)
#define		pi_s4mvp	0
#define		pi_s4mvs	4
#define		pi_s4mvk	((rqdx3_softv&((1<<pi_s4mvs)-1))<<pi_s4mvp)
#define		pi_s4lf		bit1
#define		pi_s4go		bit0
