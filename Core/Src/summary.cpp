// dump a summary of the memory

// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

#include <stdio.h>
#include <stdint.h>

#include "boundaries.h"

// print a summary of memory
//
// Print a two character summary of each block of an area of memory.
// The default block is 256 bytes.
// The two character pair might be:
// -- TT for the .text area
// -- DD for .data
// -- BB for .bss
// -- HH for the heap
// -- SS for the area declared as stack on the linker file, though threads may have stacks elsewhere 
// -- XX is the memory block is not one of the above and all the bytes in the block do not have the same value
// -- hh the summary may be a two-digit hex value if every byte of the block has the same value.

// args:
//  -- a pointer to the start of the memory area to summarize
//  -- the size of the memory area to summarize
//  -- the sie of the memory block represented by a each two-character pair
//  -- a bool flag that selects whether a block is represented by its type (false) or its hex value (true), if each byte has the same value

void summary(   unsigned char *mem,                     // start of memory region to summarize
                unsigned size,                          // size of memory region to summarize
                unsigned inc,                           // size of block in the summary
                int values)                             // if 0 simply print the type for every block
                                                        // if 1 print blocks which have all the same value as that value
    {
    int line=0;                                         // counts elements per line
    for(unsigned i=0; i<size; i += inc)                 // for each block in the size
        {
        if(line==0)                                     // if at beginning of line print address
            {
            printf("%08zx: ", (uintptr_t)(mem+i));
            }

        unsigned char b = mem[i];                       // remember first byte in block
        char buf[3];                                    // print buffer

        snprintf(buf, 3, "%02x", b);                    // save hex representation of the first byte

        for(unsigned j=0; j<inc; j++)                   // for each byte in the block
            {
            if(mem[i+j] != b || values==0)              // if the byte is different from the first byte, or we are not printing values
                {
                char sym;
                uint32_t *addr = (uint32_t *)&mem[i+j];

                if(     addr >= &_text_start      && addr < &_text_end)      sym = 'T';     // select a type depending on what range the mmory is in
                else if(addr >= &_sdata           && addr < &_edata)         sym = 'D';
                else if(addr >= &_sbss            && addr < &_ebss)          sym = 'B';
                else if(addr >= &_heap_start      && addr < &_heap_end)      sym = 'H';
                else if(addr >= &_stack_start     && addr < &_stack_end)     sym = 'S';
                else                                                         sym = '#';

                buf[0] = sym;                           // fill a two char buffer with the selected memory type
                buf[1] = sym;
                }
            }

        printf("%s ", buf);                             // print summary of this block
        line++;                                         // count up to 16 blocks per line
        if(line>=16)                                    // at end of line
            {
            printf("\n");                               // print newline
            line=0;                                     // and start new line
            }
        }

    if(line != 0)                                       // print a newline if not already at the start of a new line
        {
        printf("\n");
        }
    }

