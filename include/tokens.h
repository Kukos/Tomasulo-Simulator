#ifndef TOKENS_H
#define TOKENS_H

#include <stdint.h>

/*
    Tokens of our asm

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com

    LICENCE: GPL 3.0
*/

typedef enum 
{
    TOKEN_NONE,
    TOKEN_MOVE,
    TOKEN_CMP,
    TOKEN_JUMP,
    TOKEN_ARYTHMETIC
} token_t;

typedef enum
{
    VAR_NONE,
    VAR_REGISTER,
    VAR_MEMORY,
    VAR_VALUE
} var_t;

typedef enum
{
    JUMP_NONE,
    JUMP_EQ,
    JUMP_NEQ,
    JUMP_LT,
    JUMP_LEQ,
    JUMP_GT,
    JUMP_GEQ
} jump_t;

typedef enum
{
    OP_NONE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD
} arythemtic_t;

typedef struct Variable
{
    var_t type;
    __extension__ union
    {
        uint32_t nr;
        uint32_t val;
    };
} Variable;


typedef struct Token_move
{
    Variable dst;
    Variable src;
} Token_move;


typedef struct Token_cmp
{
    Variable src1;
    Variable src2;
} Token_cmp;

typedef struct Token_jump
{
    jump_t type;
    uint32_t line;
} Token_jump;

typedef struct Token_arythmetic
{
    arythemtic_t type;
    Variable dst;
    Variable src1;
    Variable src2;
} Token_arythmetic;

typedef struct Token
{
    token_t type;
    __extension__ union
    {
        Token_move token_move;
        Token_cmp  token_cmp;
        Token_jump token_jump;
        Token_arythmetic token_arythmetic;
    };
} Token;

/*
    Create generic token from token

    PARAMS
    @IN type - token type
    @IN token - pointer to your token

    RETURN
    NULL iff failure
    Pointer to new generic token iff success
*/
Token *token_create(token_t type, void *token);

/*
    Destroy generic Token

    PARAMS
    @IN token - pointer to generic pointer

    RETURN
    This is a void function
*/
void token_destroy(Token *token);

/*
    Print token using LOG

    PARAMS
    @IN token - pointer to generic Token

    RETURN
    This is a void function
*/
void token_dbg_print(const Token *token);

/*
    Print token on stdout

    PARAMS
    @IN token - pointer to generic Token

    RETURN
    This is a void function
*/
void token_print(const Token *token);

#endif