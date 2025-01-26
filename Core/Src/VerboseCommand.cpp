#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "libgomp.hpp"


void VerboseCommand(char *p)
    {
    extern int omp_verbose;
    extern int temp_verbose;

    if(*p == 'o')
        {
        skip(&p);
        omp_verbose = getdec(&p);
        }
    else if(*p == 't')
        {
        skip(&p);
        temp_verbose = getdec(&p);
        }

    printf("omp_verbose = %d\n", omp_verbose);
    printf("temp_verbose = %d\n", temp_verbose);
    }
