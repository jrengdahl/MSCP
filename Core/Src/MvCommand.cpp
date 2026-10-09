#include <stdio.h>
#include <ctype.h>
#include "ff.h"
#include "local.h"


void MvCommand(char *p)
    {
    char *source_path = p;
    skip (&p);
    p[-1] = 0;
    char *destination_path = p;

    if (*source_path == 0 || *destination_path == 0)
        {
        printf("Usage: mv <source file path> <destination file path>\n");
        return;
        }

    FILINFO info;
    FRESULT result = f_stat(source_path, &info);
    if (result != FR_OK)
        {
        printf("mv: cannot access %s (FatFS error %d)\n", source_path, result);
        return;
        }

    if (info.fattrib & AM_DIR)
        {
        printf("mv: %s is a directory\n", source_path);
        return;
        }

    result = f_rename(source_path, destination_path);
    if (result != FR_OK)
        {
        printf("mv: cannot rename %s to %s (FatFS error %d)\n", source_path, destination_path, result);
        return;
        }

    printf("renamed %s to %s\n", source_path, destination_path);
    }
