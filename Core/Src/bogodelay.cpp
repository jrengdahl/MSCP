#include <stdint.h>
#include "cmsis.h"

// This routine delays the number of CPU clocks given as the argument
// This version is for the Cortex M33 which loops in three clocks

// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

__OPTIMIZE("O2")
void bogodelay(uint32_t x)
    {
    __asm__ __volatile__(
   "0:   subs   %[x], 3         \n\t"
        "bcs    0b              \n"
            :
    : [x]"r"(x)
    :);
    }

__OPTIMIZE("O2")
void bogodelay(unsigned x)
    {
    bogodelay((uint32_t)x);
    }

__OPTIMIZE("O2")
void bogodelay(int x)
    {
    bogodelay((uint32_t)x);
    }

__OPTIMIZE("O2")
void bogodelay(uint64_t x)
    {
    union
        {
        struct
            {
            uint32_t lo;
            uint32_t hi;
            };
        uint64_t all;
        }tmp;

    tmp.lo = 0;
    tmp.hi = 0;
    tmp.all = x;

    do
        {
        __asm__ __volatile__(
        "0:   subs   %[x], 3         \n\t"
             "bcs    0b              \n"
        : [x]"+r"(tmp.lo)
        :
        :);
        }
    while(tmp.hi-- > 0);
    }
