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

void FdumpCommand(char *p)
    {
    // Read from the file
     if(f_open(&fil, p, FA_READ) != FR_OK)
         {
         printf("file %s could not be opened\n", p);
         return;
         }

     unsigned br; // Bytes read
     FRESULT res;

     do
         {
         res = f_read(&fil, qbuf, 512, &br);
         if(res == FR_OK)
             {
             dump(qbuf, br);
             }
         }
     while(res == FR_OK
        && br == 512
        && !ControlC);

     printf("res = %d\n", res);

     f_close(&fil);
    }
