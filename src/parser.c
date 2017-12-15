#include <parser.h>
#include <tokens.h>
#include <asm.h>
#include <filebuffer.h>
#include <darray.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <log.h>
#include <inttypes.h>
#include <stdlib.h>

/*
    Functions compare op with menmonics (at most n bytes)
    
    PARAMS
    @IN op - operand
    @IN n - number of bytes to compare

    RETURN
    false iff operand is not token mnemonic
    true iff operand is token mnemonic
*/
static ___inline___ bool is_mnemonic_token_move(const char *op, size_t n);
static ___inline___ bool is_mnemonic_token_jump(const char *op, size_t n);
static ___inline___ bool is_mnemonic_token_cmp(const char *op, size_t n);
static ___inline___ bool is_mnemonic_token_arythmetic(const char *op, size_t n);

/*
    Get arythmetic token type from string

    PARAMS
    @IN str - pointer to string
    @IN n - number of bytes to compare

    RETURN
    Type
*/
static ___inline___ arythemtic_t token_arythmetic_type_from_str(const char *str, size_t n);

/*
    Get jump token type from string

    PARAMS
    @IN str - pointer to string
    @IN n - number of bytes to compare

    RETURN
    Type
*/
static ___inline___ jump_t token_jump_type_from_str(const char *str, size_t n);

/*
    Create variable from string

    PARAMS
    @IN str - pointer to string
    @IN var - pointer to Variable

    RETURN
    Bytes from str used to create variable
*/
static ___inline___ size_t variable_create_from_str(const char *str, Variable *var);

/*
    Destroy Token from generic collection

    PARAMS
    @IN token &token *

    RETURN
    This is a void function
*/
static void token_destroy_generic(void *token);

static void token_destroy_generic(void *token)
{
    Token *_token = *(Token **)token;
    token_destroy(_token);
}

/* extern menmonics */
const Mnemonics mnemonics = {
    .mov = "mov",
    .add = "add",
    .sub = "sub",
    .div = "div",
    .mul = "mul",
    .mod = "mod",
    .cmp = "cmp",
    .je  = "je",
    .jne = "jne",
    .jgt  = "jgt",
    .jge = "jge",
    .jlt  = "jlt",
    .jle = "jle" 
};

const char memory_c         = 'M';
const char register_c       = 'R';
const char decimal_mode_c   = '#';
const char octal_mode_c     = '&';
const char binary_mode_c    = '%';

static ___inline___ bool is_mnemonic_token_move(const char *op, size_t n)
{
    return n > 0 && strncmp(op, mnemonics.mov, n) == 0;
}

static ___inline___ bool is_mnemonic_token_jump(const char *op, size_t n)
{
    return n > 0 && token_jump_type_from_str(op, n) != JUMP_NONE;
}

static ___inline___ bool is_mnemonic_token_cmp(const char *op, size_t n)
{
    return n > 0 && strncmp(op, mnemonics.cmp, n) == 0;
}

static ___inline___ bool is_mnemonic_token_arythmetic(const char *op, size_t n)
{
    return n > 0 && token_arythmetic_type_from_str(op, n) != OP_NONE;
}

static ___inline___ arythemtic_t token_arythmetic_type_from_str(const char *str, size_t n)
{
    if (n == 0)
        return OP_NONE;

    if (strncmp(str ,mnemonics.add, n) == 0)
        return OP_ADD;
    
    if (strncmp(str, mnemonics.sub, n) == 0)
        return OP_SUB;

    if (strncmp(str ,mnemonics.mul, n) == 0)
        return OP_MUL;
    
    if (strncmp(str, mnemonics.div, n) == 0)
        return OP_DIV;

    if (strncmp(str ,mnemonics.mod, n) == 0)
        return OP_MOD;

    return OP_NONE;
}

static ___inline___ jump_t token_jump_type_from_str(const char *str, size_t n)
{
    if (n == 0)
        return JUMP_NONE;

    if (strncmp(str ,mnemonics.je, n) == 0)
        return JUMP_EQ;

    if (strncmp(str, mnemonics.jne, n) == 0)
        return JUMP_NEQ;

    if (strncmp(str ,mnemonics.jgt, n) == 0)
        return JUMP_GT;

    if (strncmp(str, mnemonics.jge, n) == 0)
        return JUMP_GEQ;

    if (strncmp(str ,mnemonics.jlt, n) == 0)
        return JUMP_LT;

    if (strncmp(str, mnemonics.jle, n) == 0)
        return JUMP_LEQ;

    return JUMP_NONE;
}

