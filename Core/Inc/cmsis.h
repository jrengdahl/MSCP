// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

#ifndef CMSIS_H
#define CMSIS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_compiler.h"

#ifdef __cplusplus
inline bool COMPILER_BARRIER()
    {
    __COMPILER_BARRIER();
    return true;
    }
#else
#define COMPILER_BARRIER __COMPILER_BARRIER
#endif

#define __FORCEINLINE __attribute__((always_inline)) inline
#define __UNUSED  __attribute__((__unused__))
#define __OPTIMIZE(n) __attribute__((__optimize__(n)))
#define __FLATTEN __attribute__((__flatten__))
#define __NAKED __attribute__((__naked__))
#ifndef __NOINLINE
#define __NOINLINE __attribute__ ( (noinline) )
#endif

#define __LENGTH(x) (sizeof(x)/sizeof(x[0]))

#define __XSTRINGIFY(s) #s
#define __STRINGIFY(s) __XSTRINGIFY(s)

#define __IGNORE_WARNING(x)                         \
    _Pragma("GCC diagnostic push")                  \
    _Pragma(__STRINGIFY(GCC diagnostic ignored x))

#define __UNIGNORE_WARNING(x)                       \
    _Pragma("GCC diagnostic pop")


#define NUM_ELEMENTS(ARRAY) (sizeof(ARRAY)/sizeof(ARRAY[0]))



#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

// Versions of the __LDREX and __STREX intrinsics that
// work with reference variables and arbitrary types.
// These are implemented for pointer-sized data only.
// Unlike the CMSIS versions, these can use the offset
// addressing modes.


template<typename T>
__STATIC_INLINE T __LDREX(T volatile *place)
    {
    T value;
    __asm__ __volatile__(  "ldrex   %0, [%1]"
            : "=r" (value)
            : "r" (place));
    return value;
    }

template<typename T>
__STATIC_INLINE int __STREX(T value, T volatile *place)
    {
    int sts;
    __asm__ __volatile__(  "strex   %0, %1, [%2]"
            : "=&r" (sts)
            : "r" (value), "r" (place)
            : "memory");
    return sts;
    }

template<typename T>
__STATIC_INLINE T __LDREX(T volatile &place)
    {
    T value;
    __asm__ __volatile__(  "ldrex   %0, %1"
            : "=r" (value)
            : "m" (place));
    return value;
    }

template<typename T>
__STATIC_INLINE int __STREX(T value, T volatile &place)
    {
    int sts;
    __asm__ __volatile__(  "strex   %0, %2, %1"
            : "=&r" (sts), "=m" (place)
            : "r" (value)
            : "memory");
    return sts;
    }

#endif


#endif // CMSIS_H

