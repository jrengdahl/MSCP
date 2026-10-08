/*
 *  file = JCB.H
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  definition of $JCB structure
 *
 *  link		-- link to next JCB in chain
 *  job_list		-- the list that this JCB is currently chained on
 *  stack		-- current value of SP for this job
 *  priority		-- job priority
 */

struct $jcb
    {
    struct $jcb		*link;
    byte		*job_list;
    byte		*stack;
    word		priority;
    };
