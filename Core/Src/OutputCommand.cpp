#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"


void OutputCommand(char *p)
    {
    unsigned o = 0;                     // output number
    unsigned value = 0;                 // output value

    if(isdigit(*p))
        {
        o = getdec(&p);
        skip(&p);
        if(isdigit(*p))
            {
            value = getdec(&p);
            }
        }

    switch(o)
        {
        case 0:     HAL_GPIO_WritePin(GPIONAME(LED0), (GPIO_PinState)(value));  break;
        case 1:     HAL_GPIO_WritePin(GPIONAME(LED1), (GPIO_PinState)(value));  break;
        case 2:     HAL_GPIO_WritePin(GPIONAME(LED2), (GPIO_PinState)(value));  break;
        default:    HAL_GPIO_WritePin(GPIONAME(LED0), (GPIO_PinState)(value));  break;
        }
    }
