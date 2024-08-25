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
extern int getdec(char **p);
extern uint64_t getlong(char **p);
extern uintptr_t gethex(char **p);
extern void skip(char **p);

#define izdigit(p) (*p=='o' || ('0'<=*p&&*p<='9') || ('a'<=*p&&*p<='a') || ('A'<=*p&&*p<='A'))

#define fflush(x)

#if __cplusplus
extern "C" {
#endif

//void trigon();
//void trigoff();
void memcpy32(uint32_t *dst, uint32_t *src, uint32_t size);

#if __cplusplus
    }
#endif


#endif

