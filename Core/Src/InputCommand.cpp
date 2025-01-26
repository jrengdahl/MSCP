#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "serial.h"
#include "ContextFIFO.hpp"


void InputCommand(char *p)
    {
    unsigned i = 0;
    unsigned value = 0;
    unsigned repeat = 1;
    unsigned last = 99;

    if(isdigit(*p))
        {
        i = getdec(&p);
        skip(&p);
        if(isdigit(*p))
            {
            repeat = getdec(&p);
             }
        }

    while(repeat && !ControlC)
        {
        switch(i)
            {
            case 0:  value = HAL_GPIO_ReadPin(GPIONAME( FPGA_IRQ)); break;
            default: value = HAL_GPIO_ReadPin(GPIONAME( FPGA_IRQ)); break;
            }

        if(value != last)
            {
            printf("%u\n", value);
            last = value;
            --repeat;
            }

        yield();
        }
    }
