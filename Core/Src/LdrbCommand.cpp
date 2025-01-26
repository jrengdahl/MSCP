#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"


void LdrbCommand(char *p)
    {
    uintptr_t addr = gethex(&p);            // get the address
    uint32_t value;

    skip(&p);

    int size = 1;
    if(isdigit(*p))
        {
        size = getdec(&p);
        }


    for(int i=0; i++<size; addr += 1)
        {
        SCB->DCCIMVAC = addr;              // clean and invalidate the cache so we access the actual memory
        __asm__ __volatile__(
            "dsb SY            \n\t"
            "ldrb %0, [%1]"                 // load a word from the given address
            : "=r"(value)
            : "r"(addr)
            : );

        if(*p=='o')printf("%03o%c", (int)value, i==size?'\n':' ');
        else       printf("%02lx%c",     value, i==size?'\n':' ');                // print the result
        }
    }
