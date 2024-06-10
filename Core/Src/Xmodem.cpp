#include <stdint.h>
#include <stdio.h>
#include "serial.h"  // Your serial communication functions
#include "QSPI.h"    // Your QSPI interface functions

#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
#define PACKET_SIZE 128

#define R_PACKET 1
#define R_IPACKET 2
#define R_CHECKSUM 3
#define R_WRITE 4
#define R_TIMEOUT 5


static uint8_t xbuffer[PACKET_SIZE + 5]; // Packet buffer (with header, block number, and checksum)

static inline void putx(uint8_t ch)
    {
    _write(1, (const char *)&ch, 1);
    }

int xmodem_receive(uint8_t *qbuffer)
    {
    uint8_t packet = 1;
    int tries = 10;
    int c;
    uint32_t address = 0;
    int qi = 0;
    uint8_t checksum;
    unsigned timeout = 3'000'000;                   // allow 30 seconds (3 * 10) to get XMODEM started in TeraTerm
    int reason = 0;

    SerialRaw = true;

    while(tries-- > 0)
        {
        c = __io_getchart(timeout);             // get first byte
        if(c == -1)
            {
            reason = R_TIMEOUT;
            putx(NAK);                          // on timeout request to (re)send the packet
            }
        else if (c == SOH)                      // Receive a packet
            {
            timeout = 10'000;                  // once a packet has been received, set timeout shorter
            checksum = 0;
            for (int i = 0; i < PACKET_SIZE + 3; i++)
                {
                xbuffer[i] = __io_getchart(timeout);
                if(i>1 && i<PACKET_SIZE+2)
                    {
                    checksum += xbuffer[i];
                    }
                }

            // Validate packet (checksum, block number, etc.)
            if (xbuffer[0] != packet)
                {
                reason = R_PACKET;
                putx(NAK);
                continue;
                }

            // Validate packet (checksum, block number, etc.)
             if (xbuffer[1] != 255-packet)
                 {
                 reason = R_IPACKET;
                 putx(NAK);
                 continue;
                 }

             // Validate packet (checksum, block number, etc.)
              if (xbuffer[PACKET_SIZE + 2] != checksum)
                  {
                  reason = R_CHECKSUM;
                  putx(NAK);
                  continue;
                  }

            // Copy received data to page buffer
            for (int i = 0; i < PACKET_SIZE; i++)
                {
                qbuffer[qi++] = xbuffer[i + 2];
                }

            if (qi >= QSPI_PAGE_SIZE)
                {
                // Write the full page to QSPI flash
                if (QSPI_WritePage(&hospi1, address, qbuffer, QSPI_PAGE_SIZE) != HAL_OK)
                    {
                    reason = R_WRITE;
                    break;
                    }
                address += QSPI_PAGE_SIZE;
                qi = 0;
                }

            putx(ACK);                          // Acknowledge packet reception
            packet++;
            tries = 10;                         // reset try counter
            }

        else if (c == EOT)
            {
            putx(ACK);                          // End of transmission
            SerialRaw = false;
            return 0;                           // Success
            }
        }

    // If retries are exhausted, send CAN and abort
    putx(CAN);
    putx(CAN);
    putx(CAN);
    SerialRaw = false;
    return reason;
    }
