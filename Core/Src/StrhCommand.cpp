#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"


void StrhCommand(char *p)
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

