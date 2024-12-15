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
extern void summary(unsigned char *, unsigned, unsigned, int);
extern void RamTest(uint8_t *addr, unsigned size, unsigned repeat, unsigned limit);
extern "C" void SystemClock_PLL_Config(unsigned);
extern "C" void SystemClock_HSI_Config(void);
extern char InterpStack[2048];
extern omp_thread omp_threads[GOMP_MAX_NUM_THREADS];
bool waiting_for_command = false;

static FATFS FatFs[3];
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
                case 0:  value = HAL_GPIO_ReadPin(GPIONAME( FPGA_IRQ)); break;
                default: value = HAL_GPIO_ReadPin(GPIONAME( FPGA_IRQ)); break;
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
        else if(buf[0]=='l' && buf[1]=='d' && buf[2]=='r' && buf[3]==' ')
            {
            uintptr_t addr = gethex(&p);            // get the address
            uint32_t value;

            skip(&p);

            int size = 1;
            if(isdigit(*p))size = getdec(&p);

            for(int i=0; i++<size; addr += 4)
                {
                SCB->DCCIMVAC = addr;                   // clean and invalidate the cache so we access the actual memory
                __asm__ __volatile__(
                    "dsb SY            \n\t"
                    "ldr %0, [%1]"                  // load a word from the given address
                    : "=r"(value)
                    : "r"(addr)
                    : );

                // print the result
                if(i==size) printf("%08lx\n", value);
                else        printf("%08lx ", value);
                }
            }

//              //                              //
        HELP(  "ldrh <addr> {size {o}}          load a halfword from address using ldrh")
        else if(buf[0]=='l' && buf[1]=='d' && buf[2]=='r' && buf[3]=='h')
            {
            uintptr_t addr = gethex(&p);            // get the address
            uint32_t value;

            skip(&p);

            int size = 1;
            if(isdigit(*p))
                {
                size = getdec(&p);
                skip(&p);
                }


            for(int i=0; i++<size; addr += 2)
                {
                SCB->DCCIMVAC = addr;
                __asm__ __volatile__(
                    "dsb SY            \n\t"
                    "ldrh %0, [%1]"     // load a word from the given address
                    : "=r"(value)
                    : "r"(addr)
                    : );

                if(*p=='o')printf("%06o%c", (int)value, i==size?'\n':' ');
                else       printf("%04lx%c",     value, i==size?'\n':' ');                // print the result
                }
            }

//              //                              //
        HELP(  "ldrb <addr>                     load a byte from address using ldrb")
        else if(buf[0]=='l' && buf[1]=='d' && buf[2]=='r' && buf[3]=='b')
            {
            uintptr_t addr = gethex(&p);            // get the address
            uint32_t value;

            SCB->DCCIMVAC = addr;

            __asm__ __volatile__(
                "dsb SY            \n\t"
                "ldrb %0, [%1]"     // load a word from the given address
             : "=r"(value)
             : "r"(addr)
             : );

            printf("%02lx\n", value);                 // print the result
            }

//              //                              //
        HELP(  "str <value> <addr>              store word to address using str")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='r' && buf[3]==' ')
            {
            uint32_t value = gethex(&p);
            skip(&p);
            uintptr_t addr = gethex(&p);            // get the address

            __asm__ __volatile__(
                    "str %0, [%1]       \n\t"
                    "dsb SY"
            :
            : "r"(value), "r"(addr)
            : );

            SCB->DCCMVAC = addr;
            }

//              //                              //
        HELP(  "strh <value> <addr>             store a halfword to address using strh")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='r' && buf[3]=='h')
            {
            uint32_t value = gethex(&p);
            skip(&p);
            uintptr_t addr = gethex(&p);            // get the address

            __asm__ __volatile__(
                "strh %0, [%1]       \n\t"
                "dsb SY"
            :
            : "r"(value), "r"(addr)
            : );

            SCB->DCCMVAC = addr;
            }

