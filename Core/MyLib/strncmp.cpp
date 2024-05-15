extern "C"
int strncmp(const char *s1, const char *s2, unsigned n)
    {
    int res=0;

    for(unsigned i=0; i<n && (res=*s1-*s2)==0 && *s1 && *s2; i++)
        {
        s1++;
        s2++;
        }

    return res;
    }
