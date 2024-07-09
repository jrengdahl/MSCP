#include "context.hpp"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "cmsis.h"
#include "local.h"
#include "bogodelay.hpp"
#include "cyccnt.hpp"
#include "main.h"
#include "omp.h"
#include "tim.h"

#define FMC_WRITE_TIME 60
#define Q_ADDR_SETUP 150
#define Q_ADDR_HOLD 100
#define Q_DATA_SETUP 150
#define Q_DATA_HOLD 100
#define Q_BDOUT_HOLD 150
#define Q_SYNC_HOLD 100
#define Q_TURN 200


struct Q_Sts
    {
    unsigned IR_Read            : 1;
    unsigned IR_Written         : 1;
    unsigned SA_Read            : 1;
    unsigned SA_Written         : 1;
    unsigned BRPLY              : 1;
    unsigned BRPLY_Asserted     : 1;
    unsigned BRPLY_Deasserted   : 1;
    unsigned BSACK_Asserted     : 1;
    unsigned                    : 8;
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
    inline void operator=(const Q_Ctl& other) volatile
        {
        value = other.value; // Directly assign the 16-bit value
        }
    __UNIGNORE_WARNING("-Weffc++");
    };


#define QBASE 0x60000000
#define FADDR_ST        (*(Q_Sts volatile *)(QBASE + 0))
#define FADDR_SA        (*(uint16_t volatile *)(QBASE + 2))
#define FADDR_CT        (*(Q_Ctl volatile *)(QBASE + 4))
#define FADDR_LO        (*(uint16_t volatile *)(QBASE + 6))
#define FADDR_HI        (*(uint16_t volatile *)(QBASE + 8))
#define FADDR_DATA_OUT  (*(uint16_t volatile *)(QBASE + 10))
#define FADDR_DATA_IN   (*(uint16_t volatile *)(QBASE + 12))


static unsigned stamp2 = 0;                                 // reference time stamp for Qbus turnaround (BSYNC-to-BSYNC delay)


void write(uint32_t addr, uint16_t data)
    {
    Q_Ctl Ctl = {};                                         // init the CTL struct
    unsigned stamp;                                         // reference time stamp for Qbus timing

    FADDR_LO = addr&0xFFFF;                                 // output the address
    FADDR_HI = addr>>16;

    Ctl.BBS7 = (addr&017770000) == 017770000;               // output the other address-related signals, and enable the address
    Ctl.BWTBT = 1;
    Ctl.Q_Addr_enable = 1;
    FADDR_CT = Ctl;

    stamp = Ticks();                                        // wait the address setup time
    while(Ticks()-stamp < TicksPer(Q_ADDR_SETUP));

    Ctl.BSYNC = 1;                                          // assert BSYMC
    FADDR_CT = Ctl;

    while(Ticks()-stamp < TicksPer(Q_ADDR_SETUP + Q_ADDR_HOLD));    // wait the address hold time

    if(FADDR_ST.BRPLY)                                      // make sure BRPLY is deasserted
        {
        printf("error: BRPLY should not be asserted at beginning of write\n");
        return;
        }

    Ctl.BBS7 = 0;                                           // deassert the address and related signals
    Ctl.BWTBT = 0;
    Ctl.Q_Addr_enable = 0;
    FADDR_CT = Ctl;

    FADDR_DATA_OUT = data;                                  // output the data
    Ctl.Q_Data_enable = 0;
    FADDR_CT = Ctl;

    stamp = Ticks();
    while(Ticks()-stamp < TicksPer(Q_DATA_SETUP));          // wait the data setup time

    Ctl.BDOUT = 1;                                          // asseret BDOUT
    FADDR_CT = Ctl;

    while(!FADDR_ST.BRPLY_Asserted);                        // wait for BRPLY

    stamp = Ticks();
    while(Ticks()-stamp < TicksPer(Q_DATA_SETUP));          // wait BDOUT hold time

    Ctl.BDOUT = 0;                                          // deassert BDOUT
    FADDR_CT = Ctl;
    stamp2 = Ticks();

    stamp = Ticks();
    while(Ticks()-stamp < TicksPer(Q_DATA_HOLD));           // wait the data hold time

    while(!FADDR_ST.BRPLY_Deasserted);                      // wait for BRPLY to be deasserted

    while(Ticks()-stamp < TicksPer(Q_SYNC_HOLD));           // wait for BSYNC hold time referenced to BDOUT deasserted

    Ctl.BSYNC = 0;                                          // deassert BSYNC
    FADDR_CT = Ctl;

    stamp2 = Ticks();                                       // capture timestamp for BSYNC-to-BSYNC delay
    }
