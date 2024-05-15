// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

#ifndef BOGODELAY_HPP 
#define BOGODELAY_HPP

#include <stdint.h>
#include "cmsis.h"
#include "cyccnt.hpp"

void bogodelay(int x);
void bogodelay(unsigned x);
void bogodelay(uint32_t x);
void bogodelay(uint64_t x);

#endif // BOGODELAY_HPP
