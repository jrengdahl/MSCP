#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "serial.h"
#include "QSPI.h"

extern uint32_t qbuf[512/4];
extern void print_status_register(uint8_t regCommand);


void QspiCommand(char *p)
    {
    if(*p == 'r')
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
        printf("erasing entire SPI-NOR, this may take several minutes\n");
        QSPI_EraseChip(&hospi1);
        printf("erasing complete\n");
        }
    else if(p[0] == 'x')
        {
        int res;

        extern int xmodem_receive(uint8_t *buf);
        res = xmodem_receive((uint8_t *)&qbuf);
        if(res==0)printf("file received OK\n");
        else printf("xmodem transfer failed %d\n", res);
        }
    else printf("unrecognized subcommand\n");
    }
