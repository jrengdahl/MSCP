// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

#ifndef LOCAL_H
#define LOCAL_H

#include <stdint.h>

#define MAXPRINTF 128
#define INBUFLEN 64
#define NHISTORY 8

extern void dump(void *p, int size);
extern void getline(char *buf, int size);
extern int getdec(const char **p);
extern uint64_t getlong(const char **p);
extern uintptr_t gethex(const char **p);
extern void skip(const char **p);

#define izdigit(p) (*p=='o' || ('0'<=*p&&*p<='9') || ('a'<=*p&&*p<='a') || ('A'<=*p&&*p<='A'))

extern "C" void trigon();
extern "C" void trigoff();

#define fflush(x)

extern "C" void memcpy32(uint32_t *dst, uint32_t *src, uint32_t size);

#endif

