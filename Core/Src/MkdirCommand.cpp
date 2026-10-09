#include <stdio.h>
#include "ff.h"
#include "local.h"

void MkdirCommand(char *p)
    {
    if (*p == 0)
        {
        printf("Usage: mkdir <directory path>\n");
        return;
        }

    FRESULT result = f_mkdir(p);
    if (result != FR_OK)
        {
        printf("mkdir: cannot create %s (FatFS error %d)\n", p, result);
        return;
        }

    printf("created directory %s\n", p);
    }
