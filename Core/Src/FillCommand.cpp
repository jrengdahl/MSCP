#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"


void FillCommand(char *p)
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
