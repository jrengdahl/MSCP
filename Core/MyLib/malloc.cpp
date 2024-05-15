#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "CriticalRegion.hpp"

extern int omp_verbose;
#define DPRINT(level) if(omp_verbose>=level)printf

extern unsigned _heap_start;
extern unsigned _heap_end;
extern uintptr_t _break;


struct MemBlock
    {
    union
        {
        MemBlock *next;
        unsigned bucket;
        };
    uintptr_t useable;
    };

static const unsigned MINSIZE = 8;
static const unsigned MAXSIZE = 1024;
static const unsigned SIZES = 8;

MemBlock *FreeBlocks[SIZES];



extern "C"
void *malloc(unsigned size)
    {
    MemBlock *blk = 0;
    unsigned bucket;

    size = (size+7) & -8;                       // round size up to next 8 bytes
    assert(size >= MINSIZE && size <= MAXSIZE); // make sure it's a valid block size
    bucket = 31-__builtin_clz(size>>2);         // get the bucket number

    CRITICAL_REGION(InterruptLock)
        {
        blk = FreeBlocks[bucket];
        if(blk != 0)
            {
            FreeBlocks[bucket] = blk->next;
            DPRINT(1)("malloc old %8p %u %u\n", blk, size, 1<<(bucket+3));
            }
        else
            {
            blk = (MemBlock *)_break;
            _break += (1<<(bucket+3))+4;
            assert(_break < (uintptr_t)&_heap_end);                 // assert that the heap has not overflowed
            DPRINT(1)("malloc new %8p %u %u\n", blk, size, 1<<(bucket+3));
            }
        }

    blk->bucket = bucket;

    return (void *)&(blk->useable);
    }

void free(void *ptr)
    {
    MemBlock *blk = (MemBlock *)((uintptr_t)ptr - 4);
    unsigned bucket = blk->bucket;

    DPRINT(1)("free %8p %u\n", blk, 1<<(bucket+3));

    CRITICAL_REGION(InterruptLock)
        {
        blk->next = FreeBlocks[bucket];
        FreeBlocks[bucket] = blk;
        }
    }



void mem()
    {
    printf("%u bytes of heap left\n", (uintptr_t)&_heap_end - _break);

    int size = MINSIZE;
    for(unsigned i=0; i<SIZES; i++)
        {
        printf("%4d byte blocks: ", size);
        for(MemBlock *blk = FreeBlocks[i]; blk != 0; blk = blk->next)
            {
            printf("%p ", blk);
            } 
        printf("\n");
        size *= 2;
        }
    }
