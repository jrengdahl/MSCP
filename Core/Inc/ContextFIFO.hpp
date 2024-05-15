// threadFIFO
//
// A threadFIFO is a subclass of FIFO. The data type stored in the FIFO are Threads.
// The inherited member functions add and take are not much use.
// The subclass member functions, suspend and resume, enable multiple threads to suspend at
// a threadFIFO. When a threadFIFO is resumed the oldest suspended thread is resumed.
// One notable use of a threadFIFO is the DeferFIFO, which is used to support yield and
// timesharing among multiple threads.

// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file




#ifndef CONTEXTFIFO_HPP
#define CONTEXTFIFO_HPP

#include <context.hpp>
#include "cmsis.h"
#include "FIFO.hpp"

static const unsigned THREAD_FIFO_DEPTH = 15;    // must be a power of 2, minus 1, if this is changed, the inline ASM must be updated

class ContextFIFO : public FIFO<Context *, THREAD_FIFO_DEPTH>
    {
    public:

    void suspend();
    void suspend_switch();
    bool resume();
    void resume_switch();
    };


extern ContextFIFO DeferFIFO;


// suspend the current thread at the DeferFIFO.
// It will be resumed later by the background thread.
// If the DeferFIFO is full yield will simply return immediately without yielding.

static inline void yield()
    {
    DeferFIFO.suspend();
    }


// Called by the backgroud thread to resume any supended threads in the DeferFIFO.
// If the DeferFIFO is empty this routine does nothing.

static inline void undefer()
    {
    DeferFIFO.resume();
    }


#endif // CONTEXTFIFO_HPP

