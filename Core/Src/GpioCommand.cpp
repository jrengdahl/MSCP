#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "serial.h"
#include "bogodelay.hpp"


void LED_on()
    {
    HAL_GPIO_WritePin(GPIONAME( LED0), GPIO_PIN_SET);
    }

void LED_off()
    {
    HAL_GPIO_WritePin(GPIONAME( LED0), GPIO_PIN_RESET);
    }


void GpioCommand(char *p)
    {
    unsigned count = 1;                 // number of blinks
    unsigned delay = 1;                 // delay (in CPU clocks) between blinks
    unsigned dest = 0;                  // which LED to blink

    if(isdigit(*p))
        {
        count = getdec(&p);
        skip(&p);
        if(isdigit(*p))
            {
            delay = getdec(&p);
            skip(&p);
            if(isdigit(*p))
                {
                dest = getdec(&p);
                }
            }
        }

    for(unsigned i=0; i<count && !ControlC; i++)
        {
        switch(dest)
            {
            case  0: HAL_GPIO_WritePin(GPIONAME( LED0), (GPIO_PinState)(i&1)); break;
            case  1: HAL_GPIO_WritePin(GPIONAME( LED1), (GPIO_PinState)(i&1)); break;
            case  2: HAL_GPIO_WritePin(GPIONAME( LED2), (GPIO_PinState)(i&1)); break;
            default: HAL_GPIO_WritePin(GPIONAME( LED0), (GPIO_PinState)(i&1)); break;
            }

        bogodelay(delay);
        }
    }
