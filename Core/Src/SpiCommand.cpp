#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "spi.h"
#include "serial.h"


void SpiCommand(char *p)
    {
    if(*p=='x')
        {
        skip(&p);
        uint32_t txdata = gethex(&p);
        uint8_t rxdata;

        // HAL_SPI_Transmit(&hspi2, (const uint8_t *)&value, 1, HAL_MAX_DELAY); // Transmit data
        HAL_SPI_TransmitReceive(&hspi2, (const uint8_t *)&txdata, &rxdata, 1, HAL_MAX_DELAY); // Transmit and receive data
        printf("rx = %02x\n", rxdata);
        }
    else if(*p=='c')
        {
        for(int i=0; i<256 && !ControlC; i++)
            {
            HAL_SPI_Transmit(&hspi2, (const uint8_t *)&i, 1, HAL_MAX_DELAY); // Transmit data
            HAL_Delay(100);
            }
        }
    else
        {
        printf("SPI2 (8 LEDS) test commands\n");
        printf("spi x <data>        transmit byte\n");
        printf("spi c               counter display\n");
        }
    }
