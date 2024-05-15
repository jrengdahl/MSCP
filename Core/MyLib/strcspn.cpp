extern "C" char *strchr(const char *s, int c);

extern "C"
unsigned strcspn(const char *s, const char *reject)
    {
    unsigned n = 0;

    while (!strchr(reject, *s))
        {
        n++;
        s++;
        }

    return n;
    }
