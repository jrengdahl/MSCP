extern "C"
const char *strnchr(const char *s, int n, int c)
    {
    for(int i=0; i<n && *s!=0; i++)
        {
        if(*s == c)
            {
            return s;
            }
        ++s;
        }

    return 0;
    }
