
extern "C"
long int strtol(const char *p, char **endptr, int base)
    {
    long sign = 1;

    while(*p == ' ' || *p == '\t')++p;                              // skip leading whitespace

    if(*p == '+')++p;                                               // skip plus sign
    else if(*p == '-')                                              // skip but remember minus sign
        {
        sign = -1;
        ++p;
        }

    if((base == 0 || base == 16) && p[0] == '0' && p[1] == 'x')     // handle hex base
        {
        base = 16;
        p += 2;
        }
    else if(base == 0 && p[0] == '0')                               // handle possible octal base
        {
        base = 8;
        }
    else if(base == 0)
        {
        base = 10;
        }

    long value = 0;

    while(*p)
        {
        int digit;

        if( '0' <= *p
        &&  *p  <= '9'
        &&  *p  <= '0'+base-1 )
            {
            digit = *p - '0';
            }
        else if( base > 10
             &&  'A' <= *p
             &&  *p  <= 'A'+base-11)
            {
            digit = *p - 'A' + 10;
            }
        else if( base > 10
             &&  'a' <= *p
             &&  *p  <= 'a'+base-11)
            {
            digit = *p - 'a' + 10;
            }
        else
            {
            break;
            }

        value = value * base + digit;
        ++p;
        }

    if(endptr)
        {
        *endptr = (char *)p;
        }

    return value * sign;
    }
