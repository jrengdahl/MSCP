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

void MkfsCommand(char *p)
    {
    int blocks = getdec(&p);
    skip(&p);

    if (f_mkfs(p, FM_FAT|FM_SFD, blocks, (uint8_t *)&qbuf, 512) != FR_OK)
        {
        printf("Filesystem format failed\n");
        }
    else
        {
        printf("Filesystem formatted successfully\n");
        }
    }
