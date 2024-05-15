extern "C"
unsigned strnlen(const char *p, unsigned n)
    {
    unsigned x=0;

    while(*p && n)
        {
        p++;
        x++;
        n--;
        }

    return x;
    }
