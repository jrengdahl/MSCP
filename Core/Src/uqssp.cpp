#include <MSCP.hpp>
#include <stdint.h>
#include <stdio.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "cyccnt.hpp"
#include "Qbus.hpp"
#include "uqssp.hpp"
#include "serial.h"
#include "ContextFIFO.hpp"



command cmd = {};
response rsp = {};

FIFOctl rsp_fifo;
FIFOctl cmd_fifo;
extern uint16_t vector;

int credits = MAX_COMMANDS;


// get a descriptor from a FIFO
uint32_t GetDesc(FIFOctl &fifo)
    {
    uint32_t addr;                              // the calculated address of the descriptor
    uint32_t desc = 0;                          // the descriptor
    int owner;                                  // the owner flag isolated form the descriptor

    addr = fifo.addr + fifo.index;              // calculate the address
    QReadBlock(addr, (uint16_t *)&desc, 2);     // read the descriptor
    owner = desc>>31;                           // get the owner bit
    desc &= 0x7fffffff;                         // clear the owner bit in the descriptor
    if(owner == 0)return 0;                     // if the controller does not own the descriptor pointed to by the index, the FIFO is empty, return 0
    printf("GetDesc from FIFO %08lo, index %d, descriptor %011lo\n", fifo.addr, fifo.index, desc);
    return desc;                                // else return the descriptor
    }

// put a descriptor to a FIFO
void PutDesc(FIFOctl &fifo, uint32_t desc)
    {
    uint32_t addr;                              // the calculated address of the descriptor
    uint32_t buf;                               // a buffer for reading/writing descriptors

    printf("PutDesc to FIFO %08lo, index %d, descriptor %011lo\n", fifo.addr, fifo.index, desc);

    addr = fifo.addr + fifo.index;              // calculate the address of the indexed descriptor
    buf = desc | 0x40000000;                    // set the interrupt flag in the descriptor
    QWriteBlock(fifo.addr+2, (uint16_t *)&buf + 1, 1); // write only the upper word of the descriptor
    if((desc&0x40000000) != 0)                  // if the interrupt flag in the descriptor was set before
        {
        if(fifo.size == 1)                      // if the FIFO size is 1, always interrupt
            {
            buf = 1;                            // write a 1 to the interrupt flag
            QWriteBlock(fifo.flag, (uint16_t *)&buf, 1);
            Qinterrupt();                       // send an interrupt to the PDP-11
            }
        else                                    // if the FIFO size > 1, only interrupt if the
            {                                   // previous descriptor is not owned by the host
            addr = fifo.addr + ((fifo.index - 4) & (fifo.size*4 - 1)) + 2;   // get addr of prev descriptor
            QReadBlock(addr, (uint16_t *)&buf+1, 1); // read it
            if((buf&0x80000000) != 0)           // if owned by controller (cmd FIFO was full, or rsp FIFO was empty)
                {
                buf = 1;                        // write a 1 to the interrupt flag
                QWriteBlock(fifo.flag, (uint16_t *)&buf, 1);
                Qinterrupt();                   // send an interrupt to the PDP-11
                }
            }
        }
    fifo.index = (fifo.index + 4) & (fifo.size*4 - 1);   // increment the index, with FIFO wraparound
    }


command *GetPacket()
    {
    uint32_t desc;

    desc = GetDesc(cmd_fifo);                   // get a descriptor from the FIFO
    if(desc == 0)return nullptr;                // return nothing if empty

    QReadBlock((desc&017777777) - 4, (uint16_t *)&cmd, sizeof(command)/2); // read the packet from the host to the controller's packet buffer
    PutDesc(cmd_fifo, desc);                    // send the command packet back to the host

    return &cmd;                                // return the copy of the packet
    }

void PutPacket(response *rsp)                   // address of response packet, points to 4 byte UQSSP header
    {
    uint32_t desc;

    while((desc = GetDesc(rsp_fifo)) == 0)      // try to get a host-side response packet buffer
        {
        yield();                                // wait a bit then try again
        }

    int type = rsp->msgtype;                    // get message type
    int opcode = rsp->endcode;                  // get opcode

    if(type == 0 && (opcode&0200) != 0)         // only sequential replies are legal at this point
        {
        int cr = credits;                       // get the current credit count (originally equal to the number of command buffers)

        if(cr > 14)cr = 14;                     // can only send up to 15 credits at a time
        credits -= cr;                          // deduct the number of credits being sent
        rsp->credits = cr + 1;                  // send that number plus one
        }

    QWriteBlock((desc&017777777) - 4, (uint16_t *)rsp, (rsp->msglen + 4) / 2);  // copy the response packet to the host buffer

    PutDesc(rsp_fifo, desc);                    // send the response packet to the host
    }


// initialization handshake, initiated when the host resets the controller

void Qinit()
    {
    Q_Sts Qsts;
    uint16_t s1, s2, s3, s4;

    printf("waiting for init\n");
    while(Qsts.value=FADDR_ST, Qsts.IP_Written == 0)if(ControlC)return;     // wait for init (write to IP register)
    printf("init received\n");
    FADDR_SA = 005000;
    printf("wrote step1, waiting for response\n");
    while(Qsts.value=FADDR_ST, Qsts.SA_Written == 0)if(ControlC)return;
    printf("received %6o\n", s1=FADDR_SA);
    FADDR_SA = 010000;
    printf("wrote step2, waiting for response\n");
    while(Qsts.value=FADDR_ST, Qsts.SA_Written == 0)if(ControlC)return;
    printf("received %6o\n", s2=FADDR_SA);
    FADDR_SA = 020000;
    printf("wrote step3, waiting for response\n");
    while(Qsts.value=FADDR_ST, Qsts.SA_Written == 0)if(ControlC)return;
    printf("received %6o\n", s3=FADDR_SA);
    FADDR_SA = 040463;
    printf("wrote step4, waiting for response\n");
    while(Qsts.value=FADDR_ST, Qsts.SA_Written == 0)if(ControlC)return;
    printf("received %6o\n", s4=FADDR_SA);
    printf("init complete\n");

    rsp_fifo.size = 1 << ((s1>>8)&7);                                                // compute response FIFO size in longs
    cmd_fifo.size = 1 << ((s1>>11)&7);                                               // compute command FIFO size
    rsp_fifo.addr = (uint32_t)(s2 & 0xFFFE) | ((uint32_t)(s3 & 0x7FFF) << 16) ;      // compute resp FIFO address
    cmd_fifo.addr = rsp_fifo.addr + rsp_fifo.size*4;                                            // compute command FIFO address
    rsp_fifo.flag = rsp_fifo.addr - 2;
    cmd_fifo.flag = rsp_fifo.addr - 4;
    rsp_fifo.index = 0;
    cmd_fifo.index = 0;

    printf("rsp FIFO at %06lo, size %d\n", rsp_fifo.addr, rsp_fifo.size);
    printf("cmd FIFO at %06lo, size %d\n", cmd_fifo.addr, cmd_fifo.size);
    }
