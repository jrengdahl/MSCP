// threadFIFO
//
// A threadFIFO is a subclass of FIFO. The data type stored in the FIFO is pointer to Context.
// The inherited member functions add and take are not much use.
// The subclass member functions, suspend and resume, enable multiple threads to suspend at
// a threadFIFO. When a threadFIFO is resumed the oldest suspended thread is resumed.
// One notable use of a threadFIFO is the DeferFIFO, which is used to support yield and
// timesharing among multiple threads.

// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#include <context.hpp>
#include <ContextFIFO.hpp>
#include "cmsis.h"


__NOINLINE
__NAKED
void ContextFIFO::suspend()
    {
    __asm__ __volatile__(
    STORE_CONTEXT

    "   ldrd    r2, r3, [r0, #64]           \n"         // get nextin (r2) and nextout (r3)
    "   add     r1, r2, #1                  \n"         // increment nextin
    "   and     r1, #15                     \n"         // wrap if needed
    "   cmp     r1, r3                      \n"         // if updated nextin == nextout, the FIFO is full
    "   beq     0f                          \n"         // so go return false
    "   str     r1, [r0, #64]               \n"         // update nextin

    "   mov     r4, r9                      \n"         // unlink current thread from the ready chain
    "   ldr     r9, [r4, #40]               \n"         //
    "   str     r4, [r0, r2, lsl #2]        \n"         // save that thread in the FIFO
    );

    this->suspend_switch();

    __asm__ __volatile__(
    "0: mov     r0, #0                      \n"
    "   msr     primask, ip                 \n"         // and interrupt state interrupt state
    "   bx      lr                          \n"
    );
    }

__NOINLINE
__NAKED
void ContextFIFO::suspend_switch()
    {
    __asm__ __volatile__(
    LOAD_CONTEXT
    "   mov     r0, #1                      \n"         // return true
    "   bx      lr                          \n"         //
    );
    }


__NOINLINE
__NAKED
bool ContextFIFO::resume()
    {
    __asm__ __volatile__(
    STORE_CONTEXT

    "   ldrd    r2, r3, [r0, #64]           \n"         // get nextin (r2) and nextout (r3)
    "   cmp     r2, r3                      \n"         // if equal, the FIFO is empty
    "   beq     0f                          \n"         // so go return false
    "   add     r2, r3, #1                  \n"         // increment nextout
    "   and     r2, #15                     \n"         // wrap if needed
    "   str     r2, [r0, #68]               \n"         // update nextout

    "   ldr     r4, [r0, r3, lsl #2]        \n"         // get the next thread from FIFO[nextout]
    "   str     r9, [r4, #40]               \n"         // link the new thread as the head of the ready chain
    "   mov     r9, r4                      \n"         //
    );

    this->resume_switch();

    __asm__ __volatile__(
    "0: mov     r0, #0                      \n"         // since the FIFO is empty, return false
    "   msr     primask, ip                 \n"         // restore caller's interrupt state
    "   bx      lr                          \n"
    );

    return false;                                       // fake return to keep compiler happy
    }

__NOINLINE
__NAKED
void ContextFIFO::resume_switch()
    {
    __asm__ __volatile__(
    LOAD_CONTEXT
    "   bx      lr                          \n"         //
    );
    }

