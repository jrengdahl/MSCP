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

void CpCommand(char *p)
    {
    char *src_path = p;
    skip(&p);
    p[-1] = 0;
    char *dst_path = p;

    FIL src_file, dst_file;
    FRESULT res;
    UINT br, bw;

    // Open the source file
    res = f_open(&src_file, src_path, FA_READ);
    if (res != FR_OK)
        {
        printf("Failed to open source file: %s\n", src_path);
        return;
        }

    // Open or create the destination file
    res = f_open(&dst_file, dst_path, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK)
        {
        printf("Failed to open destination file: %s\n", dst_path);
        f_close(&src_file);
        return;
        }

    // Copy data from source to destination
    while (1)
        {
        // Read a chunk of data from the source file
        res = f_read(&src_file, qbuf, 512, &br);
        if (res != FR_OK || br == 0) break;  // Check for end of file or read error

        // Write the chunk of data to the destination file
        res = f_write(&dst_file, qbuf, br, &bw);
        if (res != FR_OK || bw < br)
            {
            printf("Failed to write to destination file: %s\n", dst_path);
            f_close(&src_file);
            f_close(&dst_file);
            return;
            }
        }

    // Close both files
    f_close(&src_file);
    f_close(&dst_file);

    // Check if the loop exited due to an error
    if (res != FR_OK)
        {
        printf("Failed to copy file: %s to %s\n", src_path, dst_path);
        return;
        }

    printf("File copied successfully: %s to %s\n", src_path, dst_path);
    }
