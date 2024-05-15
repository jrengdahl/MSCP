/* $Id: scanf.c,v 1.2 2002/08/09 20:56:57 pefo Exp $ */

/*
 * Copyright (c) 2000-2002 Opsycon AB  (www.opsycon.se)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by Opsycon AB.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Modifications by Jonathan Engdahl January 2019
   -- reformat
   -- remove dependencies on FILE, leaving only sscanf
   -- remove pmon.h

 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

extern "C" int isalpha(int c);
extern "C" int isspace(int c);


/*
 * ** fscanf --\         sscanf --\
 * **          |                  |
 * **  scanf --+-- vfscanf ----- vsscanf
 * **
 * ** This not been very well tested.. it probably has bugs
 */
int vfscanf __P((FILE *, const char *, va_list));
int vsscanf __P((const char *, const char *, va_list));


#if 0
/*
 *  scanf(fmt,va_alist)
 */

extern "C"
int scanf (const char *fmt, ...)
{
    int             count;
    va_list ap;

    va_start (ap, fmt);
    count = vfscanf (stdin, fmt, ap);
    va_end (ap);
    return (count);
}

/*
 *  fscanf(fp,fmt,va_alist)
 */

extern "C"
int fscanf (FILE *fp, const char *fmt, ...)
{
    int             count;
    va_list ap;

    va_start (ap, fmt);
    count = vfscanf (fp, fmt, ap);
    va_end (ap);
    return (count);
}
#endif

/*
 *  sscanf(buf,fmt,va_alist)
 */

extern "C"
int sscanf (const char *buf, const char *fmt, ...)
    {
    int             count;
    va_list ap;

    va_start (ap, fmt);
    count = vsscanf (buf, fmt, ap);
    va_end (ap);
    return (count);
    }

#if 0
/*
 *  vfscanf(fp,fmt,ap)
 */

extern "C"
static int vfscanf (FILE *fp, const char *fmt, va_list ap)
{
    int             count;
    char            buf[MAXLN + 1];

    if (fgets (buf, MAXLN, fp) == 0)
        return (-1);
    count = vsscanf (buf, fmt, ap);
    return (count);
}

#endif

/*
 *  vsscanf(buf,fmt,ap)
 */

extern "C"
int vsscanf (const char *buf, const char *s, va_list ap)
    {
    int             count, noassign, width, lflag;
    const char      *first = buf;

    count = noassign = width = lflag = 0;
    while (*s && *buf)
        {
        if(isspace(*s))
            {
            while (isspace(*s))s++;
            while (isspace(*buf))buf++;
            }

        else if (*s == '%')
            {
            s++;
            for (; *s; s++)
                {
                if (strchr ("dibouxcsefg%", *s))
                    break;
                if (*s == '*')
                    noassign = 1;
                else if (*s == 'l' || *s == 'L')
                    lflag = 1;
                else if (*s >= '1' && *s <= '9')
                    {
                    width = 0;
                    while(*s >= '0' && *s <= '9')
                        {
                        width = width * 10 + *s-'0';
                        s++;
                        }
                    }
                }

            if (*s == 's')
                {
                if (!width)
                    width = strcspn (buf, " \t\n\r\f\v");
                if (!noassign)
                    {
                    char *t = va_arg (ap, char *);
                    strncpy (t, buf, width);
                    t[width] = '\0';
                    count++;
                    }
                buf += width;
                }

            else if (*s == 'c')
                {
                if (!width)
                    width = 1;
                if (!noassign)
                    {
                    char *t = va_arg (ap, char *);
                    strncpy (t, buf, width);
                    t[width] = '\0';
                    count++;
                    }
                buf += width;
                }

            else if (strchr ("dobxu", *s))
                {
                int base = 10;
                int sign = 1;
                long value = 0;

                while (isspace (*buf))
                    buf++;
                if (*s == 'd' || *s == 'u')
                    base = 10;
                else if (*s == 'x')
                    base = 16;
                else if (*s == 'o')
                    base = 8;
                else if (*s == 'b')
                    base = 2;
                else
                    base = 10;

                if (!width)
                    {
                    if (isspace(s[1]) || s[1] == 0)
                        width = strcspn (buf, " \t\n\r\f\v");
                    else
                        width = strchr (buf, s[1]) - buf;
                    }

                if(*buf=='-')
                    {
                    sign = -1;
                    ++buf;
                    --width;
                    }

                while(width)
                    {
                    unsigned digit;
                    if(base==16)
                        {
                        if('0'<=*buf && *buf<='9')
                            {
                            digit = *buf-'0';
                            ++buf;
                            --width;
                            }
                        else if('A'<=*buf && *buf<='F')
                            {
                            digit = *buf-'A'+10;
                            ++buf;
                            --width;
                            }
                        else if('a'<=*buf && *buf<='f')
                            {
                            digit = *buf-'a'+10;
                            ++buf;
                            --width;
                            }
                        else
                            {
                            digit = 0;
                            width = 0;
                            }
                        }
                    else if('0'<=*buf && *buf<'0'+base)
                        {
                        digit = *buf-'0';
                        ++buf;
                        --width;
                        }
                    else
                        {
                        digit = 0;
                        width = 0;
                        }
                    value = value * base + digit;
                    }
                if (!noassign)
                    {
                    if(lflag)
                        {
                        long *dst = va_arg (ap, long *);
                        *dst = value * sign;
                        }
                    else
                        {
                        int *dst = va_arg (ap, int *);
                        *dst = (int)((value * sign) & ~0);
                        }
                    count++;
                    }
                }

            else if (*s == 'n')
                {
                if (!noassign)
                    {
                    int *v = va_arg (ap, int *);
                    *v = buf-first;
                    count++;
                    }
                }


            width = noassign = lflag = 0;
            s++;
            }

        else if (*s++ != *buf++)break;
        }

    while (isspace(*s))s++;
    if (s[0]=='%' && s[1] == 'n')
        {
        int *v = va_arg (ap, int *);
        *v = buf-first;
        count++;
        }

    return (count);
    }
