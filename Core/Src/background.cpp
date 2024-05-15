// background.cpp
// This is the entry point and background loop of the application.
// It is called at powerup from the ST Micro supplied HAL after it initilizes the hardware.

// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <omp.h>
#include "context.hpp"
#include "Port.hpp"
#include "ContextFIFO.hpp"
#include "serial.h"
#include "main.h"
#include "usbd_cdc_if.h"
#include "cyccnt.hpp"
#include "libgomp.hpp"
#include "boundaries.h"

// The DeferFIFO used by yield, for rudimentary time-slicing.
// Note that the only form of "time-slicing" occurs when a thread
// voluntarily calls "yield" to temporarily give up the CPU
// to other threads. A thread resumed by an ISR will run at interrupt
// level until it calls yield, so in some cases, that thread may call
// yield shortly after the call to suspend.

ContextFIFO DeferFIFO;

extern Port txPort;                              // ports for use by the console (serial or USB VCP)
extern Port rxPort;

extern uint32_t interp(uintptr_t);               // the command line interpreter thread

uint32_t LastTimeStamp = 0;

// This exists as a central place to put a breakpoint when certain conditions are encountered.
// To use, put a call to foo when the error condition occurs. 
int dummy = 0;
void foo()
    {
    ++dummy;
    }


// constants to enable the FPU at powerup
#define CPACR (*(uint32_t volatile *)0xE000ED88)
#define CPACR_VFPEN 0x00F00000


extern "C"
void background()                                       // powerup init and background loop
    {
    ///////////////////////////
    // powerup initialization
    ///////////////////////////

    // at powerup the stack pointer points to the end of RAM
    // the first thing that must be done is to switch to the stack defined by the linker script

    // clear the background stack
    for(uint32_t *p = &_stack_start; p<&_stack_end; p++)*p = 0;

    // switch the background thread to the background stack
    __asm__ __volatile__(
"   ldr sp, =_stack_end"
    );

    InitCyccnt();                                       // enable the cycle counter

    CPACR |= CPACR_VFPEN;                               // enable the floating point coprocessor

    libgomp_init();                                     // init the OpenMP threading system, including setting background as thread 0

    #pragma omp parallel num_threads(2)
    if(omp_get_thread_num() == 0)                       // thread 0 runs this:
        {
        while(1)                                        // run the background polling loop
            {
            gomp_poll_threads();                        // wake any OpenMP threads that have work to do

            if(DeferFIFO)                               // if anything on the DeferFIFO
                {
                undefer();                              // wake any threads that called yield
                }
            }
        }
    else if(omp_get_thread_num() == 1)                  // and thread 1 runs this:
        {
        interp(0);                                      // run the command line interpreter
        }

    // neither of the above two threads terminate, so the parallel never ends, and we should never get here
    assert(false==true);                                // Woe to those who call evil good, and good evil
    }

