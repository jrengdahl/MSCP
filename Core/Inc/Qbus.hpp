#ifndef QBUS_HPP
#define QBUS_HPP

#include "cmsis.h"


// define addresses of registers in the FPGA
#define QBASE 0x60000000
#define FADDR_ST        (*(uint16_t volatile *)(QBASE + 0))         // status register, see Q_Sts
#define FADDR_SA        (*(uint16_t volatile *)(QBASE + 2))         // the SA register
#define FADDR_CT        (*(uint16_t volatile *)(QBASE + 4))         // control register, see Q_Ctl
#define FADDR_LO        (*(uint16_t volatile *)(QBASE + 6))         // low word of Qbus address
#define FADDR_HI        (*(uint16_t volatile *)(QBASE + 8))         // high word of Qbus address (6 bits only)
#define FADDR_DATA_OUT  (*(uint16_t volatile *)(QBASE + 10))        // data to be written to Qbus
#define FADDR_DATA_IN   (*(uint16_t volatile *)(QBASE + 12))        // read the current data on the Qbus

union Q_Sts
    {
    struct
        {
        unsigned short IP_Read            : 1;
        unsigned short IP_Written         : 1;
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
            unsigned Q_Addr_enable      : 1;    // enable address onto Qbus during address phase
            unsigned Q_Data_enable      : 1;    // enable data_out onto Qbus during data phase
            unsigned Clear_SA           : 1;    // enable clear of SA register when ST is read
            unsigned                    : 2;    // Padding to make it a full 16-bit struct
            unsigned DMA_done           : 1;    // write 1 to clear BSACKg
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





extern void QbusInit();
extern void QDMAbegin();
extern void QDMAend();
extern uint16_t Qread(uint32_t addr);
extern void Qwrite(uint32_t addr, uint16_t data);
extern void QReadBlock(uint32_t addr, uint16_t *buffer, int size);
extern void QWriteBlock(uint32_t addr, uint16_t *buffer, int size);
extern void Qinterrupt();

#endif // QBUS_HPP
