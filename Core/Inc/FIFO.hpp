////////////////////////////////////////////////////////////////////////////////
// FIFO.hpp
// A single-writer-single-reader lock-free/wait-free FIFO.
// Multi-thread and multi-core.
//
// This FIFO can be simultaneously written by one thread/core, and read by another.
// There can be only one writer and one reader. A long as this restriction is observed,
// the Add and Take operations are atomic. No special CPU instructions are used (i.e. locking).
// Interrupts do not have to be disabled.
//
// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file
//
///////////////////////////////////////////////////////////////////////////////


#ifndef FIFO_HPP
#define FIFO_HPP

#include "cmsis.h"


///////////////////////////////////////////////////////////////////////////////
// class Fifo
//
// template parameter T      - Type of data this FIFO will hold.
// template parameter N      - Number of entries this FIFO will hold.
//
/////////////////////////////////////////////////////////////////////////////

template<typename T, unsigned N>
class FIFO
    {
    protected:

    T Data[N+1];                // Data store (one more than capacity, since nextIn==nextOut is empty, but nextIn+1==nextOut is full).

    unsigned nextIn = 0;        // Index of next entry to be written
    unsigned nextOut = 0;       // Index of next entry to be read.



    public:

    ///////////////////////////////////////////////////////////////////////////////
    //  FIFO::operator bool
    //
    // returns true if FIFO has any content
    ///////////////////////////////////////////////////////////////////////////////

    inline operator bool() { return nextIn != nextOut; }


    ///////////////////////////////////////////////////////////////////////////////
    //  FIFO::add
    //
    // Adds an entry to the FIFO.
    //
    // input: value  The value to be added. <b>Must not be nil!</b>
    //
    // return bool
    //     true   The value was added to the FIFO.
    //     false  The FIFO is full, the value was not added.
    ///////////////////////////////////////////////////////////////////////////////

    bool add(T value)
        {
        unsigned curNextIn;                                                         // copy of nextIn
        unsigned newNextIn;                                                         // updated index

        curNextIn = nextIn;                                                         // get the index of the slot next to be written
        newNextIn = (curNextIn + 1) % (N + 1);                                      // calc index of next slot
        if(newNextIn == nextOut)                                                    // if pointers collide it's full, return false
            {
            return false;                                                           // cannot add because the FIFO is full
            }

        Data[curNextIn] = value;                                                    // store the data to the owned slot
        __COMPILER_BARRIER();
        nextIn = newNextIn;                                                         // save new index

        return true;                                                                // return success
        }


    /////////////////////////////////////////////////////////////////////////////
    //  FIFO::take
    //
    // Takes an entry from the FIFO.
    //
    // arg: reference to where to put the value taken
    // return: true if a value was taken, false if the FIFO was empty
    ////////////////////////////////////////////////////////////////////////////

    bool take(T &value)
        {
        unsigned curNextOut;                                                        // copy of nextIn

        curNextOut = nextOut;                                                       // get the current output index
        if(curNextOut == nextIn)                                                    // if FIFO is empty
            {
            return false;                                                           // return false, the FIFO was empty and no value was returned
            }

        value = Data[curNextOut];                                                   // get the value from the current slot
        COMPILER_BARRIER();
        nextOut = (curNextOut + 1) % (N + 1);                                       // calc index of next slot

        return true;                                                                // return true, a value was returned
        }
    };


#endif // FIFO_HPP
