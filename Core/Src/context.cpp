// Context.cpp
// Implementation of the Context class.

// Copyright (c) 2023-2024 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

// This file deals with the Context only. The OpenMP thread
// includes a Context at the beginning of the thread object.


// Loading or storing the sp using ldr or str generates the warning:
// "This instruction may be unpredictable if executed on M-profile cores with interrupts enabled."
// Since the instructions below which trigger this warning are guaranteed to be executed with
// interrupts off, it is safe to ignore the warning. You can add the option:
// "-Wa,--no-warn", TO THIS FILE ONLY
// to suppress the assembler warning. I recommend that you remove that option if you make any
// changes to this module, until you have tested the changes.

// NOTE NOTE NOTE !!!
// NOTE NOTE NOTE !!!
// NOTE NOTE NOTE !!!

// This file must be compiled with the option
// -fno-toplevel-reorder
// This is to ensure that "suspend" drops through to "suspend-switch", and
// "resume" drops through to "resume-switch". This can be done by right
// clicking THIS FILE in the Eclipse Project Explorer, selecting
// Properties -> C/C++ Build -> Settings -> Tool Settings -> Miscellaneous
// and adding "-fno-toplevel-reorder" to the list of options.

// NOTE NOTE NOTE !!!
// NOTE NOTE NOTE !!!
// NOTE NOTE NOTE !!!



// The suspend and resume routines have some additional complications to support RTOS-awareness
// in the Ozone debugger. Ozone requires there to be a subroutine call that causes a context switch.
// When Ozone sees this subroutine being called it knows not to nest the Timeline display.
// Each context-switching subroutines is identified to Ozone via the (e.g. for "suspend_switch")
// OS.AddContextSwitchSymbol("Context::suspend_switch"); line in the project file (.jdebug).

// In order to accomodate this, initially a dummy call was introduced in the middle of each context
// switching routine. Later, I figured out that the the call instruction itself was unnecessary, but
// the entry point had to exist. For each routine e.g. "function", the name of the context switching
// point is named "function_switch". In the case of start, which contains two context switches,
// the dummy entry points are named "start_switch1" and "start_switch2". It is possible to comment
// out the actual call to function_switch and let function drop through to function_switch. In order
// to accomplish this, three things are needed:
// -- "function_switch" must immediatley follow "function"
// -- this file must be compiled with the option  -fno-toplevel-reorder so that the linker does not
//    move "function_switch" relative to "function".
// -- every function that is dropped into must have a dummy call somewhere, so that the optimizer
//    does not delete it. These dummy calls have been placed after the "bx lr" in "start".
// Note that the switch routines in start have not been optimized, since start is called infrequently.


#include <context.hpp>
#include <stdint.h>
#include "cmsis.h"



// Suspend the current thread into its Context object pointed to by r9,
// pop the next thread from the ready chain into the
// register set, and resume running it.
__NOINLINE
__NAKED
void Context::suspend()
    {
    __asm__ __volatile__(
    STORE_CONTEXT                                   // save the CPU registers to the current context struct
"   ldr     r9, [r9, #40]               \n"         // load r9 from the "next" pointer of the old thread
    );

//    suspend_switch();                             // call the next step. This is needed for Ozone's RTOS awareness.
//                                                  // the preceding line is commented out for drop through optimization
    }

__NOINLINE
__NAKED
void Context::suspend_switch()
    {
    __asm__ __volatile__(
    LOAD_CONTEXT                                    // load the new thread into the CPU registers
"   bx      lr                          \n"
    );                                              // return to the un-pending thread right after its call to resume
    }


// Push the current context onto the ready chain, and
// resume the context pointed to by r0
__NOINLINE
__NAKED
void Context::resume()
    {
    __asm__ __volatile__(
    STORE_CONTEXT                                   // save the old context to the current context object
"   str     r9, [r0, #40]               \n"         // save r9 to the "next" pointer of the new context
"   mov     r9, r0                      \n"         // the new context becomes the head of the ready chain
    );

//    this->resume_switch();                        // call the next step. This is needed for Ozone's RTOS awareness.
//                                                  // the preceding line is commented out for drop through optimization
    }

__NOINLINE
__NAKED
void Context::resume_switch()
    {
    __asm__ __volatile__(
    LOAD_CONTEXT                                    // load the new context into the CPU registers
"   bx      lr                          \n"
    );
    }


// Start a new thread, given its code and initial stack pointer.
// args: fn:    a pointer to a subroutine which will be run as a thread
//       newsp: the initial stack pointer of the new thread

// The newsp must point to the second word from the high end of the stack.
// The word pointed to by newsp is reserved to pass the thread's return value
// during the rendezvous at the completion of the thread. The second word
// past newsp is used for the thread's "done" flag.
//
// The thread which calls spawn is pushed to the pending stack as if it
// had called "resume". The calling thread will be unpended the first time
// the new thread suspends itself. The calling thread will continue
// executing after the call to "start". It will most likely continue executing
// before the new thread terminates.
//
// When the new thread terminates by returning from its subroutine, it returns
// to this code, which will run the next thread from the pending stack. This
// is not necessarily the thread which created the terminated thread.

__NOINLINE
__NAKED
void Context::start(THREADFN *fn, char *newsp, uintptr_t arg)
    {
    (void)fn;
    (void)newsp;
    (void)arg;

    __asm__ __volatile__(
    STORE_CONTEXT                                   // save the calling context into its thread object
"   str     r9, [r0, #40]               \n"         // point the new object to the rest of the ready chain
"   mov     r9, r0                      \n"         // the new context becomes the head of the ready chain
    );

    start_switch1();                                // call the next step. This is required for Ozone's RTOS awareness.
    }

__NOINLINE
__NAKED
void Context::start_switch1()
    {
    __asm__ __volatile__(
"   mov     sp, r2                      \n"         // setup the new thread's stack
"   mov     r2, #0                      \n"         //
"   str     r2, [sp, #0]                \n"         // clear the return value
"   str     r2, [sp, #4]                \n"         // and the "done" flag
"   mov     r0, r3                      \n"         // pass the arg in r0
"   msr     primask, ip                 \n"         // restore the new thread's interrupt state
"   blx     r1                          \n"         // start executing the code of the new thread

// when the new thread terminates, it returns here...
// it is assumed that the sp at this point has its initial value (the thread's stack usage is balanced)

"   cpsid   i                           \n"         // disable interrupts
"   mov     r1, #1                      \n"         //
"   str     r0, [sp, #0]                \n"         // save the return value
"   str     r1, [sp, #4]                \n"         // set the "done" flag

"   mov     r3, r9                      \n"         // unlink current context from the ready chain
"   ldr     r9, [r3, #40]               \n"
    );

    start_switch2();                                // call the next step. This is required for Ozone's RTOS awareness.
    }

__NOINLINE
__NAKED
void Context::start_switch2()
    {
    __asm__ __volatile__(
    LOAD_CONTEXT
"   bx      lr                          \n"
    );

// This code is never reached.
// Insert dummy calls here to prevent these function from being optimized away.
// These functions are never called, they are entered by drop-through, for optimization
    suspend_switch();                               // call the next step. This is needed for Ozone's RTOS awareness.
    this->resume_switch();                          // call the next step. This is needed for Ozone's RTOS awareness.
    }


// results of optimization steps
// >t 5000000
// 13,708,501 microseconds
// 13,308,256 microseconds
// 12,908,006 microseconds
// 12,707,884 microseconds
// 12,507,722 microseconds
// nearly 10% improvemebt
