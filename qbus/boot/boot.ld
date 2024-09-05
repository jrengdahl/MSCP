OUTPUT_FORMAT("a.out-pdp11") 

ENTRY(_boot)

_memory_start = 0171000;
_memory_end   = 0172000;

MEMORY
{
  ROM    (xrw)    : ORIGIN = 0171000,   LENGTH = 512
  RAM    (xrw)    : ORIGIN = 0040000,   LENGTH = 1024
}

SECTIONS
{
  .text :
  {
    start.o(.text)
    boot.o(.text)
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */

    . = ALIGN(2);
    _etext = .;        /* define a global symbols at end of code */
  } >ROM

  .rodata :
  {
    . = ALIGN(2);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(2);
  } >ROM

  .data :
  {
    . = ALIGN(2);
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
  } >RAM

  . = ALIGN(2);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(2);
    _ebss = .;         /* define a global symbol at bss end */
  } >RAM

}
