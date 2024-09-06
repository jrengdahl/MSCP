#ifndef UQSSP_H
#define UQSSP_H

#include "MSCP.hpp"

#define MAX_COMMANDS 16                 // the maximum number of packets that can be buffered by the controller

void Qinit();                           // initialize/synchronize MSCP communicaiton between host and controller
extern command *GetPacket();            // get a command packet sent by the host
void PutPacket(response *rsp);          // send a response packet to the host

#endif // UQSSP_H
