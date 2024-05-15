MyLib

These routines are replacements for some common library routines. They
are intended for use in very small firmwares on microprocessors with
limited firmware space, such as 64 Kbytes. The routines have the
following properties:

-- They are smaller than the versions included in glibc or newlib.

-- They do not pull through other library routines.

-- They are so simple that they did not have to be debugged. They
   were first developed on a platform with very limited debug
   support, so it was important that they worked the first time.

-- They are not necessarily fast; size and simplicity are more important.

-- They may be subsets of the POSIX version.

Use the header files that come with your compiler and library.

If you notice that the 64K flash of your microcontroller is suddenly
half filled with 32K of library routines, find which routine you called
is pulling in the stuff you don't need. For example, calling printf
pulls in a TON of other routines. Once you have identified the culprit,
write a simple replacement that implements only the functionality you
need, and add it to this library.

