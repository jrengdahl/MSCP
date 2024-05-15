extern "C"
void *memset(void *dest, int c, unsigned n)
    {
    char *d = (char *)dest;

    while(n--)*d++ = c;

    return dest;
    }
