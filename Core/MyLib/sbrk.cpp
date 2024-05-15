// sbrk implementation
// works like UNIX sbrk call

#include <stdint.h>
#include <assert.h>

extern unsigned _heap_start;
extern unsigned _heap_end;

uintptr_t _break = (uintptr_t)&_heap_start;

extern "C"
void *sbrk(intptr_t increment)
    {
    _break += increment;                                    // set _break to one past the end of the new chunk

    assert(_break < (uintptr_t)&_heap_end);                 // assert that the heap has not overflowed

    return (void *)_break;                                  // return aligned start of new chunk
    }

