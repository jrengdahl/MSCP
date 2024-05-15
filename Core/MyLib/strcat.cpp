extern "C" unsigned strlen(const char *p);

extern "C"
char *strcat(char *dest, const char *src)
    {
    unsigned dest_len = strlen(dest);
    unsigned i;

    for(i=0; src[i]; i++)dest[dest_len + i] = src[i];
    dest[dest_len + i] = 0;

    return dest;
    }
