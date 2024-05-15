extern "C"
char *strcpy(char *dest, const char *src)
    {
    unsigned i;

    for(i=0; src[i]; i++)dest[i] = src[i];
    dest[i] = 0;

    return dest;
    }
