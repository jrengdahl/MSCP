#include <stdint.h>
#include <stdio.h>
#include "cmsis.h"
#include "local.h"
#include "bogodelay.hpp"
#include "cyccnt.hpp"
#include "main.h"
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

union Q_Sts
    {
    struct
        {
        unsigned short IR_Read            : 1;
        unsigned short IR_Written         : 1;
        unsigned short SA_Read            : 1;
        unsigned short SA_Written         : 1;
        unsigned short BRPLY              : 1;
        unsigned short BRPLY_Asserted     : 1;
        unsigned short BRPLY_Deasserted   : 1;
        unsigned short BSACK              : 1;
        unsigned short                    : 8;
        };
    uint16_t value;
    };

struct Q_Ctl
    {
    union
        {
        struct
            {
            unsigned BSYNC              : 1;
            unsigned BDIN               : 1;
            unsigned BDOUT              : 1;
            unsigned BWTBT              : 1;
            unsigned BDMR               : 1;
            unsigned BREF               : 1;
            unsigned BBS7               : 1;
            unsigned BIRQ4              : 1;
            unsigned BIRQ5              : 1;
            unsigned BIRQ6              : 1;
            unsigned Q_Addr_enable      : 1;
            unsigned Q_Data_enable      : 1;
            unsigned                    : 3; // Padding to make it a full 16-bit struct
            unsigned DMA_done           : 1;
            };
        uint16_t value; // This represents the whole struct as a single 16-bit value
        };

    __IGNORE_WARNING("-Weffc++");                       // suppress warning: 'operator=' should return a reference to '*this' [-Weffc++]

    __FORCEINLINE void operator=(const Q_Ctl& other)
        {
        value = other.value; // Directly assign the 16-bit value
        }

    __FORCEINLINE void operator=(const Q_Ctl& other) volatile
        {
        value = other.value; // Directly assign the 16-bit value
        }

    __UNIGNORE_WARNING("-Weffc++");

    };


// define addresses of registers in the FPGA
#define QBASE 0x60000000
#define FADDR_ST        (*(uint16_t volatile *)(QBASE + 0))
#define FADDR_SA        (*(uint16_t volatile *)(QBASE + 2))
#define FADDR_CT        (*(uint16_t volatile *)(QBASE + 4))
#define FADDR_LO        (*(uint16_t volatile *)(QBASE + 6))
#define FADDR_HI        (*(uint16_t volatile *)(QBASE + 8))
#define FADDR_DATA_OUT  (*(uint16_t volatile *)(QBASE + 10))
#define FADDR_DATA_IN   (*(uint16_t volatile *)(QBASE + 12))


static Q_Ctl Ctl = {};                                      // CTL struct
static unsigned stamp2 = 0;                                 // reference time stamp for Qbus turnaround (BSYNC-to-BSYNC delay)
static unsigned Target = 0;

#define DELAYFOR(time) for(unsigned stamp = Ticks(); Ticks()-stamp < TicksPer(time);)
#define DELAYFOR2(time) for(; Ticks()-stamp2 < TicksPer(time);)
#define DELAYUNTIL(time) while(Ticks()-Target <0)

#define ASSERT(signal)    do {Ctl.signal = 1; FADDR_CT = Ctl.value;}while(false)
#define DEASSERT(signal)  do {Ctl.signal = 0; FADDR_CT = Ctl.value;}while(false)
#define PULSE(signal)     do {Q_Ctl tmp = Ctl; tmp.signal = 1; FADDR_CT = tmp.value;}while(false)

#define WAITFOR(signal) for(Q_Sts Sts = {}; Sts.value = FADDR_ST, Sts.signal != 1;)
#define WAITFORNOT(signal) for(Q_Sts Sts = {}; Sts.value = FADDR_ST, Sts.signal != 0;)


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
    ASSERT(BDMR);
    WAITFOR(BSACK);
    DEASSERT(BDMR);
    Target = Ticks()+ TicksPer(Q_DMA_TURN);                 // set turnaround from BSACK to BSYNC 250 ns
    }

void QDMAend()
    {
    PULSE(DMA_done);                                        // this turns off BSACK
    Target = Ticks() + TicksPer(Q_DMA_HOLDOFF);             // must wait at least 4 usec before requesting DMA again
    }


void Qaddr(uint32_t addr, int write, int byte)
    {
    DELAYUNTIL(Target);                                     // BSYNC turnaround

    FADDR_LO = addr&0xFFFF;                                 // output the address
    FADDR_HI = addr>>16;
    Ctl.BBS7 = (addr&017770000) == 017770000;               // output the other address-related signals, and enable the address
    Ctl.BWTBT = write;
    Ctl.Q_Addr_enable = 1;
    FADDR_CT = Ctl.value;

    DELAYFOR(Q_ADDR_SETUP);                                 // address setup 150 ns
    ASSERT(BSYNC);
    DELAYFOR(Q_ADDR_HOLD);                                  // address hold 100 ns

    Ctl.BBS7 = 0;                                           // deassert the address and related signals
    Ctl.BWTBT = byte;
    Ctl.Q_Addr_enable = 0;
    FADDR_CT = Ctl.value;
    }


uint16_t Qread()
    {
    uint16_t data;
    Q_Sts Sts;

    if(Sts.value=FADDR_ST, Sts.BRPLY)                       // make sure BRPLY is deasserted
        {
        printf("error: BRPLY should not be asserted at beginning of read\n");
        return 0;
        }

    ASSERT(BDIN);
    WAITFOR(BRPLY_Asserted);
    DELAYFOR(Q_RDATA_SETUP);                                // data setup after BRPLY 200 ns
    data = FADDR_DATA_IN;                                   // read the data
    DEASSERT(BDIN);
    WAITFOR(BRPLY_Deasserted);
    Target = Ticks() + TicksPer(Q_TURN);                    // capture timestamp for BRPLY off to  next BSYNC on turnaround 300
    DEASSERT(BSYNC);

    return data;
    }


void Qwrite(uint16_t data)
    {
    Q_Sts Sts;

    if(Sts.value=FADDR_ST, Sts.BRPLY)                       // make sure BRPLY is deasserted
        {
        printf("error: BRPLY should not be asserted at beginning of write\n");
        return;
        }

    FADDR_DATA_OUT = data;                                  // output the data
    ASSERT(Q_Data_enable);
    DELAYFOR(Q_DATA_SETUP);                                 // data setup 100 ns
    ASSERT(BDOUT);
    WAITFOR(BRPLY_Asserted);
    DELAYFOR(Q_BDOUT_HOLD);                                 // BDOUT hold after BRPLY 150 ns
    DEASSERT(BDOUT);
    stamp2 = Ticks();
    DELAYFOR2(Q_DATA_HOLD);                                 // data hold after BDOUT off 100 ns
    DEASSERT(Q_Data_enable);
    WAITFOR(BRPLY_Deasserted);
    Target = Ticks() + TicksPer(Q_TURN);                    // capture timestamp for BRPLY-to-BSYNC turnaround 300
    DELAYFOR2(Q_SYNC_HOLD);                                 // sync hold after BDOUT off 175 ns
    DEASSERT(BSYNC);
    }


void QWriteWord(uint32_t addr, uint16_t data)
    {
    QDMAbegin();
    Qaddr(addr, 1, 0);
    Qwrite(data);
    QDMAend();
    }

uint16_t QReadWord(uint32_t addr)
    {
    uint16_t data;

    QDMAbegin();
    Qaddr(addr, 0);
    data = Qread();
    QDMAend();
    return data;
    }
