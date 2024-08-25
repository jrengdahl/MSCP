// Routines to access Qbus using registers in the FPGA


#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "cmsis.h"
#include "local.h"
#include "cyccnt.hpp"
#include "Qbus.hpp"

#define FMC_WRITE_TIME 60                       // minimum FMC write cycle time

#define Q_ADDR_SETUP 150                        // addresss setup before BSYNC asserted
#define Q_ADDR_HOLD 100                         // address hold after BSYNC asserted
#define Q_DATA_SETUP 100                        // data setup before BDOUT asserted on write transaction
#define Q_BDOUT_HOLD 150                        // BDOUT hold after receipt of BREPLY
#define Q_DATA_HOLD 100                         // data hold after BDOUT deasserted
#define Q_SYNC_HOLD 175                         // BSYNC hold after BDOUT deasserted
#define Q_TURN 300                              // turnaround from BRPLY deasserted to next BSYNC asserted
#define Q_RDATA_SETUP 200                       // after BRPLY asserted, read data setup before taking data sample and deasserting BDIN
#define Q_DMA_TURN 250                          // delay from BSACK asserted to BSYNC asserted by DMA master
#define Q_DMA_HOLDOFF 4000                      // min delay from BSACK deasserted to next assertion of BDMR




static Q_Ctl Ctl = {};                                      // CTL struct
static unsigned stamp2 = 0;                                 // reference time stamp for Qbus turnaround (BSYNC-to-BSYNC delay)
static unsigned Target = 0;

#define DELAYFOR(time)  do{__COMPILER_BARRIER(); for(unsigned stamp = Now(), end = TicksPer(time); Now()-stamp  < end;); __COMPILER_BARRIER();}while(false)
#define DELAYFOR2(time) do{__COMPILER_BARRIER(); for(unsigned                end = TicksPer(time); Now()-stamp2 < end;); __COMPILER_BARRIER();}while(false)
#define DELAYUNTIL(target) do{__COMPILER_BARRIER(); if((target)-Now()<TicksPer(Q_DMA_HOLDOFF))while((int)(target)-(int)Now() >0); __COMPILER_BARRIER();}while(false)

#define ASSERT(signal)    do {__COMPILER_BARRIER(); Ctl.signal = 1; FADDR_CT = Ctl.value; __COMPILER_BARRIER();}while(false)
#define DEASSERT(signal)  do {__COMPILER_BARRIER(); Ctl.signal = 0; FADDR_CT = Ctl.value; __COMPILER_BARRIER();}while(false)
#define PULSE(signal)     do {Q_Ctl tmp = Ctl; tmp.signal = 1; FADDR_CT = tmp.value;}while(false)

#define WAITFOR(signal)    do{__COMPILER_BARRIER(); for(Q_Sts Sts = {}; Sts.value = FADDR_ST, Sts.signal != 1;); __COMPILER_BARRIER();}while(false)
#define WAITFORNOT(signal) do{__COMPILER_BARRIER(); for(Q_Sts Sts = {}; Sts.value = FADDR_ST, Sts.signal != 0;); __COMPILER_BARRIER();}while(false)


void mark() {__COMPILER_BARRIER();}

void QbusInit()
    {
    FADDR_SA = 0;
    FADDR_LO = 0;
    FADDR_HI = 0;
    FADDR_DATA_OUT = 0;
    FADDR_CT = 0x80;                // Turn all outputs off and clear the BSACK FF
    (void)FADDR_ST;                 // read to clear status bits
    }

void QDMAbegin()
    {
    DELAYUNTIL(Target);                                     // wait until at least 4 usec since lst DMA
    __disable_irq();
    ASSERT(BDMR);
    WAITFOR(BSACK);
    DEASSERT(BDMR);
    Target = Now()+ TicksPer(Q_DMA_TURN);                 // set turnaround from BSACK to BSYNC 250 ns
    }

void QDMAend()
    {
    PULSE(DMA_done);                                        // this turns off BSACK
    __enable_irq();
    Target = Now() + TicksPer(Q_DMA_HOLDOFF);             // must wait at least 4 usec before requesting DMA again
    }


