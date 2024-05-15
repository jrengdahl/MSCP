extern "C"
__attribute__((__optimize__("O2")))                 // HACK: for some reason -Os breaks this function
unsigned strlen(const char *s)
    {
    unsigned n=0;

    while(*s++ != 0)
        {
        ++n;
        }

    return n;
    }
