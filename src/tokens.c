#include <stdlib.h>
#include <common.h>
#include <tokens.h>
#include <log.h>
#include <string.h>
#include <inttypes.h>
#include <asm.h>

/*
    Print Variable using in debug mode using LOG

    PARAMS
    @IN var - pointer to Variable

    RETURN
    This is a void function
*/
___unused___ static void variable_dbg_print(const Variable *var);

/*
    Print Variable

    PARAMS
    @IN var - pointer to Variable

    RETURN
    This is a void function
*/
static void variable_print(const Variable *var);

/*
    Get const string from token jump type

    PARAMS
    @IN token - pointer to token_jump

    RETURN
    NULL iff failure
    Pointer to const string iff success
*/
___unused___ static const char *token_jump_get_str_from_type(const Token_jump *token);

/*
    Get const string from token arythmetic type

    PARAMS
    @IN token - pointer to token_arytmetic

    RETURN
    NULL iff failure
    Pointer to const string iff success
*/
___unused___ static const char *token_arythmetic_get_str_from_type(const Token_arythmetic *token);

___unused___ static const char *token_jump_get_str_from_type(const Token_jump *token)
{
    TRACE();

    if (token == NULL)
        ERROR("token == NULL\n", NULL);

    switch (token->type)
    {
        case JUMP_EQ:
            return mnemonics.je;
        case JUMP_NEQ:
            return mnemonics.jne;
        case JUMP_GT:
            return mnemonics.jgt;
        case JUMP_GEQ:
            return mnemonics.jge;
        case JUMP_LT:
            return mnemonics.jlt;
        case JUMP_LEQ:
            return mnemonics.jle;
        default:
        {
            LOG("Unsupported token type\n");
            return NULL;
        }
    }
    return NULL;
}

___unused___ static const char *token_arythmetic_get_str_from_type(const Token_arythmetic *token)
{
    TRACE();

    if (token == NULL)
        return NULL;

    switch (token->type)
    {
        case OP_ADD:
            return mnemonics.add;
        case OP_SUB:
            return mnemonics.sub;
        case OP_DIV:
            return mnemonics.div;
        case OP_MUL:
            return mnemonics.mul;
        case OP_MOD:
            return mnemonics.mod;
        default:
        {
            LOG("Unsupported token type\n");
            return NULL;
        }
    }

    return NULL;
}

___unused___ static void variable_dbg_print(const Variable *var)
{
    TRACE();

    if (var == NULL)
        return;

    switch (var->type)
    {
        case VAR_MEMORY:
        {
            LOG("\tMEM[ %" PRIu32" ]\n", var->nr);
            break;
        }
        case VAR_REGISTER:
        {
            LOG("\tREG[ %" PRIu32" ]\n", var->nr);
            break;
        }
        case VAR_VALUE:
        {
            LOG("\t VAL = %" PRIu32 "\n", var->val);
            break;
        }
        default:
            LOG("\tUnsupported Variable type\n");
    }
}

static void variable_print(const Variable *var)
{
    TRACE();

    if (var == NULL)
        return;

    switch (var->type)
    {
        case VAR_MEMORY:
        {
            printf("%c%" PRIu32 " ",memory_c ,var->nr);
            break;
        }
        case VAR_REGISTER:
        {
            printf("%c%" PRIu32 " ",register_c ,var->nr);
            break;
        }
        case VAR_VALUE:
        {
            printf("%c%" PRIu32 " ",decimal_mode_c ,var->val);
            break;
        }
        default:
            break;
    }
}

Token *token_create(token_t type, void *token)
{
    Token *g_token;
    void *dst;
    size_t size;

    TRACE();

    if (token == NULL)
        ERROR("token == NULL\n", NULL);

    g_token = (Token *)malloc(sizeof(Token));
    if (g_token == NULL)
        ERROR("malloc error\n", NULL);

    g_token->type = type;
    switch (type)
    {
        case TOKEN_MOVE:
        {
            dst = (void *)&g_token->token_move;
            size = sizeof(g_token->token_move);
            break;
        }
        case TOKEN_ARYTHMETIC:
        {
            dst = (void *)&g_token->token_arythmetic;
            size = sizeof(g_token->token_arythmetic);
            break;
        }
        case TOKEN_CMP:
        {
            dst = (void *)&g_token->token_cmp;
            size = sizeof(g_token->token_cmp);
            break;
        }
        case TOKEN_JUMP:
        {
            dst = (void *)&g_token->token_jump;
            size = sizeof(g_token->token_jump);
            break;
        }
        default:
            ERROR("Unsupported token type\n", NULL);
    }

    (void)memcpy(dst, token, size);
    return g_token;
}