uint16_t Qread(uint32_t addr)
    {
    uint16_t data;

    DELAYUNTIL(Target);                                     // BSYNC turnaround

    FADDR_LO = addr&0xFFFF;                                 // output the address
    FADDR_HI = addr>>16;
    Ctl.BBS7 = (addr&017770000) == 017770000;               // output the other address-related signals, and enable the address
    Ctl.BWTBT = 0;
    Ctl.Q_Addr_enable = 1;
    FADDR_CT = Ctl.value;

    DELAYFOR(Q_ADDR_SETUP);                                 // address setup 150 ns
    ASSERT(BSYNC);
//    mark();
    DELAYFOR(Q_ADDR_HOLD);                                  // address hold 100 ns

    Ctl.BBS7 = 0;                                           // deassert the address and related signals
    Ctl.Q_Addr_enable = 0;
    Ctl.BDIN = 1;                                           // assert BDIN
    FADDR_CT = Ctl.value;

//    mark();
    WAITFOR(BRPLY);
    DELAYFOR(Q_RDATA_SETUP);                                // data setup after BRPLY 200 ns
    data = FADDR_DATA_IN;                                   // read the data
    DEASSERT(BDIN);
    WAITFORNOT(BRPLY);
    Target = Now() + TicksPer(Q_TURN);                    // capture timestamp for BRPLY off to  next BSYNC on turnaround 300
    DEASSERT(BSYNC);

    return data;
    }


void Qwrite(uint32_t addr, uint16_t data)
    {
    DELAYUNTIL(Target);                                     // BSYNC turnaround

    FADDR_LO = addr&0xFFFF;                                 // output the address
    FADDR_HI = addr>>16;
    Ctl.BBS7 = (addr&017770000) == 017770000;               // output the other address-related signals, and enable the address
    Ctl.BWTBT = 1;
    Ctl.Q_Addr_enable = 1;
    FADDR_CT = Ctl.value;

    DELAYFOR(Q_ADDR_SETUP);                                 // address setup 150 ns
    ASSERT(BSYNC);
//    mark();
    DELAYFOR(Q_ADDR_HOLD);                                  // address hold 100 ns

    FADDR_DATA_OUT = data;                                  // output the data to the FPGA data register

    Ctl.BBS7 = 0;                                           // deassert the address and related signals
    Ctl.BWTBT = 0;
    Ctl.Q_Addr_enable = 0;
    Ctl.Q_Data_enable = 1;
    FADDR_CT = Ctl.value;

    DELAYFOR(Q_DATA_SETUP);                                 // data setup 100 ns
    ASSERT(BDOUT);
//    mark();
    WAITFOR(BRPLY);
    DELAYFOR(Q_BDOUT_HOLD);                                 // BDOUT hold after BRPLY 150 ns
    DEASSERT(BDOUT);
    stamp2 = Now();
    DELAYFOR2(Q_DATA_HOLD);                                 // data hold after BDOUT off 100 ns
    DEASSERT(Q_Data_enable);
    WAITFORNOT(BRPLY);
    Target = Now() + TicksPer(Q_TURN);                    // capture timestamp for BRPLY-to-BSYNC turnaround 300
    DELAYFOR2(Q_SYNC_HOLD);                                 // sync hold after BDOUT off 175 ns
    DEASSERT(BSYNC);
    }

void QReadBlock(uint32_t addr, uint16_t *buffer, int size)
    {
    QDMAbegin();
    for(int i=0; i<size; i++)
        {
        buffer[i] = Qread(addr+i*2);
        if((i&7)==7 && i<size)
            {
            QDMAend();
            QDMAbegin();
            }
        }
    QDMAend();
    }

void QWriteBlock(uint32_t addr, uint16_t *buffer, int size)
    {
    QDMAbegin();
    for(int i=0; i<size; i++)
        {
        Qwrite(addr+i*2, buffer[i]);
        if((i&7)==7 && i<size)
            {
            QDMAend();
            QDMAbegin();
            }
        }
    QDMAend();
    }



uint16_t vector = 0;

void Qinterrupt()
    {
    if(vector)
        {

        }
    }

