extern "C"
int memcmp(const void *vs1, const void *vs2, unsigned n)
    {
    int res = 0;
    const char *s1 = (const char *)vs1;
    const char *s2 = (const char *)vs2;

    for(unsigned i=0; i < n && (res = *s1 - *s2) == 0; i++)
        {
        s1++;
        s2++;
        }

    return res;
    }
