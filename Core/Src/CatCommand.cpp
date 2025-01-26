#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "serial.h"
#include "diskio.h"
#include "ff.h"

extern FATFS FatFs[3];
extern FIL fil;
extern uint32_t qbuf[512/4];

void CatCommand(char *p)
    {
    if(f_open(&fil, p, FA_READ) == FR_OK)
        {
        unsigned br; // Bytes read
        FRESULT res;

        do
            {
            res = f_read(&fil, qbuf, 512, &br);
            if(res == FR_OK)
                {
                _write(1, (const char *)&qbuf, br);
                }
            }
        while(res == FR_OK
           && br == 512
           && !ControlC);

        if(res)
            {
            printf("res = %d\n", res);
            }

        f_close(&fil);
        }
    else
        {
        printf("file %s could not be opened\n", p);
        }
    }
