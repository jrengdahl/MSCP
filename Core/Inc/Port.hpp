#ifndef PORT_HPP
#define PORT_HPP

#include <context.hpp>
#include "cmsis.h"

class Port
    {
    Context *first;

    public:

    void *suspend();
    void suspend_switch();
    bool resume(void * x = 0);
    void resume_switch();

    inline operator bool() { return first != nullptr; }

    };

#endif // PORT_HPP

