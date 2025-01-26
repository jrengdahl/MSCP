#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "boundaries.h"

extern void summary(unsigned char *, unsigned, unsigned, int);

void SumCommand(char *p)
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
