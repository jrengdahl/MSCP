#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "omp.h"
#include "tim.h"
#include "cyccnt.hpp"
#include "bogodelay.hpp"

extern void commas(uint32_t x);

void CalibrateCommand(char *p)
    {
    int size = 32;
    uint32_t ticks = 0;
    uint32_t lastTIM2 = 0;
    uint32_t elapsedTIM2 = 0;
    float last_wtime;
    float elapsed_wtime;
    uint64_t count = 1;

    if(isdigit(*p))                                                 // while there is any data left on the command line
        {
        count = getlong(&p);                                        // get the count
        skip(&p);
        if(isdigit(*p))                                             // while there is any data left on the command line
            {
            size = getdec(&p);                                      // get the size
            }
        }

    if(size==64)
        {
        __disable_irq();
        last_wtime = omp_get_wtime(0);
        lastTIM2 = __HAL_TIM_GET_COUNTER(&htim2);
        Elapsed();
        bogodelay(count);
        ticks = Elapsed();
        elapsedTIM2 = __HAL_TIM_GET_COUNTER(&htim2) - lastTIM2;
        elapsed_wtime = omp_get_wtime(0) - last_wtime;
        __enable_irq();
        }
    else
        {
        __disable_irq();
        last_wtime = omp_get_wtime(0);
        lastTIM2 = __HAL_TIM_GET_COUNTER(&htim2);
        Elapsed();
        bogodelay((uint32_t)count);
        ticks = Elapsed();
        elapsedTIM2 = __HAL_TIM_GET_COUNTER(&htim2) - lastTIM2;
        elapsed_wtime = omp_get_wtime(0) - last_wtime;
        __enable_irq();
        }
    commas(ticks);
    printf(" microseconds by CYCCNT\n");
    commas(elapsedTIM2);
    printf(" microseconds by TIM2\n");
    printf("%lf microseconds by wtime\n", elapsed_wtime*1000000.0);
    }
