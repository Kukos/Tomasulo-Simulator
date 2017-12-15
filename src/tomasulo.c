#include <arch.h>
#include <tokens.h>
#include <tomasulo.h>
#include <log.h>
#include <compiler.h>
#include <darray.h>
#include <stdlib.h>
#include <common.h>
#include <inttypes.h>
#include <getch.h>

typedef struct Tomasulo_data
{
    Darray *is_array;
    uint32_t cycle;
} Tomasulo_data;

static Tomasulo_data tomasulo_data;

#define current_cycle() tomasulo_data.cycle
#define reset_terminal() \
    do { \
        if (system("tput reset") == -1) \
            FATAL("Terminal reset failed\n"); \
    } while (0)

#define reset_io(IO)      (void)memset((void *)IO, 0, sizeof(*IO))
#define reset_rsc(RSC)    (void)memset((void *)RSC, 0, sizeof(*RSC))
/*
    Init tomasulo

    PARAMS
    NO PARAMS

    RETURN
    This is a void function
*/
static ___inline___ void tomasulo_init(void);

/*
    Exec next cycle

    PARAMS
    NO PARAMS

    RETURN
    This is a void function
*/
static ___inline___ void tomasulo_next_cycle(void);

/*
    Add new iinstruction from token to tracking by Tomasulo algo

    PARAMS
    @IN token - pointer to token

    RETURN
    Pointer to tracked is
*/
static ___inline___ Instructions_status *tomasulo_add_instruction_to_tracking(Token *token);

/*
    Deinit whole tomasulo data

    PARAMS
    NO PARAMS

    RETURN This is a void function
*/
static ___inline___ void tomasulo_deinit(void);

/*
    Helper for deinit

    PARAMS
    @IN is -  (void *)&data

    RETURN
    This is a void function
*/
static void __is_destroy(void *is);

/*
    Is RSC busy ?

    PARAMS
    @IN rsc_array - array of RSC
    @IN rsc_array_size - @rsc_array length

    RETURN
    true iff all rsc are busy
    false iff at least 1 is free
*/
static ___inline___ bool is_rsc_busy(const Reservation_station_chunk *rsc_array, size_t rsc_array_size);

/*
    Is IO buffer busy ?

    PARAMS
    @IN io_array - array of IO_info
    @IN io_array_size - @io_array length

    RETURN
    true iff all IO buffers are busy
    false iff at least 1 is free
*/
static ___inline___ bool is_io_busy(const IO_info *io_array, size_t io_array_size);

/* wrappers for rsc and io busy */
#define is_rsc_mul_div_mod_busy() is_rsc_busy((const Reservation_station_chunk *)board.rs.mul, RS_MUL_DIV_MOD_SIZE)
#define is_rsc_add_sub_busy()     is_rsc_busy((const Reservation_station_chunk *)board.rs.add, RS_ADD_SUB_SIZE)
#define is_rsc_cmp_busy()         is_rsc_busy((const Reservation_station_chunk *)&board.rs.cmp, 1)
#define is_io_load_busy()         is_io_busy((const IO_info *)board.load_buffer.load, LOAD_BUFFER_SIZE)
#define is_io_write_busy()        is_io_busy((const IO_info *)board.write_buffer.write, WRITE_BUFFER_SIZE)

/*
    Get first free rsc in array

    PARAMS
    @IN rsc_array - array of RSC
    @IN rsc_array_size - @rsc_array length

    RETURN
    NULL iff all rsc are busy
    Pointer to free rsc iff is any
*/
static ___inline___ Reservation_station_chunk *get_first_free_rsc(const Reservation_station_chunk *rsc_array, size_t rsc_array_size);

/*
   Get first free io buffer in array

    PARAMS
    @IN io_array - array of IO_info
    @IN io_array_size - @io_array length

    RETURN
    NULL iff all io buffers are busy
    Pointer to free io buffer iff is any
*/
static ___inline___ IO_info *get_first_free_io(const IO_info *io_array, size_t io_array_size);

