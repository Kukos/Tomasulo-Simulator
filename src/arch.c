#include <arch.h>
#include <log.h>
#include <compiler.h>
#include <inttypes.h>

/* our board */
Board board;

/*
    Get const char* from type
*/
static ___inline___ const char *state_get_str(state_t state);
static ___inline___ const char *job_get_str(job_t job);


/*
    Set register as FREE

    PARAMS
    @IN reg - pointer to register

    RETURN
    This is a void function
*/
static ___inline___ void register_set_free(Register_info *reg);

/*
    Arythemtic operations

    @dst = @src1 op @src2
    
    PARAMS
    @IN dst - destination
    @IN src1 - source1
    @IN src2 - source2

    RETURN
    This is a void function
*/
static ___inline___ void do_add(Register_info *dst, Register_info *src1, Register_info *src2);
static ___inline___ void do_sub(Register_info *dst, Register_info *src1, Register_info *src2);
static ___inline___ void do_mul(Register_info *dst, Register_info *src1, Register_info *src2);
static ___inline___ void do_div(Register_info *dst, Register_info *src1, Register_info *src2);
static ___inline___ void do_mod(Register_info *dst, Register_info *src1, Register_info *src2);

/*
    Jump to line iff CF is set to properly value

    PARAMS
    @IN line - line to jump

    RETURN
    This is a void function
*/
static ___inline___ void do_jeq(uint32_t line);
static ___inline___ void do_jneq(uint32_t line);
static ___inline___ void do_jgt(uint32_t line);
static ___inline___ void do_jgeq(uint32_t line);
static ___inline___ void do_jlt(uint32_t line);
static ___inline___ void do_jleq(uint32_t line);

/*
    Print info
*/
static ___inline___  void memory_dump(void);
static ___inline___  void register_dump(const Register_info *reg);
static ___inline___ void registers_dump(void);

static ___inline___ const char *state_get_str(state_t state)
{
    switch (state)
    {
        case STATE_FREE:
            return "FREE";
        case STATE_BUSY:
            return "BUSY";
        default:
            return NULL;
    }

    return NULL;
}

static ___inline___ const char *job_get_str(job_t job)
{
    switch (job)
    {
        case JOB_ARYTHMETIC:
            return "ARYTHMETIC";
        case JOB_CMP:
            return "CMP";
        case JOB_IDLE:
            return "IDLE";
        case JOB_JUMP:
            return "JUMP";
        case JOB_LOAD:
            return "LOAD";
        case JOB_STORE:
            return "STORE";
        default:
            return NULL;
    }

    return NULL;
}

static const char *arythmetic_get_str_from_type(arythemtic_t type)
{
    TRACE();

    switch (type)
    {
        case OP_ADD:
            return "ADD";
        case OP_SUB:
            return "SUB";
        case OP_DIV:
            return "DIV";
        case OP_MUL:
            return "MUL";
        case OP_MOD:
            return "MOD";
        default:
            return NULL;
    }

    return NULL;
}

void go_to_next_instruction(void)
{
    TRACE();
    ++board.pc;
}

static ___inline___ void register_set_free(Register_info *reg)
{
    TRACE();

    if (reg == NULL)
        return;

    reg->state      = STATE_FREE;
    reg->job        = JOB_IDLE;
    reg->aryth_type = OP_NONE;

    (void)memset((void *)&reg->worker, 0, sizeof(Worker));
}

static ___inline___ void do_add(Register_info *dst, Register_info *src1, Register_info *src2)
{
    TRACE();

    if (dst == NULL || src1 == NULL || src2 == NULL)
        return;

    dst->val = src1->val + src2->val;

    /* free registers */
    register_set_free(dst);
    register_set_free(src1);
    register_set_free(src2);
}

static ___inline___ void do_sub(Register_info *dst, Register_info *src1, Register_info *src2)
{
    TRACE();

    if (dst == NULL || src1 == NULL || src2 == NULL)
        return;

    dst->val = src1->val - src2->val;

    /* free registers */
    register_set_free(dst);
    register_set_free(src1);
    register_set_free(src2);
}

static ___inline___ void do_mul(Register_info *dst, Register_info *src1, Register_info *src2)
{
    TRACE();

    if (dst == NULL || src1 == NULL || src2 == NULL)
        return;

    dst->val = src1->val * src2->val;

    /* free registers */
    register_set_free(dst);
    register_set_free(src1);
    register_set_free(src2);
}

static ___inline___ void do_div(Register_info *dst, Register_info *src1, Register_info *src2)
{
    TRACE();

    if (dst == NULL || src1 == NULL || src2 == NULL)
        return;

    dst->val = src1->val / src2->val;

    /* free registers */
    register_set_free(dst);
    register_set_free(src1);
    register_set_free(src2);
}

static ___inline___ void do_mod(Register_info *dst, Register_info *src1, Register_info *src2)
{
    TRACE();

    if (dst == NULL || src1 == NULL || src2 == NULL)
        return;

    dst->val = src1->val % src2->val;

    /* free registers */
    register_set_free(dst);
    register_set_free(src1);
    register_set_free(src2);
}

static ___inline___ void do_jeq(uint32_t line)
{
    TRACE();

    if (board.cf == 0)
        board.pc = line;
}

static ___inline___ void do_jneq(uint32_t line)
{
    TRACE();

    if (board.cf != 0)
        board.pc = line;
}

static ___inline___ void do_jgt(uint32_t line)
{
    TRACE();

    if (board.cf == 1)
        board.pc = line;
}

static ___inline___ void do_jgeq(uint32_t line)
{
    TRACE();

    if (board.cf >= 0)
        board.pc = line;
}

static ___inline___ void do_jlt(uint32_t line)
{
    TRACE();

    if (board.cf == -1)
        board.pc = line;
}

