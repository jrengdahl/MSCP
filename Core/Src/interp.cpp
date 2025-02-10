// a command line interpreter for debugging the target

// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "bogodelay.hpp"
#include "serial.h"
#include "random.hpp"
#include "cyccnt.hpp"
#include "boundaries.h"
#include "context.hpp"
#include "ContextFIFO.hpp"
#include "libgomp.hpp"
#include "omp.h"
#include "tim.h"
#include "spi.h"
#include "octospi.h"
#include "QSPI.h"
#include "diskio.h"
#include "ff.h"
#include "Qbus.hpp"
#include "uqssp.hpp"
#include "MSCP.hpp"



extern void bear();
extern "C" char *strchrnul(const char *s, int c);   // POSIX function but not included in newlib, see https://linux.die.net/man/3/strchr

char buf[INBUFLEN];
bool waiting_for_command = false;

FATFS FatFs[_VOLUMES];
FIL fil;
uint32_t qbuf[512/4];

uint32_t table[]=
        {
        0x00000000,
        0x11111111,
        0x22222222,
        0x33333333,
        0x44444444,
        0x55555555,
        0x66666666,
        0x77777777,
        0x88888888,
        0x99999999,
        0xaaaaaaaa,
        0xbbbbbbbb,
        0xcccccccc,
        0xdddddddd,
        0xeeeeeeee,
        0xffffffff,
        0x12345678,
        0x23456789,
        0x3456789a,
        0x456789ab
        };




// print help string, see usage below
#define HELP(s) else if(buf[0]=='?' && puts(s) && 0){}

