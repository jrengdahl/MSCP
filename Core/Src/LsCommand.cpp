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

void LsCommand(char *p)
    {
    FRESULT res;
    DIR dir;
    FILINFO fno;
    FATFS *fs;
    DWORD fre_clust, fre_sect, tot_sect;

    // Open the directory
    res = f_opendir(&dir, p); /* Open the directory */
    if (res != FR_OK)
        {
        printf("Failed to open directory: %d\n", res);
        return;
        }

    while (1)
        {
        res = f_readdir(&dir, &fno);                    /* Read a directory item */
        if (res != FR_OK || fno.fname[0] == 0) break;   /* Break on error or end of dir */
        if (fno.fattrib & AM_DIR)
            {                     /* It is a directory */
            printf("  <DIR>  %s\n", fno.fname);
            }
        else
            {                                        /* It is a file */
            printf("  %8lu  %s\n", fno.fsize, fno.fname);
            }
        }
    f_closedir(&dir);

    f_getfree(p, &fre_clust, &fs);
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;

    if(tot_sect/2 <100*1024)
        {
        printf("volume size: %lu KB\n", tot_sect / 2);
        printf("free space:  %lu KB\n", fre_sect / 2);
        }
    else if(tot_sect/2 <100*1024*1024)
        {
        printf("volume size: %lu MB\n", tot_sect / (2*1024));
        printf("free space:  %lu MB\n", fre_sect / (2*1024));
        }
    else
        {
        printf("volume size: %lu GB\n", tot_sect / (2*1024*1024));
        printf("free space:  %lu GB\n", fre_sect / (2*1024*1024));
        }
    }
