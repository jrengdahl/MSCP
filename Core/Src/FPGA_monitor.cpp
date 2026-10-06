#include <stdint.h>
#include <stdio.h>
#include <omp.h>
#include "main.h"
#include "cmsis.h"
#include "context.hpp"
#include "Port.hpp"

extern int getline_nchar;       // When this is nonzero, the user has started typing a line on the console. It is impolite to interrupt that line.

Port FPGA_Port;


void Clear_FPGA_IRQ()
    {
    *(volatile unsigned short *)0x6000000e = 0;
    }

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
    {
    if (GPIO_Pin == FPGA_IRQ_Pin)
        {
        Clear_FPGA_IRQ();
        FPGA_Port.resume();
        }
    }


void FPGA_monitor()
    {
    while(true)
        {
        FPGA_Port.suspend();

        if(getline_nchar == 0)
            {
            printf("FPGA_IRQ\n");
            putchar('>');
            fflush(stdout);
            }
        }
    }
