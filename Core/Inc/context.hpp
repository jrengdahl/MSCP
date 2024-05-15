// Context.hpp

// A simple but very fast threading system for bare metal ARM processors.
//
// Copyright (c) 2023-2024 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file
//
//
// A thread is represented by its non-volatile registers.
// The r9 register points to the current Context object, where the running Context was loaded from,
// and where it will besaved when it is n longer running.
// Contexts can be chained via their "next" pointer. Typically r9 points to a chain of Contexts.
// The last Context in this chain is the background, which MUST NEVER suspend itself.

// A thread can be in one of three states;
// -- running: it currently is running on the CPU and its state is contained in the CPU registers.
// -- suspended: the thread is saved in a Context object, and is waiting to be resumed. A thread in this
//    state is typically waiting for an event, or waiting to be started as part of a team of OpenMP threads.
// -- pending: a thread that has resumed another thread has its state pushed on the pending context stack.
//    A thread in this state is waiting for a running thread to suspend itself.
//
// The threading system is non-preemptive. That is, a thread never leaves the running state other than
// by its own action (suspend itself, resume another thread, or terminate).
//
// A thread consists of:
// -- a subroutine. Multiple threads can be running the same code.
// -- a context. The context consists of the non-volatile registers (r4-r11, sp, lr, on some systems
//    excluding fixed registers such as r9). The volatile registers r0-r3 and ip are not members of a
//    saved context because this threading system is non-preemptive -- a thread state change is only
//    initiated by a subroutine call, and volatile registers do not need to be saved across a call.
//    Furthermore, the pc is saved in lr by a subroutine call, thus the pc is not explicitly saved.
// -- a stack, pointed to by sp. The Context functions that receive a stack as an argument are passed a
//    reference to an array of char. In this way the templated functions can determine the stack size,
//    whereas if a pointer were passed the size information would be lost.
//
// Multiple threads can be running virtually concurrently on each CPU. In actuality, virtually
// concurrent threads run serially -- whenever the running thread enters the suspended or pending state,
// another thread takes over, until it too suspends or pends. However, on a multi-core system multiple
// threads can actually run concurrently.
//
// Memory available to a thread is either local (stack) or globally shared with all other threads.
// This implementation does not provide thread-private static memory. On single and multi-core systems
// the threads share the same address space. It is assumed that the address space is coherent (all
// threads see the same content for all addresses, regardless of cacheing or which core they are on).
//
// This implementation stores the thread chain pointer (points to a linked list of pending threads, not to
// be confused with the normal sp) in r9. This removes one more register from the pool allocatable by the
// compiler, but has the following significant benefit:
// -- the thread switching code becomes much faster and simpler.
// -- the compiler requires fewer registers to implement a context switch, thus the threadFIFO algorithm
//    can be implemented using only the five volatile registers. This greatly simplifies the threadFIFO
//    suspend and resume routines.
// -- Threads can access variables in the current OpenMP omp_thread object with one ldr or str instruction.
// 
// Non-preemptive thread switch is very fast -- A thread switch takes 300 ns on a 100 MHz Cortex-M33.
// Obviously, a fully preemptive, prioritized, time-sliced RTOS has its advantages, but where sheer
// performance outweighs these advantages, a non-preemtive system such as this is necessary. The other
// alternative would be to implement the firmware as a complex state machine, but 45 years experience coding
// real-time firmware has convinced me that a set of cooperating threads are much easier to design, implement,
// and debug than a huge single-threaded state machine.



#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>
#include "cmsis.h"

// Save Context
// Saves the interrupt state in ip.
// Disables interrupts, this is important while the stacks are being swapped.
// Save the current running thread in its Context object, which is pointed to by r9.
// The saved registers include the interrupt state and the SP.
#define STORE_CONTEXT                           \
"   mrs     ip, primask                 \n"     \
"   cpsid   i                           \n"     \
"   stm     r9, {r4-r8, r10-ip, lr}     \n"     \
"   str     sp, [r9, #36]               \n"


// Load Context
// Loads a thread from its context object, which is pointed to by r9.
// The loaded registers include the SP and the interrupt state in ip.
// Restores the saved interrupt state.
#define LOAD_CONTEXT                            \
"   ldm     r9, {r4-r8, r10-ip, lr}     \n"     \
"   ldr     sp, [r9, #36]               \n"     \
"   msr     primask, ip                 \n"




// the code of a thread
typedef uint32_t THREADFN(uintptr_t arg);

// the context of a thread
class Context
    {
    private:

    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
                        // skip over r9 which holds the current thread pointer
    uint32_t r10;
    uint32_t r11;
    uint32_t ip;        // holds the interrupt state
    uint32_t lr;        // holds the execution address

    uint32_t sp;        // sp, which can't be accessed by the ldm/stm instructions

    Context *next;      // contextchain pointer, this continues the LIFO chain pointed to by r9


    public:

    // Constructor
    Context() : r4(0), r5(0), r6(0), r7(0), r8(0), r10(0), r11(0), ip(0), lr(0), sp(0), next(0)
        {
        }

    void static suspend();                  // suspend self until resumed
    void static suspend_switch();           // an entry point to resume after the condition is tested (for Ozone RTOS awareness)

    void resume();                          // resume a suspended thread
    void resume_switch();                   // an entry point to resume after the condition is tested (for Ozone RTOS awareness)

    void start(THREADFN *fn, char *sp, uintptr_t arg); // an internal function to start a new thread
    void start_switch1();                   // an entry point to resume after the condition is tested (for Ozone RTOS awareness)
    void start_switch2();                   // an entry point to resume after the condition is tested (for Ozone RTOS awareness)


    // spawn a new thread
    template<unsigned N>
    __FORCEINLINE void spawn(THREADFN *fn,  // code
               char (&stack)[N],            // reference to the stack. The template can determine the stack size from the reference.
               uintptr_t arg = 0)
        {
        start(fn, &stack[N-8], arg);        // reserve two words at stack top, then call the start function, passing the initial sp
        }

    static void init();                     // powerup init of the thread system (inits the thread stack pointer in r9)

    // Thread::done -- test whether a thread is running
    // arg: the thread's stack
    // return: true if the thread is done, false if it is still running.
    // The result is only valid after the thread has been started (and you are waiting for it to complete).
    template<unsigned N>
    static inline bool done(char (&stack)[N])
        {
        return stack[N-4] == 1;
        }


    // get a pointer to the current context
    static Context *pointer()
        {
        Context *r9;

        __asm__ __volatile__(
                "   mov %[r9], r9"           // fetch r9, probably into r0
                : [r9]"=r"(r9)
                :
                :
                );
        return r9;
        }

    };





#endif // CONTEXT_H