void interp()
    {
    bear();
    printf("hello, world!\n");
    printf("build: %s %s\n", __DATE__, __TIME__);

    if(f_mount(&FatFs[0], "0:", 1) != FR_OK)
        {
        printf("FATFS mount error on SD card 0\n");
        }
    else
        {
        printf("FATFS mount OK on SD card 0\n");
        }

    if(f_mount(&FatFs[1], "1:", 1) != FR_OK)
        {
        printf("FATFS mount error on SD card 1\n");
        }
    else
        {
        printf("FATFS mount OK on SD card 1\n");
        }

    if(f_mount(&FatFs[2], "2:", 1) != FR_OK)
        {
        printf("FATFS mount error on SPI-NOR\n");
        }
    else
        {
        printf("FATFS mount OK on SPI-NOR\n");
        }


    while(1)
        {
        char *p;

        yield();
        libgomp_reinit();                                                       // reset things in OPenMP such as the default number of threads
        ControlC = false;
        SerialRaw = false;
        putchar('>');                                                           // output the command line prompt
        fflush(stdout);
        waiting_for_command = true;
        getline(buf, INBUFLEN);                                                 // get a command line
        waiting_for_command = false;
        p = buf;
        skip(&p);                                                               // skip command and following whitespace

        if(buf[0]==0 || buf[0]=='\r' || buf[0]=='\n' || buf[0]==' '){}          // ignore blank lines

        HELP(  "Addresses, data, and sizes are generally in hex.")

        HELP(  "d <addr> <size>}                dump memory")
        else if(buf[0]=='d' && buf[1]==' ')                                                    // memory dump d <hex address> <dec size> {<dump width>}
            {
            extern void DumpCommand(char *p);
            DumpCommand(p);
            }

        HELP(  "m <addr> <data>...              modify memory")
        else if(buf[0]=='m' && buf[1]==' ')                                                    // memory modify <address> <data> ...
            {
            extern void ModifyCommand(char *p);
            ModifyCommand(p);
            }

        HELP(  "f <addr> <size> <data>          fill memory")
        else if(buf[0]=='f' && buf[1]==' ')
            {
            extern void FillCommand(char *p);
            FillCommand(p);
            }

        HELP(  "rt <addr> <size>...             ram test")
        else if(buf[0]=='r' && buf[1]=='t')
            {
            extern void RamTestCommand(char *p);
            RamTestCommand(p);
            }

        HELP(  "i <pin> [<repeat>]              report inputs")
        else if(buf[0]=='i' && buf[1] == ' ')
            {
            extern void InputCommand(char *p);
            InputCommand(p);
            }

        HELP(  "o <pin> <value>                 set an output pin")
        else if(buf[0]=='o' && buf[1] == ' ')
            {
            extern void OutputCommand(char *p);
            OutputCommand(p);
            }

        HELP(  "g <count> <delay> <dest>        toggle a gpio")
        else if(buf[0]=='g' && buf[1]==' ')
            {
            extern void GpioCommand(char *p);
            GpioCommand(p);
            }

        HELP(  "clk <freq in MHz>               set CPU clock")
        else if(buf[0]=='c' && buf[1]=='l' && buf[2]=='k')
            {
            extern void ClkCommand(char *p);
            ClkCommand(p);
            }

        HELP(  "c <count>                       calibrate")
        else if(buf[0]=='c' && buf[1]==' ')
            {
            extern void CalibrateCommand(char *p);
            CalibrateCommand(p);
            }

        HELP(  "ldr <addr>                      load a word from address using ldr")
        else if(buf[0]=='l' && buf[1]=='d' && buf[2]=='r' && buf[3]==' ')
            {
            extern void LdrCommand(char *p);
            LdrCommand(p);
            }

        HELP(  "ldrh <addr> {size {o}}          load a halfword from address using ldrh")
        else if(buf[0]=='l' && buf[1]=='d' && buf[2]=='r' && buf[3]=='h')
            {
            extern void LdrhCommand(char *p);
            LdrhCommand(p);
            }

        HELP(  "ldrb <addr>                     load a byte from address using ldrb")
        else if(buf[0]=='l' && buf[1]=='d' && buf[2]=='r' && buf[3]=='b')
            {
            extern void LdrbCommand(char *p);
            LdrbCommand(p);
            }

        HELP(  "str <value> <addr>              store word to address using str")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='r' && buf[3]==' ')
            {
            extern void StrCommand(char *p);
            StrCommand(p);
            }

        HELP(  "strh <value> <addr>             store a halfword to address using strh")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='r' && buf[3]=='h')
            {
            extern void StrhCommand(char *p);
            StrhCommand(p);
            }

        HELP(  "strb <value> <addr>             store a byte to address using strb")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='r' && buf[3]=='b')
            {
            extern void StrbCommand(char *p);
            StrbCommand(p);
            }

        HELP(   "sum {<addr> <size> <block>}     summarize memory")
        else if(buf[0]=='s' && buf[1]=='u' && buf[2]=='m')
            {
            extern void SumCommand(char *p);
            SumCommand(p);
            }

        HELP(  "t <num>                         measure thread switch time")
        else if(buf[0]=='t' && buf[1]==' ')
            {
            extern void ThreadTestCommand(char *p);
            ThreadTestCommand(p);
            }

        HELP(  "tmp                             read the temperature sensor")
        else if(buf[0]=='t' && buf[1]=='m' && buf[2]=='p')
            {
            extern void TemperatureCommand(char *p);
            TemperatureCommand(p);
            }

        HELP(  "omp <num>                       run an OMP test")
        else if(buf[0]=='o' && buf[1]=='m' && buf[2]=='p')
            {
            extern void OmpTestCommand(char *p);
            OmpTestCommand(p);
            }

        HELP(  "v <type> <num>                  set verbosity level")
        else if(buf[0]=='v' && (buf[1]==' ' || buf[1]==0))
            {
            extern void VerboseCommand(char *p);
            VerboseCommand(p);
            }

        HELP(  "stk                             dump stacks")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='k')
            {
            extern void StackCommand(char *p);
            StackCommand(p);
            }

        HELP(  "mem                             display free memory")
        else if(buf[0]=='m' && buf[1]=='e' && buf[2]=='m')
            {
            extern void mem();
            mem();
            }

        HELP(  "spi <command> {<value>}         SPI2 tests")
        else if(buf[0]=='s' && buf[1]=='p' && buf[2]=='i')
            {
            extern void SpiCommand(char *p);
            SpiCommand(p);
            }

        HELP(  "q                               QSPI tests")
        else if(buf[0]=='q')
            {
            extern void QspiCommand(char *p);
            QspiCommand(p);
            }

        HELP(  "mkfs <blocks> <path>            FATFS format a FATFS drive")
        else if(buf[0]=='m' && buf[1]=='k' && buf[2]=='f' && buf[3]=='s')
            {
            extern void MkfsCommand(char *p);
            MkfsCommand(p);
            }

        HELP(  "mnt <path>                      mount a FATFS volume")
        else if(buf[0]=='m' && buf[1]=='n' && buf[2]=='t')
            {
            extern void MntCommand(char *p);
            MntCommand(p);
            }

        HELP(  "cat <path>                      copy a file to the console")
        else if(buf[0]=='c' && buf[1]=='a' && buf[2]=='t')
            {
            extern void CatCommand(char *p);
            CatCommand(p);
            }

        HELP(  "dump <path>                     dump a file to the console")
        else if(buf[0]=='d' && buf[1]=='u' && buf[2]=='m')
            {
            extern void FdumpCommand(char *p);
            FdumpCommand(p);
            }

        HELP(  "ls <path>                       list the contents of a directory")
        else if(buf[0]=='l' && buf[1]=='s')
            {
            extern void LsCommand(char *p);
            LsCommand(p);
            }

        HELP(  "cp <source path> <dest path>    copy a file")
        else if(buf[0]=='c' && buf[1]=='p')
            {
            extern void CpCommand(char *p);
            CpCommand(p);
            }

        HELP(  "diff <path1> <path2>            compare two files")
        else if(buf[0]=='d' && buf[1]=='i' && buf[2]=='f')
            {
            extern void DiffCommand(char *p);
            DiffCommand(p);
            }



