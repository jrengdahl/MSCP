extern "C" long int strtol(const char *p, char **endptr, int base);

extern "C"
int atoi(const char *nptr)
    {
    return strtol(nptr, 0, 10);
    }