/* Wrappers for rsc and io get_first_free */
#define get_first_free_mul_div_mod() get_first_free_rsc((const Reservation_station_chunk *)board.rs.mul, RS_MUL_DIV_MOD_SIZE)
#define get_first_free_add_sub()     get_first_free_rsc((const Reservation_station_chunk *)board.rs.add, RS_ADD_SUB_SIZE)
#define get_first_free_cmp()         get_first_free_rsc((const Reservation_station_chunk *)&board.rs.cmp, 1)
#define get_first_free_io_load()     get_first_free_io((const IO_info *)board.load_buffer.load, LOAD_BUFFER_SIZE)
#define get_first_free_io_write()    get_first_free_io((const IO_info *)board.write_buffer.write, WRITE_BUFFER_SIZE)

/*
    Helper for execute Write / Load

    PARAMS
    @IN io_array - array of IO buffers
    @IN io_array_size - @io_array length

    RETURN
    This is a void function
*/
static void execute_io(IO_info *io_array, size_t io_array_size);

/*
    Helper for execute ayrthmetic / cmp

    PARAMS
    @IN rsc_array - array of RSC
    @IN rsc_array_size - @rsc_array length

    RETURN
    This is a void function
*/
static void execute_rsc(Reservation_station_chunk *rsc_array, size_t rsc_array_size);

/*
    Clear dependency Helper for
    dependency_clear_and_prepare_work

    PARAMS
    @IN dep - dependency

    RETURN
    This is a void function
*/
static ___inline___ void __dependency_clear(Dependency *dep);


/*
    Clear dependency on whole dependency array
    and schedule new work for io / rsc waiting for this job

    PARAMS
    @IN dep_array - array of dependencies
    @IN dep_array_size - @dep_array length

    RETURN
    This is a void function
*/
static ___inline___ void dependency_clear_and_prepare_work(Dependency *dep_array, size_t dep_array_size);

#define dependency_clear_and_prepare_work_io(IO) \
    dependency_clear_and_prepare_work((Dependency *)IO->dependency, DEPENDENCY_NR_OF_SLOT_IO)

#define dependency_clear_and_prepare_work_rsc(RSC) \
    dependency_clear_and_prepare_work((Dependency *)RSC->dependency, DEPENDENCY_NR_OF_SLOT_RSC)

#define io_can_do_job(IO) \
    ((IO->state == STATE_BUSY) \
     && (!IO->has_dependency[DEPENDENCY_FROM_DST] && !IO->has_dependency[DEPENDENCY_FROM_SRC1]))

#define rsc_can_do_job(RSC) \
    ((RSC->state == STATE_BUSY) && \
    (!RSC->has_dependency[DEPENDENCY_FROM_DST] && !RSC->has_dependency[DEPENDENCY_FROM_SRC1] \
     && !RSC->has_dependency[DEPENDENCY_FROM_SRC2]))

/*
    Execute all load / write /  arythmetic operation
    iff they are ready or decrement wait time

    PARAMS
    NO PARAMS

    This is a void function
*/
static ___inline___ void execute_load(void);
static ___inline___ void execute_write(void);
static ___inline___ void execute_arythmetic(void);

/*
    Execute all operation iff ready or decrement wait time
    
    PARAMS
    NO PARAMS

    This is a void function
*/
static ___inline___ void execute(void);

/*
    Print tomasulot state

    PARAMS
    NO PARAMS

    RETURN
    This is a void function
*/
static ___inline___ void tomasulo_print(void);

/*
    Helper for prepare_work: Prepare variable to work (check register state)

    PARAMS
    @IN var - pointer to variable
    @IN worker - pointer to Worker
    @IN dep_slot - index in dependency array

    RETURN
    This is a void function
*/
static void __prepare_work(Variable *var, Worker *worker, int dep_slot);

/*
    Prepare Work for set worker (check register state)

    PARAMS
    @IN worker - pointer to Worker

    RETURN
    This is a void function
*/
static ___inline___ void prepare_work(Worker *worker);

/*
    Helper for setup_work: Setup variable to work
    Dont check if regitser is busy

    PARAMS
    @IN var - pointer to variable
    @IN worker - pointer to Worker
    @IN dep_slot - index in dependency array

    RETURN
    This is a void function
*/
static void __setup_work(Variable *var, Worker *worker, int dep_slot);

/*
    Prepare Work for worker (dont check register sttae)

    PARAMS
    @IN worker - pointer to Worker

    RETURN
    This is a void function
*/
static ___inline___ void setup_work(Worker *worker);
/*
    Fetch new token

    PARAMS
    @IN token - new token

    RETURN
    This is a void function
*/
static void fetch(Token *token);

