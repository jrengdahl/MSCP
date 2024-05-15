// this is an extremely lightweight calloc
// I assume that nothing is ever freed

extern "C" void *malloc(unsigned s);
extern "C" void *memset(void *dest, int c, unsigned n);


extern "C"
void *calloc(unsigned num, unsigned size)
    {
    char *tmp;

    tmp = (char *)malloc(num*size);     // get the block of memory
    memset(tmp,0,num*size);             // set the memory to zero
    return (void *)tmp;
    }
