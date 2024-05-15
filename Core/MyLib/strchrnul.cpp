extern "C"
const char *strchrnul(const char *s, int c)
    {
    while(s && *s!=0 && *s!=c)s++;

    return s;
    }