/*
    Checks Arch for unfinished jobs

    PARAMS
    NO PARAMS

    RETURN
    true iff is any unfinished job
    false iff all jobs  are finished
*/
static ___inline___ bool wait_for_unfinished_job(void);

/*
    Check if variable has dependency

    PARAMS
    Var - pointer to variable

    RETURN
    true iff variable has dep
    false iff variable has not any dep
*/
static ___inline___ bool var_works_with_dep(const Variable *var);

static void __is_destroy(void *is)
{
    Instructions_status *__is = *(Instructions_status **)is;
    FREE(__is);
}

static ___inline___ Reservation_station_chunk *get_first_free_rsc(const Reservation_station_chunk *rsc_array, size_t rsc_array_size)
{
    size_t i;

    TRACE();

    for (i = 0; i < rsc_array_size; ++i)
        if (rsc_array[i].state == STATE_FREE)
            return (Reservation_station_chunk *)&rsc_array[i];

    return NULL;
}

static ___inline___ IO_info *get_first_free_io(const IO_info *io_array, size_t io_array_size)
{
    size_t i;

    TRACE();

    for (i = 0; i < io_array_size; ++i)
        if (io_array[i].state == STATE_FREE)
            return (IO_info *)&io_array[i];

    return NULL;
}

static ___inline___  bool is_rsc_busy(const Reservation_station_chunk *rsc_array, size_t rsc_array_size)
{
    size_t i;

    TRACE();

    for (i = 0; i < rsc_array_size; ++i)
        if (rsc_array[i].state == STATE_FREE)
            return false;

    return true;
}

static ___inline___  bool is_io_busy(const IO_info *io_array, size_t io_array_size)
{
    size_t i;

    TRACE();

    for (i = 0; i < io_array_size; ++i)
        if (io_array[i].state == STATE_FREE)
            return false;

    return true;
}

static ___inline___ void dependency_clear_and_prepare_work(Dependency *dep_array, size_t dep_array_size)
{
    size_t i;

    TRACE();

    for (i = 0; i < dep_array_size; ++i)
    {
        __dependency_clear(&dep_array[i]);
        if (dep_array[i].type != DEPENDENCY_NONE)
        {
            LOG("Has dependency, now it is clear, setup worker\n");
            setup_work((Worker *)&dep_array[i]);
        }
    }
}

static ___inline___ void __dependency_clear(Dependency *dep)
{
    TRACE();

    switch (dep->type)
    {
        case DEPENDENCY_IO:
        {
            LOG("Clear IO dependency for slot %d\n", dep->slot);
            dep->io->has_dependency[dep->slot] = false;
            break;
        }
        case DEPENDENCY_OP:
        {
            LOG("Clear RSC dependency for slot %d\n", dep->slot);
            dep->rsc->has_dependency[dep->slot] = false;
            break;
        }
        case DEPENDENCY_NONE:
        {
            break;
        }
        default:
        {
            break;
        }
    }
}

static void execute_io(IO_info *io_array, size_t io_array_size)
{
    size_t i;
    IO_info *io;

    TRACE();
    for (i = 0; i < io_array_size; ++i)
    {
        io = &io_array[i];

        /* Can be executed  */
        if (io_can_do_job(io))
        {
            /* completed */
            if (io->wait_time == 0)
            {
                LOG("IO %d JOB completed\n", io->job);
                if (io->dst.type == VAR_REGISTER)
                    copy_data_to_reg(io->dst.nr, &io->src);
                else if (io->dst.type == VAR_MEMORY)
                    copy_data_to_memory(io->dst.nr, &io->src);

                /* operation complete lets notify dependency */
                dependency_clear_and_prepare_work_io(io);

                io->is->exec_cycle = current_cycle();
                
                reset_io(io);
                io->state = STATE_FREE;

            }
            else /* still waiting */
                --io->wait_time;
        }
    }
}

