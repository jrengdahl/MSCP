/*
 * The original source was downloaded from:
 * http://www.menie.org/georges/embedded/index.html
 *
 * Edit history:
 * Jonathan Engdahl xx-SEP-2010 Adapted from the original download.
 * Jonathan Engdahl 04-Jun-2013 Added dummy "l" qualifier.
 * Jonathan Engdahl 06-Sep-2016 Fix conversion of 64-bit numbers.
 */


 /*
    Copyright
        2001, 2002 Georges Menie (www.menie.org)
    stdarg version contributed by Christian Ettinger

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include <stdarg.h> /* for varargs stuff */
#include <stdio.h>  /* for std I/O function declarations */
#include <string.h> /* for sprintf function declaration */
#include <stdint.h>


static inline void writechar( char *pCh, unsigned c )
    {
    *pCh = c;
    }

static void printchar(char **str, int c)
    {
    writechar( *str, c );
    ++(*str);
    }

#define PAD_RIGHT 1
#define PAD_ZERO 2

static int prints(char **out, char *bufend, const char *string, int width, int pad)
{
    int pc = 0, padchar = ' ';

    if (width > 0) {
        int len = 0;
        const char *ptr;
        for (ptr = string; *ptr; ++ptr) ++len;
        if (len >= width) width = 0;
        else width -= len;
        if (pad & PAD_ZERO) padchar = '0';
    }
    if (!(pad & PAD_RIGHT)) {
        for ( ; width > 0 && *out<bufend; --width) {
            printchar (out, padchar);
            ++pc;
        }
    }
    for ( ; *string && *out<bufend; ++string) {
        printchar (out, *string);
        ++pc;
    }
    for ( ; width > 0 && *out<bufend; --width) {
        printchar (out, padchar);
        ++pc;
    }

    return pc;
}

/* the following should be enough for 64 bit int */
#define PRINT_BUF_LEN 20

/*
    Name:       printi
                Format a number into a print buffer.

    args:
        out     pointer to current position in output buffer
        bufend  pointer to last byte of output buffer
        i       number to convert
        b       base
        sg      sign flag, 0= unsigned, 1 = signed
        width   field width
        pad     pad flags
        letbase 'a' or 'A', depending on whether alpha characters are to be lower or upper case
*/

static int printi(char **out, char *bufend, long i, int b, int sg, int width, int pad, int letbase)
{
    char print_buf[PRINT_BUF_LEN];
    char *s;
    int t, neg = 0, pc = 0;
    unsigned long u = i;

    if (i == 0) {
        writechar( &print_buf[0], '0' );
        writechar( &print_buf[1], '\0' );
        return prints (out, bufend, print_buf, width, pad);
    }

    if (sg && b == 10 && i < 0) {
        neg = 1;
        u = -i;
    }

    s = print_buf + PRINT_BUF_LEN-1;
    writechar( s, '\0' );

    while (u) {
        t = u % b;
        if ( t >= 10 ) {
            t += letbase - '0' - 10;
        }
        writechar( --s, t + '0' );
        u /= b;
    }

    if (neg) {
        if ( width && (pad & PAD_ZERO) && *out<bufend ) {
            printchar (out, '-');
            ++pc;
            --width;
        }
        else {
            writechar( --s, '-' );
        }
    }

    return pc + prints (out, bufend, s, width, pad);
}

