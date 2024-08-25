#ifndef MSCP_H
#define MSCP_H

#include <stdint.h>

typedef short word;
typedef unsigned short uword;
typedef unsigned char byte;

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

#define I_OPCD          (8 << 8)                        /* inv opcode */
#define I_FLAG          (9 << 8)                        /* inv flags */
#define I_MODF          (10 << 8)                       /* inv modifier */
#define I_BCNT          (12 << 8)                       /* inv byte cnt */
#define I_LBN           (28 << 8)                       /* inv LBN */
#define I_VRSN          (12 << 8)                       /* inv version */
#define I_FMTI          (28 << 8)                       /* inv format */




struct longword
    {
    uword lo;
    word hi;
    };

struct unit_identifier
    {
    long serno_lo;
    word serno_hi;
    byte model;
    byte dev_class;
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
        byte bytes[48];                     // force size to be 64 bytes

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

        // read parameters
        struct
            {
            long        bytecount;
            struct
                {
                long    buffer_address;
                long                : 32;
                long                : 32;
                };
            long LBN;
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

        // online parameters
        struct
            {
            word        multiunit_code;
            word        unit_flags;

            struct
                {
                byte spndles;
                long                : 24;
                };

            struct unit_identifier id;
            long        media_type_identifier;
            long                    : 32;
            struct
                {
                uword lo;
                word hi;
                }unit_size;                      // size in LBNs
            long        volume_serial_number;    // optional, often zero
            };

        // read parameters
        struct
            {
            long                    : 32;
            long                    : 32;
            long                    : 32;

            long        first_bad_LBN;
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




#endif // MSCP_H
