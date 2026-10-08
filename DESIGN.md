# DESIGN.md — MSCP SSD for Q-bus PDP-11

**Status:** Working design record  
**Last updated:** 2026-10-07

This document records established design direction, known constraints, and unresolved questions for the MSCP solid-state disk controller project. It is intentionally conservative: items not yet decided are marked as such rather than filled in by assumption.

---

## 1. Project goal

Develop a solid-state mass-storage controller that appears to a Q-bus PDP-11 as an MSCP disk controller.

The implementation must provide:

- the Q-bus-facing port behavior required by UQSSP / the U/Q Storage Systems Port protocol,
- the MSCP command/response behavior required by the host operating system,
- reliable block storage implemented on modern nonvolatile memory,
- behavior compatible with real PDP-11 software rather than merely with one emulator.

The RQDX3 is an important reference implementation because it is a real DEC Q-bus MSCP controller.

---

## 2. Protocol layering

Keep the design divided into two logical layers.

### 2.1 UQSSP / port layer

Responsibilities include, as applicable:

- IP and SA register behavior,
- controller initialization handshake,
- communications-area layout,
- command descriptor ring,
- response descriptor ring,
- ownership/interlock rules,
- polling,
- interrupts,
- DMA access to host memory,
- message envelopes / transport,
- port fatal-error handling.

The primary documentary baseline is:

`doc/AA-L621A-TK.txt`

AA-L621A-TK is titled *Storage System UNIBUS Port Description*. It is being used as the closest surviving DEC port specification, with Q-bus-specific behavior cross-checked against the RQDX3 implementation and other Q-bus evidence.

Do not assume that every UNIBUS-specific detail transfers unchanged to Q-bus.

### 2.2 MSCP layer

Responsibilities include:

- controller and unit state,
- MSCP command decoding,
- command sequencing rules,
- response generation,
- controller/unit characteristics,
- read/write operations,
- error/status reporting,
- MSCP flow-control semantics where applicable.

The primary documentary baseline is:

`doc/AA-L619A-TK.txt`

Keep MSCP semantics independent from the mechanics of the Q-bus port whenever practical.

---

## 3. Reference implementations and evidence

The following are useful sources, but they do not all have the same authority.

### Primary

- DEC AA-L619A-TK — MSCP Basic Disk Functions
- DEC AA-L621A-TK — Storage System UNIBUS Port Description
- Original RQDX3 firmware/source material

### Secondary

- SIMH PDP-11 MSCP/UQSSP implementation
- Other modern Q-bus MSCP emulator implementations
- Reverse-engineering notes produced during this project

When documenting a behavior discovered in RQDX3 source, classify it as one of:

1. required by the DEC specification,
2. an RQDX3 implementation choice,
3. a likely Q-bus/UQSSP rule inferred from RQDX3,
4. uncertain.

This distinction is important because the project is implementing a compatible controller, not cloning every incidental detail of the RQDX3.

---

## 4. Development environment

The project is developed on Windows.

Normal workflow:

- `cmd.exe` is the primary command shell.
- Cygwin command-line utilities may be on `PATH`, but Cygwin `bash` is not normally used.
- Git is Git for Windows.
- Git operations are normally performed from the command line.
- git-cola may be used as a GUI.
- The repository may be pushed to GitHub by the user.
- Some embedded projects use STM32CubeIDE managed builds.
- Other embedded projects use simple hand-written Makefiles.
- CMake is intentionally not part of the workflow.
- ARM cross-compilers may be locally built with crosstool-ng.

If this repository is an STM32CubeIDE managed-build project, generated makefiles are not authoritative source files.

---

## 5. Storage device

The current NOR flash device is:

**Winbond W25Q128JVS**

Relevant geometry:

| Term | Size | Meaning |
|---|---:|---|
| Flash page | 256 bytes | Page-program granularity |
| Flash erase sector | 4096 bytes | Smallest erase granularity |
| Flash erase block | 32 KiB | Optional larger erase unit |
| Flash erase block | 64 KiB | Optional larger erase unit |
| Total device capacity | 16 MiB | 128 Mbit |

Important NOR behavior:

- Programming can change bits from `1` to `0`.
- Returning a programmed `0` to `1` requires erasing the containing erase sector.
- A page-program operation must respect the 256-byte page boundary.
- The 4 KiB erase sector is therefore the fundamental unit that constrains update strategy.

---

## 6. FatFS terminology and mapping

FatFS uses the word **sector** for a logical disk sector. The W25Q128JV data sheet also uses **sector** for a 4 KiB erase unit.

These are different concepts and must not be conflated.

Use:

- **FatFS logical sector** for the unit presented through `diskio`.
- **flash erase sector** for the W25Q128JV 4 KiB erase unit.
- **flash page** for the 256-byte program unit.
- **flash erase block** for the 32 KiB or 64 KiB larger erase units.

The FatFS logical-sector size must be verified from the project configuration before assuming 512 bytes.

If the configured FatFS logical-sector size is 512 bytes, then:

- one 4 KiB flash erase sector contains eight FatFS sectors,
- one 512-byte FatFS sector spans two flash pages.

Updating a logical sector may therefore require a read-modify-erase-program operation on the containing 4 KiB flash erase sector whenever any bit must transition from `0` back to `1`.

### Not yet decided

The exact strategy for handling partial updates has **not** yet been recorded as a final design decision.

Possible approaches include:

- a single 4 KiB RAM shadow buffer,
- a small sector cache,
- copy-on-write / log-structured storage,
- a flash translation layer,
- another scheme chosen after considering endurance and failure recovery.

Do not assume one of these has been selected.

