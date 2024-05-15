extern "C" char *strchr(const char *s, int c);


extern "C"
unsigned strspn(const char *s, const char *accept)
    {
    unsigned n = 0;

    while (strchr(accept, *s))
        {
        n++;
        s++;
        }

    return n;
    }
