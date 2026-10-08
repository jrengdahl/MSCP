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
    uint8_t *buf1 = &((uint8_t *)&qbuf)[0];
    uint8_t *buf2 = &((uint8_t *)&qbuf)[32];
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
    UINT br1, br2;    // bytes read for file 1 and 2


    // Compare the files byte by byte, reading them in 16 byte chunks.
    while (1)
        {
        // Read a chunk from each file
        res = f_read(&f1, buf1, 16, &br1);
        if (res != FR_OK)
            {
            printf("Failed to read from file: %s at offset %ld\n", path1,offset);
            break;
            }

        res = f_read(&f2, buf2, 16, &br2);
        if (res != FR_OK)
            {
            printf("Failed to read from file: %s at offset %ld\n", path2, offset);
            break;
            }

        // If both br1 and br2 are 0, we've reached the end of both files
        if (br1 == 0 && br2 == 0)
            {
            printf("Files are identical\n");
            break;
            }

        // If br1 and br2 differ the files are of different lengths
        if (br1 != br2)
            {
            printf("Files have different length: %ld %ld\n", offset+br1, offset+br2);
            dump(buf1, br1);
            dump(buf2, br2);
            break;
            }

        bool diff = false;
        // Compare the chunks
        for (unsigned i = 0; i < br1; i++)
            {
            if (buf1[i] != buf2[i])
                {
                // Found a difference
                diff = true;
                printf("Difference found at offset: %lu\n", offset + i);
                dump(buf1, br1);
                dump(buf2, br2);
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