extern "C"
int vsnprintf(char *buf, size_t size, const char *format, va_list args )
{
    int width, frac, pad;
    int pc = 0;
    char *out = buf;
    char *bufend = out+size-1;
    char scr[2];
    int longflag = 0;
    long value;

    for (; *format != 0  && out<bufend; ++format) {
        if (*format == '%') {
            ++format;
            width = frac = pad = 0;
            if (*format == '\0') break;
            if (*format == '%') goto out;
            if (*format == '-') {
                ++format;
                pad = PAD_RIGHT;
            }
            while (*format == '0') {
                ++format;
                pad |= PAD_ZERO;
            }
            for ( ; *format >= '0' && *format <= '9'; ++format) {
                width *= 10;
                width += *format - '0';
            }
            if(*format == '.') {
                ++format;
                for ( ; *format >= '0' && *format <= '9'; ++format) {
                    frac *= 10;
                    frac += *format - '0';
                }
            }
            if ( *format == 'l') {
                longflag = 1;
                ++format;
            }
            if ( *format == 'z') {
                if(sizeof(unsigned *) > sizeof(unsigned)) {
                    longflag = 1;
                }
                ++format;
            }
            if ( *format == 's' ) {
                char *s = (char *)va_arg( args, intptr_t );
                pc += prints (&out, bufend, s?s:"(null)", width, pad);
                continue;
            }
            if ( *format == 'd' ) {
                if(longflag)value = va_arg( args, long );
                else        value = va_arg( args, int );
                pc += printi (&out, bufend, value, 10, 1, width, pad, 'a');
                continue;
            }
            if ( *format == 'o' ) {
                if(longflag)value = va_arg( args, long );
                else        value = va_arg( args, int ) & 0xFFFFFFFF;
                pc += printi (&out, bufend, value, 8, 0, width, pad, 'a');
                continue;
            }
            if ( *format == 'x' ) {
                if(longflag)value = va_arg( args, long );
                else        value = va_arg( args, int ) & 0xFFFFFFFF;
                pc += printi (&out, bufend, value, 16, 0, width, pad, 'a');
                continue;
            }
            if ( *format == 'X' ) {
                if(longflag)value = va_arg( args, long );
                else        value = va_arg( args, int ) & 0xFFFFFFFF;
                pc += printi (&out, bufend, value, 16, 0, width, pad, 'A');
                continue;
            }
            if ( *format == 'p') {

                if(sizeof(unsigned *) > sizeof(unsigned))longflag=1;
                if(longflag)value = va_arg( args, long );
                else        value = va_arg( args, int ) & 0xFFFFFFFF;
                //printchar (&out, '0');
                //printchar (&out, 'x');
                pc += printi (&out, bufend, value, 16, 0, width, pad, 'A');
                continue;
            }
            if ( *format == 'u' ) {
                if(longflag)value = va_arg( args, long );
                else        value = va_arg( args, int ) & 0xFFFFFFFF;
                pc += printi (&out, bufend, value, 10, 0, width, pad, 'a');
                continue;
            }
            // An abbreviated implementation of %f. This may not handle negative numbers correctly, I have not thought about it.
            // Ignore the number of fraction digits for now, and always print 3.
            #ifdef __SIZEOF_FLOAT__
            if ( *format == 'f' || *format == 'g' || *format == 'e' ) {
                union
                    {
                    double value;
                    uint64_t i;
                    } v;

                v.value = va_arg( args, double );
                if(v.i == 0x7FF0000000000000)
                    {
                    pc += prints (&out, bufend, "inf", width, pad);
                    continue;
                    }
                pc += printi (&out, bufend, (long)v.value, 10, 1, width-4, pad, 'a');
                pc += prints (&out, bufend, ".", 1, 0);
                pc += printi (&out, bufend, (long)((v.value*1000-(long)v.value*1000)), 10, 0, 3, PAD_ZERO, 'a');
                continue;
            }
            #endif
            if ( *format == 'c' ) {
                /* char are converted to int then pushed on the stack */
                writechar( &scr[0], (char)va_arg( args, int ) );
                writechar( &scr[1], '\0' );
                pc += prints (&out, bufend, scr, width, pad);
                continue;
            }
        }
        else {
        out:
            if (out<bufend) {
                printchar (&out, *format);
                ++pc;
            }
        }
    }
    if (out) {
        writechar( out, '\0' );
    }
    va_end( args );
    return pc;
}


extern "C"
int snprintf(char *out, size_t size, const char *format, ...)
    {
    va_list args;

    va_start(args, format);

    return vsnprintf(out, size, format, args);
    }


extern "C"
int vsprintf(char *out, const char *format, va_list args )
    {
    return vsnprintf(out, 65535, format, args);
    }

extern "C"
int sprintf(char *out, const char *format, ...)
    {
    va_list args;

    va_start(args, format);
    return vsnprintf(out, 65535, format, args);
    }

#ifdef TEST_PRINTF
extern "C"
int main(void)
{
    char *ptr = "Hello world!";
    char *np = 0;
    int i = 5;
    unsigned int bs = sizeof(int)*8;
    int mi;
    char buf[80];

    mi = (1 << (bs-1)) + 1;
    printf("%s\n", ptr);
    printf("printf test\n");
    printf("%s is null pointer\n", np);
    printf("%d = 5\n", i);
    printf("%d = - max int\n", mi);
    printf("char %c = 'a'\n", 'a');
    printf("hex %x = ff\n", 0xff);
    printf("hex %02x = 00\n", 0);
    printf("signed %d = unsigned %u = hex %x\n", -3, -3, -3);
    printf("%d %s(s)%", 0, "message");
    printf("\n");
    printf("%d %s(s) with %%\n", 0, "message");
    sprintf(buf, "justif: \"%-10s\"\n", "left"); printf("%s", buf);
    sprintf(buf, "justif: \"%10s\"\n", "right"); printf("%s", buf);
    sprintf(buf, " 3: %04d zero padded\n", 3); printf("%s", buf);
    sprintf(buf, " 3: %-4d left justif.\n", 3); printf("%s", buf);
    sprintf(buf, " 3: %4d right justif.\n", 3); printf("%s", buf);
    sprintf(buf, "-3: %04d zero padded\n", -3); printf("%s", buf);
    sprintf(buf, "-3: %-4d left justif.\n", -3); printf("%s", buf);
    sprintf(buf, "-3: %4d right justif.\n", -3); printf("%s", buf);

    return 0;
}

/*
 * if you compile this file with
 *   gcc -Wall $(YOUR_C_OPTIONS) -DTEST_PRINTF -c printf.c
 * you will get a normal warning:
 *   printf.c:214: warning: spurious trailing `%' in format
 * this line is testing an invalid % at the end of the format string.
 *
 * this should display (on 32bit int machine) :
 *
 * Hello world!
 * printf test
 * (null) is null pointer
 * 5 = 5
 * -2147483647 = - max int
 * char a = 'a'
 * hex ff = ff
 * hex 00 = 00
 * signed -3 = unsigned 4294967293 = hex fffffffd
 * 0 message(s)
 * 0 message(s) with %
 * justif: "left      "
 * justif: "     right"
 *  3: 0003 zero padded
 *  3: 3    left justif.
 *  3:    3 right justif.
 * -3: -003 zero padded
 * -3: -3   left justif.
 * -3:   -3 right justif.
 */

#endif
