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

void DiffCommand(char *p)
    {
    char *path1 = p;
    skip(&p);
    p[-1] = 0;
    char *path2 = p;

    FIL f1, f2;
    FRESULT res;

    // Open the file 1
    res = f_open(&f1, path1, FA_READ);
    if (res != FR_OK)
        {
        printf("Failed to open first file: %s\n", path1);
        return;
        }

    // Open file 2
    res = f_open(&f2, path2, FA_READ);
    if (res != FR_OK)
        {
        printf("Failed to open file 2: %s\n", path2);
        f_close(&f1);
        return;
        }

    DWORD offset = 0;
    UINT br1, br2;


    // Compare the files byte by byte
    while (1)
        {
        // Read a chunk from each file
        res = f_read(&f1, (uint8_t *)&qbuf[0], 16, &br1);
        if (res != FR_OK)
            {
            printf("Failed to read from file: %s\n", path1);
            break;
            }

        res = f_read(&f2, (uint8_t *)&qbuf[32], 16, &br2);
        if (res != FR_OK)
            {
            printf("Failed to read from file: %s\n", path2);
            break;
            }

        // If both br1 and br2 are 0, we've reached the end of both files
        if (br1 == 0 && br2 == 0)
            {
            printf("Files are identical\n");
            break;
            }

        // If both br1 and br2 are 0, we've reached the end of both files
        if (br1 != br2)
            {
            printf("Files have different length: %d %d\n", br1, br2);
            dump(&qbuf[0], 16);
            dump(&qbuf[32], 16);
            break;
            }


        bool diff = false;
        // Compare the chunks
        for (unsigned i = 0; i < br1; i++)
            {
            if (qbuf[i] != qbuf[32+i])
                {
                // Found a difference
                diff = true;
                printf("Difference found at offset: %lu\n", offset + i);
                dump(&qbuf[0], 16);
                dump(&qbuf[32], 16);
                break;
                }
            }
        if(diff)break;

        // Update the offset
        offset += br1;
        }

    // Close both files
    f_close(&f1);
    f_close(&f2);
    }
