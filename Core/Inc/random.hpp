// Random number generators, from Donald Knuth.
// I probably found these in his "The Art of Computer Programming" many years ago.
// See the Wikipedia article https://en.wikipedia.org/wiki/Linear_congruential_generator
// While possibly not the best random number generators, they are very fast, and are
// good enough for software testing.

// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

#ifndef RANDOM_HPP
#define RANDOM_HPP

#include <stdint.h>

static inline uint64_t random64(uint64_t &seed)
    {
    seed = seed * 6364136223846793005 + 1442695040888963407;        // calculate the next state

    return seed;                                                    // return the whole thing
    }

static inline uint32_t random32(uint32_t &seed)
    {
    seed = seed * 1664525 + 1013904223;                             // calculate the next state

    return seed;                                                    // return the whole thing
    }

#endif // RANDOM_HPP
