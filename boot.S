    .org 173000

    reply_packet = 7004
    cmd_packet = 7104 
    comm_area = 7204

// Four step init process

start:
        mov     $1000,sp        // point the sp to somewhere we know there is RAM, and this program is not there
        mov     $unit,r0        // the disk unit number, default 0
        mov     $172150, r1     // address of the IP register
        mov     $table, r4      // address of the table of constants    
        mov     $4000,r5        // s1 mask, this will become s2, s3, and s4 as it is shfted left in the loop
        mov     r1,r2           // get the address of the SA register
        clr     (r2)+           // write to IP, resetting the controller, and increment r2 to point to SA

        // r0   unit number
        // r1   address of IP register
        // r2   address of SA register
        // r3   scratch
        // r4   pointer into table of constants
        // r5   step mask

        // the four-step init loop
10$:    mov     (r2), r3        // get SA register
        bmi     60$             // if the error bit is set, jump to halt
        bit     r5,r3           // test the SA value with the step mask to see if the controller has written the currently expected step to SA
        beq     10$             // if not the expected step yet, then loop, waiting for either an error or the next step
        mov     (r4)+,(r2)      // write the host response to the SA register
        asl     r5              // shift to get the mask for the next step
        bpl     10$             // loop until completion of step 4, as indicated by shifting of mask to bit 15


/* Send ONL, READ commands */

30$:    tstb    (r4)            // check for end of table of constants
        beq     done            // 0 = yes

        // loop to clear the packets, leaving r2 pointing to the comm area
        mov     $reply_packet-4,r2  // clr pkts
40$:    clr     (r2)+
        cmp     r2,#comm_area
        blo     40$

        // r0   unit number
        // r1   address of IP register
        // r2   pointer to reply and command FIFOs (each with only one entry)
        // r3   scratch
        // r4   pointer into table of constants, pointing to the start of the command table
        // r5   contains 100000 (bit 15 set) from the final asl, which can be used to set the OWNER bit in packet descriptors 


        // setup an online or read command and reply
        // for the source LBN and the the buffer will both be zero
        movb    (r4)+,cpkt-4    // set command packet length from the table into the UQSSP header (-4)
        movb    r0,cpkt+4       // set the unit number in the command packet
        movb    (r4)+,cpkt+10   // set command opcode from the table
        movb    (r4)+,cpkt+15   // set the param (bits 15-8 of bytecount for read)
        mov     $rpkt,(r2)+     // save the address of the reply buffer as the low word of the reply descriptor
        mov     r5,(r2)+        // save r5, with the OWNER bit set, as the high word of the reply descriptor
        mov     $ckpt,(r2)+     // save the address of the command buffer as the low word of the reply descriptor
        mov     r5,(r2)         // save r5, with the OWNER bit set, as the high word of the command descriptor
        cmp     -(r2),-(r2)     // restore r2 back to start of FIFOs (decrement r2 by 4 (using a one-word instruction that only affects the condition)
        tst     (r1)            // wake ctrl by reading the IP register

50$:    tst     (r2)            // see if controller has returned the reply buffer to the host
        bmi     50$             // as long as the reply is still owned by te controller
        tst     rpkt+12         // check the reply status word
        beq     30$             // if status was zero then go sent the next message
$60:    halt                    // halt on error

// Boot block read in, jump to 0

done:   clr     (r1)            // re-initilize the controller (for M+, which I assume refers to RSX-11M+) 
        clr     r3              // some requirement for the second state boot?
        mov     $st+020,r4      // this is the address of the table of constants
        clr     r5              //

        // r0   unit number
        // r1   address of IP register, i.e. the boot device's CSR
        // r2   pointer to reply and command FIFOs
        // r3   0
        // r4   pointer to the table of constants
        // r5   0

        clr     pc              // jump to zero, the start of the second stage boot loader

 /* Data */


table:
        // table of four responses to the controller's prompts during the four-step init process
        .word   0100000         // no interrupts, no vector, ring size = 1, "valid" bit set
        .word   comm_area       // the address of the third word of the communication area
        .word   0000000         // the upper bits of the address of the communication area
        .word   0000001         // the GO bit, but don't bother with error packets


        // command table, contains constants for the commands required to load the next stage bootloader
        // online command
        .byte   20              // online packet length for the UQSSP header (apparently UQSSP can truncate bytes that are zero at the end of a packet)
        .byte   11              // online command
        .byte   0               // reserved byte of online cmd, must be zero (see size field of read command) 

        // constant for the read command
        .byte   40              // read packet length for the UQSSP header (32 bytes, up though LBN)
        .byte   41              // read command
        .byte   2               // read parameter, bits 15-8 of bytecountm i.e. 512 bytes

        // end of table flag, the first byte of all previous command table entries must be nonzero
        .word   0