//              //                              //
        HELP(  "fpga <cmd>                      FPGA menu")
        else if(buf[0]=='f' && buf[1] == 'p' && buf[2] == 'g' && buf[3] == 'a')
            {
            if(p[0] == 'c' && p[1] == 'r')
                {
                skip(&p);
                if(isdigit(*p))
                    {
                    int val = getdec(&p);
                    HAL_GPIO_WritePin(GPIONAME(CRESET_N), (GPIO_PinState)(val));
                    }
                else
                    {
                    HAL_GPIO_WritePin(GPIONAME(CRESET_N), (GPIO_PinState)(0));
                    HAL_Delay(2);
                    HAL_GPIO_WritePin(GPIONAME(CRESET_N), (GPIO_PinState)(1));
                    }
                }
            else if(p[0] == '1')
                {
                skip(&p);
                uint32_t addr = gethex(&p);
                uint16_t volatile *hword = (uint16_t volatile *)addr;

                hword[0] = 0x2211;
                hword[1] = 0x4433;
                hword[2] = 0x6655;
                hword[3] = 0x8877;

                SCB->DCCIMVAC = addr;

                uint16_t rb[4];
                rb[0] = hword[0];
                rb[1] = hword[1];
                rb[2] = hword[2];
                rb[3] = hword[3];

                printf("%04x %04x %04x %04x\n", rb[0], rb[1], rb[2], rb[3]);
                }
            else if(p[0] == '2')
                {
                skip(&p);
                uint32_t *addr = (uint32_t *)gethex(&p);

                memcpy32(addr, table, sizeof(table));
                SCB->DCCIMVAC = (uint32_t)addr;
                SCB->DCCIMVAC = (uint32_t)addr+32;
                SCB->DCCIMVAC = (uint32_t)addr+64;
                memcpy32(qbuf, addr, sizeof(table));
                }
            else
                {
                printf("fpga commands\n");
                printf("fpga cr        pulse CRESET_N\n");
                printf("fpga 1 <addr>  write and read back four halfwords\n");
                printf("fpga 2 <addr>  write and read back an 80 byte table\n");
                }
            }



