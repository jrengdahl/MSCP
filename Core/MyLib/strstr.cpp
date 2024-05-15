extern "C" unsigned strlen(const char *p);

extern "C"
char *strstr(const char *haystack, const char *needle)
    {
    unsigned i;
    unsigned j;
    unsigned m = strlen(needle);                            // length of the string to find
    unsigned n = strlen(haystack);                          // length of the string to search

    if(n<m)return 0;                                        // if the needle is larger than the haystack it cannot be found there
    n -= m;                                                 // do not start a string match after position n-m, since the remaining haystack would be smaller than the needle

    for(i=0;i<=n;i++)                                       // for each possible starting position in the haystack
        {
        for(j=0; j<m && haystack[i+j]==needle[j]; j++);     // try to match the needle
        if(j==m)return(char *)&haystack[i];                 // if a match is found return a pointer to the start of the substring
        }

    return 0;
    }
