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

void MntCommand(char *p)
    {
    int drv = *p-'0';

    if(drv<0 || drv > 2)
        {
        printf("invalid drive number\n");
        return;
        }

    printf("mounting drive %d %s\n", drv, p);

    if(f_mount(&FatFs[drv], p, 1) != FR_OK)
        {
        printf("FATFS mount error\n");
        }
    else
        {
        printf("FATFS mount OK\n");
        }
    }
