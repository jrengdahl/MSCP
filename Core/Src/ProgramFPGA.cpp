/*
 * ProgramFPGA.cpp
 *
 *  Created on: Feb 15, 2025
 *      Author: engdahl
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "spi.h"
#include "octospi.h"
#include "QSPI.h"
#include "diskio.h"
#include "ff.h"
#include "gpio.h"

extern uint32_t qbuf[512/4];

void ProgramFPGA(char *p = 0)
    {
    const char *filename = "2:qbus.hex.bin";
    FIL src_file;
    FRESULT fres = FR_OK;
    UINT bytes_read;
    HAL_StatusTypeDef sres = HAL_OK;
    int size = 0;
    const int BLKSIZE = 512;

    if(p && *p)
        {
        filename = p;
        }

    printf("programming FPGA with %s\n", filename);

    // Open the source file
    fres = f_open(&src_file, filename, FA_READ);
    if(fres != FR_OK)
        {
        printf("Failed to open source file: %s\n", filename);
        return;
        }

    // toggle CRESET_N low then high
    HAL_GPIO_WritePin(GPIONAME(CRESET_N),(GPIO_PinState)0);
    HAL_Delay(300);
    HAL_GPIO_WritePin(GPIONAME(CRESET_N),(GPIO_PinState)1);
    HAL_Delay(150);

    // copy file to FPGA
    while(1)
        {
        // Read a chunk of data from the source file
        trigon(0);
        fres = f_read(&src_file, qbuf, BLKSIZE, &bytes_read);
        trigoff(0);

        if (fres != FR_OK || bytes_read == 0)break; // Check for end of file or read error

        // Write the chunk of data to the FPGA
        sres = HAL_SPI_Transmit(&hspi5, (const uint8_t*) (&qbuf), bytes_read, HAL_MAX_DELAY);
        if (sres != HAL_OK)break;

        size += bytes_read;
        }

    // Close file
    f_close(&src_file);

    memset(qbuf, 0, sizeof(qbuf));
    HAL_SPI_Transmit(&hspi5, (const uint8_t*) (&qbuf), 128, HAL_MAX_DELAY);

    // Check if the loop exited due to an error
    if(fres != FR_OK || sres != HAL_OK)
        {
        printf("Failed to copy file to FPGA: fres = %d, sres = %d\n", fres, sres);
        }
    else
        {
        printf("programming complete, %d bytes written\n", size);
        }

    unsigned cdone = HAL_GPIO_ReadPin(GPIONAME(CDONE));
    unsigned nstatus = HAL_GPIO_ReadPin(GPIONAME(NSTATUS));

    printf("CDONE = %d, NSTATUS = %d\n", cdone, nstatus);
    }


