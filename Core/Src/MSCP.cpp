#include <stdint.h>
#include <stdio.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "cyccnt.hpp"
#include "Qbus.hpp"
#include "uqssp.h"
#include "MSCP.h"
#include "serial.h"
#include "ContextFIFO.hpp"


extern response rsp;


void poll()
    {
    command *cmd;

    while(!ControlC)
        {
        cmd = GetPacket();
        if(cmd == nullptr)
            {
            yield();
            continue;
            }

        if(cmd->msgtype == 0 && cmd->vcid == 0)
            {
            switch(cmd->opcode)
                {
            case OP_ONL:
                printf("packet received with opcode %d\n", cmd->opcode);
                break;
            case OP_RD:
                printf("packet received with opcode %d\n", cmd->opcode);
                break;
            default:
                printf("packet received with opcode %d\n", cmd->opcode);
                rsp.endcode = OP_END;
                rsp.flags = 0;
                rsp.status = ST_CMD | I_OPCD;
                rsp.msglen = 12;
                rsp.msgtype = 0;
                PutPacket(&rsp);
                }
            }
        else
            {
            printf("received packet, type %d, vcid %d\n", cmd->msgtype, cmd->vcid);
            }
        }


    }