static ___inline___ size_t variable_create_from_str(const char *str, Variable *var)
{
    char *ptr;
    size_t i = 0;
    int base = 10;
    uint32_t *number = &var->nr;


    /* skip whitespaces */
    while (isspace(str[i]))
        ++i;
    var->type = VAR_NONE;

    if (toupper(str[i]) == memory_c)
        var->type = VAR_MEMORY;
    else if (toupper(str[i]) == register_c)
        var->type = VAR_REGISTER;
    else
    {
        if (str[i] == decimal_mode_c)
        {
            var->type = VAR_VALUE;
            base = 10;
            number = &var->val;
        }
        else if (str[i] == octal_mode_c)
        {
            var->type = VAR_VALUE;
            base = 8;
            number = &var->val;
        }
        else if (str[i] == binary_mode_c)
        {
            var->type = VAR_VALUE;
            base = 2;
            number = &var->val;
        }
    }
        

    ++i;        
    /* get number */
    *number = (uint32_t)strtoull(&str[i], &ptr, base);
    i += (size_t)(ptr - &str[i]);

    return i;
}

Token **parse(const char *file, size_t *size)
{
    File_buffer *fb;

    /* temporary tokens */
    Token_arythmetic token_arythmetic;
    Token_cmp token_cmp;
    Token_jump token_jump;
    Token_move token_move;

    Darray *darray; /* darray with tokens */
    Token *token = NULL; /* generic token */

    const char *buf; /* buffer from file_buffer */
    size_t buf_size; /* size of file buffer buf */

    size_t i;
    size_t j;
    size_t k;

    char *ptr;

    Token **result = NULL;
    size_t tokens = 0;

    TRACE();

    fb = file_buffer_create_from_path(file, PROT_READ | PROT_WRITE, O_RDWR);
    if (fb == NULL)
        ERROR("file_buffer create error\n", NULL);

    darray = darray_create(DARRAY_UNSORTED, 0, sizeof(Token *), NULL);
    if (darray == NULL)
    {
        file_buffer_destroy(fb);
        ERROR("darray create error\n", NULL);
    }

    buf = file_buffer_get_buff(fb);
    buf_size = (size_t)file_buffer_get_size(fb);

    LOG("Parsing asm into tokens\n");
    i = 0;
    while (i < buf_size)
    {
        /* skip whitespaces */
        while (i < buf_size && isspace(buf[i]))
            ++i;

        /* get word */
        j = i;
        while (i < buf_size && !isspace(buf[i]))
            ++i;
        k = i;

        /* skip whitespaces */
        while (i < buf_size && isspace(buf[i]))
            ++i;

        if (i>- buf_size)
            break;

        /* translate operand */
        if (is_mnemonic_token_arythmetic(&buf[j], k - j))
        {
            LOG("Arythmetic token\n");

            /* create token */
            token_arythmetic.type = token_arythmetic_type_from_str(&buf[j], k - j);
            i += variable_create_from_str(&buf[i], &token_arythmetic.dst);
            i += variable_create_from_str(&buf[i], &token_arythmetic.src1);
            i += variable_create_from_str(&buf[i], &token_arythmetic.src2);

            token = token_create(TOKEN_ARYTHMETIC, (void *)&token_arythmetic);
        }
        else if (is_mnemonic_token_cmp(&buf[j], k - j))
        {
            LOG("Compare token\n");

            i += variable_create_from_str(&buf[i], &token_cmp.src1);
            i += variable_create_from_str(&buf[i], &token_cmp.src2);

            token = token_create(TOKEN_CMP, (void *)&token_cmp);
        }
        else if (is_mnemonic_token_jump(&buf[j], k - j))
        {
            LOG("Jump token\n");

            token_jump.type = token_jump_type_from_str(&buf[j], k - j);
            /* skip whitespaces */
            while (i < buf_size && isspace(buf[i]))
                ++i;

            token_jump.line = (uint32_t)strtoull(&buf[i], &ptr, 10);

            i += (size_t)(ptr - &buf[i]);

            token = token_create(TOKEN_JUMP, (void *)&token_jump);
        }
        else if (is_mnemonic_token_move(&buf[j], k - j))
        {
            LOG("Move token\n");

            i += variable_create_from_str(&buf[i], &token_move.dst);
            i += variable_create_from_str(&buf[i], &token_move.src);

            token = token_create(TOKEN_MOVE, (void *)&token_move);
        }

        if (token != NULL)
        {
            token_dbg_print(token);
            (void)darray_insert(darray, (void *)&token);
            token = NULL;
        }
    }
    file_buffer_destroy(fb);
    
    tokens = (size_t)darray_get_num_entries(darray);
    LOG("Copy tokens to array\n");
    result = (Token **)malloc(sizeof(Token *) * tokens);
    if (result == NULL)
    {
        darray_destroy_with_entries(darray, token_destroy_generic);
        ERROR("malloc error\n", NULL);
    }

    i = 0;
    for_each_data(darray, Darray, token)
        result[i++] = token;

    darray_destroy(darray);

    *size = tokens;
    return result;
}