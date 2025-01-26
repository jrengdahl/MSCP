#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"

extern void RamTest(uint8_t *addr, unsigned size, unsigned repeat, unsigned limit);

void RamTestCommand(char *p)
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
