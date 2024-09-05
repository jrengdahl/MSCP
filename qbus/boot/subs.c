#include <stdint.h>
#include "boot.h"


void do_step(word mask, word response)
    {
    word sa;

    do
        {
        sa = SA;
        if(sa<0)halt();
        }
    while((sa&mask) == 0);

    SA = response;
    }


void clear_memory()
    {
    for(word *p = (word *)BASE; p<(word *)(BASE+300); *p++ = 0);
    }

void send_and_wait()
    {
    struct response *rsp = (struct response *)RSP_PACKET;
    struct command *cmd = (struct command *)CMD_PACKET;
    struct FIFOs *fifos = (struct FIFOs *)COMM_AREA;

    fifos->rsp.lo = (uintptr_t)&rsp->cmdref & 01777777;
    fifos->rsp.hi = 0100000;
    fifos->cmd.lo = (uintptr_t)&cmd->cmdref & 01777777;
    fifos->cmd.hi = 0100000;

    COMPILE_BARRIER;

    (void)IP;

    do
        {
        COMPILE_BARRIER;
        if(SA<0)halt();
        }
    while(fifos->rsp.hi < 0);

    if(rsp->status != 0)halt();
    }

