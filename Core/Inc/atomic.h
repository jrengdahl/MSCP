/*
 * ATOMIC
 * A macro to atomically perform any operation on a single variable.
*/

// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#ifndef ATOMIC_H
#define ATOMIC_H

#include "cmsis.h"

#define atomic(tmp, dest)                               \
    for(int sts = 1; sts==1; sts=0)                     \
      for(__typeof__(dest) tmp = (__typeof__(dest))0;   \
           sts!=0 && (tmp = __LDREX(dest), true);       \
           sts=__STREX(tmp, dest))

#endif // ATOMIC_H
