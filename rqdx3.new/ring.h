/*
 *  file = RING.H
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  definition of $RING structure
 *
 *  base		-- base address of ring in host memory
 *  flag		-- address of flag word
 *  size		-- size of ring, in longs
 *  mask		-- mask for size-1 (since size is a power of two)
 *  index		-- index of current descriptor in ring
 */

struct $ring
    {
    long		base;
    long		flag;
    word		size;
    word		mask;
    word		index;
    };
