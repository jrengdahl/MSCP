#include <stdint.h>
#include "boot.h"

void boot()
    {
    struct response *rsp = (struct response *)RSP_PACKET;
    struct command *cmd = (struct command *)CMD_PACKET;
    int read_size;

    clear_memory();

    IP = 0;
    do_step(004000, 0100000);
    do_step(010000, COMM_AREA);
    do_step(020000, 0000000);
    do_step(040000, 0000001);

    cmd->msglen = 32;
    cmd->opcode = 011;
    send_and_wait();
    
    if(rsp->unit_size.hi != 0
    || rsp->unit_size.lo >64)
        {
        read_size = 64u * 512u;
        }
    else
        {
        read_size = rsp->unit_size.lo * 512;
        }

    cmd->msglen = 64;
    cmd->opcode = 041;
    cmd->bytecount.lo = read_size;
    send_and_wait();

    ZERO();

    while(1);
    }