static void execute_rsc(Reservation_station_chunk *rsc_array, size_t rsc_array_size)
{
    size_t i;
    Reservation_station_chunk *rsc;

    TRACE();

    for (i = 0; i < rsc_array_size; ++i)
    {
        rsc = &rsc_array[i];

        /* Can be executed */
        if (rsc_can_do_job(rsc))
        {
            /* completed */
            if (rsc->wait_time == 0)
            {
                switch (rsc->job)
                {
                    case JOB_CMP:
                    {
                        LOG("Cmp JOB completed\n");
                        do_cmp(&board.registers.regs[rsc->src1.nr],
                               &board.registers.regs[rsc->src2.nr]);

                        break;
                    }
                    case JOB_ARYTHMETIC:
                    {
                        LOG("Arythmetic JOB %d completed\n", rsc->aryth_type);
                        do_arythmetic(rsc->aryth_type,
                                      &board.registers.regs[rsc->dst.nr],
                                      &board.registers.regs[rsc->src1.nr],
                                      &board.registers.regs[rsc->src2.nr]);
                        break;
                    }
                    default:
                        break;
                }

                /* operation complete lets notify dependency */
                dependency_clear_and_prepare_work_rsc(rsc);

                rsc->is->exec_cycle = current_cycle();

                reset_rsc(rsc);
                rsc->state = STATE_FREE;
                rsc->job = JOB_IDLE;

            }
            else /* still waiting */
                --rsc->wait_time;
        }
    }
}

static ___inline___ void execute_load(void)
{
    TRACE();

    execute_io((IO_info *)board.load_buffer.load, (size_t)LOAD_BUFFER_SIZE);
}

static ___inline___ void execute_write(void)
{
    TRACE();

    execute_io((IO_info *)board.write_buffer.write, (size_t)WRITE_BUFFER_SIZE);
}

static ___inline___ void execute_arythmetic(void)
{
    TRACE();

    execute_rsc((Reservation_station_chunk *)board.rs.add, RS_ADD_SUB_SIZE);
    execute_rsc((Reservation_station_chunk *)board.rs.mul, RS_MUL_DIV_MOD_SIZE);
    execute_rsc((Reservation_station_chunk *)&board.rs.cmp, 1);
}

static ___inline___ void execute(void)
{
    TRACE();

    execute_load();
    execute_arythmetic();
    execute_write();
}

static ___inline___ void tomasulo_print(void)
{
    Instructions_status *is;
    TRACE();

    printf("Cycle = %" PRIu32 "\n", current_cycle());
    board_dump();

    printf("Instructions\n");
    for_each_data(tomasulo_data.is_array, Darray, is)
    {
        printf("Instruction:\t");
        token_print(is->token);
        printf("Issue cycle   = %" PRIu32 "\n", is->issue_cycle);
        printf("Execute cycle = %" PRIu32 "\n", is->exec_cycle);
    }
    printf("\n");
}

static ___inline___ void tomasulo_init(void)
{
    TRACE();

    (void)memset(&tomasulo_data, 0, sizeof(Tomasulo_data));
    tomasulo_data.is_array = darray_create(DARRAY_UNSORTED, 0, sizeof(Instructions_status *), NULL);

    reset_board();
}

static ___inline___ void tomasulo_deinit(void)
{
    TRACE();

    darray_destroy_with_entries(tomasulo_data.is_array, __is_destroy);
}

static ___inline___ void tomasulo_next_cycle(void)
{
    TRACE();

    ++current_cycle();
}

static ___inline___ Instructions_status *tomasulo_add_instruction_to_tracking(Token *token)
{
    Instructions_status *is;

    TRACE();

    LOG("Add token to tracking\n");
    token_dbg_print(token);

    is = (Instructions_status *)malloc(sizeof(Instructions_status));
    if (is == NULL)
        FATAL("Malloc error\n");

    is->token = token;
    is->exec_cycle = 0;
    is->issue_cycle = current_cycle();

    darray_insert(tomasulo_data.is_array, (void *)&is);

    return is;
}

static ___inline___ void setup_work(Worker *worker)
{
    TRACE();
    switch (worker->type)
    {
        case WORKER_IO:
        {
            __setup_work(&worker->io->dst, worker, DEPENDENCY_FROM_DST);
            __setup_work(&worker->io->src, worker, DEPENDENCY_FROM_SRC1);
            break;
        }
        case WORKER_OP:
        {
            __setup_work(&worker->rsc->dst, worker, DEPENDENCY_FROM_DST);
            __setup_work(&worker->rsc->src1, worker, DEPENDENCY_FROM_SRC1);
            __setup_work(&worker->rsc->src2, worker, DEPENDENCY_FROM_SRC2);
            break;
        }
        default:
            break;
    }
}

