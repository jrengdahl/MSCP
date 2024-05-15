extern "C" unsigned strlen(const char *p);

extern "C"
char *strncat(char *dest, const char *src, unsigned n)
    {
    unsigned dest_len = strlen(dest);
    unsigned i;

    for(i=0; i<n && src[i]; i++)dest[dest_len + i] = src[i];
    dest[dest_len + i] = 0;

    return dest;
    }
