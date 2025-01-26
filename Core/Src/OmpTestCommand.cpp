#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"


void OmpTestCommand(char *p)
    {
            extern void omp_hello(int);
            extern void omp_for(int);
            extern void omp_single(int);
            extern void permute(int colors_arg, int balls, int plevel_arg, int verbose_arg);

            int test = 0;

            if(!isdigit(*p))
                {
                printf("0: omp_hello,  test #pragma omp parallel num_threads(arg)\n");
                printf("1: omp_for,    test #pragma omp parallel for num_threads(arg)\n");
                printf("2: omp_single, test #pragma omp single, arg is team size\n");
                printf("3: permute(colors, ball, plevel, verbose), test omp_task\n");
                }
            else
                {
                test = getdec(&p);                              // get the count
                skip(&p);

                switch(test)
                    {
                case 0: omp_hello(getdec(&p));      break;
                case 1: omp_for(getdec(&p));        break;
                case 2: omp_single(getdec(&p));     break;
                case 3:
                    int colors = getdec(&p);
                    skip(&p);
                    int balls = getdec(&p);
                    skip(&p);
                    int plevel = *p?getdec(&p):32;
                    skip(&p);
                    int v = *p?getdec(&p):0;
                    permute(colors, balls, plevel, v);
                    break;
                    }
                }

    }