//              //                              //
        HELP(  "strb <value> <addr>             store a byte to address using strb")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='r' && buf[3]=='b')
            {
            uint32_t value = gethex(&p);
            skip(&p);
            uintptr_t addr = gethex(&p);            // get the address

            __asm__ __volatile__(
                "strb %0, [%1]       \n\t"
                "dsb SY"
            :
            : "r"(value), "r"(addr)
            : );

            SCB->DCCMVAC = addr;
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
        else if(buf[0]=='v' && (buf[1]==' ' || buf[1]==0))
            {
            extern int omp_verbose;
            extern int temp_verbose;

            if(*p == 'o')
                {
                skip(&p);
                omp_verbose = getdec(&p);
                }
            else if(*p == 't')
                {
                skip(&p);
                temp_verbose = getdec(&p);
                }

            printf("omp_verbose = %d\n", omp_verbose);
            printf("temp_verbose = %d\n", temp_verbose);
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
        HELP(  "fmt <blocks> <path>             FATFS format a FATFS drive")
        else if(buf[0]=='f' && buf[1]=='m' && buf[2]=='t')
            {
            int blocks = getdec(&p);
            skip(&p);

            if (f_mkfs(p, FM_FAT|FM_SFD, blocks, (uint8_t *)&qbuf, 512) != FR_OK)
                {
                printf("Filesystem format failed\n");
                }
            else
                {
                printf("Filesystem formatted successfully\n");
                }
            }

//              //                              //
        HELP(  "mnt <path>                      mount a FATFS volume")
        else if(buf[0]=='m' && buf[1]=='n' && buf[2]=='t')
            {
            int drv = *p-'0';

            if(drv<0 || drv > 2)
                {
                printf("invalid drive number\n");
                continue;
                }

            if(f_mount(&FatFs[drv], p, 1) != FR_OK)
                {
                printf("FATFS mount error\n");
                }
            else
                {
                printf("FATFS mount OK\n");
                }
            }

//              //                              //
        HELP(  "cat <path>                      copy a file to the console")
        else if(buf[0]=='c' && buf[1]=='a' && buf[2]=='t')
            {
            if(f_open(&fil, p, FA_READ) == FR_OK)
                {
                unsigned br; // Bytes read
                FRESULT res;

                do
                    {
                    res = f_read(&fil, qbuf, 512, &br);
                    if(res == FR_OK)
                        {
                        _write(1, (const char *)&qbuf, br);
                        }
                    }
                while(res == FR_OK
                   && br == 512
                   && !ControlC);

                if(res)
                    {
                    printf("res = %d\n", res);
                    }

                f_close(&fil);
                }
            else
                {
                printf("file %s could not be opened\n", p);
                }
            }

//              //                              //
        HELP(  "dump <path>                     dump a file to the console")
        else if(buf[0]=='d' && buf[1]=='u' && buf[2]=='m')
            {
            // Read from the file
             if(f_open(&fil, p, FA_READ) != FR_OK)
                 {
                 printf("file %s could not be opened\n", p);
                 continue;
                 }

             unsigned br; // Bytes read
             FRESULT res;

             do
                 {
                 res = f_read(&fil, qbuf, 512, &br);
                 if(res == FR_OK)
                     {
                     dump(qbuf, br);
                     }
                 }
             while(res == FR_OK
                && br == 512
                && !ControlC);

             printf("res = %d\n", res);

             f_close(&fil);
             }


//              //                              //
        HELP(  "ls <path>                       list the contents of a directory")
        else if(buf[0]=='l' && buf[1]=='s')
            {
            FRESULT res;
            DIR dir;
            FILINFO fno;
            FATFS *fs;
            DWORD fre_clust, fre_sect, tot_sect;

            // Open the directory
            res = f_opendir(&dir, p); /* Open the directory */
            if (res != FR_OK)
                {
                printf("Failed to open directory: %d\n", res);
                continue;
                }

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

            f_getfree(p, &fre_clust, &fs);
            tot_sect = (fs->n_fatent - 2) * fs->csize;
            fre_sect = fre_clust * fs->csize;

            if(tot_sect/2 <100*1024)
                {
                printf("volume size: %lu KB\n", tot_sect / 2);
                printf("free space:  %lu KB\n", fre_sect / 2);
                }
            else if(tot_sect/2 <100*1024*1024)
                {
                printf("volume size: %lu MB\n", tot_sect / (2*1024));
                printf("free space:  %lu MB\n", fre_sect / (2*1024));
                }
            else
                {
                printf("volume size: %lu GB\n", tot_sect / (2*1024*1024));
                printf("free space:  %lu GB\n", fre_sect / (2*1024*1024));
                }
            }

//              //                              //
        HELP(  "cp <source path> <dest path>    copy a file")
        else if(buf[0]=='c' && buf[1]=='p')
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
                continue;
                }

            // Open or create the destination file
            res = f_open(&dst_file, dst_path, FA_WRITE | FA_CREATE_ALWAYS);
            if (res != FR_OK)
                {
                printf("Failed to open destination file: %s\n", dst_path);
                f_close(&src_file);
                continue;
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
                    continue;
                    }
                }

            // Close both files
            f_close(&src_file);
            f_close(&dst_file);

            // Check if the loop exited due to an error
            if (res != FR_OK)
                {
                printf("Failed to copy file: %s to %s\n", src_path, dst_path);
                continue;
                }

            printf("File copied successfully: %s to %s\n", src_path, dst_path);
            }

//              //                              //
        HELP(  "diff <path1> <path2>            compare two files")
        else if(buf[0]=='d' && buf[1]=='i' && buf[2]=='f')
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
                continue;
                }

            // Open file 2
            res = f_open(&f2, path2, FA_READ);
            if (res != FR_OK)
                {
                printf("Failed to open file 2: %s\n", path2);
                f_close(&f1);
                continue;
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
            }



//              //                              //
        HELP(  "b <cmd> <args>                  Qbus menu")
        else if(buf[0]=='b' && buf[1]==' ')
            {
            if(p[0] == 'r' && p[1] == ' ')
                {
                uint16_t values[8];

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
                    skip(&p);
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
                uint16_t values[8];
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

                while((isxdigit(*p) || *p=='o') && count <8)
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

            else printf("unrecognized subcommand\n");
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


