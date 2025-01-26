#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "context.hpp"
#include "ContextFIFO.hpp"
#include "omp.h"


void ThreadTestCommand(char *p)
    {
    Context *ctx = Context::pointer();      // get the context pointer for thread 0
    unsigned count = getdec(&p);            // get the number of iterations
    unsigned i = count;
    float start = 0;
    float ticks = 0;

    start = omp_get_wtime_float();

    #pragma omp parallel num_threads(2)
    if(omp_get_thread_num() == 0)
        {
        while(i)
            {
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            Context::suspend();
            }
        }
    else
        {
        while(i)
            {
            --i;
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            ctx->resume();
            }
        }

    ticks = omp_get_wtime_float() - start;

    printf("\n%lf nsec\n", ticks*1000000000.0/(count*40));
    }
