// a command line interpreter for debugging the target

// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#include <context.hpp>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <ContextFIFO.hpp>
#include <math.h>
#include "cmsis.h"
#include "local.h"
#include "bogodelay.hpp"
#include "serial.h"
#include "random.hpp"
#include "cyccnt.hpp"
#include "main.h"
#include "boundaries.h"
#include "libgomp.hpp"
#include "omp.h"
#include "tim.h"
#include "spi.h"
#include "octospi.h"
#include "QSPI.h"
#include "diskio.h"
#include "ff.h"



extern void bear();
extern "C" char *strchrnul(const char *s, int c);   // POSIX function but not included in newlib, see https://linux.die.net/man/3/strchr
extern void summary(unsigned char *, unsigned, unsigned, int);
extern void RamTest(uint8_t *addr, unsigned size, unsigned repeat, unsigned limit);
extern "C" void SystemClock_PLL_Config(unsigned);
extern "C" void SystemClock_HSI_Config(void);
extern char InterpStack[2048];
extern omp_thread omp_threads[GOMP_MAX_NUM_THREADS];
bool waiting_for_command = false;

static FATFS FatFs;
static FIL fil;
static uint32_t qbuf[512/4];



void print_status_register(uint8_t regCommand)
{
    uint8_t statusReg = 0;

    if (QSPI_ReadStatusReg(&hospi1, regCommand, &statusReg) != HAL_OK)
    {
        printf("Error reading Status Register %02x\n", regCommand);
    }
    else
    {
        printf("Status Register %02x: 0x%02X\n", regCommand, statusReg);
    }
}



// print a large number with commas
void commas(uint32_t x)
    {
    if(x>=1000000000)
        {
        printf("%lu,%03lu,%03lu,%03lu", x/1000000000, x/1000000%1000, x/1000%1000, x%1000);
        } // those are "L"s after the threes, not ones
    else if(x>=1000000)
        {
        printf("%lu,%03lu,%03lu", x/1000000, x/1000%1000, x%1000);
        }
    else if(x>=1000)
        {
        printf("%lu,%03lu", x/1000%1000, x%1000);
        }
    else printf("%lu", x);
    }

// print help string, see usage below
#define HELP(s) else if(buf[0]=='?' && puts(s) && 0){}

char buf[INBUFLEN];

