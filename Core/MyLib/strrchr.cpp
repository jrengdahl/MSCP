extern "C" unsigned strlen(const char *p);

extern "C"
char *strrchr(const char *s, int c)
    {
    int i;
    int n = strlen(s);

    for(i=n-1;i>=0 && *s;i--)if(s[i]==c)return (char *)&s[i];

    return 0;
    }
