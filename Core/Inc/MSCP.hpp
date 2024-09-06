#ifndef MSCP_H
#define MSCP_H

#include <stdint.h>

typedef short word;
typedef unsigned short uword;
typedef unsigned char byte;

/* Misc constants */

#define UID_DISK        2                               /* disk class */
#define UID_RA92       29                               /* a 1.5 GB DU disk */
#define DU_SD32        0x25664040                       /* media ID for DU and SD32 (32 gig SD card) */
#define DU_SD16        0x25664020                       /* media ID for DU and SD16 (16 gig SD card) */


/* Opcodes */

#define OP_ABO          1                               /* b: abort */
#define OP_GCS          2                               /* b: get command status */
#define OP_GUS          3                               /* b: get unit status */
#define OP_SCC          4                               /* b: set controller char */
#define OP_AVL          8                               /* b: available */
#define OP_ONL          9                               /* b: online */
#define OP_SUC          10                              /* b: set unit char */
#define OP_DAP          11                              /* b: det acc paths - nop */
#define OP_ACC          16                              /* b: access */
#define OP_CCD          17                              /* d: compare - nop */
#define OP_ERS          18                              /* b: erase */
#define OP_FLU          19                              /* d: flush - nop */
#define OP_ERG          22                              /* t: erase gap */
#define OP_CMP          32                              /* b: compare */
#define OP_RD           33                              /* b: read */
#define OP_WR           34                              /* b: write */
#define OP_WTM          36                              /* t: write tape mark */
#define OP_POS          37                              /* t: reposition */
#define OP_FMT          47                              /* d: format */
#define OP_AVA          64                              /* b: unit now avail */
#define OP_END          0x80                            /* b: end flag */

/* Unit flags */

#define UF_RPL          0x8000                          /* d: ctrl bad blk repl */
#define UF_WPH          0x2000                          /* b: wr prot hwre */
#define UF_WPS          0x1000                          /* b: wr prot swre */
#define UF_EXA          0x0400                          /* b: exclusive NI */
#define UF_WPD          0x0100                          /* b: wr prot data NI */
#define UF_RMV          0x0080                          /* d: removable */
#define UF_CMW          0x0002                          /* cmp writes NI */
#define UF_CMR          0x0001                          /* cmp reads NI */


/* Status codes */

#define ST_SUC          0                               /* b: successful */
#define ST_CMD          1                               /* b: invalid cmd */
#define ST_ABO          2                               /* b: aborted cmd */
#define ST_OFL          3                               /* b: unit offline */
#define ST_AVL          4                               /* b: unit avail */
#define ST_MFE          5                               /* b: media fmt err */
#define ST_WPR          6                               /* b: write prot err */
#define ST_CMP          7                               /* b: compare err */
#define ST_DAT          8                               /* b: data err */
#define ST_HST          9                               /* b: host acc err */
#define ST_CNT          10                              /* b: ctrl err */
#define ST_DRV          11                              /* b: drive err */
#define ST_FMT          12                              /* t: formatter err */
#define ST_BOT          13                              /* t: BOT encountered */
#define ST_TMK          14                              /* t: tape mark */
#define ST_RDT          16                              /* t: record trunc */
#define ST_POL          17                              /* t: pos lost */
#define ST_SXC          18                              /* b: serious exc */
#define ST_LED          19                              /* t: LEOT detect */
#define ST_BBR          20                              /* d: bad block */
#define ST_DIA          31                              /* b: diagnostic */
#define ST_V_SUB        5                               /* subcode */
#define ST_V_INV        8                               /* invalid op */

/* Status invalid command subcodes */

#define I_OPCD          (8 << ST_V_SUB)                  /* inv opcode */
#define I_FLAG          (9 << ST_V_SUB)                  /* inv flags */
#define I_MODF          (10 << ST_V_SUB)                 /* inv modifier */
#define I_BCNT          (12 << ST_V_SUB)                 /* inv byte cnt */
#define I_LBN           (28 << ST_V_SUB)                 /* inv LBN */
#define I_VRSN          (12 << ST_V_SUB)                 /* inv version */
#define I_FMTI          (28 << ST_V_SUB)                 /* inv format */




struct longword
    {
    uword lo;
    word hi;
    };

struct unit_identifier
    {
    long serno_lo;                      // will be 3141592654 (for now)
    word serno_hi;                      // will be 0
    byte model;                         // model will be UID_RA92 (29) (fake ID)
    byte dev_class;                     // dev class will be UID_DISK (2)
    };

struct command
    {
    // UQSSP header
    uword       msglen;
    unsigned    credits     : 4;
    unsigned    msgtype     : 4;
    byte        vcid;

    // MSCP header
    long        cmdref;
    word        unit;
    word                    : 16;
    byte        opcode;
    byte                    : 8;
    word        modifiers;

    union
        {
        byte bytes[48];                     // force sizeof to be 64 bytes

        // online command parameters
        struct
            {
            word                    : 16;
            word        unit_flags;
            long                    : 32;
            long                    : 32;
            long                    : 32;
            long        device_dependent;
            long                    : 32;
            };

        // read/write command parameters
        struct
            {
            long        bytecount;
            long        buffer_address;
            long                : 32;
            long                : 32;
            long        LBN;
            };


        };
    };


struct response
    {
    // UQSSP header
    uword       msglen;                 // length of payload (starting with MSCP header)
    unsigned    credits     : 4;
    unsigned    msgtype     : 4;
    byte        vcid;

    // MSCP header
    long        cmdref;
    word        unit;
    word        sequence;
    byte        endcode;
    byte        flags;
    word        status;

    union
        {
        byte bytes[48];                     // force size to be 64 bytes

        // online response parameters
        struct
            {
            word        multiunit_code;
            word        unit_flags;
            byte        spndles;
            long                    : 24;

            struct unit_identifier id;                  // (8 bytes)
            long        media_type_identifier;
            long                    : 32;
            long        unit_size;               // size in LBNs
            long        volume_serial_number;    // optional, often zero
            };

        // read response parameters
        struct
            {
            long                    : 32;
            long                    : 32;
            long                    : 32;
            long                    : 32;

            long        first_bad_LBN;      // first bad block
            };
        };
    };



struct FIFOctl
    {
    uint32_t addr;      // address in PDP-11 memory of the FIFO data area
    uint32_t flag;      // address in PDP-11 memory of the flag word
    uint16_t size;      // size, in longwords, of the FIFO
    uint16_t index;     //
    };


extern void MSCP_poll();


#endif // MSCP_H
