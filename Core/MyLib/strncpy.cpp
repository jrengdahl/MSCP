extern "C"
char *strncpy(char *dest, const char *src, unsigned n)
    {
    unsigned i;

    for(i=0; i<n && src[i]; i++)dest[i] = src[i];
    for (; i<n; i++)dest[i] = 0;

    return dest;
    }