void interp()
    {

    bear();
    printf("hello, world!\n");
    printf("build: %s %s\n", __DATE__, __TIME__);

    if(f_mount(&FatFs, "0:", 1) != FR_OK)
        {
        printf("FATFS mount error on SPI-NOR\n");
        }
    else
        {
        printf("FATFS mount OK on SPI-NOR\n");
        }


    while(1)
        {
        const char *p;

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

//              //                              //
        HELP(  "d <addr> <size>}                dump memory")
        else if(buf[0]=='d' && buf[1]==' ')                                                    // memory dump d <hex address> <dec size> {<dump width>}
            {
            unsigned *a;
            int s;

            a = (unsigned *)gethex(&p);                                         // get the first arg
            skip(&p);
            if(isxdigit(*p))s = gethex(&p);                                      // the second arg is the size
            else s = 4;

            dump(a, s);                                                         // do the dump
            }

//              //                              //
        HELP(  "m <addr> <data>...              modify memory")
        else if(buf[0]=='m' && buf[1]==' ')                                                    // memory modify <address> <data> ...
            {
            unsigned *a;
            unsigned v;

            a = (unsigned *)gethex(&p);                                         // get the first arg
            skip(&p);
            while(isxdigit(*p))                                                 // while there is any data left on the command line
                {
                v = gethex(&p);                                                 // get the data
                skip(&p);                                                       // skip that arg and folowing whitespace
                *a++ = v;                                                       // store the data
                }
            }


//              //                              //
        HELP(  "f <addr> <size> <data>          fill memory")
        else if(buf[0]=='f' && buf[1]==' ')
            {
            uint32_t *a;
            uintptr_t s;
            uint32_t v;

            a = (uint32_t *)gethex(&p);                     // get the first arg
            skip(&p);
            s = gethex(&p);                                 // the second arg is the size
            skip(&p);
            v = gethex(&p);                                 // value is the third arg

            while(s>0)                                      // fill memory with the value
                {
                *a++ = v;
                s -= sizeof(*a);
                }
            }


//              //                              //
        HELP(  "rt <addr> <size>...             ram test")
        else if(buf[0]=='r' && buf[1]=='t')
            {
            uint8_t *addr;
            unsigned size;
            int repeat = 1;
            unsigned limit = 10;

            addr = (uint8_t *)gethex(&p);                   // get the first arg (address)
            skip(&p);
            size = gethex(&p);                              // get second arg (size of area to test)
            skip(&p);
            if(isdigit(*p))                                 // get optional third arg (repeat count)
                {
                repeat = getdec(&p);
                skip(&p);
                if(isdigit(*p))                             // get optional fourth arg (max errors to report)
                    {
                    limit = getdec(&p);
                    }
                }

            RamTest(addr, size, repeat, limit);
            }


#if 0
        //              //                              //
        HELP(  "i <pin> [<repeat>]              report inputs")
        else if(buf[0]=='i' && buf[1] == ' ')
            {
            unsigned i = 0;
            unsigned value = 0;
            unsigned repeat = 1;
            unsigned last = 99;

            if(isdigit(*p))
                {
                i = getdec(&p);
                skip(&p);
                if(isdigit(*p))
                    {
                    repeat = getdec(&p);
                     }
                }

            while(repeat && !ControlC)
                {
                switch(i)
                    {
                case 0:  value = HAL_GPIO_ReadPin(GPIONAME( SW1)); break;
                default: value = HAL_GPIO_ReadPin(GPIONAME( SW1)); break;
                    }

                if(value != last)
                    {
                    printf("%u\n", value);
                    last = value;
                    --repeat;
                    }

                yield();
                }
            }
#endif


//              //                              //
        HELP(  "o <pin> <value>                 set an output pin")
        else if(buf[0]=='o' && buf[1] == ' ')
            {
            unsigned o = 0;                     // output number
            unsigned value = 0;                 // output value

            if(isdigit(*p))
                {
                o = getdec(&p);
                skip(&p);
                if(isdigit(*p))
                    {
                    value = getdec(&p);
                    }
                }

            switch(o)
                {
                case 0:     HAL_GPIO_WritePin(GPIONAME(LED0), (GPIO_PinState)(value));  break;
                case 1:     HAL_GPIO_WritePin(GPIONAME(LED1), (GPIO_PinState)(value));  break;
                case 2:     HAL_GPIO_WritePin(GPIONAME(LED2), (GPIO_PinState)(value));  break;
                default:    HAL_GPIO_WritePin(GPIONAME(LED0), (GPIO_PinState)(value));  break;
                }
            }

//              //                              //
        HELP(  "g <count> <delay> <dest>        toggle a gpio")
        else if(buf[0]=='g' && buf[1]==' ')
            {
            unsigned count = 1;                 // number of blinks
            unsigned delay = 1;                 // delay (in CPU clocks) between blinks
            unsigned dest = 0;                  // which LED to blink

            if(isdigit(*p))
                {
                count = getdec(&p);
                skip(&p);
                if(isdigit(*p))
                    {
                    delay = getdec(&p);
                    skip(&p);
                    if(isdigit(*p))
                        {
                        dest = getdec(&p);
                        }
                    }
                }

            for(unsigned i=0; i<count && !ControlC; i++)
                {
                switch(dest)
                    {
                    case  0: HAL_GPIO_WritePin(GPIONAME( LED0), (GPIO_PinState)(i&1)); break;
                    case  1: HAL_GPIO_WritePin(GPIONAME( LED1), (GPIO_PinState)(i&1)); break;
                    case  2: HAL_GPIO_WritePin(GPIONAME( LED2), (GPIO_PinState)(i&1)); break;
                    default: HAL_GPIO_WritePin(GPIONAME( LED0), (GPIO_PinState)(i&1)); break;
                    }

                bogodelay(delay);
                }
            }


        //              //                              //
        HELP(  "clk <freq in MHz>               set CPU clock")
        else if(buf[0]=='c' && buf[1]=='l' && buf[2]=='k')
            {
            if(isdigit(*p))                         // if an arg is given
                {
                unsigned clk = getdec(&p);          // set the clock frequency to the new value
                SystemClock_HSI_Config();           // have to deselect the PLL before reprogramming it
                SystemClock_PLL_Config(clk);        // set the PLL to the new frequency

                // set the TIM2 prescaler to the new frequency so that it always ticks at 1 MHz
                htim2.Instance->PSC = (clk/2) - 1;  // set the prescale value
                htim2.Instance->EGR = TIM_EGR_UG;   // generate an update event to update the prescaler immediately
                }

            printf("CPU clock is %u MHz\n", CPU_CLOCK_FREQUENCY);
            if(CPU_CLOCK_FREQUENCY > 100)
                {
                printf("trace may not be stable at frequencies over 100 MHz\n");
                }
            }


//              //                              //
        HELP(  "c <count>                       calibrate")
        else if(buf[0]=='c' && buf[1]==' ')
            {
            int size = 32;
            uint32_t ticks = 0;
            uint32_t lastTIM2 = 0;
            uint32_t elapsedTIM2 = 0;
            float last_wtime;
            float elapsed_wtime;

            uint64_t count = 1;

            if(isdigit(*p))                                                 // while there is any data left on the command line
                {
                count = getlong(&p);                                        // get the count
                skip(&p);
                if(isdigit(*p))                                             // while there is any data left on the command line
                    {
                    size = getdec(&p);                                      // get the size
                    }
                }

            if(size==64)
                {
                __disable_irq();
                last_wtime = omp_get_wtime(0);
                lastTIM2 = __HAL_TIM_GET_COUNTER(&htim2);
                Elapsed();
                bogodelay(count);
                ticks = Elapsed();
                elapsedTIM2 = __HAL_TIM_GET_COUNTER(&htim2) - lastTIM2;
                elapsed_wtime = omp_get_wtime(0) - last_wtime;
                __enable_irq();
                }
            else
                {
                __disable_irq();
                last_wtime = omp_get_wtime(0);
                lastTIM2 = __HAL_TIM_GET_COUNTER(&htim2);
                Elapsed();
                bogodelay((uint32_t)count);
                ticks = Elapsed();
                elapsedTIM2 = __HAL_TIM_GET_COUNTER(&htim2) - lastTIM2;
                elapsed_wtime = omp_get_wtime(0) - last_wtime;
                __enable_irq();
                }
            commas(ticks);
            printf(" microseconds by CYCCNT\n");
            commas(elapsedTIM2);
            printf(" microseconds by TIM2\n");
            printf("%lf microseconds by wtime\n", elapsed_wtime*1000000.0);
            }


        //              //                              //
        HELP(  "ldr <addr>                      load a word from address using ldr")
        else if(buf[0]=='l' && buf[1]=='d' && buf[2]=='r')
            {
            uintptr_t addr = gethex(&p);            // get the address
            uint32_t value;

            __asm__ __volatile__("ldr %0, [%1]"     // load a word from the given address
             : "=r"(value)
             : "r"(addr)
             : );

            printf("%lx\n", value);                 // print the result
            }

//              //                              //
        HELP(  "str <value> <addr>              store value to address using str")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='r')
            {
            uint32_t value = gethex(&p);
            skip(&p);
            uintptr_t addr = gethex(&p);            // get the address

            __asm__ __volatile__("str %0, [%1]"
            :
            : "r"(value), "r"(addr)
            : );
            }


//               //                              //
        HELP(   "sum {<addr> <size> <block>}     summarize memory")
        else if(buf[0]=='s' && buf[1]=='u' && buf[2]=='m')
            {
            unsigned char *addr = (unsigned char *)&_memory_start;
            unsigned size = (unsigned char *)&_memory_end - (unsigned char *)&_memory_start;
            unsigned inc = 256;
            int values = 0;
            bool defaults = false;

            if(*p == 'v')                                       // flag, if specified print value of RAM if all bytes are the same
                {
                values = 1;
                skip(&p);
                }

            if(*p == 0)                                         // if no other args then assume the default memory ranges from the linker script
                {
                defaults = true;
                }

            else if(isxdigit(*p))
                {
                addr = (unsigned char *)gethex(&p);             // the first arg is the base address, in hex
                skip(&p);
                if(isxdigit(*p))                                // the second arg is the memory size, in hex
                    {
                    size = gethex(&p);
                    skip(&p);
                    if(isxdigit(*p))                            // the third arg is the block size, in hex
                        {
                        inc = gethex(&p);
                        }
                    }
                }

            summary(addr, size, inc, values);

            if(defaults && &_memory2_start != &_memory2_end)
                {
                printf("\n");
                summary((unsigned char *)&_memory2_start, (unsigned char *)&_memory2_end - (unsigned char *)&_memory2_start, inc, values);
                }
            }


//              //                              //
        HELP(  "t <num>                         measure thread switch time")
        else if(buf[0]=='t' && buf[1]==' ')
            {
            Context *ctx = Context::pointer();      // get the context pointer for thread 0
            unsigned count = getdec(&p);            // get the number of iterations
            unsigned i = count;
            float start = 0;
            float ticks = 0;

            start = omp_get_wtime_float();

            #pragma omp parallel num_threads(2)
            if(omp_get_thread_num() == 0)
                {
                while(i)
                    {
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    Context::suspend();
                    }
                }
            else
                {
                while(i)
                    {
                    --i;
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    ctx->resume();
                    }
                }

            ticks = omp_get_wtime_float() - start;

            printf("\n%lf nsec\n", ticks*1000000000.0/(count*40));
             }


//              //                              //
        HELP(  "tmp                             read the temperature sensor")
        else if(buf[0]=='t' && buf[1]=='m' && buf[2]=='p')
            {
            extern int read_temperature_raw();
            extern int read_temperature();
            int last_avg = -1;
            int repeat = 50;
            int avg = 0;
            double last_change = omp_get_wtime();
            double now;
            bool first_time = true;
            const int NAVG = 50;

            if(*p == 'r')
                {
                printf("raw temperature = %d\n", read_temperature_raw());
                continue;
                }

            if(isdigit(*p))                                                 // while there is any data left on the command line
                {
                repeat = getdec(&p);                                        // get the count
                }

            while(repeat && !ControlC)
                {
                int tmp = read_temperature();

                if(first_time) avg = tmp;
                else           avg = (avg*(NAVG-1) + tmp)/NAVG;

                if(avg != last_avg)
                    {
                    now = omp_get_wtime();
                    printf("temperature: %3d C, %3d F, avg = %3d C, elapsed = %5.3f\n", tmp, (tmp*18 + 325)/10, avg, now-last_change);
                    last_avg = avg;
                    last_change = now;
                    -- repeat;
                    }

                first_time = false;

                yield();
                }
            }

//              //                              //
        HELP(  "omp <num>                       run an OMP test")
        else if(buf[0]=='o' && buf[1]=='m' && buf[2]=='p')
            {
            extern void omp_hello(int);
            extern void omp_for(int);
            extern void omp_single(int);
            extern void permute(int colors_arg, int balls, int plevel_arg, int verbose_arg);

            int test = 0;

            if(!isdigit(*p))
                {
                printf("0: omp_hello,  test #pragma omp parallel num_threads(arg)\n");
                printf("1: omp_for,    test #pragma omp parallel for num_threads(arg)\n");
                printf("2: omp_single, test #pragma omp single, arg is team size\n");
                printf("3: permute(colors, ball, plevel, verbose), test omp_task\n");
                }
            else
                {
                test = getdec(&p);                              // get the count
                skip(&p);

                switch(test)
                    {
                case 0: omp_hello(getdec(&p));      break;
                case 1: omp_for(getdec(&p));        break;
                case 2: omp_single(getdec(&p));     break;
                case 3:
                    int colors = getdec(&p);
                    skip(&p);
                    int balls = getdec(&p);
                    skip(&p);
                    int plevel = *p?getdec(&p):32;
                    skip(&p);
                    int v = *p?getdec(&p):0;
                    permute(colors, balls, plevel, v);
                    break;
                    }
                }
            }

//              //                              //
        HELP(  "v <type> <num>                  set verbosity level")
        else if(buf[0]=='v' && buf[1]==' ')
            {
            extern int omp_verbose;
            extern int temp_verbose;

            if(*p == 'o')
                {
                skip(&p);
                if(isdigit(*p))
                    {
                    omp_verbose = getdec(&p);
                    }
                }
            else if(*p == 't')
                {
                skip(&p);
                if(isdigit(*p))
                    {
                    temp_verbose = getdec(&p);
                    }
                }
            }

//              //                              //
        HELP(  "stk                             dump stacks")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='k')
            {
            for(int i=0; i<GOMP_MAX_NUM_THREADS; i++)
                {
                extern const char *thread_names[];
                printf("\%s stack, %d bytes:\n", thread_names[i], omp_threads[i].stack_high - omp_threads[i].stack_low);
                dump(omp_threads[i].stack_low, omp_threads[i].stack_high - omp_threads[i].stack_low);
                }
            }

