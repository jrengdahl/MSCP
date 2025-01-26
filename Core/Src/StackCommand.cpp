#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "libgomp.hpp"

void StackCommand(char *p)
    {
    for(int i=0; i<GOMP_MAX_NUM_THREADS; i++)
        {
        extern const char *thread_names[];
        printf("\%s stack, %d bytes:\n", thread_names[i], omp_threads[i].stack_high - omp_threads[i].stack_low);
        dump(omp_threads[i].stack_low, omp_threads[i].stack_high - omp_threads[i].stack_low);
        }
    }
