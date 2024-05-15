// Port.cpp

// A communication/synchronization port, sort of like Occam's

// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#include <stdint.h>
#include "cmsis.h"
#include <context.hpp>
#include <Port.hpp>


// Suspend the current context at the port
// pop the next thread from the pending stack into the
// register set, and resume running it.
__NOINLINE
__NAKED
void *Port::suspend()
    {
    __asm__ __volatile__(
    STORE_CONTEXT

"   mov     r3, r9                      \n"         // unlink current context from the ready chain
"   ldr     r9, [r3, #40]               \n"

"   ldr     r2, [r0]                    \n"         // link current context as head of Port chain
"   str     r2, [r3, #40]               \n"         // make the suspended context the new head of the ContextChain
"   str     r3, [r0]                    \n"         // save ContextChain pointer
    );

    this->suspend_switch();
    }

__NOINLINE
__NAKED
void Port::suspend_switch()
    {
    __asm__ __volatile__(
    LOAD_CONTEXT
"   mov     r0, #1                      \n"         // return true from resume of the new ready context
"   bx      lr                          \n"         // r0 will be non-zero on return to indicate that the corresponding resume succeeded
    );                                              // return to the un-pending thread right after its call to resume
    }


// Push the current thread onto the pending chain, and
// resume the first thread in a chain
__NOINLINE
__NAKED
bool Port::resume(void * thing)
    {
    (void)thing;

    __asm__ __volatile__(
    STORE_CONTEXT

"   ldr     r2, [r0]                    \n"         // look at the head of the chain
"   cbz     r2, 0f                      \n"         // go return false if sp is zero

"   ldr     r3, [r2, #40]               \n"         // unlink the new context from the ContextChain
"   str     r3, [r0]                    \n"         // 

"   str     r9, [r2, #40]               \n"         // link the current ready context chain to the new thread
"   mov     r9, r2                      \n"         // make the new context the running contet 

    );

    this->resume_switch();

    __asm__ __volatile__(
"0: mov     r0, #0                      \n"         // return false
"   msr     primask, ip                 \n"         // restore previous interrupt state
"   bx      lr                          \n"         //
    );

    return false;                                   // dummy return to keep the compiler happy
    }

__NOINLINE
__NAKED
void Port::resume_switch()
    {
    __asm__ __volatile__(
    LOAD_CONTEXT
"   mov     r0, r1                      \n"         // arg to resume gets returned from suspend
"   bx      lr                          \n"         // return to the new thread
    );
    }

