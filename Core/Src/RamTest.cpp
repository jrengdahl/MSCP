// a simplistic RAM test

// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

// This uses the algorithm I developed in 1978 for a
// 4 MHz Z-80 with 2 Kbytes of RAM. It may not be the best
// RAM test, but it's small and fast enough for very
// small systems.


#include <ContextFIFO.hpp>
#include <stdio.h>
#include "cyccnt.hpp"
#include "local.h"

extern bool ControlC;
extern void commas(uint32_t x);

void RamTest(uint8_t *addr, unsigned size, unsigned repeat, unsigned limit)
    {
    uint8_t x = 0;              // expected value
    uint32_t ticks = 0;
    unsigned errors = 0;

    for(unsigned r=0; r<repeat && errors<limit && !ControlC; r++)
        {
        Elapsed();

        for(unsigned i=0; i<size; i++)addr[i] = 0;      // clear RAM

        x = 0;                                          // init expected value
        for(unsigned j=0; j<5 && errors<limit && !ControlC; j++)  // test for broken address and data lines
            {
            x+= 51;
            for(unsigned i=0; i<size; i++)               // add 51 to each byte in the test area
                {
                addr[i] += 51;

                // TEMPORARILY UNCOMMENT THIS TO VERIFY THAT THE RAM TEST WORKS
                // if(r==5 && j==3 && i==512)addr[i] += 1;
                }


            for(unsigned i=0; i<size && errors<limit && !ControlC; i++) // verify that each byte has the expected value
                {
                if(addr[i] != x)
                    {
                    ++errors;
                    printf("RAM failed at %08zx, expected %02x found %02x\n", (uintptr_t)&addr[i], x, addr[i]);
                    dump(&addr[(i-32)&-16], 80);
                    addr[i] = x;
                    }
                }
            }

        ticks = Elapsed();
        commas(ticks);
        printf(" microseconds, %u errors\n", errors);

        yield();
        }
    }
