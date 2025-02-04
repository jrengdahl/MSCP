This project contains a board design and firmware for a Qbus MSCP SSD board.

The problem this is intended to solve is the lack of hard drives
compatible with PDP-11 computers. The boards built by DEC seem to last
forever, and can be powered by modern PC power supplies, but the hard
drives of that era are 40+ years old, and finding one of any size that
still works, and is affordable, is both difficult and risky. There are
hard drive emulators available for industrial users, but these are
expensive. These solutions typically involve a board with a MFM or SCSI
interface that emulates a drive, and have to be used with a legacy hard
drive controller such as an RQDX3, or a SCSI interface board, which
itself can be expensive. I did buy an MFM drive emulator, but have been
unable to get it working with my UDC11 hard drive controllers. 

At present this is very much in development. I have been able to boot a
small test program from the SSD on a KDJ11D (PDP-11/53).

The current firmware is based on my H503 project. For now the firmware
consists of a command line interpreter which will implement commands to
check out the hardware on the board. Like the H503 project, building the
firmware requires my version of GCC which implements OpenMP for bare
metal mcrocontrollers.

The board contains:

-- an STM32H723ZG microcontroller
   -- ARM Cortex-M7 CPU
   -- clock speed up to 550 MHz
   -- 1 megabyte of on-chip flash
   -- 320 K of contiguous on-chip RAM, plus several chunks of smaller sizes 
   -- several SPI interfaces
   -- an FMC external memory interface, which is a multiplexed
      bus with 23 bits of address and 16 bits of data,
      connected to the FPGA

-- a Efinix Trion T8Q144 FPGA. The FPGA will handle the logic of
   interfacing QBus to the microcontroller.

-- a 16 megabyte SPI NOR flash memory, mainly to hold the
   FPGA bitstream.

-- two SD card slots to contain the user data

-- a QBus interface that should be capable of both slave and master
   operation. The interface uses 3.3 volt, 5 volt tolerant LVC logic for
   bus receivers, and MOSFETS for bus drivers.

-- debug aids:
   -- a SWD/trace connector for a Segger Jtrace Pro M
   -- a USB virtual terminal interface
   -- a handful of LEDs
   -- any unused pins on the uC and FPGA are brought out to test points

The board is a half-wide QBus board, the same size as a KDJ11-A
or RQDX3.

The board was designed with KiCAD, and manufactured by JLCPCB. The cost
for the first two prototype boards fabricated, populated, and soldered
was around $200. This is my first PCB project of this size and
complexity. (Previous boards were Arduino Nano sized.) I expect I will
have to turn the board a couple times before I get everything right. The
target cost of the final board in lots of 25 will be around $50. 

I found online documents describing the MSCP protocol, and the source
code for the RQDX3 and SIMH. I have implemented the startup and MSCP
read command, and have been able to boot and run a test program from the
SSD into the PDP-11/53 CPU.

