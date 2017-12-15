#ifndef TOMASULO_ASM_H
#define TOMASULO_ASM_H


/*
    Simple asm for simple architecture.
    Only mov, jumps and arthimetic to make tomasulo simpler.

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com

    LICENCE: GPL 3.0
*/


/* Simple asm mnemonics, only needed to simulate Tomasulo */
typedef struct Mnemonics
{  

    const char *mov; /* mov dst src1 (ds1 = src1) */

    const char *add; /* add dst src1 src2 (dst = src1 + src2) */
    const char *sub; /* sub dst src1 src2 (dst = src1 - src2) */
    const char *mul; /* mul dst src1 src2 (dst = src1 * src2) */
    const char *div; /* div dst src1 src2 (dst = src1 / src2) */
    const char *mod; /* mod dst src1 src2 (dst = src1 % src2) */

    const char *cmp; /* cmp src1 src2 
                        (CF = 0 iff src1 == src2)
                        (CF = 1 iff src1 > src2)
                        (CF = -1 iff src1 < src2) 
                     */

    const char *je;  /* je src1  (if (CF == 0)  PC = src1) */
    const char *jne; /* jne src1 (if (CF != 0)  PC = src1) */
    const char *jlt; /* jlt src1  (if (CF == -1) PC = src1) */
    const char *jle; /* jle src1 (if (CF != -1) PC = src1) */
    const char *jgt; /* jgt src1  (if (CF == 1)  PC = src1) */
    const char *jge; /* jge src1 (if (CF != 1)  PC = src1) */

} Mnemonics;

extern const Mnemonics mnemonics;
extern const char memory_c;
extern const char register_c;
extern const char decimal_mode_c;
extern const char octal_mode_c;
extern const char binary_mode_c;

#endif
