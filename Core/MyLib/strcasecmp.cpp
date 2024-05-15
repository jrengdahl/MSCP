extern "C"
int strcasecmp(const char *s1, const char *s2)
    {
    while(   *s1
          && *s2
          && (   (*s1 == *s2)
              || (*s1>='A' && *s1<='Z' && *s2-*s1 == 'a'-'A')
              || (*s2>='A' && *s2<='Z' && *s1-*s2 == 'a'-'A')))
        {
        s1++;
        s2++;
        }

    return *s1 - *s2;
    }
