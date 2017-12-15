#ifndef TOMASULO_H
#define TOMASULO_H

/*
    Simulator of popular tomasulo algorithm

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com

    LICENCE: GPL 3.0
*/

#include <tokens.h>

/*
    Simulate tomasulo on set of instructions

    PARAMS
    @IN program - set of instructions
    @IN num_instr - number of instruction in set of instructions

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int tomasulo(Token **program, size_t num_instr);

#endif