static ___inline___ void prepare_work(Worker *worker)
{
    TRACE();
    switch (worker->type)
    {
        case WORKER_IO:
        {
            __prepare_work(&worker->io->dst, worker, DEPENDENCY_FROM_DST);
            __prepare_work(&worker->io->src, worker, DEPENDENCY_FROM_SRC1);
            break;
        }
        case WORKER_OP:
        {
            __prepare_work(&worker->rsc->dst, worker, DEPENDENCY_FROM_DST);
            __prepare_work(&worker->rsc->src1, worker, DEPENDENCY_FROM_SRC1);
            __prepare_work(&worker->rsc->src2, worker, DEPENDENCY_FROM_SRC2);
            break;
        }
        default:
            break;
    }
}

static void __setup_work(Variable *var, Worker *worker, int dep_slot)
{
    Register_info *r;
    TRACE();
    
    if(var->type != VAR_REGISTER)
        return;

    r = &board.registers.regs[var->nr];

    r->worker = *worker;
    r->worker_slot = dep_slot;
    r->state = STATE_BUSY;

    switch (worker->type)
    {
        case WORKER_OP:
        {
            LOG("R%" PRIu32 " is free, setup rsc job %d [ %d ]\n", r->nr, r->job, r->aryth_type);
            r->job = worker->rsc->job;
            r->aryth_type = worker->rsc->aryth_type;
            break;
        }
        case WORKER_IO:
        {
            LOG("R%" PRIu32 " is free, setup io job %d\n", r->nr, r->job);
            r->job = worker->io->job;
            break;
        }
        default:
            break;
    }
}

static void __prepare_work(Variable *var, Worker *worker, int dep_slot)
{
    Register_info *r;
    int i;

    TRACE();

    /* prepare only registers */
    if (var->type != VAR_REGISTER)
        return;

    r = &board.registers.regs[var->nr];
    if (r->state == STATE_BUSY && memcmp(worker, &r->worker, sizeof(Worker)))
    {
        LOG("R%" PRIu32 " is busy, wait for this %d job [ %d ]\n", r->nr, r->job, r->aryth_type);

        /* notify that Im waiting for this job */
        switch (r->worker.type)
        {
            case WORKER_OP:
            {
                for (i = 0; i < DEPENDENCY_NR_OF_SLOT_RSC; ++i)
                    if (memcmp(worker, &r->worker.rsc->dependency[i], sizeof(Worker)) == 0)
                    {
                        LOG("The same dependency on %d, so do nothing\n", i);
                        return;
                    }

                worker->slot = dep_slot;
                LOG("Prepare work in rsc for R%" PRIu32 "\n", r->nr);
                r->worker.rsc->dependency[r->worker_slot] = *(Dependency *)worker;
                break;
            }
            case WORKER_IO:
            {
                for (i = 0; i < DEPENDENCY_NR_OF_SLOT_IO; ++i)
                    if (memcmp(worker, &r->worker.io->dependency[i], sizeof(Worker)) == 0)
                    {
                        LOG("The same dependency on %d, so do nothing\n", i);
                        return;
                    }

                worker->slot = dep_slot;
                LOG("Prepare work in io for R%" PRIu32 "\n", r->nr);
                r->worker.io->dependency[r->worker_slot] = *(Dependency *)worker;
                break;
            }
            default:
                break;
        }
        
        /* I have dependency */
        switch (worker->type)
        {
            case WORKER_OP:
            {
                LOG("Set RSC dependency in %d slot\n", dep_slot);
                worker->rsc->has_dependency[dep_slot] = true;
                break;
            }
            case WORKER_IO:
            {
                LOG("Set IO dependency in %d slot\n", dep_slot);
                worker->io->has_dependency[dep_slot] = true;
                break;
            }
            default:
                break;
        }
    }
    else
        __setup_work(var, worker, dep_slot);
}

