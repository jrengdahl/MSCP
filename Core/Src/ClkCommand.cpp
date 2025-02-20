#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "tim.h"
#include "cyccnt.hpp"

extern "C" void SystemClock_HSI_Config(void);
extern "C" void SystemClock_PLL_Config(unsigned);

void SetClock(unsigned clk)
    {
    SystemClock_HSI_Config();               // have to deselect the PLL before reprogramming it
    SystemClock_PLL_Config(clk);            // set the PLL to the new frequency

    // set the TIM2 prescaler to the new frequency so that it always ticks at 1 MHz
    htim2.Instance->PSC = (clk / 2) - 1;    // set the prescale value
    htim2.Instance->EGR = TIM_EGR_UG;       // generate an update event to update the prescaler immediately
    }

void ClkCommand(char *p)
    {
    if(isdigit(*p))                         // if an arg is given
        {
        unsigned clk = getdec(&p);          // set the clock frequency to the new value
        SetClock(clk);
        }

    printf("CPU clock is %u MHz\n", CPU_CLOCK_FREQUENCY);
    if(CPU_CLOCK_FREQUENCY > 100)
        {
        printf("trace may not be stable at frequencies over 100 MHz\n");
        }
    }
