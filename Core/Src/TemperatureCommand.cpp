#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "omp.h"
#include "serial.h"
#include "ContextFIFO.hpp"


void TemperatureCommand(char *p)
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

#define in(x) ((x)/1000)
#define fr(x) ((x)/100 - ((x)/1000)*10)

    if(*p == 'r')
        {
        printf("raw temperature = %d\n", read_temperature_raw());
        return;
        }

    if(isdigit(*p))                                                 // while there is any data left on the command line
        {
        repeat = getdec(&p);                                        // get the count
        }

    while(repeat && !ControlC)
        {
        int tmp = read_temperature();
        int tmpF = (tmp*18 + 325)/10;

        if(first_time) avg = tmp;
        else           avg = (avg*(NAVG-1) + tmp)/NAVG;

        if(avg-last_avg > 500 || avg-last_avg > 500)
            {
            now = omp_get_wtime();
            printf("temperature: %3d.%01d C, %3d.%01d F, avg = %3d.%01d C, elapsed = %5.3f\n", in(tmp), fr(tmp), in(tmpF), fr(tmpF), in(avg), fr(avg), now-last_change);
            last_avg = avg;
            last_change = now;
            -- repeat;
            }

        first_time = false;

        yield();
        }
    }