//              //                              //
        HELP(  "mem                             display free memory")
        else if(buf[0]=='m' && buf[1]=='e' && buf[2]=='m')
            {
            extern void mem();
            mem();
            }

//              //                              //
        HELP(  "spi <command> {<value>}         SPI2 tests")
        else if(buf[0]=='s' && buf[1]=='p' && buf[2]=='i')
            {
            if(*p=='x')
                {
                skip(&p);
                uint32_t txdata = gethex(&p);
                uint8_t rxdata;

                // HAL_SPI_Transmit(&hspi2, (const uint8_t *)&value, 1, HAL_MAX_DELAY); // Transmit data
                HAL_SPI_TransmitReceive(&hspi2, (const uint8_t *)&txdata, &rxdata, 1, HAL_MAX_DELAY); // Transmit and receive data
                printf("rx = %02x\n", rxdata);
                }
            else if(*p=='c')
                {
                for(int i=0; i<256; i++)
                    {
                    HAL_SPI_Transmit(&hspi2, (const uint8_t *)&i, 1, HAL_MAX_DELAY); // Transmit data
                    HAL_Delay(100);
                    }
                }
            }


//              //                              //
        HELP(  "q                               QSPI tests")
        else if(buf[0]=='q' && buf[1]==' ')
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



//              //                              //
        HELP(  "ff                              FATFS tests")
        else if(buf[0]=='f' && buf[1]=='f')
            {
            if(p[0] == 'f')
                {
                if (f_mkfs("0:", FM_FAT|FM_SFD, 4096, (uint8_t *)&qbuf, 512) != FR_OK)
                    {
                    printf("Filesystem format failed on SPI-NOR\n");
                    }
                else
                    {
                    printf("Filesystem formatted successfully on SPI-NOR\n");

                    if(f_mount(&FatFs, "0:", 1) != FR_OK)
                        {
                        printf("FATFS mount error on SPI-NOR\n");
                        }
                    else
                        {
                        printf("FATFS mount OK on SPI-NOR\n");
                        }
                    }
                }
            else if(p[0] == 'm')
                {
                if(f_mount(&FatFs, "0:", 1) != FR_OK)
                    {
                    printf("FATFS mount error on SPI-NOR\n");
                    }
                else
                    {
                    printf("FATFS mount OK on SPI-NOR\n");
                    }
                }
            else if(p[0] == 'w')
                {

                // Create and write to a file
                if(f_open(&fil, "0:hello.txt", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
                    {
                    UINT bw; // Bytes written

                    f_write(&fil, "Hello, FATFS!", 13, &bw);
                    f_close(&fil);
                    }
                }
            else if(p[0] == 'r')
                {
                skip(&p);

                // Read from the file
                if(f_open(&fil, p, FA_READ) == FR_OK)
                    {
                    unsigned br; // Bytes read
                    FRESULT res;

                    do
                        {
                        res = f_read(&fil, qbuf, sizeof(qbuf), &br);
                        if(res == FR_OK)
                            {
                            dump(qbuf, br);
                            }
                        }
                    while(res == FR_OK
                       && br == sizeof(qbuf)
                       && !ControlC);

                    printf("res = %d\n", res);

                    f_close(&fil);
                    }
                else
                    {
                    printf("file %s could not be opened\n", p);
                    }
                }
            else if(p[0] == 'l' && p[1] == 's')
                {
                skip(&p);

                FRESULT res;
                DIR dir;
                FILINFO fno;

                // Open the directory
                res = f_opendir(&dir, p); /* Open the directory */
                if (res == FR_OK)
                    {
                    while (1)
                        {
                        res = f_readdir(&dir, &fno);                    /* Read a directory item */
                        if (res != FR_OK || fno.fname[0] == 0) break;   /* Break on error or end of dir */
                        if (fno.fattrib & AM_DIR)
                            {                     /* It is a directory */
                            printf("  <DIR>  %s\n", fno.fname);
                            }
                        else
                            {                                        /* It is a file */
                            printf("  %8lu  %s\n", fno.fsize, fno.fname);
                            }
                        }
                    f_closedir(&dir);
                    }
                else
                    {
                    printf("Failed to open directory: %d\n", res);
                    }
                }
            else printf("unrecognized subcommand\n");
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
            }


        // print the help screen
        else if(buf[0]=='?')
            {
            }

        // else I dunno what you want
        else printf("illegal command\n");
        }

    }


