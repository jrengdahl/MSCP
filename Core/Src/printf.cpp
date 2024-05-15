// printf, etc. are subset implementations of standard C library routines.
// Use the regular #include <stdio.h>.

// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "local.h"
#include "mutex.hpp"

extern "C" int _write  (int file, const char *ptr, int len);
extern "C" int _writenl(int file, const char *ptr, int len);

static mutex PrintfMutex;

char printbuf[MAXPRINTF];                                   // a single printf buffer shared by all threads


// C library printf
extern "C"
int printf(const char *fmt, ...)
    {
    va_list args;
    va_start(args, fmt);

    return vprintf(fmt, args);
    }


// C library vprintf
extern "C"
int vprintf(const char *fmt, va_list args)
    {
    PrintfMutex.lock();

    int len = vsnprintf(printbuf, MAXPRINTF, fmt, args);        // format the message into the shared buffer

    _write(1, printbuf, len);

    PrintfMutex.unlock();

    return len;
    }


// C library puts
extern "C"
int puts(const char *str)
    {
    unsigned len = strlen(str);

    PrintfMutex.lock();
    _writenl(1, str, len);
    PrintfMutex.unlock();

    return len;
    }


// C library putchar
extern "C"
int putchar(const int c)
    {
    char buf = c;

    _write(1, &buf, 1);

    return c;
    }
