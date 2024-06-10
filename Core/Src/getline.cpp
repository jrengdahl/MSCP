#include <stdio.h>
#include <stdint.h>
#include "serial.h"
#include "local.h"

// getline and associated routines

// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

extern "C" int __io_getchar();


// get a decimal number from the command line buffer
int getdec(const char **p)
    {
    int x=0;

    if(**p=='-')
        {
        (*p)++;
        return -getdec(p);
        }
    while(1)
        {
        if('0'<=**p&&**p<='9')x = (x<<3) + (x<<1) + **p-'0';
        else if( **p == '\''){}
        else return x;
        (*p)++;
        }
    }

// get a long decimal number from the command lne buffer
uint64_t getlong(const char **p)
    {
    uint64_t x=0;

    if(**p=='-')
        {
        (*p)++;
        return -getdec(p);
        }
    while(1)
        {
        if('0'<=**p&&**p<='9')x = (x<<3) + (x<<1) + **p-'0';
        else if( **p == '\''){}
        else return x;
        (*p)++;
        }
    }

// get a hex number from the command line buffer
uintptr_t gethex(const char **p)
    {
    uintptr_t x=0;

    while(1)
        {
        if('0'<=**p&&**p<='9')     x = (x<<4) + **p-'0';
        else if('a'<=**p&&**p<='f')x = (x<<4) + **p-'a'+10;
        else if('A'<=**p&&**p<='F')x = (x<<4) + **p-'A'+10;
        else if( **p == '\''){}
        else return x;
        (*p)++;
        }
    }

// skip a pointer to the start of the next argument in the command line buffer
void skip(const char **p)
    {
    while(**p && **p!=' ')(*p)++;                                               // skip the arg that p points to
    while(**p && **p==' ')(*p)++;                                               // skip any following whitespace
    }

//move the cursor forward
static void forward(int col)
    {
    printf("\033[%dC",col);
    }

// move the cursor back
static void back(int col)
    {
    if(col==0)return;
    printf("\033[%dD",col);
    }

// print n blanks on the screen
static void blank(int n)
    {
    for(int i=0;i<n;i++)putchar(' ');
    }


// get one character from stdin. Translate \r to \n.
int get1()
    {
    char c;

    c = __io_getchar();

    if(c == '\r')
        {
        c = '\n';
        }

    return c;
    }

#define HMASK (NHISTORY-1)
static int first=1;
static char lastline[NHISTORY][INBUFLEN];
static int lln=0;
int getline_nchar = 0;


// fill a command line buffer from stdin, handling command line editing and history
// assumes an ANSI terminal
void getline(   char *buf,                                                      // input buffer to be filled
                int size)                                                       // size of buffer
    {
    int i;                                                                      // general purpose loop counter
    int c;                                                                      // current input character
    int x=0;                                                                    // cursor position
    int n=0;                                                                    // length of line
    int tll=0;
    
    if(first)
        {
        for(i=0;i<NHISTORY;i++)
            {
            lastline[i][0]=0;
            }
        first=0;
        }
    
    fflush(stdout);
    buf[0] = 0;
    while(n<size-1 && (c=get1()) !='\n')                                        // repeat until buffer filled or end of line
        {
        if(c==0x1b)                                                             // escape sequences
            {
            if((c=get1())=='[')
                {
                c=get1();
                if(c=='C')                                                      // ->
                    {
                    if(x<n)
                        {
                        x++;
                        forward(1);
                        }
                    }
                else if(c=='D')                                                 // <-
                    {
                    if(x>0)
                        {
                        x--;
                        back(1);
                        }
                    }
                else if(c=='A')                                                 // up arrow
                    {
                    if(tll<NHISTORY)
                        {
                        back(x);
                        blank(n);
                        back(n);
                        x=0;
                        n=0;
                        tll++;
                        for(i=0;lastline[(lln-tll)&HMASK][i];i++)
                            {
                            buf[i] = lastline[(lln-tll)&HMASK][i];
                            putchar(buf[i]);
                            x++;
                            n++;
                            }
                        buf[n]=0;
                        }
                    }
                else if(c=='B')                                                 // down arrow
                    {
                    back(x);
                    blank(n);
                    back(n);
                    x=0;
                    n=0;
                    if(tll)tll--;
                    if(tll)
                        {
                        for(i=0;lastline[(lln-tll)&HMASK][i];i++)
                            {
                            buf[i] = lastline[(lln-tll)&HMASK][i];
                            putchar(buf[i]);
                            x++;
                            n++;
                            }
                        buf[n]=0;
                        }
                    }
                else if(c=='1'&& (c=get1())=='~')                               // home
                    {
                    back(x);
                    x = 0;
                    }
                else if(c=='4'&& (c=get1())=='~')                               // end
                    {
                    forward(n-x);
                    x = n;
                    }
                else if(c=='5' && (c=get1())=='~')                              // page up (erase line)
                    {
                    back(x);
                    blank(n);
                    back(n);
                    x=0;
                    n=0;
                    buf[0] = 0;
                    }
                else                                                            // unknown escape sequence
                    {
                    printf("$[%c",c);
                    }
                }
            else if(c=='O'&& (c=get1())=='P'){}
            else                                                                // unknown escape sequence
                {   
                printf("$%c",c);
                }
            }
        else if(c==8)                                                           // backspace
            {
            if(x>0)
                {
                x--;
                back(1);
                for(i=x;i<n;i++)
                    {
                    buf[i]=buf[i+1];
                    if(buf[i])putchar(buf[i]);
                    else putchar(' ');
                    }
                n--;
                back(n-x+1);
                }
            }
        else if(c==0x7f)                                                        // del
            {
            if(x<n)
                {
                for(i=x;i<n;i++)
                    {
                    buf[i]=buf[i+1];
                    if(buf[i])putchar(buf[i]);
                    else putchar(' ');
                    }
                n--;
                back(n-x+1);
                }
            }
        else if(ControlC)                                                       // ^C (erase line)
            {
            back(x);
            blank(n);
            back(n);
            buf[0] = 0;
            x=0;
            n=0;
            }
        else
            {   
            for(i=n;i>x;i--)buf[i] = buf[i-1];
            buf[x]=c;
            n++;
            buf[n]=0;
            for(i=x;i<n;i++)putchar(buf[i]);
            x++;
            if(x<n)back(n-x);
            }
        fflush(stdout);
        getline_nchar = n;
        }
    if(x<n)forward(n-x);
    putchar('\n');

    for(i=0;i<n;i++)lastline[lln][i] = buf[i];
    lastline[lln][n]=0;
    lln = (lln+1)&HMASK;
    getline_nchar = 0;
    }   

