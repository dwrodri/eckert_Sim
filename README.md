# Eckert Simulator

This is a simulator of the simple CPU with a microprogrammed control unit.
For now, the best description of this simulator is still on 
[Dr. Eckert's website](http://www.cs.binghamton.edu/~reckert/hardwire3new.html). 

##Installation and Run instructions
This is very much a WYSIWYG layout. I've used CMake for the building process, but
there's only one C file (with no dependencies!), so you can compile it in one line with any C
compiler that supports binary literals (e.g. `0b011010101`). **NOTE:** There was some very strange behavior 
observed when this code was compiled with the `-O3` flag, so I'd suggest staying clear of using any optimization flags.

There are three input files for the program:
* `ram.txt`: this is the "program". It is a raw text file where each line is a 12-bit value. Currently, the program 
supports a max size of 256 lines, but this wouldn't be hard to adjust.

* `addr.txt`: This is the mapping between opcodes and ROM addresses. To extend this instruction set, put the address of 
the entry point for the new instruction.

* `uprog.txt`: This gets loaded into memory as the "control matrix". The `clock_tick()` function pulls a "µ-instruction"
from the ROM, an modifies registers according to the bits in the instruction.

##Future Work

I'd really like to build a decent TUI suing curses, but that'll be on the back burner for quite some time.

