# AGENTS.md — MSCP SSD Project

## Project purpose

This repository contains an experimental solid-state MSCP disk controller for Q-bus PDP-11 systems.

From the PDP-11 side the project has two distinct protocol layers:

1. **UQSSP / U/Q Storage Systems Port** — the Q-bus port protocol: initialization, IP/SA register behavior, communications area, command and response descriptor rings, polling, interrupts, DMA, and message transport.
2. **MSCP** — the higher-level mass-storage command/response protocol carried over the port.

Keep these layers conceptually separate when analyzing or changing the code.

## Primary sources of truth

Use sources in this order unless the user explicitly says otherwise:

1. `DESIGN.md` for established project design decisions.
2. DEC primary documentation in the repository, especially:
   - `doc/AA-L619A-TK.txt` — MSCP Basic Disk Functions
   - `doc/AA-L621A-TK.txt` — Storage System UNIBUS Port Description
3. Original DEC RQDX3 firmware/source material, is present in the repository under rqdx3.new. This directory
   contains the RQDX3 source code available on the web, with the following modifications in .mac files:
   -- in names containing a period, the period is replaced by underscrore.
   -- .global statements have been placed before subroutines.
4. Existing project source code and comments.
5. Secondary implementations such as SIMH or other MSCP emulators.

If two sources disagree, do not silently choose one. Report the conflict and identify the sources.

Do not treat comments, emulator behavior, or generated code as more authoritative than the DEC specifications without a specific reason.

## Working style

- Diagnose before modifying.
- For build failures, first reproduce the failure and identify the root cause.
- Explain the proposed fix before making broad or architectural changes.
- Prefer small, reviewable changes.
- Do not refactor unrelated code while fixing a specific problem.
- Do not commit to Git.
- Do not push to any remote.
- Leave the working tree in a state that can be reviewed with `git diff`.
- Do not delete or overwrite reference material.

When asked only to investigate, do not change files.

## Development environment

The normal host development environment is Windows.

- The primary shell is traditional `cmd.exe`, not PowerShell and not Cygwin `bash`.
- Cygwin utilities may be present on `PATH` and may be invoked from `cmd.exe`.
- Git is Git for Windows from git-scm, not Cygwin Git.
- Git repositories push to GitHub.
- This project uses STM32CubeIDE managed/auto-generated makefiles, invoked using "make -f ../Makefile".
  The top level Makefile create dummy .cyclo files which would otherwise be missing due to the used-built GCC cross compiler not supporting cyclomatic complexity.
  However, the auto-generated makefiles under Debug assume the .cyclo file are present, thus the top
  level makefile makes sure the dummy .cyclo files exists, then it calls the auto-generated makefile in Debug.
- Do not introduce CMake.
- Do not replace the existing build system merely to simplify tool operation.
- ARM cross-compilers were built locally with crosstool-ng. Determine which `arm-none-eabi-*` tools are actually being used before drawing conclusions from compiler behavior.
- Though the project creates a bare-metal image, the cross compiler does support -fopenmp.

## STM32CubeIDE managed-build projects

This repository is an STM32CubeIDE managed-build project:

- Treat `.project`, `.cproject`, linker scripts, `.ioc`, and source files as project inputs.
- Treat generated files under directories such as `Debug/` and `Release/` as build artifacts unless the repository clearly uses them differently.
- Do **not** make persistent fixes by editing generated `makefile`, `sources.mk`, `subdir.mk`, `objects.list`, or similar files.
- Build-setting changes normally belong in STM32CubeIDE / `.cproject`, not in generated makefiles.
- Do not hand-edit `.cproject` unless explicitly asked or unless the change is very well understood and the user has approved that approach.
- Preserve STM32Cube-generated `/* USER CODE BEGIN */` / `/* USER CODE END */` regions.
- Do not alter `.ioc` or regenerate CubeMX code unless explicitly requested.

## Protocol-analysis rules

When working on UQSSP or MSCP:

- Use DEC terminology where practical.
- Distinguish **port-driver behavior** from **MSCP server behavior**.
- Distinguish host-visible protocol rules from implementation details of the RQDX3.
- Do not infer a Q-bus rule merely from a UNIBUS-specific statement in AA-L621A-TK; cross-check against Q-bus/RQDX3 behavior.
- When reverse-engineering RQDX3 code, identify which conclusions are:
  - explicitly specified by DEC documentation,
  - directly demonstrated by the firmware,
  - inferred from the firmware,
  - or still uncertain.
- Preserve PDP-11/Q-bus byte ordering, word ordering, and bus-address semantics exactly.
- Be especially careful with ownership bits, interrupt transitions, ring wraparound, initialization handshakes, and DMA address width.

## Flash-storage terminology

The project uses a Winbond **W25Q128JVS** QSPI NOR flash device.

Use unambiguous terminology:

- **Flash page**: 256 bytes; page-program granularity.
- **Flash erase sector**: 4096 bytes (4 KiB); smallest erase granularity.
- **Flash erase block**: 32 KiB or 64 KiB erase region.
- **FatFS logical sector**: logical disk sector presented to FatFS; verify the configured size in this project before assuming a value.

Never use a bare name such as `SECTOR_SIZE` when the meaning could be confused between a FatFS logical sector and a flash erase sector.

Prefer names such as:

```c
FLASH_PAGE_SIZE
FLASH_ERASE_SECTOR_SIZE
FLASH_ERASE_BLOCK_SIZE
FATFS_SECTOR_SIZE
```

Remember that NOR programming changes bits from 1 to 0; changing a programmed 0 back to 1 requires erasing the containing erase sector.

Do not invent a wear-leveling, journaling, caching, or flash-translation design unless `DESIGN.md` establishes it or the user asks to design one.

## Repository exploration

Avoid consuming time and context by recursively reading large vendor or generated trees without need.

Start with:

- top-level build files,
- `DESIGN.md`,
- project-owned source and headers,
- linker scripts,
- protocol-specific source,
- the exact files named by compiler/linker diagnostics.

Read CMSIS, STM32 HAL, FatFS internals, generated build output, or large DEC source trees only when needed to answer the specific question.

## Build/debug workflow

For a build problem:

1. Identify the active build system.
2. Identify the exact compiler/linker executable being used.
3. Reproduce the failure if permitted.
4. Read the complete relevant diagnostic.
5. Trace the problem to the minimum relevant set of files.
6. Explain the root cause.
7. Propose the smallest fix.
8. Modify files only if requested or clearly permitted.
9. Rebuild if useful.
10. Show what changed.

For linker problems, use map files and tools such as `arm-none-eabi-nm`, `size`, `objdump`, and `readelf` when they materially clarify the issue.

## Design documentation

Before making an architectural decision, read `DESIGN.md`.

When the user explicitly establishes or changes an architectural decision, propose an update to `DESIGN.md`. Do not silently rewrite design history.

If the code disagrees with `DESIGN.md`, report the discrepancy rather than assuming either is correct.
