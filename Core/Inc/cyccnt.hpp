// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

#ifndef CYCCNT_HPP
#define CYCCNT_HPP

#include <stdint.h>

#define xDWT_CONTROL (*(uint32_t volatile *)0xE0001000)
#define xCYCCNT (*(uint32_t volatile *)0xE0001004)
#define xDEMCR (*(uint32_t volatile *)0xE000EDFC)
#define xLAR (*(uint32_t volatile *)0xE0001FB0)

inline void InitCyccnt()
    {
    xDEMCR |= 0x01000000;            // enable trace
    xLAR = 0xC5ACCE55;               // <-- added unlock access to DWT (ITM, etc.)registers
    xCYCCNT = 0;                     // clear DWT cycle counter
    xDWT_CONTROL |= 1;               // enable DWT cycle counter
    }

extern unsigned CPU_CLOCK_FREQUENCY;
#define CPU_FREQ_MHZ CPU_CLOCK_FREQUENCY
#define CPU_FREQ_KHZ (CPU_FREQ_MHZ * 1000)

#define USEC ((xCYCCNT + CPU_FREQ_MHZ/2)/CPU_FREQ_MHZ)

inline uint32_t millis() { return (xCYCCNT + CPU_FREQ_KHZ/2)/CPU_FREQ_KHZ; }

extern uint32_t LastTimeStamp;

static inline unsigned Elapsed()
    {
    uint32_t current = xCYCCNT;
    unsigned delta = (current - LastTimeStamp)/CPU_FREQ_MHZ;
    LastTimeStamp = current;
    return delta;
    }

static inline void TimeStamp()
    {
    LastTimeStamp = xCYCCNT;
    }

static inline unsigned NsSince()
    {
    return (xCYCCNT - LastTimeStamp)*1000/CPU_FREQ_MHZ;
    }

static inline void Pause()
    {
    LastTimeStamp -= xCYCCNT;
    }

static inline void Resume()
    {
    LastTimeStamp += xCYCCNT;
    }


static __FORCEINLINE unsigned Now()
    {
    return xCYCCNT;
    }

static __FORCEINLINE unsigned TicksPer(unsigned ns)
    {
    return (ns*CPU_FREQ_MHZ)/1000;
    }




#endif //CYCCNT_HPP
