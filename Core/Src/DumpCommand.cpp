#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"


void DumpCommand(char *p)
    {
    unsigned *a;
    int s;

    a = (unsigned *)gethex(&p);                                         // get the first arg
    skip(&p);
    if(isxdigit(*p))s = gethex(&p);                                      // the second arg is the size
    else s = 4;

    dump(a, s);                                                         // do the dump
    }
