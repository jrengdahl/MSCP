extern "C"
void *memcpy(void *dest, const void *src, unsigned n)
    {
    char *d = (char *)dest;
    const char *s = (const char *)src;

    while(n--)*d++ = *s++;

    return dest;
    }
