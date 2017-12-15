#ifndef PARSER_H
#define PARSER_H

#include <tokens.h>
#include <stddef.h>

/*
    Simple parser asm to tokens

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com

    LICENCE: GPL 3.0
*/

/*
    Parse file to array of Tokens*

    PARAMS
    @IN file - path to file
    @OUT size - size of array

    RETURN
    NULL iff failure
    Pointer to array fo Token* iff success
*/
Token **parse(const char *file, size_t *size);


#endif