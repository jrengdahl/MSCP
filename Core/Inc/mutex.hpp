#ifndef _MUTEX_H
#define _MUTEX_H

#include "ContextFIFO.hpp"
#include "CriticalRegion.hpp"

class mutex
    {
    bool flag;
    ContextFIFO mwait;

    public:

    mutex() : flag(false)
        {
        }

    inline void lock()
        {
        CRITICAL_REGION(InterruptLock)
            {
            while(flag)
                {
                mwait.suspend();
                }
            flag = true;
            }
        }

    inline void unlock()
        {
        flag = false;
        if(mwait)
            {
            mwait.resume();
            }
        }
    };

#endif
