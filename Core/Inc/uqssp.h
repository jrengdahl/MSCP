#ifndef UQSSP_H
#define UQSSP_H

#include "MSCP.h"

#define MAX_COMMANDS 16

extern command *GetPacket();
void PutPacket(response *rsp);
void Qinit();

#endif // UQSSP_H
