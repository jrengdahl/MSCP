#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define UNUSED __attribute__((__unused__))

extern "C"
void __assert_func (
    const char      * __file,
    int             __line,
    const char      * __function,
    const char      * __assertion)
    {
    printf("assert(%s) failed in function %s, file %s, at line %d\n",__assertion,__function,__file,__line);
    while(true);
    }

extern "C"
void __cxa_pure_virtual()
    {
    printf("pure virtual called!!!\n");
    assert(false);
    }

extern "C"
void __cxa_atexit(void)
    {
    }

void operator delete(void * p UNUSED)
    {
    printf("operator delete called!!!\n");
    assert(false);
    }

void operator delete(void* UNUSED, long unsigned int UNUSED)
    {
    printf("operator delete called!!!\n");
    assert(false);
    }

void *__dso_handle = 0;

extern "C"
void exit( int exit_code UNUSED)
    {
    printf("exit called!!!\n");
    assert(false);
    }

int __errno = 0;


