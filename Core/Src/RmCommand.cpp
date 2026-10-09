#include <stdio.h>
#include <ctype.h>
#include "ff.h"
#include "local.h"

void RmCommand(char *p)
    {
    char *path = p;

    if (*path == 0)
        {
        printf("Usage: rm <file path>\n");
        return;
        }

    FILINFO info;
    FRESULT result = f_stat(path, &info);
    if (result != FR_OK)
        {
        printf("rm: cannot access %s (FatFS error %d)\n", path, result);
        return;
        }

    if (info.fattrib & AM_DIR)
        {
        printf("rm: %s is a directory\n", path);
        return;
        }

    result = f_unlink(path);
    if (result != FR_OK)
        {
        printf("rm: cannot remove %s (FatFS error %d)\n", path, result);
        return;
        }

    printf("removed %s\n", path);
    }
