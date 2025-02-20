#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "QSPI.h"    // QSPI interface functions
#include "serial.h"  // serial communication functions
#include "local.h"
#include "tim.h"


static const int NHIST = 50;
static int hist[NHIST];
static uint32_t h_start = 0;

static void hstart()
    {
    h_start = __HAL_TIM_GET_COUNTER(&htim2);
    }

static void hstop()
    {
    uint32_t now = __HAL_TIM_GET_COUNTER(&htim2);
    uint32_t elapsed = now - h_start;
    uint32_t index = elapsed/1000;
    if(index >= NHIST)index=NHIST-1;
    ++hist[index];
    }

static void phist()
    {
    for(int i=0; i<NHIST; i++)
        {
        printf("%4d: %13d\n", i, hist[i]);
        }
    }



// ASCII communication characters used by the Xmodem protocol
#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

#define PACKET_SIZE 128         // size of an XMODEM packet

static const unsigned FIRST_TIMEOUT = 3'000'000;
static const unsigned LATER_TIMEOUT = 50'000;

static uint8_t xbuffer[PACKET_SIZE + 5]; // Packet buffer (with header, block number, and checksum)


// routine to put a raw character to the console
static inline void putx(uint8_t ch)
    {
    _write(1, (const char *)&ch, 1);
    }


// discard input bytes until a timeout occurs
// this is part of recovering from a protocol error
// if we get confused in the middle of a packet, we want to wait until the host stops sending before we send a NAK and start processing input
static void xflush()
    {
    int c;                                  // the character we got from the input stream

    do
        {
        c = __io_getchart(LATER_TIMEOUT);
        }
    while(c != -1);
    }



// xmodem_receive
// Receive a file from the console via Xmodem, and write it to the SPI-NOR starting at address 0
// In some cases the file received is a mass storage device image, possibly in FATFS format.
// input: qbuffer, a pointer to a 512 byte buffer
// return: a status code, 0 means OK

// functions used:
// int __io_getchart(unsigned timeout)  get a raw console character, with timeout in microseconds.
//                                      returns a character (0-255) or -1 for timeout


int xmodem_receive(uint8_t *qbuffer)
    {
    uint32_t packet = 1;                        // packet counter
    int tries = 10;                             // retry counter
    int c;                                      // the character we got from the input stream
    uint32_t address = 0;                       // address of the next block in the SPI NOR
    int qi = 0;                                 // index into the qbuffer
    uint8_t checksum = 0;                       // packet checksum
    unsigned timeout = FIRST_TIMEOUT;           // initial value: allow 30 seconds (3 * 10) to get XMODEM started in TeraTerm
    int duplicates = 0;
    int outoforder = 0;
    int badseq = 0;
    int badchecksum = 0;
    int packet_timeouts = 0;
    int char_timeouts = 0;
    int packet_starts = 0;

    SerialRaw = true;                           // tell the console to ignore special characters such as control-C and newline

    while(tries-- > 0)
        {
        hstart();
        c = __io_getchart(timeout);             // get first byte
        hstop();

        if(c == -1)                             // if timeout,
            {
            putx(NAK);                          // on timeout request to (re)send the packet
            ++packet_timeouts;
            }

        else if (c == SOH)                      // start receiving a packet
            {
            ++packet_starts;
            if(timeout == FIRST_TIMEOUT)
                {
                tries = 10;
                packet_timeouts = 0;
                timeout = LATER_TIMEOUT;        // once a packet has been started, set timeout shorter
                }

            checksum = 0;                       // init the packet checksum

            for (int i = 0; i < PACKET_SIZE + 3; i++) // for each character in the packet
                {
                hstart();
                c = __io_getchart(timeout);     // get the next character
                hstop();
                if(c == -1)break;               // abort packet if timeout
                xbuffer[i] = c;                 // save the data in the packet buffer
                if(i>1 && i<PACKET_SIZE+2)      // if this is a header or payload byte
                    {
                    checksum += xbuffer[i];     // add it to the checksum
                    }
                }

            if(c == -1)
                {
                xflush();                           // wait until the host stops sending data
                putx(NAK);                          // on timeout request to (re)send the packet
                ++char_timeouts;
                continue;
                }

            // now that a complete packet has been received
            // Validate packet (checksum, block number, etc.)

            if (xbuffer[1] != 255-xbuffer[0])       // verify the packet number check
                {
                xflush();                           // wait until the host stops sending data
                putx(NAK);
                ++badseq;
                continue;
                }

            if (xbuffer[PACKET_SIZE + 2] != checksum) // verify the checksum
                {
                xflush();                               // wait until the host stops sending data
                putx(NAK);
                ++badchecksum;
                continue;
                }

            if (xbuffer[0] == ((packet-1) & 255))       // if packet is a repeat of the last packet, just ACK it
                {
                putx(ACK);
                ++duplicates;
                continue;
                }

            if (xbuffer[0] != (packet & 255))           // check the packet number
                {
                xflush();                               // wait until the host stops sending data
                putx(NAK);
                ++outoforder;
                continue;
                }

            // at this point we have received a good expected packet

            // Copy received data to page buffer
            for (int i = 0; i < PACKET_SIZE; i++)
                {
                qbuffer[qi++] = xbuffer[i + 2];
                }

            // If the sector buffer is full write it to the SPI NOR.
            // It is the host's responsibility to ensure that the download size is an integral number of sectors.
            // This is a valid assumption since the download is a FATFS or other file system image.
            if (qi >= QSPI_PAGE_SIZE)
                {
                // Write the full page to QSPI flash
                if (QSPI_WritePage(&hospi1, address, qbuffer, QSPI_PAGE_SIZE) != HAL_OK)
                    {
                    SerialRaw = false;
                    printf("bad write\n");
                    return -1;
                    }
                address += QSPI_PAGE_SIZE;  // increment to the next sector address
                qi = 0;                     // and reset the data index to the beginning of the sector buffer
                }

            putx(ACK);                          // Acknowledge packet reception
            packet++;                           // count the packet
            tries = 10;                         // reset try counter
            }

        else if (c == EOT)
            {
            putx(ACK);                          // End of transmission
            SerialRaw = false;
            printf("file received OK\n");
            printf("packet count = %ld\n", packet);
            printf("packet starts = %d\n", packet_starts);
            printf("packet timeouts = %d\n", packet_timeouts);
            printf("character timeouts = %d\n", char_timeouts);
            printf("badchecksum = %d\n", badchecksum);
            printf("duplicates = %d\n", duplicates);
            printf("badseq = %d\n", badseq);
            printf("outoforder = %d\n", outoforder);
            phist();
            return 0;                        // Success
            }
        }

    // If retries are exhausted, send CAN and abort
    xflush();                           // wait until the host stops sending data
    putx(CAN);
    putx(CAN);
    putx(CAN);
    SerialRaw = false;
    printf("xmodem transfer failed\n");
    printf("packet count = %ld, qi = %d, checksum = %02x\n", packet, qi, checksum);
    printf("last packet = %d, expected %d\n", xbuffer[0], (int)(packet&255));
    printf("packet starts = %d\n", packet_starts);
    printf("packet timeouts = %d\n", packet_timeouts);
    printf("character timeouts = %d\n", char_timeouts);
    printf("badchecksum = %d\n", badchecksum);
    printf("duplicates = %d\n", duplicates);
    printf("badseq = %d\n", badseq);
    printf("outoforder = %d\n", outoforder);
    dump(xbuffer, PACKET_SIZE + 3);
    phist();
    return -1;
    }


