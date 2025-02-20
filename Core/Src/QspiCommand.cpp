#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "serial.h"
#include "QSPI.h"
#include "diskio.h"

extern uint32_t qbuf[512/4];
extern void print_status_register(uint8_t regCommand);

extern "C" DRESULT QSPI_ioctl (BYTE pdrv, BYTE cmd, void *buff);

void EraseQSPI()
    {
    printf("erasing entire SPI-NOR, this may take several minutes\n");
    QSPI_EraseChip(&hospi1);
    printf("erasing complete\n");
    }

void QspiCommand(char *p)
    {
    if(p[0] == 'r')
        {
        int count = 1;

        skip(&p);
        uint32_t addr = gethex(&p);
        skip(&p);
        if(isxdigit(*p))count = gethex(&p);

        for(int i=0; i<count; i++)
            {
            QSPI_ReadPage(&hospi1, addr, (uint8_t *)&qbuf, 256);
            printf("%08x\n", (unsigned)addr);
            dump(qbuf, 256);
            addr += 256;
            }
        }
    else if(p[0] == 's')
        {
        print_status_register(READ_STATUS_REG_1_CMD);
        print_status_register(READ_STATUS_REG_2_CMD);
        print_status_register(READ_STATUS_REG_3_CMD);
        }
    else if(p[0] == 'f')
        {
        skip(&p);
        uint32_t addr = gethex(&p);
        skip(&p);
        uint32_t data = gethex(&p);

        for(int i=0; i<256/4; i++)qbuf[i] = data;

        QSPI_WritePage(&hospi1, addr, (uint8_t *)&qbuf, 256);
        }
    else if(p[0] == 'e' && p[1] != 'e')
        {
        int count = 1;

        skip(&p);
        uint32_t addr = gethex(&p);
        skip(&p);
        if(isxdigit(*p))count = gethex(&p);

        for(int i=0; i<count; i++)
            {
            QSPI_EraseSector(&hospi1, addr);
            addr += 256;
            }
        }
    else if(p[0] == 'e' && p[1] == 'e')
        {
        EraseQSPI();
        }
    else if(p[0] == 'c')
        {
        uint32_t addr = 0;
        DWORD count;
        DWORD i;
        int j;

        QSPI_ioctl(0, GET_SECTOR_COUNT, &count);  // get sector count (first arg, pdrv, is ignored)

        for(i=0; i<count; i++)
            {
            QSPI_ReadPage(&hospi1, addr, (uint8_t *)&qbuf, 256);
            for(j=0; j<256/4; j++)
                {
                if(qbuf[j] != 0xffffffff)
                    {
                    printf("chip is not erased at %lx\n", addr);
                    break;
                    }
                }
            if(j != 256/4)break;
            addr += 256;
            }
        if(i == count)printf("entire chip is erased\n");
        }
    else if(p[0] == 'x')
        {
        extern unsigned CPU_CLOCK_FREQUENCY;
        extern void SetClock(unsigned clk);
        extern int xmodem_receive(uint8_t *buf);

        printf("CPU clock will be set to 250 MHz during the download\n");
        unsigned prev_clk = CPU_CLOCK_FREQUENCY;
        SetClock(250);                                  // 100 MHz doesn't work well for long USB packets on this processor for some unknown reason

        int res = xmodem_receive((uint8_t *)&qbuf);
        if(res==0)printf("file received OK\n");
        else printf("xmodem transfer failed\n");

        SetClock(prev_clk);
        }
    else
        {
        printf("quad-SPI commands:\n");
        printf("  q r <addr> <count>       read and dump pages\n");
        printf("  q s                      print status bytes\n");
        printf("  q f <addr> <data>        fill a page with data word\n");
        printf("  q e <addr> <count>       erase sectors\n");
        printf("  q ee                     erase entire chip\n");
        printf("  q c                      erase check entire chip\n");
        printf("  q x                      download and write image via Xmodem\n");
        }
    }
