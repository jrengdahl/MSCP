#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"


void ModifyCommand(char *p)
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