void token_destroy(Token *token)
{
    TRACE();

    if (token == NULL)
        return;

    FREE(token);
}

#ifdef DEBUG_MODE
void token_dbg_print(const Token *token)
{
    const Token_arythmetic *token_arythmetic = (Token_arythmetic *)&token->token_arythmetic;
    const Token_cmp *token_cmp = (Token_cmp *)&token->token_cmp;
    const Token_jump *token_jump = (Token_jump *)&token->token_jump;
    const Token_move *token_move = (Token_move *)&token->token_move;

    TRACE();

    if (token == NULL)
        return;

    switch (token->type)
    {
        case TOKEN_ARYTHMETIC:
        {
            LOG("Token Arythmetic:\n");
            LOG("\tTYPE:\t%s\n", token_arythmetic_get_str_from_type(token_arythmetic));
            LOG("\tDST:\n");
            variable_dbg_print(&token_arythmetic->dst);
            LOG("\tSRC1:\n");
            variable_dbg_print(&token_arythmetic->src1);
            LOG("\tSRC2:\n");
            variable_dbg_print(&token_arythmetic->src2);
            break;
        }
        case TOKEN_CMP:
        {
            LOG("Token cmp:\n");
            LOG("\tSRC1:\n");
            variable_dbg_print(&token_cmp->src1);
            LOG("\tSRC2:\n");
            variable_dbg_print(&token_cmp->src2);
            break;
        }
        case TOKEN_JUMP:
        {
            LOG("Token jump:\n");
            LOG("\tTYPE:\t%s\n", token_jump_get_str_from_type(token_jump));
            LOG("\tLine = %" PRIu32 "\n", token_jump->line);
            break;
        }
        case TOKEN_MOVE:
        {
            LOG("Token move:\n");
            LOG("\tDST:\n");
            variable_dbg_print(&token_move->dst);
            LOG("\tSRC1:\n");
            variable_dbg_print(&token_move->src);
            break;
        }
        default:
            LOG("Unsupported token type\n");
    }
}
#else
void token_dbg_print(const Token *token)
{
    (void)token;
}
#endif

void token_print(const Token *token)
{
    const Token_arythmetic *token_arythmetic = (Token_arythmetic *)&token->token_arythmetic;
    const Token_cmp *token_cmp = (Token_cmp *)&token->token_cmp;
    const Token_jump *token_jump = (Token_jump *)&token->token_jump;
    const Token_move *token_move = (Token_move *)&token->token_move;

    TRACE();

    if (token == NULL)
        return;

    switch (token->type)
    {
        case TOKEN_ARYTHMETIC:
        {
            printf("%s ", token_arythmetic_get_str_from_type(token_arythmetic));
            variable_print(&token_arythmetic->dst);
            variable_print(&token_arythmetic->src1);
            variable_print(&token_arythmetic->src2);
            printf("\n");

            break;
        }
        case TOKEN_CMP:
        {
            printf("%s ", mnemonics.cmp);
            variable_print(&token_cmp->src1);
            variable_print(&token_cmp->src2);
            printf("\n");

            break;
        }
        case TOKEN_JUMP:
        {
            printf("%s ", token_jump_get_str_from_type(token_jump));
            printf("%" PRIu32 "\n", token_jump->line);
            printf("\n");

            break;
        }
        case TOKEN_MOVE:
        {
            printf("%s ", mnemonics.mov);
            variable_print(&token_move->dst);
            variable_print(&token_move->src);
            printf("\n");

            break;
        }
        default:
            LOG("Unsupported token type\n");
    }
}