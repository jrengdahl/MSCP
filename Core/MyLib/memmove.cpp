extern "C"
void *memmove(void *dest, const void *src, unsigned n)
    {
    char *d = (char *)dest;
    const char *s = (const char *)src;

    if(d <= s)
        {
        for(unsigned i = 0; i < n; i++)
            {
            d[i] = s[i];
            }
        }
    else
        {
        for (unsigned i = n; i > 0 ; i--)
            {
            d[i-1] = s[i-1];
            }
        }

    return dest;
    }
