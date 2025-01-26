// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

#ifndef SERIAL_H
#define SERIAL_H

#include "Fifo.hpp"

extern volatile bool ControlC;
extern bool SerialRaw;
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
int __io_getchart(unsigned time);
int _write(int file, const char *ptr, int len);

int vcp_kbhit();
int vcp_getchar();

#ifdef __cplusplus
    }
#endif

#endif // SERIAL_H
