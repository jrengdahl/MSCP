start:
        mov     $160000,sp      // point the sp to somewhere we know there is RAM, and this program is not there
        jmp     _boot           // start the C code
