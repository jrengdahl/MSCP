#include <stdio.h>
#include <omp.h>

void omp_hello(int arg)
    {
    if(arg==0)arg=4;
    #pragma omp parallel num_threads(arg)
    printf("hello, world! %d\n", omp_get_thread_num());
    }

void omp_for(int arg)
    {
    if(arg==0)arg=4;
    #pragma omp parallel for num_threads(arg)
    for(int i=0; i<16; i++)
        {
        printf("%d ", omp_get_thread_num());
        }
    printf("\n");
    }

void omp_single(int arg)
    {
    if(arg==0)arg=4;
    #pragma omp parallel num_threads(arg)
        {
        int id = omp_get_thread_num();
        #pragma omp single
        printf("s%d ", id);
        printf("%d ", id);
        }
    printf("\n");
    }