---

## 7. Persistence, wear, and failure recovery

The usage of the SPI-NOR device will be low, and will primarily be used to store FPGA bitstreams used at boot time.
Write performance of the SPI-NOR is not a primary concern. For example, when writing or erasing a FATFS sector, it is expected
that the entire flash sector may have to be read, modified, and written back.
The SD cards will be where MSCP storage will be located.
Performance of the SD cards should be as good as possible without creating complexity.

For now, wear leveling, flash error handing, bad sector remapping, and the like will not be implemented.
Assume that the flash devices are good, and the lifetimes will not be exceeded. 

These items remain design questions unless implemented and documented elsewhere:

- expected write rate,
- required flash endurance,
- wear leveling,
- power-loss safety,
- metadata journaling,
- recovery after interrupted erase/program,
- bad-flash handling,
- allocation of spare erase sectors,
- whether FatFS directly occupies the NOR address space or sits above a translation layer.

Any future decision here should be added explicitly to this document because it affects both reliability and the `diskio` implementation.

---

## 8. Q-bus hardware interface

The Q-bus interface is implemented an Efinex FPGA. The FPGA programming is described in System Verilog in
qbus/qbus.sv. (Note that I am still learning System Verilog, so I am using only a subset of SV features).

The PC Board design is is a KiCad project in subdirectory QBusH723Z-JLCPCB-0.1.

Do not modify the PC board.

---

## 9. UQSSP initialization

The implementation should eventually document the controller side of the initialization sequence step by step, cross-referenced to AA-L621A-TK and the RQDX3 source.

At minimum, the design record should eventually state:

- IP reset behavior,
- SA values for initialization steps 1 through 4,
- host-provided parameters accepted at each step,
- communications-area base-address formation,
- response/command ring sizes,
- interrupt/vector selection,
- Q-bus-specific address bits,
- transition to normal operation,
- fatal-error behavior during initialization.

**Status:** detailed controller-side sequence not yet transcribed into this design document.

---

## 10. Command and response rings

The implementation must preserve the semantics of the DEC port protocol, including:

- descriptor ownership,
- ring ordering,
- wraparound,
- command polling,
- response availability,
- interrupt conditions,
- descriptor/address alignment,
- packet/message layout,
- DMA ordering requirements.

The exact software data structures and concurrency strategy are not yet documented here.

When implementing or reviewing them, pay special attention to race conditions between:

- the PDP-11 host,
- Q-bus DMA/bus-master activity,
- MCU interrupt handlers,
- background storage processing.

---

## 11. MSCP scope

The minimum MSCP command subset required for useful operation should be based on:

1. AA-L619A-TK,
2. the requirements of the target PDP-11 operating systems,
3. observed RQDX3 behavior.

Do not add commands merely because another emulator implements them.

The MSCP implementation shall be a subset required by normal user operation of
RT-11 and the PDP-11 version of 2.11 BSD UNIX.
Special disk operations such as formatting and testing are not required.
Support for XXDP is not required.

---

## 12. Compatibility targets

The project goal is compatibility with real Q-bus PDP-11 software.

Specific operating systems and diagnostic programs to be used for acceptance testing have not yet been formally listed in this document.

Testing will be done with
-- small bare-metal test programs run on a PDP-11/53.
-- I want to be able to boot RT-11 and 2.11 BSD, but I have not tried them yet.

---

## 13. Naming conventions

Prefer names that expose protocol or hardware meaning.

Examples:

```c
FLASH_PAGE_SIZE
FLASH_ERASE_SECTOR_SIZE
FLASH_ERASE_BLOCK_SIZE
FATFS_SECTOR_SIZE
```

Avoid ambiguous names such as:

```c
SECTOR_SIZE
BLOCK_SIZE
```

unless the scope makes the meaning unmistakable.

Similarly, distinguish terms such as:

- MSCP logical block,
- FatFS logical sector,
- flash erase sector,
- flash erase block,
- UQSSP command descriptor,
- MSCP command packet.

---

## 14. Open design questions

The following should be resolved and recorded as the project progresses:

- Q: What MCU/FPGA hardware partition is used for the Q-bus interface?
  A: The Q-bus registers are implemented in the FPGA. The microcontroller accesses its side of the registers via the FMC bus which is connected to the FPGA.

- Q: How are Q-bus bus-master/DMA cycles implemented?
  A:      DMA is implemented in microcontroller firmware.

- What UQSSP behavior differs materially between the UNIBUS description and the Q-bus RQDX3?
- What exact MSCP subset is required for the initial usable controller?
- Q: What logical sector size is configured in FatFS?
  A: 512 bytes.

- What flash update/caching/translation scheme will be used?
- Q: What power-loss guarantees are required?
  A: for now, ignore power-loss.

- Q: Is wear leveling necessary for the expected workload?
  A: For now, ignore wear leveling.

- Q: How will the project expose one or more MSCP units?
  A: There will be two SD cards. Each card may implement one or more MSCP units.
     Each MSCP drive will be represented as an image stored in a single FATFS file on an SD card.
     The FATFS filename will reflect the unit number.

- What unit/media identifiers will be reported?
- What is the initial virtual-disk capacity and geometry policy?
- What diagnostics will be implemented?
- Which PDP-11 operating systems will form the acceptance-test matrix?

---

## 15. Design-change practice

When a design question is resolved:

1. Record the decision here.
2. Record the reason, especially where several plausible alternatives existed.
3. Note any compatibility consequence.
4. Identify the code/modules affected.
5. Keep unresolved questions explicitly unresolved rather than letting implementation accidents become undocumented design decisions.
