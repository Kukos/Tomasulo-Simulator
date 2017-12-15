#ifndef ARCH_H
#define ARCH_H

#include <stdint.h>
#include <generic.h>
#include <tokens.h>
#include <stdbool.h>

/*
    Description of architecture emulated in tomasulo

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com

    LICENCE: GPL 3.0
*/

/* CYCLES */
#define CYCLES_MOV_REG 1
#define CYCLES_MOV_MEM 5
#define CYCLES_ADD 5
#define CYCLES_SUB 5
#define CYCLES_MUL 10
#define CYCLES_DIV 10
#define CYCLES_MOD 10
#define CYCLES_CMP 2

/* registers R0 - R31 */
#define REGISTERS_NUM 32

/* buffers to load from mem to reg */
#define LOAD_BUFFER_SIZE 3

/* buffers to write reg to memory */
#define WRITE_BUFFER_SIZE 3

/* reservation station for add / sub and mul / div / mod */
#define RS_ADD_SUB_SIZE 3
#define RS_MUL_DIV_MOD_SIZE 2

#define RAM_SIZE 64

typedef DWORD reg_t;
typedef DWORD program_counter_t;
typedef int compare_flag_t;

/* register state */
typedef enum
{
    STATE_FREE,
    STATE_BUSY
} state_t;

/* register job */
typedef enum
{
    JOB_IDLE,
    JOB_LOAD,
    JOB_STORE,
    JOB_CMP,
    JOB_JUMP,
    JOB_ARYTHMETIC
} job_t;

typedef enum
{
    DEPENDENCY_NONE,
    DEPENDENCY_IO,
    DEPENDENCY_OP
} dependency_t;

typedef enum
{
    DEPENDENCY_FROM_DST  = 0,
    DEPENDENCY_FROM_SRC1 = 1,
    DEPENDENCY_FROM_SRC2 = 2
} dependency_slot_t;

#define DEPENDENCY_NR_OF_SLOT_IO  2
#define DEPENDENCY_NR_OF_SLOT_RSC 3

typedef enum
{
    WORKER_NONE,
    WORKER_IO,
    WORKER_OP
} worker_t;

typedef struct Dependency
{
    dependency_t type;
    dependency_slot_t slot;
    __extension__ union
    {
        struct IO_info *io;
        struct Reservation_station_chunk *rsc;
    };
} Dependency;

typedef Dependency Worker;

/* Register information (job, state, value) */
typedef struct Register_info
{
    uint32_t nr;
    reg_t val;

    state_t state;
    job_t job;
    arythemtic_t aryth_type;

    Worker worker;
    dependency_slot_t worker_slot;
} Register_info;


typedef struct Registers
{
    Register_info regs[REGISTERS_NUM];
} Registers;

typedef struct RAM
{
    DWORD memory[RAM_SIZE];
} RAM;

/* info about instructions */
typedef struct Instructions_status
{
    uint32_t issue_cycle; /* read fetch decode (as 1 time) */
    uint32_t exec_cycle; /* execute time */

    Token *token;
} Instructions_status;

/* common I/O info */
typedef struct IO_info
{
    Variable dst;
    Variable src;

    state_t state;
    job_t job;

    int32_t wait_time; /* time to end */

    bool has_dependency[DEPENDENCY_NR_OF_SLOT_IO]; /* we depened from another op */
    Dependency dependency[DEPENDENCY_NR_OF_SLOT_IO]; /* this depends from us */

    Instructions_status *is;
} IO_info;

typedef struct Load_buffer
{
    IO_info load[LOAD_BUFFER_SIZE];
} Load_buffer;

typedef struct Write_buffer
{
    IO_info write[WRITE_BUFFER_SIZE];
} Write_buffer;

/* element in reservation statsion */
typedef struct Reservation_station_chunk
{
    state_t state;
    job_t job;
    arythemtic_t aryth_type;

    bool has_dependency[DEPENDENCY_NR_OF_SLOT_RSC]; /* we depened from another op */
    Dependency dependency[DEPENDENCY_NR_OF_SLOT_RSC]; /* this depends from us */
    
    int32_t wait_time; /* time to end */

    Instructions_status *is;

    Variable dst;
    Variable src1;
    Variable src2;
} Reservation_station_chunk;

/* buffers for operations */
typedef struct Reservation_stations
{
    Reservation_station_chunk add[RS_ADD_SUB_SIZE];
    Reservation_station_chunk mul[RS_MUL_DIV_MOD_SIZE];
    Reservation_station_chunk cmp;
} Reservation_stations;

typedef struct Board
{
    Registers               registers;
    RAM                     ram;
    Reservation_stations    rs;
    Write_buffer            write_buffer;
    Load_buffer             load_buffer;
    program_counter_t       pc;
    compare_flag_t          cf;
} Board;

extern Board board;

/*
    Reset Board

    PARAMS
    NO PARAMS

    RETURN
    This is a void function
*/
void reset_board(void);

/*
    Do Compare 2 registers

    PARAMS
    @IN r1 - pointer to 1st register
    @IN r2 - pointer to 2nd register

    RETURN
    This is a void function
*/
void do_cmp(Register_info *r1, Register_info *r2);

/*
    Do Arythmetic operation

    PARAMS
    @IN type - type of OP
    @IN dst - pointer to destination reg
    @IN src1 - pointer to source1 reg
    @IN src2 - pointer to source2 reg

    RETURN
    This is a void function
*/
void do_arythmetic(arythemtic_t type, Register_info *dst, Register_info *src1, Register_info *src2);


/*
    Go to next program instruction

    PARAMS
    NO PARAMS

    RETURN
    This is a void function
*/
void go_to_next_instruction(void);

/*
    Do jump to line

    PARAMS
    @IN type - jump type
    @IN line - line to jump

    RETURN
    This is a void function
*/
void do_jump(jump_t type, uint32_t line);

/*
    Copy data from variable to register

    PARAMS
    @IN num_reg - register number
    @IN var - variable

    RETURN
    This is a void function
*/
void copy_data_to_reg(uint32_t num_reg, Variable *var);

/*
    Copy data from variable to memory

    PARAMS
    @IN addr - address of memory
    @IN var - variable

    RETURN
    This is a void function
*/
void copy_data_to_memory(uint32_t addr, Variable *var);

/*
    Print on stdout dump about whole board

    PARAMS
    NO PARAMS

    RETURN
    This is a void function
*/
void board_dump(void);


#endif