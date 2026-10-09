#include <stdio.h>
#include "ff.h"
#include "local.h"

void RmdirCommand(char *p)
    {
    if (*p == 0)
        {
        printf("Usage: rmdir <directory path>\n");
        return;
        }

    FILINFO info;
    FRESULT result = f_stat(p, &info);
    if (result != FR_OK)
        {
        printf("rmdir: cannot access %s (FatFS error %d)\n", p, result);
        return;
        }

    if (!(info.fattrib & AM_DIR))
        {
        printf("rmdir: %s is not a directory\n", p);
        return;
        }

    result = f_unlink(p);
    if (result != FR_OK)
        {
        printf("rmdir: cannot remove %s (FatFS error %d)\n", p, result);
        return;
        }

    printf("removed directory %s\n", p);
    }