static ___inline___ void do_jleq(uint32_t line)
{
    TRACE();

    if (board.cf <= 0)
        board.pc = line;
}

static ___inline___  void memory_dump(void)
{
    size_t i;

    TRACE();

    printf("RAM %zu size\n", (size_t)RAM_SIZE);
    for (i = 0; i < (size_t)RAM_SIZE; ++i)
        printf("MEM[ %zu ] = %ld\n", i, board.ram.memory[i]);
}


static ___inline___  void register_dump(const Register_info *reg)
{
    TRACE();

    if (reg == NULL)
        return;

    printf("R%" PRIu32 "\n", reg->nr);
    printf("\tState = %s\n", state_get_str(reg->state));
    printf("\tJob   = %s", job_get_str(reg->job));
    if (reg->job == JOB_ARYTHMETIC)
        printf(" [ %s ]", arythmetic_get_str_from_type(reg->aryth_type));
    printf("\n");
    printf("\tValue = %lu\n", reg->val);
}

static ___inline___ void registers_dump(void)
{
    size_t i;

    for (i = 0; i < (size_t)REGISTERS_NUM; ++i)
        register_dump(&board.registers.regs[i]);
}

void reset_board(void)
{
    size_t i;

    TRACE();

    LOG("Reseting board\n");

    (void)memset(&board, 0, sizeof(Board));

    for (i = 0; i < REGISTERS_NUM; ++i)
        board.registers.regs[i].nr = (uint32_t)i;

    for (i = 0; i < LOAD_BUFFER_SIZE; ++i)
        board.load_buffer.load[i].state = STATE_FREE;

    for (i = 0; i < WRITE_BUFFER_SIZE; ++i)
        board.write_buffer.write[i].state = STATE_FREE;

    for (i = 0; i < RS_ADD_SUB_SIZE; ++i)
        board.rs.add[i].state = STATE_FREE;

    for (i = 0; i < RS_MUL_DIV_MOD_SIZE; ++i)
        board.rs.mul[i].state = STATE_FREE;

    board.rs.cmp.state = STATE_FREE;
}

void do_cmp(Register_info *r1, Register_info *r2)
{
    TRACE();

    if (r1 == NULL || r2 == NULL)
        return;

    /* set CF */
    if (r1->val == r2->val)
        board.cf = 0;
    else if (r1->val < r2->val)
        board.cf = -1;
    else
        board.cf = 1;

    /* free registers */
    register_set_free(r1);
    register_set_free(r2);
}

void do_arythmetic(arythemtic_t type, Register_info *dst, Register_info *src1, Register_info *src2)
{
    TRACE();

    switch (type)
    {
        case OP_ADD:
        {
            do_add(dst, src1, src2);
            break;
        }
        case OP_SUB:
        {
            do_sub(dst, src1, src2);
            break;
        }
        case OP_MUL:
        {
            do_mul(dst, src1, src2);
            break;
        }
        case OP_DIV:
        {
            do_div(dst, src1, src2);
            break;
        }
        case OP_MOD:
        {
            do_mod(dst, src1, src2);
            break;
        }
        default:
            LOG("Unsupported op type\n");
    }
}

void do_jump(jump_t type, uint32_t line)
{
    TRACE();

    /* fisrt go to next, if jump failed stay there, else jut jump */
    go_to_next_instruction();
    switch (type)
    {
        case JUMP_EQ:
        {
            do_jeq(line);
            break;
        }
        case JUMP_NEQ:
        {
            do_jneq(line);
            break;
        }
        case JUMP_LT:
        {
            do_jlt(line);
            break;
        }
        case JUMP_LEQ:
        {
            do_jleq(line);
            break;
        }
        case JUMP_GT:
        {
            do_jgt(line);
            break;
        }
        case JUMP_GEQ:
        {
            do_jgeq(line);
            break;
        }
        default:
            LOG("Unsupported jump type\n");
    }
}

void copy_data_to_reg(uint32_t reg_num, Variable *var)
{
    Register_info *reg;
    TRACE();

    if (reg_num > REGISTERS_NUM)
        return;

    reg = &board.registers.regs[reg_num];

    switch (var->type)
    {
        case VAR_MEMORY:
        {
            if (var->nr > RAM_SIZE)
                return;

            reg->val = board.ram.memory[var->nr];
            break;
        }
        case VAR_REGISTER:
        {
            if (var->nr > REGISTERS_NUM)
                return;

            reg->val = board.registers.regs[var->nr].val;
            register_set_free(&board.registers.regs[var->nr]);
            break;
        }
        case VAR_VALUE:
        {
            reg->val = var->val;
            break;
        }
        default:
            break;
    }

    register_set_free(reg);
}

void copy_data_to_memory(uint32_t addr, Variable *var)
{
    TRACE();

    if (addr > RAM_SIZE)
        return;

    switch (var->type)
    {
        case VAR_MEMORY:
        {
            if (var->nr > RAM_SIZE)
                return;

            board.ram.memory[addr] = board.ram.memory[var->nr];
            break;
        }
        case VAR_REGISTER:
        {
            if (var->nr > REGISTERS_NUM)
                return;

            board.ram.memory[addr] = board.registers.regs[var->nr].val;
            register_set_free(&board.registers.regs[var->nr]);
            break;
        }
        case VAR_VALUE:
        {
            board.ram.memory[addr] = var->val;
            break;
        }
        default:
            break;
    }
}

void board_dump(void)
{
    TRACE();
    
    printf("ARCH %zu bits\n", sizeof(DWORD) << 3);
    printf("PC = %lu\n", board.pc);
    printf("CF = %d\n", board.cf);
    printf("\n");
    registers_dump();
    memory_dump();
    printf("\n");
}