static ___inline___ bool var_works_with_dep(const Variable *var)
{
    const Register_info *r;

    TRACE();

    if (var->type != VAR_REGISTER)
        return false;

    r = &board.registers.regs[var->nr];
    switch (r->worker.type)
    {
        case WORKER_OP:
            {
                return (r->worker.rsc->dependency[r->worker_slot].type != DEPENDENCY_NONE);
            }
            case WORKER_IO:
            {
                return (r->worker.io->dependency[r->worker_slot].type != DEPENDENCY_NONE);
            }
            default:
                break;
    }

    return false;
}

static void fetch(Token *token)
{
    TRACE();

    const Token_move *tmove;
    const Token_cmp *tcmp;
    const Token_jump *tjump;
    const Token_arythmetic *taryth;
    Reservation_station_chunk *rsc;
    IO_info *io;
    Instructions_status *is;
    Worker worker;
    int32_t time = 0;

    switch (token->type)
    {
        case TOKEN_JUMP:
        {
            LOG("Token jump fetched\n");

            tjump = (Token_jump *)&token->token_jump;
            if (!is_rsc_cmp_busy()) /* cmp flag is set correctly */
            {
                LOG("Cmp rsc is free, so jump now\n");
                do_jump(tjump->type, tjump->line);
                is = tomasulo_add_instruction_to_tracking(token);
                is->exec_cycle = current_cycle();
            }
            else
                LOG("Cmp rsc busy, waiting\n");
            break;
        }
        case TOKEN_CMP:
        {
            LOG("Token cmp fetched\n");
            tcmp = (Token_cmp *)&token->token_cmp;
            if (!is_rsc_cmp_busy())
            {
                LOG("Cmp rsc free, try to setup work\n");

                if (!var_works_with_dep(&tcmp->src1) && !var_works_with_dep(&tcmp->src2))
                    LOG("Regs has not work with dep, setup work\n");
                else
                {
                    LOG("Regs has dependency, waiting\n");
                    break;
                }

                rsc = get_first_free_cmp();

                /* set cmp job */
                rsc->state = STATE_BUSY;
                rsc->job = JOB_CMP;
                rsc->wait_time = CYCLES_CMP;
                rsc->src1 = tcmp->src1;
                rsc->src2 = tcmp->src2;
                rsc->is = tomasulo_add_instruction_to_tracking(token);

                worker.type = WORKER_OP;
                worker.rsc = rsc;

                prepare_work(&worker);

                go_to_next_instruction();
            }
            else
                LOG("Cmp rsc busy, waiting\n");

            break;
        }
        case TOKEN_ARYTHMETIC:
        {
            taryth = (Token_arythmetic *)&token->token_arythmetic;
            LOG("Token arythmetic fetched %d\n", taryth->type);
            switch (taryth->type)
            {
                case OP_ADD:
                {
                    if (time == 0)
                        time = CYCLES_ADD;
                }
                case OP_SUB:
                {
                    if (time == 0)
                        time = CYCLES_ADD;

                    if (!is_rsc_add_sub_busy())
                    {
                        LOG("Add-sub rsc is free, try setup work\n");

                        if (!var_works_with_dep(&taryth->dst) && !var_works_with_dep(&taryth->src1) 
                            && !var_works_with_dep(&taryth->src2))
                        LOG("Regs has not work with dep, setup work\n");
                        else
                        {
                            LOG("Regs has dependency, waiting\n");
                            break;
                        }

                        rsc = get_first_free_add_sub();

                        /* set job */
                        rsc->state = STATE_BUSY;
                        rsc->job = JOB_ARYTHMETIC;
                        rsc->aryth_type = taryth->type;
                        rsc->wait_time = time;
                        rsc->dst = taryth->dst;
                        rsc->src1 = taryth->src1;
                        rsc->src2 = taryth->src2;
                        rsc->is = tomasulo_add_instruction_to_tracking(token);

                        worker.type = WORKER_OP;
                        worker.rsc = rsc;
                     
                        prepare_work(&worker);

                        go_to_next_instruction();
                    }
                    else
                        LOG("Add-sub rsc busy, waiting\n");

                    break;
                }
                case OP_MUL:
                {
                    if (time == 0)
                        time = CYCLES_MUL;
                }
                case OP_DIV:
                {
                    if (time == 0)
                        time = CYCLES_DIV;
                }
                case OP_MOD:
                {
                    if (time == 0)
                        time = CYCLES_MOD;

                    if (!is_rsc_mul_div_mod_busy())
                    {
                        LOG("Mul-Div-Mod rsc free, try setup work\n");

                        if (!var_works_with_dep(&taryth->dst) && !var_works_with_dep(&taryth->src1) 
                            && !var_works_with_dep(&taryth->src2))
                        LOG("Regs has not work with dep, setup work\n");
                        else
                        {
                            LOG("Regs has dependency, waiting\n");
                            break;
                        }

                        rsc = get_first_free_mul_div_mod();

                        /* set job */
                        rsc->state = STATE_BUSY;
                        rsc->job = JOB_ARYTHMETIC;
                        rsc->aryth_type = taryth->type;
                        rsc->wait_time = time;
                        rsc->dst = taryth->dst;
                        rsc->src1 = taryth->src1;
                        rsc->src2 = taryth->src2;
                        rsc->is = tomasulo_add_instruction_to_tracking(token);

                        worker.type = WORKER_OP;
                        worker.rsc = rsc;
                     
                        prepare_work(&worker);

                        go_to_next_instruction();
                    }
                    else
                        LOG("Mul-Div-Mod rsc busy, waiting\n");

                    break;
                }
                default:
                    break;
            }
            break;
        }
        case TOKEN_MOVE:
        {
            tmove = (Token_move *)&token->token_move;

            LOG("Token move fetched\n");
            /* one of them is memory */
            if (tmove->dst.type == VAR_MEMORY || tmove->src.type == VAR_MEMORY)
                time = CYCLES_MOV_MEM;
            else /* register to register or value to register */
                time = CYCLES_MOV_REG;

            if (tmove->dst.type == VAR_REGISTER)
            {
                LOG("Assume is load\n");

                if (!is_io_load_busy())
                {
                    LOG("IO buffer(Load) free, try setup work\n");

                    if (!var_works_with_dep(&tmove->dst) && !var_works_with_dep(&tmove->src))
                        LOG("Regs has not work with dep, setup work\n");
                    else
                    {
                        LOG("Regs has dependency, waiting\n");
                        break;
                    }

                    io = get_first_free_io_load();
                    io->dst = tmove->dst;
                    io->src = tmove->src;
                    io->state = STATE_BUSY;
                    io->job = JOB_LOAD;
                    io->wait_time = time;
                    
                    io->is = tomasulo_add_instruction_to_tracking(token);

                    worker.type = WORKER_IO;
                    worker.io = io;
                     
                    prepare_work(&worker);

                    go_to_next_instruction();
                }
                else
                    LOG("IO buffer(Load) busy, waiting\n");
            }
            else
            {
                LOG("Assume io store\n");
                if (!is_io_write_busy())
                {
                    LOG("IO buffer(Store) free, try setup work\n");

                    if (!var_works_with_dep(&tmove->dst) && !var_works_with_dep(&tmove->src))
                        LOG("Regs has not work with dep, setup work\n");
                    else
                    {
                        LOG("Regs has dependency, waiting\n");
                        break;
                    }

                    io = get_first_free_io_write();
                    io->dst = tmove->dst;
                    io->src = tmove->src;
                    io->state = STATE_BUSY;
                    io->job = JOB_STORE;
                    io->wait_time = time;
                    
                    io->is = tomasulo_add_instruction_to_tracking(token);

                    worker.type = WORKER_IO;
                    worker.io = io;
                     
                    prepare_work(&worker);

                    go_to_next_instruction();
                }
                else
                    LOG("IO buffer(Store) busy, waiting\n");
            }
            break;
        }
        default:
            break;

    }
}

static ___inline___ bool wait_for_unfinished_job(void)
{
    TRACE();

    size_t i;
    for (i = 0; i < REGISTERS_NUM; ++i)
        if (board.registers.regs[i].state == STATE_BUSY)
            return true;

    return false;
}

int tomasulo(Token **program, size_t num_instr)
{
    TRACE();

    LOG("Init tomasulo\n");
    tomasulo_init();

    while (board.pc < num_instr || wait_for_unfinished_job())
    {
        if (board.pc < num_instr)
            fetch(program[board.pc]);

        execute();
        tomasulo_print();
        tomasulo_next_cycle();

        printf("Type any key to go to next cycle\n");
        getch();
        reset_terminal();
    }

    tomasulo_print();
    LOG("Deinit tomasulo\n");
    tomasulo_deinit();
    return 0;
}