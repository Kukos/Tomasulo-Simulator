# Tomasulo-Simulator

Simulator of old CPU algorithm to rename register and make some instruction in parallel.
The basic description can be found on wikipedia: https://en.wikipedia.org/wiki/Tomasulo_algorithm

### Short description
Lets assume that cpu has some registers that compiler cant use.
CPU has #IO registers for memory io operations and also #RS for arythmetic operations.
So during decoding instruction tomasulo renaming registers to IO or RS, setting dependency to avoid
races and waiting for results.

Here is the presentation about that: https://www.cc.gatech.edu/~milos/Teaching/CS6290F07/4_Tomasulo.pdf
made by Georgia Tech.

### Simulator
I wrote simple parser (in pure C) that doesnt check error, so be sure that your gramma is correct.
On stdout simulator prints all information about architecture in each cycle, to go to next cycle
you have to type any key on stdin.

### Tests
Test code to see how tomasulo works are included in ./data directory.

#### Compile
Just type make
