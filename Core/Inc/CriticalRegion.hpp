// A safe disable/enable interrupt wrapper

// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

#ifndef CRITICALREGION_HPP
#define CRITICALREGION_HPP

#include "cmsis.h"

class InterruptLock
    {
    private:

    bool first;

    public:


    __FORCEINLINE InterruptLock() : first(true)
        {
        __disable_irq();
        }

    __FORCEINLINE ~InterruptLock()
        {
        __enable_irq();
        }

    __FORCEINLINE bool test()
        {
        return first;
        }

    __FORCEINLINE void update()
        {
        first = false;
        }

    };


#define CRITICAL_REGION(Type) for(Type lock; lock.test(); lock.update())

#endif // CRITICALREGION_HPP