//              //                              //
#define MAXCOUNT 8
        HELP(  "b <cmd> <args>                  Qbus menu")
        else if(buf[0]=='b')
            {
            if(p[0] == 'r' && p[1] == ' ')
                {
                uint16_t values[MAXCOUNT];

                skip(&p);

                int repeat = 1;
                if(*p=='r')
                    {
                    ++p;
                    repeat = getdec(&p);
                    skip(&p);
                    }

                uint32_t addr = gethex(&p);            // get the address
                skip(&p);

                bool octal = false;
                if(*p=='o')
                    {
                    octal = true;
                    skip(&p);
                    }

                int count = 1;
                if(isdigit(*p))
                    {
                    count = getdec(&p);
                    if(count > MAXCOUNT)count = MAXCOUNT;
                    }

                for(int rpt=0; rpt<repeat && !ControlC; yield(), rpt++)
                    {
                    QDMAbegin();
                    for(int i=0; i<count; i++)
                        {
                        values[i] = Qread(addr+i*2);
                        }
                    QDMAend();
                    }

                for(int i=0; i<count; i++)
                    {
                    if(octal)printf("%06o\n", (int)values[i]);
                    else printf("%04x\n", values[i]);                // print the result
                    }
                }

            else if(p[0] == 'w' && p[1]=='w')
                {
                uint16_t values[MAXCOUNT];
                int count = 0;

                skip(&p);

                int repeat = 1;
                if(*p=='r')
                    {
                    ++p;
                    repeat = getdec(&p);
                    skip(&p);
                    }

                uint32_t addr = gethex(&p);             // get the address
                skip(&p);

                while(count < MAXCOUNT && (isxdigit(*p) || *p=='o'))
                    {
                    values[count++] = gethex(&p);            // get the data
                    skip(&p);
                    }

                for(int rpt=0; rpt<repeat && !ControlC; yield(), rpt++)
                    {
                    QDMAbegin();
                    for(int i=0; i<count; i++)
                        {
                        Qwrite(addr+i*2, values[i]);
                        }
                    QDMAend();
                    }
                }

            else if(p[0] == 'd' && p[1] == ' ')
                {
                skip(&p);

                unsigned addr = gethex(&p);            // get the address
                skip(&p);

                int size = 2;
                if(isxdigit(*p) || *p=='o')
                    {
                    size = gethex(&p);
                    skip(&p);
                    }

                bool octal = false;
                if(*p=='o')
                    {
                    octal = true;
                    }

                printf("Qbus dump %08o %06o\n", addr, size);

                uint16_t buffer[8];

                for(int i=0; i<size; i+= 16, addr += 16)
                    {
                    QReadBlock(addr, buffer, 8);

                    if(octal)printf("%08o ", addr);
                    else printf("%06x ", addr);

                    for(int j=0; j<8; j++)
                        {
                        if(octal)printf("%06o ", buffer[j]);
                        else printf("%04x ",buffer[j]);
                        }
                    printf("\n");
                    }
                }

            else
                {
                printf("bus commands:\n");
                printf("b r {r<repeat count>} <addr> {o} {<count>}   read words from Qbus\n");
                printf("b ww {r<repeat count>} <addr> <data> ...     write words to Qbus\n");
                printf("b d <addr> {o} {<count>}                     dump words from Qbus\n");
                printf("Controller addresses:\n");
                printf("0x60000000 IP, PDP-11 read = poll; PDP-11 write = init controller, data ignored\n");
                printf("               controller read = read status and clear latched status bits\n");
                printf("               controller write = NOP\n");
                printf(" bit  0: IP was read\n");
                printf(" bit  1: IP was written\n");
                printf(" bit  2: SA was read\n");
                printf(" bit  3: SA was written\n");
                printf(" bit  4: state of BRPLY\n");
                printf(" bit  5: BRPLY was asserted\n");
                printf(" bit  6: BRPLY was deasserted\n");
                printf(" bit  7: state of BSACK\n");
                printf(" bit  15-8: read as zero\n");
                printf("0x60000002 SA controller writes status, reads address\n");
                printf("0x60000004 CT bus control bits\n");
                printf(" bit  0: BSYNC\n");
                printf(" bit  1: BDIN\n");
                printf(" bit  2: BDOUT\n");
                printf(" bit  3: BWTBT\n");
                printf(" bit  4: BDMR\n");
                printf(" bit  5: BREF\n");
                printf(" bit  6: BBS7\n");
                printf(" bit  7: BIRQ4\n");
                printf(" bit  8: BIRQ5\n");
                printf(" bit  9: BIRQ6\n");
                printf(" bit 10: enable address onto Qbus during address phase\n");
                printf(" bit 11: enable data_out onto Qbus during data phase\n");
                printf(" bit 12: Clear SA when IP is read by controller\n");
                printf(" bit 13: spare\n");
                printf(" bit 14: spare\n");
                printf(" bit 15: write 1 to signal DMA_done (clear BSACK), reads back as 0\n");
                printf("0x60000006 LO low address\n");
                printf("0x60000008 HI high address (6 bits)\n");
                printf("0x6000000A DATA_OUT data to be written to Qbus\n");
                printf("0x6000000C DATA_IN data read from Qbus\n");
                }
            }

//              //                              //
        HELP(  "mscp                            test MSCP")
        else if(buf[0]=='m' && buf[1]=='s' && buf[2]=='c' && buf[3]=='p')
            {
            Qinit();
            MSCP_poll();
            }

        // print the help screen
        else if(buf[0]=='?')
            {
            }

        // else I dunno what you want
        else printf("illegal command\n");
        }

    }


