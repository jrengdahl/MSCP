extern "C"
int strcmp(const char *s1, const char *s2)
    {
    int res=0;

    while((res=*s1-*s2)==0 && *s1 && *s2)
        {
        s1++;
        s2++;
        }

    return res;
    }
