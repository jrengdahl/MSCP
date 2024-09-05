
typedef short word;
typedef unsigned short uword;
typedef unsigned char byte;

struct longword
    {
    uword lo;
    word hi;
    };


#define COMPILE_BARRIER  __asm__ __volatile__("":::"memory")
#define halt()  __asm__ __volatile__("halt")


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
        byte bytes[48];                     // force size of UQSSP + MSCP to be 64 bytes

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
            struct longword  bytecount;
            struct longword  buffer_address;
            long                : 32;
            long                : 32;
            struct longword  LBN;
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
            byte        spndles;
            long                    : 24;

            struct unit_identifier id;                  // (8 bytes)
            long        media_type_identifier;
            long                    : 32;
            struct longword  unit_size;               // size in LBNs
            long        volume_serial_number;    // optional, often zero
            };

        // read parameters
        struct
            {
            long                    : 32;   // (byte count)
            long                    : 32;   // (descriptor)
            long                    : 32;
            long                    : 32;

            long        first_bad_LBN;      // first bad block
            };
        };
    };


struct descriptor
    {
    word lo;
    word hi;
    };

struct FIFOs
    {
    struct descriptor rsp;
    struct descriptor cmd;
    };


typedef void FN();

#define ZERO (*(FN *)0)

#define IP *(word volatile *)0172150
#define SA *(word volatile *)0172152

#define BASE 040000
#define RSP_PACKET (BASE + 0004)
#define CMD_PACKET (BASE + 0104)
#define COMM_AREA  (BASE + 0204)



extern void do_step(word mask, word response);
extern void clear_memory();
extern void send_and_wait();


