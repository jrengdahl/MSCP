#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "cyccnt.hpp"
#include "Qbus.hpp"
#include "uqssp.hpp"
#include "MSCP.hpp"
#include "serial.h"
#include "ContextFIFO.hpp"
#include "ff.h"



extern response rsp;
extern FIL fil;
extern uint32_t qbuf[512/4];


// get get packets from host, process them, and send replies

void MSCP_poll()
    {
    command *cmd;

    while(!ControlC)
        {
        cmd = GetPacket();                              // get a command packet
        if(cmd == nullptr)                              // if nothing returned
            {
            yield();                                    // wait a bit, let other processes run
            continue;                                   // and go try again
            }

        memset(&rsp, 0, sizeof(rsp));

        if(cmd->msgtype == 0 && cmd->vcid == 0)         // if sequential MSCP command
            {
            switch(cmd->opcode)                         // process the command
                {
            case OP_ONL:                                // online
                {
                printf("OP_ONL packet received, unit = %d\n", cmd->unit);

                char name[16];
                snprintf(name, sizeof(name), "UNIT%d.img", cmd->unit);

                if(f_open(&fil, name, FA_READ | FA_WRITE) != FR_OK)
                    {
                    printf("opening %s failed\n", name);
                    return;
                    }
                printf("%s online\n", name);

                unsigned size = (f_size(&fil) + 511)/ 512;
                printf("size = %d blocks\n", size);

                rsp.msglen = 44;
                rsp.msgtype = 0;
                rsp.cmdref = cmd->cmdref;
                rsp.unit = cmd->unit;
                rsp.endcode = OP_ONL | OP_END;
                rsp.flags = 0;
                rsp.status = ST_SUC;
                rsp.multiunit_code = cmd->unit;
                rsp.unit_flags = 0;
                rsp.id.dev_class = UID_DISK;
                rsp.id.model = UID_RA92;
                rsp.id.serno_hi = 0;
                rsp.id.serno_lo = cmd->unit; // on the RQDX3 this is the unit number
                rsp.media_type_identifier = DU_SD32;
                rsp.unit_size = size;
                rsp.volume_serial_number = 3141592654;
                PutPacket(&rsp);

                break;
                }

            case OP_RD:                                 // read
                {
                unsigned start = cmd->LBN * 512;
                unsigned size = cmd->bytecount;
                uint32_t addr = cmd->buffer_address;

                printf("OP_RD packet received, LBN = %ld, size = %d, dest = %08lo\n", cmd->LBN, size, addr);

                for(unsigned off=0; off<size; off += 512)
                    {
                    unsigned br; // Bytes read
                    unsigned expected;
                    FRESULT res;

                    res = f_lseek(&fil, start + off);
                    if(res != FR_OK)
                        {
                        printf("seek to %d failed\n", start + off);
                        return;
                        }

                    memset(qbuf, 0, sizeof(qbuf));

                    if(size-off < 512)
                        {
                        expected = size-off;
                        }
                    else
                        {
                        expected = 512;
                        }

                    res = f_read(&fil, qbuf, expected, &br);
                    if(res == FR_OK)
                        {
                        QWriteBlock(addr, (uint16_t *)&qbuf, 512/2);
                        addr += 512;
                        }
                    else
                        {
                        printf("block read failed at offset %d, bytes read %d, status %d\n", off, br, res);
                        return;
                        }
                    }

                rsp.msglen = 32;
                rsp.msgtype = 0;
                rsp.cmdref = cmd->cmdref;
                rsp.unit = cmd->unit;
                rsp.endcode = OP_RD | OP_END;
                rsp.flags = 0;
                rsp.status = ST_SUC;
                PutPacket(&rsp);

                break;
                }
            default:                                    // unimplemented command
                printf("packet received with opcode %d\n", cmd->opcode);
                rsp.endcode = OP_END;
                rsp.flags = 0;
                rsp.status = ST_CMD | I_OPCD;
                rsp.msglen = 12;
                rsp.msgtype = 0;
                PutPacket(&rsp);
                }
            }
        else                                            // unrecognized protocol
            {
            printf("received packet, type %d, vcid %d\n", cmd->msgtype, cmd->vcid);
            }
        }


    }
