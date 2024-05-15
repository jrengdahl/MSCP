// Customize omp.h

#pragma once

// Include the next omp.h found in the search path
#include_next <omp.h>

// Put custom code here

// This is a version of omp_get_wtime that returns a float rather than a double.
// This is more efficient on processors that only have single precision floating point hardware.
extern "C" float omp_get_wtime_float();
extern "C" float omp_get_wtick_float();

extern float omp_get_wtime(int);
extern float omp_get_wtick(int);
