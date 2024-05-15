// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

#ifndef SERIAL_H
#define SERIAL_H

#include "Fifo.hpp"

extern bool ControlC;
extern FIFO<char, 64> ConsoleFifo;
extern bool PollRx();
extern bool __io_kbhit();
extern bool __io_txrdy();
extern int __io_putchark(int ch);

#ifdef __cplusplus
extern "C" {
#endif

int __io_putchar(int ch);
int __io_getchar();

int vcp_kbhit();
int vcp_getchar();

#ifdef __cplusplus
    }
#endif

#endif // SERIAL_H
