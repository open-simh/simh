/* 6809.c: SWTP 6809 CPU simulator^

   Copyright (c) 2005-2012, William Beech

       Permission is hereby granted, free of charge, to any person obtaining a
       /opy of this software and associated documentation files (the "Software"),
       to deal in the Software without restriction, including without limitation
       the rights to use, copy, modify, merge, publish, distribute, sublicense,
       and/or sell copies of the Software, and to permit persons to whom the
       Software is furnished to do so, subject to the following conditions:

       The above copyright notice and this permission notice shall be included in
       all copies or substantial portions of the Software.

       THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
       IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
       FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
       WILLIAM A. BEECH BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
       IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
       CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

       Except as contained in this notice, the name of William A. Beech shall not
       be used in advertising or otherwise to promote the sale, use or other dealings
       in this Software without prior written authorization from William A. Beech.

    The following copyright notice applies to the SWTP 6809 source, binary, and documentation:
 
    Original code published in 2024, written by Richard F Lukes
    Copyright (c) 2024, Richard F Lukes
 
       Permission is hereby granted, free of charge, to any person obtaining a
       copy of this software and associated documentation files (the "Software"),
       to deal in the Software without restriction, including without limitation
       the rights to use, copy, modify, merge, publish, distribute, sublicense,
       and/or sell copies of the Software, and to permit persons to whom the
       Software is furnished to do so, subject to the following conditions:
 
       The above copyright notice and this permission notice shall be included in
       all copies or substantial portions of the Software.
 
       THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
       IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
       FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
       THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
       IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
       CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
       Except as contained in this notice, the names of The Authors shall not be
       used in advertising or otherwise to promote the sale, use or other dealings
       in this Software without prior written authorization from the Authors.

    MODIFICATIONS:

        23 Apr 15 -- Modified to use simh_debug
        21 Apr 20 -- Richard Brinegar numerous fixes for flag errors
	04 Apr 24 -- Richard Lukes - modified for 6809

    NOTES:
       cpu                  Motorola M6809 CPU

       The register state for the M6809 CPU is:

       A<0:7>               Accumulator A
       B<0:7>               Accumulator B
       IX<0:15>             Index Register
       IY<0:15>             Index Register
       CCR<0:7>             Condition Code Register
           EF                   Entire flag
           FF                   FIRQ flag
           HF                   half-carry flag
           IF                   IRQ flag
           NF                   negative flag
           ZF                   zero flag
           VF                   overflow flag
           CF                   carry flag
       PC<0:15>             program counter
       UP<0:15>             User Stack Pointer
       SP<0:15>             Stack Pointer

       The M6809 is an 8-bit CPU, which uses 16-bit registers to address
       up to 64KB of memory.

       The 72 basic instructions come in 1, 2, and 3-byte flavors.

       This routine is the instruction decode routine for the M6809.
       It is called from the CPU board simulator to execute
       instructions in simulated memory, starting at the simulated PC.
       It runs until 'reason' is set non-zero.

       General notes:

       1. Reasons to stop.  The simulator can be stopped by:

            WAI instruction
            I/O error in I/O simulator
            Invalid OP code (if ITRAP is set on CPU)
            Invalid mamory address (if MTRAP is set on CPU)

       2. Interrupts.
          There are 4 types of interrupt, and in effect they do a
          hardware CALL instruction to one of 4 possible high memory addresses.

       3. Non-existent memory.
            On the SWTP 6809, reads to non-existent memory
            return 0FFH, and writes are ignored.
*/

#include <stdio.h>

#include "swtp_defs.h"

#define UNIT_V_OPSTOP   (UNIT_V_UF)     /* Stop on Invalid Opcode */
#define UNIT_OPSTOP     (1 << UNIT_V_OPSTOP)

/* Flag values to set proper positions in CCR */
#define EF      0x80
#define FF      0x40
#define HF      0x20
#define IF      0x10
#define NF      0x08
#define ZF      0x04
#define VF      0x02
#define CF      0x01

/* PSH/PUL Post Byte register positions */
#define PSH_PUL_Post_Byte_PC	0x80
#define PSH_PUL_Post_Byte_S_U	0x40
#define PSH_PUL_Post_Byte_Y	0x20
#define PSH_PUL_Post_Byte_X	0x10
#define PSH_PUL_Post_Byte_DP	0x08
#define PSH_PUL_Post_Byte_B	0x04
#define PSH_PUL_Post_Byte_A	0x02
#define PSH_PUL_Post_Byte_CC	0x01

#define TFR_EXG_Post_Nybble_D	0
#define TFR_EXG_Post_Nybble_X	1
#define TFR_EXG_Post_Nybble_Y	2
#define TFR_EXG_Post_Nybble_U	3
#define TFR_EXG_Post_Nybble_S	4
#define TFR_EXG_Post_Nybble_PC	5
#define TFR_EXG_Post_Nybble_A	8
#define TFR_EXG_Post_Nybble_B	9
#define TFR_EXG_Post_Nybble_CC	10
#define TFR_EXG_Post_Nybble_DP	11

/* Exclusive OR macro for logical expressions */
#define EXOR(a,b) (((a)!=0) != ((b)!=0))

/* Macros to handle the flags in the CCR */
#define CCR_MSK (EF|FF|HF|IF|NF|ZF|VF|CF)
#define TOGGLE_FLAG(FLAG)   (CCR ^= (FLAG))
#define SET_FLAG(FLAG)      (CCR |= (FLAG))
#define CLR_FLAG(FLAG)      (CCR &= ~(FLAG))
#define GET_FLAG(FLAG)      (CCR & (FLAG))

#define COND_SET_FLAG(COND,FLAG) \
    if (COND) SET_FLAG(FLAG); else CLR_FLAG(FLAG)

#define COND_SET_FLAG_N(VAR) \
    if ((VAR) & 0x80) SET_FLAG(NF); else CLR_FLAG(NF)
#define COND_SET_FLAG_N_16(VAR) \
    if ((VAR) & 0x8000) SET_FLAG(NF); else CLR_FLAG(NF)

#define COND_SET_FLAG_Z(VAR) \
    if ((VAR) == 0) SET_FLAG(ZF); else CLR_FLAG(ZF)
#define COND_SET_FLAG_Z_16(VAR) \
    if ((VAR) == 0) SET_FLAG(ZF); else CLR_FLAG(ZF)

#define COND_SET_FLAG_C(VAR) \
    if ((VAR) & 0x100) SET_FLAG(CF); else CLR_FLAG(CF)
#define COND_SET_FLAG_C_16(VAR) \
    if ((VAR) & 0x10000) SET_FLAG(CF); else CLR_FLAG(CF)

/* local global variables */

int32 A = 0;                            /* Accumulator A */
int32 B = 0;                            /* Accumulator B */
int32 D = 0;				/* Accumulator D */
int32 DP = 0;                           /* Direct Page Register */
int32 IX = 0;                           /* Index register X */
int32 IY = 0;                           /* Index register Y */
int32 UP = 0;                           /* User Stack pointer */
int32 SP = 0;                           /* Hardware Stack pointer */
int32 CCR = EF|FF|IF;                   /* Condition Code Register */
int32 saved_PC = 0;                     /* Saved Program counter */
int32 previous_PC = 0;                  /* Previous previous Program counter */
int32 last_PC = 0;                      /* Last Program counter */
int32 PC;                               /* Program counter */
int32 INTE = 0;                         /* Interrupt Enable */

int32 int_req = 0;                      /* Interrupt request */
int32 mem_fault = 0;                    /* memory fault flag */

#define m6809_NAME    "Motorola M6809 Processor Chip"

/* function prototypes */

t_stat m6809_reset(DEVICE *dptr);
t_stat m6809_boot(int32 unit_num, DEVICE *dptr);
t_stat m6809_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches);
t_stat m6809_deposit(t_value value, t_addr addr, UNIT *uptr, int32 switches);
static const char* m6809_desc(DEVICE *dptr) { return m6809_NAME; }


void dump_regs(void);
void dump_regs1(void);
int32 fetch_byte(int32 flag);
int32 fetch_word();
uint8 pop_sp_byte(void);
uint8 pop_up_byte(void);
uint16 pop_sp_word(void);
uint16 pop_up_word(void);
void push_sp_byte(uint8 val);
void push_up_byte(uint8 val);
void push_sp_word(uint16 val);
void push_up_word(uint16 val);
void go_rel(int32 cond);
void go_long_rel(int32 cond);
int32 get_rel_addr(void);
int32 get_long_rel_addr(void);
int32 get_dir_byte_val(void);
int32 get_dir_word_val(void);
int32 get_imm_byte_val(void);
int32 get_imm_word_val(void);
int32 get_dir_addr(void);
int32 get_indexed_byte_val(void);
int32 get_indexed_word_val(void);
int32 get_indexed_addr(void);
int32 get_ext_addr(void);
int32 get_ext_byte_val(void);
int32 get_ext_word_val(void);
int32 get_flag(int32 flag);
void condevalVa(int32 op1, int32 op2);
void condevalVa16(int32 op1, int32 op2);
void condevalVs(int32 op1, int32 op2);
void condevalVs16(int32 op1, int32 op2);
void condevalHa(int32 op1, int32 op2);

/* external routines */

extern void CPU_BD_put_mbyte(int32 addr, int32 val);
extern void CPU_BD_put_mword(int32 addr, int32 val);
extern int32 CPU_BD_get_mbyte(int32 addr);
extern int32 CPU_BD_get_mword(int32 addr);

/* CPU data structures

   m6809_dev        CPU device descriptor
   m6809_unit       CPU unit descriptor
   m6809_reg        CPU register list
   m6809_mod        CPU modifiers list */

UNIT m6809_unit = { UDATA (NULL, 0, 0) };

REG m6809_reg[] = {
    { HRDATA (PC, saved_PC, 16) },
    { HRDATA (A, A, 8) },
    { HRDATA (B, B, 8) },
    { HRDATA (DP, DP, 8) },
    { HRDATA (IX, IX, 16) },
    { HRDATA (IY, IY, 16) },
    { HRDATA (SP, SP, 16) },
    { HRDATA (UP, UP, 16) },
    { HRDATA (CCR, CCR, 8) },
    { FLDATA (INTE, INTE, 16) },
    { ORDATA (WRU, sim_int_char, 8) },
    { NULL }  };

MTAB m6809_mod[] = {
    { UNIT_OPSTOP, UNIT_OPSTOP, "ITRAP", "ITRAP", NULL },
    { UNIT_OPSTOP, 0, "NOITRAP", "NOITRAP", NULL },
    { 0 }  };

DEBTAB m6809_debug[] = {
    { "ALL", DEBUG_all },
    { "FLOW", DEBUG_flow },
    { "READ", DEBUG_read },
    { "WRITE", DEBUG_write },
    { "LEV1", DEBUG_level1 },
    { "LEV2", DEBUG_level2 },
    { "REG", DEBUG_reg },
    { "ASM", DEBUG_asm },
    { NULL }
};

DEVICE m6809_dev = {
    "CPU",                              //name
    &m6809_unit,                        //units
    m6809_reg,                          //registers
    m6809_mod,                          //modifiers
    1,                                  //numunits
    16,                                 //aradix
    16,                                 //awidth
    1,                                  //aincr
    16,                                 //dradix
    8,                                  //dwidth
    &m6809_examine,                     //examine
    &m6809_deposit,                     //deposit
    &m6809_reset,                       //reset
    &m6809_boot,                        //boot
    NULL,                               //attach
    NULL,                               //detach
    NULL,                               //ctxt
    DEV_DEBUG,                          //flags
    0,                                  //dctrl
    m6809_debug,                        //debflags
    NULL,                               //msize
    NULL,                               //lname
    NULL,                               //help routine
    NULL,                               //attach help routine
    NULL,                               //help context
    &m6809_desc                         //device description
};

static const char *opcode[] = {
"NEG", "?01?", "?02?", "COM",           //0x00
"LSR", "?05?", "ROR", "ASR",
"ASL", "ROL", "DEC", "???",
"INC", "TST", "JMP", "CLR",
"PG2", "PG3", "NOP", "SYNC",            //0x10
"?14?", "?15?", "LBRA", "LBSR",
"?18?", "DAA", "ORCC", "?1B?",
"ANDCC", "SEX", "EXG", "TFR",
"BRA", "BRN", "BHI", "BLS",             //0x20
"BCC", "BCS", "BNE", "BEQ",
"BVC", "BVS", "BPL", "BMI",
"BGE", "BLT", "BGT", "BLE",
"LEAX", "LEAY", "LEAS", "LEAU",         //0x30
"PSHS", "PULS", "PSHU", "PULU",
"?38?", "RTS", "ABX", "RTI",
"CWAI", "MUL", "?3E?", "SWI",
"NEGA", "?41?", "?42?", "COMA",         //0x40
"LSRA", "?45?", "RORA", "ASRA",
"ASLA", "ROLA", "DECA", "?4B?",
"INCA", "TSTA", "?4E?", "CLRA",
"NEGB", "?51?", "?52?", "COMB",         //0x50
"LSRB", "?55?", "RORB", "ASRB",
"ASLB", "ROLB", "DECB", "?5B?",
"INCB", "TSTB", "?5E?", "CLRB",
"NEG", "?61?", "?62?", "COM",           //0x60
"LSR", "?65?", "ROR", "ASR",
"ASL", "ROL", "DEC", "?6B?",
"INC", "TST", "JMP", "CLR",
"NEG", "?71?", "?72?", "COM",           //0x70
"LSR", "?75?", "ROR", "ASR",
"ASL", "ROL", "DEC", "?7B?",
"INC", "TST", "JMP", "CLR",
"SUBA", "CMPA", "SBCA", "SUBD",         //0x80
"ANDA", "BITA", "LDA", "?87?",
"EORA", "ADCA", "ORAA", "ADDA",
"CMPX", "BSR", "LDX", "?8F?",
"SUBA", "CMPA", "SBCA", "SUBD",         //0x90
"ANDA", "BITA", "LDA", "STA",
"EORA", "ADCA", "ORA", "ADDA",
"CMPX", "JSR", "LDX", "STX",
"SUBA", "CMPA", "SBCA", "SUBD",         //0xA0
"ANDA", "BITA", "LDA", "STA",
"EORA", "ADCA", "ORA", "ADDA",
"CMPX", "JSR", "LDX", "STX",
"SUBA", "CMPA", "SBCA", "SUBD",         //0xB0
"ANDA", "BITA", "LDA", "STA",
"EORA", "ADCA", "ORA", "ADDA",
"CMPX", "JSR", "LDX", "STX",
"SUBB", "CMPB", "SBCB", "ADDD",         //0xC0
"ANDB", "BITB", "LDB", "?C7?",
"EORB", "ADCB", "ORB", "ADDB",
"LDD", "?CD?", "LDU", "?CF?",
"SUBB", "CMPB", "SBCB", "ADDD",         //0xD0
"ANDB", "BITB", "LDB", "STB",
"EORB", "ADCB", "ORB", "ADDB",
"LDD", "STD", "LDU", "STU",
"SUBB", "CMPB", "SBCB", "ADDD",         //0xE0
"ANDB", "BITB", "LDB", "STB",
"EORB", "ADCB", "ORB", "ADDB",
"LDD", "STD", "LDU", "STU",
"SUBB", "CMPB", "SBCB", "ADDD",         //0xF0
"ANDB", "BITB", "LDB", "STB",
"EORB", "ADCB", "ORB", "ADDB",
"LDD", "STD", "LDU", "STU"
};

int32 oplen[256] = {
2,0,0,2,2,0,2,2,2,2,2,0,2,2,2,2,        //0x00
2,2,1,1,0,0,3,3,0,1,2,0,2,1,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,0,1,1,1,2,1,0,1,
1,0,0,1,1,0,1,1,1,1,1,0,1,1,0,1,        //0x40
1,0,0,1,1,0,1,1,1,1,1,0,1,1,0,1,
2,0,0,2,2,0,2,2,2,2,2,0,2,2,2,2,
3,0,0,3,3,0,3,3,3,3,3,0,3,3,3,3,
2,2,2,3,2,2,2,0,2,2,2,2,3,2,3,0,        //0x80
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
2,2,2,3,2,2,2,0,2,2,2,2,3,0,3,0,        //0xC0
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3
};

t_stat sim_instr (void)
{
    int32 reason;
    int32 IR;
    int32 OP;
    int32 OP2;		/* Used for 2-byte opcodes */

    int32 hi;		/* hi bit/nybble/byte */
    int32 lo;		/* lo bit/nybble/byte */
    int32 op1;		/* operand #1 */
    int32 op2;		/* operand #2 - used for CC evaluation */
    int32 result;	/* temporary value */
    int32 addr;		/* temporary address value */

    /* used for 6809 instruction decoding - TFT, EXG, PSH, PUL*/
    int32 Post_Byte = 0;
    int32 Src_Nybble = 0;
    int32 Dest_Nybble = 0;
    int32 Src_Value = 0;
    int32 Dest_Value = 0;

    PC = saved_PC & ADDRMASK;           /* load local PC */
    reason = 0;

    /* Main instruction fetch/decode loop */

    while (reason == 0) {               /* loop until halted */
        if (sim_interval <= 0)          /* check clock queue */
            if ((reason = sim_process_event ())) {
                break;
            }
        if (mem_fault != 0) {           /* memory fault? */
            mem_fault = 0;              /* reset fault flag */
            reason = STOP_MEMORY;
            break;
        }
        if (int_req > 0) {              /* interrupt? */
        /* 6809 interrupts not implemented yet.  None were used,
           on a standard SWTP 6809. All I/O is programmed. */
            reason = STOP_HALT;         /* stop simulation */
            break;

        }                               /* end interrupt */
        if (sim_brk_summ &&
            sim_brk_test (PC, SWMASK ('E'))) { /* breakpoint? */
            reason = STOP_IBKPT;        /* stop simulation */
            break;
        }

        sim_interval--;
        last_PC = previous_PC;
        previous_PC = PC;
        IR = OP = fetch_byte(0);        /* fetch instruction */

        /* The Big Instruction Decode Switch */

        switch (IR) {

/* 0x00 - direct mode */
            case 0x00:                  /* NEG dir */
                addr = get_dir_addr();
                op1 = CPU_BD_get_mbyte(addr);
 		COND_SET_FLAG(op1 != 0, CF);
                COND_SET_FLAG(op1 == 0x80, VF);
		result = 0 - op1;
		result &= 0xFF;
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                break;

            case 0x03:                  /* COM dir */
                addr = get_dir_addr();
                op1 = CPU_BD_get_mbyte(addr);
		result = (~op1) & 0xFF;
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                CLR_FLAG(VF);
                SET_FLAG(CF);
                break;

            case 0x04:                  /* LSR dir */
                addr = get_dir_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = op1 >> 1;
                CPU_BD_put_mbyte(addr, result);
		CLR_FLAG(NF);
		COND_SET_FLAG_Z(result);
                /* H,V not affected */
 		COND_SET_FLAG(op1 & 0x01, CF);
                break;

            case 0x06:                  /* ROR dir */
                addr = get_dir_addr();
                op1 = CPU_BD_get_mbyte(addr);
		result = op1 >> 1;
                if (get_flag(CF)) {
                    result |= 0x80;
                }
                CPU_BD_put_mbyte(addr, result);
		COND_SET_FLAG_N(result);
		COND_SET_FLAG_Z(result);
		/* H,V unaffected */
 		COND_SET_FLAG(op1 & 0x01, CF);
                break;

            case 0x07:                  /* ASR dir */
                addr = get_dir_addr();
                op1 = CPU_BD_get_mbyte(addr);
		result = (op1 >> 1) | (op1 & 0x80);
                CPU_BD_put_mbyte(addr, result);
                /* H undefined */
		COND_SET_FLAG_N(result);
		COND_SET_FLAG_Z(result);
                /* V unaffected */
 		COND_SET_FLAG(op1 & 0x01, CF);
                break;

            case 0x08:                  /* ASL dir */
                addr = get_dir_addr();
                op1 = CPU_BD_get_mbyte(addr);
		result = (op1 << 1) & BYTEMASK;
                CPU_BD_put_mbyte(addr, result);
                /* H is undefined */
		COND_SET_FLAG_N(result);
		COND_SET_FLAG_Z(result);
 		COND_SET_FLAG(op1 & 0x80, CF);
                COND_SET_FLAG(EXOR(get_flag(NF),get_flag(CF)), VF);
                break;

            case 0x09:                  /* ROL dir */
                addr = get_dir_addr();
                op1 = CPU_BD_get_mbyte(addr);
		result = (op1 << 1) & BYTEMASK;
                if (get_flag(CF)) {
                    result |= 0x01;
                }
                CPU_BD_put_mbyte(addr, result);
		COND_SET_FLAG_N(result);
		COND_SET_FLAG_Z(result);
 		COND_SET_FLAG(op1 & 0x80, CF);
                COND_SET_FLAG(EXOR(get_flag(NF),get_flag(CF)), VF);
                break;

            case 0x0A:                  /* DEC dir */
                addr = get_dir_addr();
                op1 = CPU_BD_get_mbyte(addr);
		result = (op1 - 1) & BYTEMASK;
                CPU_BD_put_mbyte(addr, result);
		COND_SET_FLAG_N(result);
		COND_SET_FLAG_Z(result);
 		COND_SET_FLAG(op1 == 0x80, VF);
                /* C unaffected */
                break;

            case 0x0C:                  /* INC dir */
                addr = get_dir_addr();
                op1 = CPU_BD_get_mbyte(addr);
		result = (op1 + 1) & BYTEMASK;
                CPU_BD_put_mbyte(addr, result);
		COND_SET_FLAG_N(result);
		COND_SET_FLAG_Z(result);
 		COND_SET_FLAG(op1 == 0x7F, VF);
                /* C unaffected */
                break;

            case 0x0D:                  /* TST dir */
		result = get_dir_byte_val();
		COND_SET_FLAG_N(result);
		COND_SET_FLAG_Z(result);
 		CLR_FLAG(VF);
                /* C unaffected */
                break;

            case 0x0E:                  /* JMP dir */
                PC = get_dir_addr();
                break;

            case 0x0F:                  /* CLR dir */
                CPU_BD_put_mbyte(get_dir_addr(), 0);
                /* H not affected */
                CLR_FLAG(NF);
                SET_FLAG(ZF);
                CLR_FLAG(VF);
                CLR_FLAG(CF);
                break;

/* 0x10 */
            case 0x10:                  /* 2-byte opcodes */
                /* fetch second byte of opcode */
                OP2 = fetch_byte(1);
        	switch (OP2) {
            	    case 0x21:              /* LBRN rel */
		        /* Branch Never - essentially a NOP */
		        go_long_rel(0);
                        break;
                    case 0x22:              /* LBHI rel */
                        go_long_rel(!(get_flag(CF) || get_flag(ZF)));
                        break;
                    case 0x23:              /* LBLS rel */
                        go_long_rel(get_flag(CF) || get_flag(ZF));
                        break;
                    case 0x24:              /* LBCC rel */
                        go_long_rel(!get_flag(CF));
                        break;
                    case 0x25:              /* LBCS rel */
                        go_long_rel(get_flag(CF));
                        break;
                    case 0x26:              /* LBNE rel */
                        go_long_rel(!get_flag(ZF));
                        break;
                    case 0x27:              /* LBEQ rel */
                        go_long_rel(get_flag(ZF));
                        break;
                    case 0x28:              /* LBVC rel */
                        go_long_rel(!get_flag(VF));
                        break;
                    case 0x29:              /* LBVS rel */
                        go_long_rel(get_flag(VF));
                        break;
                    case 0x2A:              /* LBPL rel */
                        go_long_rel(!get_flag(NF));
                        break;
                    case 0x2B:              /* LBMI rel */
                        go_long_rel(get_flag(NF));
                        break;
                    case 0x2C:              /* LBGE rel */
                        go_long_rel(get_flag(NF) == get_flag(VF));
                        break;
                    case 0x2D:              /* LBLT rel */
                        go_long_rel(get_flag(NF) != get_flag(VF));
                        break;
                    case 0x2E:              /* LBGT rel */
                        go_long_rel((get_flag(ZF) == 0) && (get_flag(NF) == get_flag(VF)));
                        break;
                    case 0x2F:              /* LBLE rel */
                        go_long_rel(get_flag(ZF) || (get_flag(NF) != get_flag(VF)));
			break;

		    case 0x3F:              /* SWI2 */
                        SET_FLAG(EF);
                        push_sp_word(PC);
                        push_sp_word(UP);
                        push_sp_word(IY);
                        push_sp_word(IX);
                        push_sp_byte(DP);
                        push_sp_byte(B);
                        push_sp_byte(A);
                        push_sp_byte(CCR);
                        PC = CPU_BD_get_mword(0xFFF4);
                        break;

            	    case 0x83:                  /* CMPD imm */
                        /* do not modify D|A|B */
			D = (A << 8) | (B & BYTEMASK);
                	op2 = get_imm_word_val();
                	result = D - op2;
                	COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(D, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0x8C:                  /* CMPY imm */
                        /* do not modify Y */
                	op2 = get_imm_word_val();
                	result = IY - op2;
                	COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(IY, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0x8E:                  /* LDY imm */
                	IY = get_imm_word_val();
                	COND_SET_FLAG_N_16(IY);
                	COND_SET_FLAG_Z_16(IY);
                	CLR_FLAG(VF);
                	break;

            	    case 0x93:                  /* CMPD dir */
                        /* do not modify D|A|B */
			D = (A << 8) | (B & BYTEMASK);
                	op2 = get_dir_word_val();
                	result = D - op2;
			COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(D, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0x9C:                  /* CMPY dir */
                        /* do not modify Y */
                	op2 = get_dir_word_val();
                	result = IY - op2;
			COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(IY, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0x9E:                  /* LDY dir */
                	IY = get_dir_word_val();
                	COND_SET_FLAG_N_16(IY);
                	COND_SET_FLAG_Z_16(IY);
                	CLR_FLAG(VF);
                	break;

            	    case 0x9F:                  /* STY dir */
                	CPU_BD_put_mword(get_dir_addr(), IY);
                	COND_SET_FLAG_N_16(IY);
                	COND_SET_FLAG_Z_16(IY);
                	CLR_FLAG(VF);
                	break;

            	    case 0xA3:                  /* CMPD ind */
                        /* do not modify D|A|B */
			D = (A << 8) | (B & BYTEMASK);
                	op2 = get_indexed_word_val();
                	result = D - op2;
			COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(D, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0xAC:                  /* CMPY ind */
                        /* do not modify Y */
                	op2 = get_indexed_word_val();
                	result = IY - op2;
			COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(IY, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0xAE:                  /* LDY ind */
                	IY = get_indexed_word_val();
                	COND_SET_FLAG_N_16(IY);
                	COND_SET_FLAG_Z_16(IY);
                	CLR_FLAG(VF);
                	break;

            	    case 0xAF:                  /* STY ind */
                	CPU_BD_put_mword(get_indexed_addr(), IY);
                	COND_SET_FLAG_N_16(IY);
                	COND_SET_FLAG_Z_16(IY);
                	CLR_FLAG(VF);
                	break;

            	    case 0xB3:                  /* CMPD ext */
                        /* Do not modify D|A|B */
			D = (A << 8) | (B & BYTEMASK);
                	op2 = get_ext_word_val();
                	result = D - op2;
			COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(D, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0xBC:                  /* CMPY ext */
                        /* Do not modify D|A|B */
                	op2 = get_ext_word_val();
                	result = IY - op2;
			COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(IY, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0xBE:                  /* LDY ext */
                	IY = get_ext_word_val();
                	COND_SET_FLAG_N_16(IY);
                	COND_SET_FLAG_Z_16(IY);
                	CLR_FLAG(VF);
                	break;

            	    case 0xBF:                  /* STY ext */
                	CPU_BD_put_mword(get_ext_addr(), IY);
                	COND_SET_FLAG_N_16(IY);
                	COND_SET_FLAG_Z_16(IY);
                	CLR_FLAG(VF);
                	break;

            	    case 0xCE:                  /* LDS imm */
                	SP = get_imm_word_val();
                	COND_SET_FLAG_N_16(SP);
                	COND_SET_FLAG_Z_16(SP);
                	CLR_FLAG(VF);
                	break;

            	    case 0xDE:                  /* LDS dir */
                	SP = get_dir_word_val();
                	COND_SET_FLAG_N_16(SP);
                	COND_SET_FLAG_Z_16(SP);
                	CLR_FLAG(VF);
                	break;

                    case 0xDF:                  /* STS dir */
                	CPU_BD_put_mword(get_dir_addr(), SP);
                	COND_SET_FLAG_N_16(SP);
                	COND_SET_FLAG_Z_16(SP);
                	CLR_FLAG(VF);
                	break;

            	    case 0xEE:                  /* LDS ind */
                	SP = get_indexed_word_val();
                	COND_SET_FLAG_N_16(SP);
                	COND_SET_FLAG_Z_16(SP);
                	CLR_FLAG(VF);
                	break;

            	    case 0xEF:                  /* STS ind */
                	CPU_BD_put_mword(get_indexed_addr(), SP);
                	COND_SET_FLAG_N_16(SP);
                	COND_SET_FLAG_Z_16(SP);
                	CLR_FLAG(VF);
                	break;

            	    case 0xFE:                  /* LDS ext */
                	SP = get_ext_word_val();
                	COND_SET_FLAG_N_16(SP);
                	COND_SET_FLAG_Z_16(SP);
                	CLR_FLAG(VF);
                	break;

            	    case 0xFF:                  /* STS ext */
                	CPU_BD_put_mword(get_ext_addr(), SP);
                	COND_SET_FLAG_N_16(SP);
                	COND_SET_FLAG_Z_16(SP);
                	CLR_FLAG(VF);
                	break;
		}
		break;

/* Ox11 */
            case 0x11:                  /* 2-byte opcodes */
                /* fetch second byte of opcode */
                OP2 = fetch_byte(1);
        	switch (OP2) {

		    case 0x3F:		/* SWI3 */
                        SET_FLAG(EF);
                        push_sp_word(PC);
                        push_sp_word(UP);
                        push_sp_word(IY);
                        push_sp_word(IX);
                        push_sp_byte(DP);
                        push_sp_byte(B);
                        push_sp_byte(A);
                        push_sp_byte(CCR);
                        PC = CPU_BD_get_mword(0xFFF2);
                        break;

            	    case 0x83:                  /* CMPU imm */
                        /* Do not modify UP */
                	op2 = get_imm_word_val();
                	result = UP - op2;
                	COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(UP, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0x8C:                  /* CMPS imm */
                        /* Do not modify SP */
                	op2 = get_imm_word_val();
                	result = SP - op2;
                	COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(SP, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0x93:                  /* CMPU dir */
                        /* Do not modify UP */
                	op2 = get_dir_word_val();
                	result = UP - op2;
			COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(UP, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0x9C:                  /* CMPS dir */
                        /* Do not modify SP */
                	op2 = get_dir_word_val();
                	result = SP - op2;
			COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(SP, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0xA3:                  /* CMPU ind */
                        /* Do not modify UP */
                	op2 = get_indexed_word_val();
                	result = UP - op2;
			COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(UP, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0xAC:                  /* CMPS ind */
                        /* Do not modify SP */
                	op2 = get_indexed_word_val();
                	result = SP - op2;
			COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(SP, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0xB3:                  /* CMPU ext */
                        /* Do not modify UP */
                	op2 = get_ext_word_val();
                	result = UP - op2;
			COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(UP, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

            	    case 0xBC:                  /* CMPS ext */
                        /* Do not modify SP */
                	op2 = get_ext_word_val();
                	result = SP - op2;
			COND_SET_FLAG_N_16(result);
                	COND_SET_FLAG_Z_16(result);
                	condevalVs16(SP, op2);
                	COND_SET_FLAG_C_16(result);
                	break;

                    default:
                        reason = STOP_OPCODE;     /* stop simulation */
                        break;
		}
		break;

            case 0x12:                  /* NOP */
                break;

            case 0x13:                  /* SYNC inherent*/
                /* Interrupts are not implemented */
                reason = STOP_HALT;     /* stop simulation */
                break;

            case 0x16:                  /* LBRA relative */
		go_long_rel(1);
                break;

            case 0x17:                  /* LBSR relative */
                addr = get_long_rel_addr();
                push_sp_word(PC);
                PC = (PC + addr) & ADDRMASK;
                break;

            case 0x19:                  /* DAA inherent */
                lo = A & 0x0F;
                hi = (A >> 4) & 0x0F;
                if ((lo > 9) || get_flag(HF)) {
                    /* add correction factor of 0x06 for lower nybble */
                    A += 0x06;
                }
                if ((hi > 9) || get_flag(CF) || ((hi > 8) && (lo > 9))) {
                    /* add correction factor of 0x60 for upper nybble */
                    A += 0x60;
                    A |= 0x100; /* set the C flag */
                }
                COND_SET_FLAG_C(A);
                A = A & BYTEMASK;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                break;

            case 0x1A:                  /* ORCC imm */
		CCR = CCR | get_imm_byte_val();
                break;

            case 0x1C:                  /* ANDCC imm */
		CCR = CCR & get_imm_byte_val();
                break;

            case 0x1D:                  /* SEX inherent */
		if (B & 0x80) {
		    A = 0xFF;
		} else {
		    A = 0;
		}
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                break;

            case 0x1E:                  /* EXG imm */
		Post_Byte = get_imm_byte_val();
		Src_Nybble = (Post_Byte >> 4) & 0x0F;
		Dest_Nybble = Post_Byte & 0x0F;
		if ((Src_Nybble <= 5 && Dest_Nybble > 5) || (Src_Nybble > 5 && Dest_Nybble <= 5)) {
                    // EXG with unaligned register sizes
                    reason = STOP_OPCODE;
		}
                /* read register values */
		if (Src_Nybble <= 5 && Dest_Nybble <= 5) {
		    /* 16-bit register */
		    /* read source register */
		    switch (Src_Nybble) {
		        case TFR_EXG_Post_Nybble_D: Src_Value = (A << 8) | (B & BYTEMASK); break;
		        case TFR_EXG_Post_Nybble_X: Src_Value = IX; break;
		        case TFR_EXG_Post_Nybble_Y: Src_Value = IY; break;
		        case TFR_EXG_Post_Nybble_U: Src_Value = UP; break;
		        case TFR_EXG_Post_Nybble_S: Src_Value = SP; break;
		        case TFR_EXG_Post_Nybble_PC: Src_Value = PC; break;
			default: break;
		    }
		    /* read destination register */
		    switch (Dest_Nybble) {
		        case TFR_EXG_Post_Nybble_D: Dest_Value = (A << 8) | (B & BYTEMASK); break;
		        case TFR_EXG_Post_Nybble_X: Dest_Value = IX; break;
		        case TFR_EXG_Post_Nybble_Y: Dest_Value = IY; break;
		        case TFR_EXG_Post_Nybble_U: Dest_Value = UP; break;
		        case TFR_EXG_Post_Nybble_S: Dest_Value = SP; break;
		        case TFR_EXG_Post_Nybble_PC: Dest_Value = PC; break;
			default: break;
		    }
		}
		if ((Src_Nybble >= 8 && Src_Nybble <= 11) && (Dest_Nybble >= 8 && Dest_Nybble <= 11)) {
		    /* 8-bit register read */
                    /* read source register */
		    switch (Src_Nybble) {
		        case TFR_EXG_Post_Nybble_A: Src_Value = A; break;
		        case TFR_EXG_Post_Nybble_B: Src_Value = B; break;
		        case TFR_EXG_Post_Nybble_CC: Src_Value = CCR; break;
		        case TFR_EXG_Post_Nybble_DP: Src_Value = DP; break;
			default: break;
		    }
		    /* read destination register */
		    switch (Dest_Nybble) {
		        case TFR_EXG_Post_Nybble_A: Dest_Value = A; break;
		        case TFR_EXG_Post_Nybble_B: Dest_Value = B; break;
		        case TFR_EXG_Post_Nybble_CC: Dest_Value = CCR; break;
		        case TFR_EXG_Post_Nybble_DP: Dest_Value = DP; break;
			default: break;
		    }
		}
                /* write register values */
		if (Src_Nybble <= 5 && Dest_Nybble <= 5) {
		    /* 16-bit register */
                    /* write source register */
		    switch (Src_Nybble) {
		        case TFR_EXG_Post_Nybble_D:
			    A = Dest_Value >> 8;
			    B = Dest_Value & 0XFF;
			    break;
		        case TFR_EXG_Post_Nybble_X: IX = Dest_Value; break;
		        case TFR_EXG_Post_Nybble_Y: IY = Dest_Value; break;
		        case TFR_EXG_Post_Nybble_U: UP = Dest_Value; break;
		        case TFR_EXG_Post_Nybble_S: SP = Dest_Value; break;
		        case TFR_EXG_Post_Nybble_PC: PC = Dest_Value; break;
			default: break;
		    }
                    /* write destination register */
		    switch (Dest_Nybble) {
		        case TFR_EXG_Post_Nybble_D:
			    A = Src_Value >> 8;
			    B = Src_Value & 0XFF;
			    break;
		        case TFR_EXG_Post_Nybble_X: IX = Src_Value; break;
		        case TFR_EXG_Post_Nybble_Y: IY = Src_Value; break;
		        case TFR_EXG_Post_Nybble_U: UP = Src_Value; break;
		        case TFR_EXG_Post_Nybble_S: SP = Src_Value; break;
		        case TFR_EXG_Post_Nybble_PC: PC = Src_Value; break;
			default: break;
		    }
		}
		if ((Src_Nybble >= 8 && Src_Nybble <= 11) && (Dest_Nybble >= 8 && Dest_Nybble <= 11)) {
		    /* 8-bit register */
                   /* write source register */
		    switch (Src_Nybble) {
		        case TFR_EXG_Post_Nybble_A: A = Dest_Value; break;
		        case TFR_EXG_Post_Nybble_B: B = Dest_Value; break;
		        case TFR_EXG_Post_Nybble_CC: CCR = Dest_Value; break;
		        case TFR_EXG_Post_Nybble_DP: DP = Dest_Value; break;
			default: break;
		    }
                    /* write destination register */
		    switch (Dest_Nybble) {
		        case TFR_EXG_Post_Nybble_A: A = Src_Value; break;
		        case TFR_EXG_Post_Nybble_B: B = Src_Value; break;
		        case TFR_EXG_Post_Nybble_CC: CCR = Src_Value; break;
		        case TFR_EXG_Post_Nybble_DP: DP = Src_Value; break;
			default: break;
		    }
		}
                break;

            case 0x1F:                  /* TFR imm */
		Post_Byte = get_imm_byte_val();
		Dest_Nybble = Post_Byte & 0x0F;
		Src_Nybble = Post_Byte >> 4;
		if ((Src_Nybble <= 5 && Dest_Nybble > 5) || (Src_Nybble > 5 && Dest_Nybble <= 5)) {
                    // TFR with unaligned register sizes
                    // NOTE: Hitachi 6809 documentation does describe some scenarios for mis-matched register sizes!
                    reason = STOP_OPCODE;
		}
		if ((Src_Nybble <= 5) && (Dest_Nybble <= 5)) {
		    /* 16-bit registers */
		    /* read source register */
		    switch (Src_Nybble) {
		        case TFR_EXG_Post_Nybble_D: Src_Value = (A << 8) | (B & BYTEMASK); break;
		        case TFR_EXG_Post_Nybble_X: Src_Value = IX; break;
		        case TFR_EXG_Post_Nybble_Y: Src_Value = IY; break;
		        case TFR_EXG_Post_Nybble_U: Src_Value = UP; break;
		        case TFR_EXG_Post_Nybble_S: Src_Value = SP; break;
		        case TFR_EXG_Post_Nybble_PC: Src_Value = PC; break;
			break;
		    }
		    /* write source register */
		    switch (Dest_Nybble) {
		        case TFR_EXG_Post_Nybble_D:
			    A = Src_Value >> 8;
			    B = Src_Value & 0XFF;
			    break;
		        case TFR_EXG_Post_Nybble_X: IX = Src_Value; break;
		        case TFR_EXG_Post_Nybble_Y: IY = Src_Value; break;
		        case TFR_EXG_Post_Nybble_U: UP = Src_Value; break;
		        case TFR_EXG_Post_Nybble_S: SP = Src_Value; break;
		        case TFR_EXG_Post_Nybble_PC: PC = Src_Value;; break;
			break;
		    }
		}
		if ((Src_Nybble >= 8 && Src_Nybble <= 11) && (Dest_Nybble >= 8 && Dest_Nybble <= 11)) {
		    /* 8-bit registers */
                    /* read the source register */
		    switch (Src_Nybble) {
		        case TFR_EXG_Post_Nybble_A: Src_Value = A; break;
		        case TFR_EXG_Post_Nybble_B: Src_Value = B; break;
		        case TFR_EXG_Post_Nybble_CC: Src_Value = CCR; break;
		        case TFR_EXG_Post_Nybble_DP: Src_Value = DP; break;
			break;
		    }
		    /* write the destination register */
		    switch (Dest_Nybble) {
		        case TFR_EXG_Post_Nybble_A: A = Src_Value; break;
		        case TFR_EXG_Post_Nybble_B: B = Src_Value; break;
		        case TFR_EXG_Post_Nybble_CC: CCR = Src_Value; break;
		        case TFR_EXG_Post_Nybble_DP: DP = Src_Value; break;
			break;
		    }
		}
                break;

/* 0x20 - relative mode */
            case 0x20:                  /* BRA rel */
                go_rel(1);
                break;
            case 0x21:                  /* BRN rel */
		/* Branch Never - essentially a NOP */
                break;
            case 0x22:                  /* BHI rel */
                go_rel(!(get_flag(CF) || get_flag(ZF)));
                break;
            case 0x23:                  /* BLS rel */
                go_rel(get_flag(CF) || get_flag(ZF));
                break;
            case 0x24:                  /* BCC rel */
                go_rel(!get_flag(CF));
                break;
            case 0x25:                  /* BCS rel */
                go_rel(get_flag(CF));
                break;
            case 0x26:                  /* BNE rel */
                go_rel(!get_flag(ZF));
                break;
            case 0x27:                  /* BEQ rel */
                go_rel(get_flag(ZF));
                break;
            case 0x28:                  /* BVC rel */
                go_rel(!get_flag(VF));
                break;
            case 0x29:                  /* BVS rel */
                go_rel(get_flag(VF));
                break;
            case 0x2A:                  /* BPL rel */
                go_rel(!get_flag(NF));
                break;
            case 0x2B:                  /* BMI rel */
                go_rel(get_flag(NF));
                break;
            case 0x2C:                  /* BGE rel */
                go_rel(get_flag(NF) == get_flag(VF));
                break;
            case 0x2D:                  /* BLT rel */
                go_rel(get_flag(NF) != get_flag(VF));
                break;
            case 0x2E:                  /* BGT rel */
                go_rel((get_flag(ZF) == 0) && (get_flag(NF) == get_flag(VF)));
                break;
            case 0x2F:                  /* BLE rel */
                go_rel(get_flag(ZF) || (get_flag(NF) != get_flag(VF)));
                break;
/* 0x30 */
            case 0x30:                  /* LEAX ind */
		IX = get_indexed_addr();
		COND_SET_FLAG_Z_16(IX);
                break;

            case 0x31:                  /* LEAY ind */
		IY = get_indexed_addr();
		COND_SET_FLAG_Z_16(IY);
                break;

            case 0x32:                  /* LEAS ind */
		SP = get_indexed_addr();
		/* does not affect CCR */
                break;

            case 0x33:                  /* LEAU ind */
		UP = get_indexed_addr();
		/* does not affect CCR */
                break;

            case 0x34:                  /* PSHS */
		Post_Byte = get_imm_byte_val();
		if (Post_Byte & PSH_PUL_Post_Byte_PC) push_sp_word(PC);
		if (Post_Byte & PSH_PUL_Post_Byte_S_U) push_sp_word(UP);
		if (Post_Byte & PSH_PUL_Post_Byte_Y) push_sp_word(IY);
		if (Post_Byte & PSH_PUL_Post_Byte_X) push_sp_word(IX);
		if (Post_Byte & PSH_PUL_Post_Byte_DP) push_sp_byte(DP);
		if (Post_Byte & PSH_PUL_Post_Byte_B) push_sp_byte(B);
		if (Post_Byte & PSH_PUL_Post_Byte_A) push_sp_byte(A);
		if (Post_Byte & PSH_PUL_Post_Byte_CC) push_sp_byte(CCR);
                break;

            case 0x35:                  /* PULS */
		Post_Byte = get_imm_byte_val();
		if (Post_Byte & PSH_PUL_Post_Byte_CC) CCR = pop_sp_byte();
		if (Post_Byte & PSH_PUL_Post_Byte_A) A = pop_sp_byte();
		if (Post_Byte & PSH_PUL_Post_Byte_B) B = pop_sp_byte();
		if (Post_Byte & PSH_PUL_Post_Byte_DP) DP = pop_sp_byte();
		if (Post_Byte & PSH_PUL_Post_Byte_X) IX = pop_sp_word();
		if (Post_Byte & PSH_PUL_Post_Byte_Y) IY = pop_sp_word();
		if (Post_Byte & PSH_PUL_Post_Byte_S_U) UP = pop_sp_word();
		if (Post_Byte & PSH_PUL_Post_Byte_PC) PC = pop_sp_word();
                break;

            case 0x36:                  /* PSHU */
		Post_Byte = get_imm_byte_val();
		if (Post_Byte & PSH_PUL_Post_Byte_PC) push_up_word(PC);
		if (Post_Byte & PSH_PUL_Post_Byte_S_U) push_up_word(SP);
		if (Post_Byte & PSH_PUL_Post_Byte_Y) push_up_word(IY);
		if (Post_Byte & PSH_PUL_Post_Byte_X) push_up_word(IX);
		if (Post_Byte & PSH_PUL_Post_Byte_DP) push_up_byte(DP);
		if (Post_Byte & PSH_PUL_Post_Byte_B) push_up_byte(B);
		if (Post_Byte & PSH_PUL_Post_Byte_A) push_up_byte(A);
		if (Post_Byte & PSH_PUL_Post_Byte_CC) push_up_byte(CCR);
                break;

            case 0x37:                  /* PULU */
		Post_Byte = get_imm_byte_val();
		if (Post_Byte & PSH_PUL_Post_Byte_CC) CCR = pop_up_byte();
		if (Post_Byte & PSH_PUL_Post_Byte_A) A = pop_up_byte();
		if (Post_Byte & PSH_PUL_Post_Byte_B) B = pop_up_byte();
		if (Post_Byte & PSH_PUL_Post_Byte_DP) DP = pop_up_byte();
		if (Post_Byte & PSH_PUL_Post_Byte_X) IX = pop_up_word();
		if (Post_Byte & PSH_PUL_Post_Byte_Y) IY = pop_up_word();
		if (Post_Byte & PSH_PUL_Post_Byte_S_U) SP = pop_up_word();
		if (Post_Byte & PSH_PUL_Post_Byte_PC) PC = pop_up_word();
                break;

            case 0x39:                  /* RTS */
                PC = pop_sp_word();
                break;

            case 0x3A:                  /* ABX */
		/* this is an UNSIGNED operation! */
		IX = (IX + B) & 0xFFFF;
		/* no changes to CCR */
		break;

            case 0x3B:                  /* RTI */
                CCR = pop_sp_byte();
		if (get_flag(EF)) {
		    /* entire state flag */
                    A = pop_sp_byte();
                    B = pop_sp_byte();
                    DP = pop_sp_byte();
                    IX = pop_sp_word();
                    IY = pop_sp_word();
                    UP = pop_sp_word();
		}
                PC = pop_sp_word();
                break;

            case 0x3C:                  /* CWAI */
		/* AND immediate byte with CCR
		CCR &= get_imm_byte_val();
		SET_FLAG(EF);
                /* push register state */
                push_sp_word(PC);
                push_sp_word(UP);
                push_sp_word(IY);
                push_sp_word(IX);
                push_sp_byte(DP);
                push_sp_byte(B);
                push_sp_byte(A);
                push_sp_byte(CCR);
                /* wait for an interrupt */
                reason = STOP_HALT;
		break;

            case 0x3D:                  /* MUL */
		D = A * B;
		B = D & BYTEMASK;
		A = D >> 8;
                COND_SET_FLAG_Z_16(D);
                COND_SET_FLAG(B & 0x80, CF);
                break;

            case 0x3F:                  /* SWI */
                SET_FLAG(EF);
                push_sp_word(PC);
                push_sp_word(UP);
                push_sp_word(IY);
                push_sp_word(IX);
                push_sp_byte(DP);
                push_sp_byte(B);
                push_sp_byte(A);
                push_sp_byte(CCR);
                SET_FLAG(FF);
                SET_FLAG(IF);
                PC = CPU_BD_get_mword(0xFFFA);
                break;

/* 0x40 - inherent mode */
            case 0x40:                  /* NEGA */
                COND_SET_FLAG(A != 0, CF);
                COND_SET_FLAG(A == 0x80, VF);
                A = 0 - A;
                A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                break;

            case 0x43:                  /* COMA */
                A = ~A;
		A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                SET_FLAG(CF);
                break;

            case 0x44:                  /* LSRA */
                COND_SET_FLAG(A & 0x01, CF);
                A = (A >> 1) & BYTEMASK;
                CLR_FLAG(NF);
                COND_SET_FLAG_Z(A);
		/* H,V unaffected */
                break;

            case 0x46:                  /* RORA */
                op1 = A;
                A = A >> 1;
                if (get_flag(CF)) {
                    A |= 0x80;
                }
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
		/* H,V unaffected */
                COND_SET_FLAG(op1 & 0x01, CF);
                break;

            case 0x47:                  /* ASRA */
                COND_SET_FLAG(A & 0x01, CF);
                A = (A >> 1) | (A & 0x80);
		/* H undefined */
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
		/* V unaffected */
                break;

            case 0x48:                  /* ASLA */
                op1 = A;
                A = (A << 1) & BYTEMASK;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
 		COND_SET_FLAG(op1 & 0x80, CF);
                COND_SET_FLAG(EXOR(get_flag(NF),get_flag(CF)), VF);
                break;

            case 0x49:                  /* ROLA */
                op1 = A;
                A = (A << 1) & BYTEMASK;
                if (get_flag(CF)) {
                    A |= 0x01;
                }
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                COND_SET_FLAG(op1 & 0x80, CF);
                COND_SET_FLAG(EXOR(get_flag(NF),get_flag(CF)), VF);
                break;

            case 0x4A:                  /* DECA */
                COND_SET_FLAG(A == 0x80, VF);
                A = (A - 1) & BYTEMASK;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                /* C unaffected */
                break;

            case 0x4C:                  /* INCA */
                COND_SET_FLAG(A == 0x7F, VF);
                A = (A + 1) & BYTEMASK;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                /* C unaffected */
                break;

            case 0x4D:                  /* TSTA */
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
		/* C unaffected */
                break;

            case 0x4F:                  /* CLRA */
                A = 0;
                CLR_FLAG(NF);
                SET_FLAG(ZF);
                CLR_FLAG(VF);
                CLR_FLAG(CF);
                break;

/* 0x50 - inherent modes */
            case 0x50:                  /* NEGB */
                COND_SET_FLAG(B != 0, CF);
                COND_SET_FLAG(B == 0x80, VF);
                B = 0 - B;
                B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                break;

            case 0x53:                  /* COMB */
                B = ~B;
                B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                SET_FLAG(CF);
                break;

            case 0x54:                  /* LSRB */
                COND_SET_FLAG(B & 0x01, CF);
                B = (B >> 1) & BYTEMASK;
                CLR_FLAG(NF);
                COND_SET_FLAG_Z(B);
		/* H,V unaffected */
                break;

            case 0x56:                  /* RORB */
                op1 = B;
                B = B >> 1;
                if (get_flag(CF)) {
                    B |= 0x80;
                }
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
		/* H,V unaffected */
                COND_SET_FLAG(op1 & 0x01, CF);
                break;

            case 0x57:                  /* ASRB */
                COND_SET_FLAG(B & 0x01, CF);
                B = (B >> 1) | (B & 0x80);
		/* H undefined */
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
		/* C unaffected */
                break;

            case 0x58:                  /* ASLB */
                op1 = B;
                B = (B << 1) & BYTEMASK;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
 		COND_SET_FLAG(op1 & 0x80, CF);
                COND_SET_FLAG(EXOR(get_flag(NF),get_flag(CF)), VF);
                break;

            case 0x59:                  /* ROLB */
                op1 = B;
                B = (B << 1) & BYTEMASK;
                if (get_flag(CF)) {
                    B |= 0x01;
                }
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                COND_SET_FLAG(op1 & 0x80, CF);
                COND_SET_FLAG(EXOR(get_flag(NF),get_flag(CF)), VF);
                break;

            case 0x5A:                  /* DECB */
                COND_SET_FLAG(B == 0x80, VF);
                B = (B - 1) & BYTEMASK;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                /* C unaffected */
                break;

            case 0x5C:                  /* INCB */
                COND_SET_FLAG(B == 0x7F, VF);
                B = (B + 1) & BYTEMASK;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                /* C unaffected */
                break;

            case 0x5D:                  /* TSTB */
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                /* C unaffected */
                break;

            case 0x5F:                  /* CLRB */
                B = 0;
                CLR_FLAG(NF);
                SET_FLAG(ZF);
                CLR_FLAG(VF);
                CLR_FLAG(CF);
                break;

/* 0x60 - index mode */
            case 0x60:                  /* NEG ind */
                addr = get_indexed_addr();
                op1 = CPU_BD_get_mbyte(addr);
                COND_SET_FLAG(op1 != 0, CF);
                COND_SET_FLAG(op1 == 0x80, VF);
                result = (0 - op1) & BYTEMASK;
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                break;

            case 0x63:                  /* COM ind */
                addr = get_indexed_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = (~op1) & 0xFF;
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                CLR_FLAG(VF);
                SET_FLAG(CF);
                break;

            case 0x64:                  /* LSR ind */
                addr = get_indexed_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = op1 >> 1;
                CPU_BD_put_mbyte(addr, result);
                CLR_FLAG(NF);
                COND_SET_FLAG_Z(result);
		/* H,V unaffected */
                COND_SET_FLAG(op1 & 0x01, CF);
                break;

            case 0x66:                  /* ROR ind */
                addr = get_indexed_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = op1 >> 1;
                if (get_flag(CF)) {
                    result |= 0x80;
                }
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
		/* H,V unaffected */
                COND_SET_FLAG(op1 & 0x01, CF);
                break;

            case 0x67:                  /* ASR ind */
                addr = get_indexed_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = (op1 >> 1) | (op1 & 0x80);
                CPU_BD_put_mbyte(addr, result);
		/* H undefined */
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
		/* V unaffected */
                COND_SET_FLAG(op1 & 0x01, CF);
                break;

            case 0x68:                  /* ASL ind */
                addr = get_indexed_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = (op1 << 1) & BYTEMASK;
                CPU_BD_put_mbyte(addr, result);
		/* H is undefined */
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
 		COND_SET_FLAG(op1 & 0x80, CF);
                COND_SET_FLAG(EXOR(get_flag(NF),get_flag(CF)), VF);
                break;

            case 0x69:                  /* ROL ind */
                addr = get_indexed_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = (op1 << 1) & BYTEMASK;
                if (get_flag(CF)) {
		    result |= 0x01;
                }
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                COND_SET_FLAG(op1 & 0x80, CF);
                COND_SET_FLAG(EXOR(get_flag(NF),get_flag(CF)), VF);
                break;

            case 0x6A:                  /* DEC ind */
                addr = get_indexed_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = (op1 - 1) & BYTEMASK;
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                COND_SET_FLAG(op1 == 0x80, VF);
                /* C unaffected */
                break;

            case 0x6C:                  /* INC ind */
                addr = get_indexed_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = (op1 + 1) & BYTEMASK;
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                COND_SET_FLAG(op1 == 0x7F, VF);
                /* C unaffected */
                break;

            case 0x6D:                  /* TST ind */
                result = get_indexed_byte_val();
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                CLR_FLAG(VF);
                /* C unaffected */
                break;

            case 0x6E:                  /* JMP ind */
                PC = get_indexed_addr();
                break;

            case 0x6F:                  /* CLR ind */
                CPU_BD_put_mbyte(get_indexed_addr(), 0);
		/* H not affected */
                CLR_FLAG(NF);
                SET_FLAG(ZF);
                CLR_FLAG(VF);
                CLR_FLAG(CF);
                break;

/* 0x70 - extended modes */
            case 0x70:                  /* NEG ext */
                addr= get_ext_addr();
                op1 = CPU_BD_get_mbyte(addr);
                COND_SET_FLAG(op1 != 0, CF);
                COND_SET_FLAG(op1 == 0x80, VF) ;
                result = (0 - op1) & BYTEMASK;
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                break;

            case 0x73:                  /* COM ext */
                addr = get_ext_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = (~op1) & 0xFF;
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                CLR_FLAG(VF);
                SET_FLAG(CF);
                break;

            case 0x74:                  /* LSR ext */
                addr = get_ext_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = op1 >> 1;
                CPU_BD_put_mbyte(addr, result);
                CLR_FLAG(NF);
                COND_SET_FLAG_Z(result);
		/* H,V unaffected */
                COND_SET_FLAG(op1 & 0x01, CF);
                break;

            case 0x76:                  /* ROR ext */
                addr = get_ext_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = op1 >> 1;
                if (get_flag(CF)) {
                    result |= 0x80;
                }
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
		/* H,V unaffected */
                COND_SET_FLAG(op1 & 0x01, CF);
                break;

            case 0x77:                  /* ASR ext */
                addr = get_ext_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = (op1 >> 1) | (op1 & 0x80);
                CPU_BD_put_mbyte(addr, result);
		/* H undefined */
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
		/* V unaffected */
		COND_SET_FLAG(op1 & 0x01, CF);
                break;

            case 0x78:                  /* ASL ext */
                addr = get_ext_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = (op1 << 1) & BYTEMASK;
                CPU_BD_put_mbyte(addr, result);
		/* H is undefined */
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                COND_SET_FLAG(op1 & 0x80, CF);
                COND_SET_FLAG(EXOR(get_flag(NF),get_flag(CF)), VF);
                break;

            case 0x79:                  /* ROL ext */
                addr = get_ext_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = (op1 << 1) & BYTEMASK;
                if (get_flag(CF)) {
		    result |= 0x01;
                }
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                COND_SET_FLAG(op1 & 0x80, CF);
                COND_SET_FLAG(EXOR(get_flag(NF),get_flag(CF)), VF);
                break;

            case 0x7A:                  /* DEC ext */
                addr = get_ext_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = (op1 - 1) & BYTEMASK;
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                COND_SET_FLAG(op1 == 0x80, VF);
                /* C unaffected */
                break;

            case 0x7C:                  /* INC ext */
                addr = get_ext_addr();
                op1 = CPU_BD_get_mbyte(addr);
                result = (op1 + 1) & BYTEMASK;
                CPU_BD_put_mbyte(addr, result);
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                COND_SET_FLAG(op1 == 0x7F, VF);
                /* C unaffected */
                break;

                /* C unaffected */
            case 0x7D:                  /* TST ext */
                result = get_ext_byte_val();
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                CLR_FLAG(VF);
                /* C unaffected */
                break;

            case 0x7E:                  /* JMP ext */
                PC = get_ext_addr();
                break;

            case 0x7F:                  /* CLR ext */
                CPU_BD_put_mbyte(get_ext_addr(), 0);
		/* H not affected */
                CLR_FLAG(NF);
                SET_FLAG(ZF);
                CLR_FLAG(VF);
                CLR_FLAG(CF);
                break;
/* 0x80 - immediate mode (except for BSR) */
            case 0x80:                  /* SUBA imm */
                op1 = A;
                op2 = get_imm_byte_val();
                A = A - op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVs(op1, op2);
                break;

            case 0x81:                  /* CMPA imm */
                /* Do not modify A */
                op2 = get_imm_byte_val();
                result = A - op2;
                COND_SET_FLAG_C(result);
                result &= 0xFF;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                condevalVs(A, op2);
                break;

            case 0x82:                  /* SBCA imm */
                op1 = A;
                op2 = get_imm_byte_val() + get_flag(CF);
                A = A - op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVs(op1, op2);
                break;

            case 0x83:                  /* SUBD imm */
		D = (A << 8) | (B & BYTEMASK);
                op2 = get_imm_word_val();
		result = D - op2;
                COND_SET_FLAG_C_16(result);
		result &= 0xFFFF;
		A = result >> 8;
		B = result & BYTEMASK;
                COND_SET_FLAG_N_16(result);
                COND_SET_FLAG_Z_16(result);
                condevalVs16(D, op2);
                break;

            case 0x84:                  /* ANDA imm */
                A = (A & get_imm_byte_val()) & BYTEMASK;
                CLR_FLAG(VF);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                break;

            case 0x85:                  /* BITA imm */
                /* Do not modify A */
                result = (A & get_imm_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                CLR_FLAG(VF);
                break;

            case 0x86:                  /* LDA imm */
                A = get_imm_byte_val();
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0x88:                  /* EORA imm */
                A = (A ^ get_imm_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0x89:                  /* ADCA imm */
                op1 = A;
                op2 = get_imm_byte_val() + get_flag(CF);
                A = A + op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVa(op1, op2);
                break;

            case 0x8A:                  /* ORA imm */
                A = A | get_imm_byte_val();
		A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0x8B:                  /* ADDA imm */
                op1 = A;
                op2 = get_imm_byte_val();
                A = A + op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVa(op1, op2);
                break;

            case 0x8C:                  /* CMPX imm */
                /* Do not modify X */
                op2 = get_imm_word_val();
                result = IX - op2;
                COND_SET_FLAG_C_16(result);
		result &= 0xFFFF;
                COND_SET_FLAG_N_16(result);
                COND_SET_FLAG_Z_16(result);
		condevalVs16(IX, op2);
                break;

            case 0x8D:                  /* BSR rel */
                addr = get_rel_addr();
                push_sp_word(PC);
                PC = (PC + addr) & ADDRMASK;
                break;

            case 0x8E:                  /* LDX imm */
                IX = get_imm_word_val();
                COND_SET_FLAG_N_16(IX);
                COND_SET_FLAG_Z_16(IX);
                CLR_FLAG(VF);
                break;

/* 0x90 - direct mode */
            case 0x90:                  /* SUBA dir */
		op1 = A;
                op2 = get_dir_byte_val();
                A = A - op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVs(op1, op2);
                break;

            case 0x91:                  /* CMPA dir */
                /* Do not modify A */
		op2 = get_dir_byte_val();
                result = A - op2;
                COND_SET_FLAG_C(result);
		result &= 0xFF;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                condevalVs(A, op2);
                break;

            case 0x92:                  /* SBCA dir */
                op1 = A;
		op2 = get_dir_byte_val() + get_flag(CF);
                A = A - op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVs(op1, op2);
                break;

            case 0x93:                  /* SUBD dir */
		D = (A << 8) | (B & BYTEMASK);
                op2 = get_dir_word_val();
		result = D - op2;
                COND_SET_FLAG_C_16(result);
		result &= 0xFFFF;
		A = result >> 8;
		B = result & BYTEMASK;
                COND_SET_FLAG_N_16(result);
                COND_SET_FLAG_Z_16(result);
                condevalVs16(D, op2);
                break;

            case 0x94:                  /* ANDA dir */
                A = (A & get_dir_byte_val()) & BYTEMASK;
                CLR_FLAG(VF);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                break;

            case 0x95:                  /* BITA dir */
                /* Do not modify A */
                result = (A & get_dir_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                CLR_FLAG(VF);
                break;

            case 0x96:                  /* LDA dir */
                A = get_dir_byte_val();
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0x97:                  /* STA dir */
                CPU_BD_put_mbyte(get_dir_addr(), A);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0x98:                  /* EORA dir */
                A = (A ^ get_dir_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0x99:                  /* ADCA dir */
                op1 = A;
                op2 = get_dir_byte_val() + get_flag(CF);
                A = A + op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVa(op1, op2);
                break;

            case 0x9A:                  /* ORA dir */
                A = A | get_dir_byte_val();
		A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0x9B:                  /* ADDA dir */
                op1 = A;
                op2 = get_dir_byte_val();
                A = A + op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVa(op1, op2);
                break;

            case 0x9C:                  /* CMPX dir */
                /* Do not modify X */
                op2 = get_dir_word_val();
                result = IX - op2;
                COND_SET_FLAG_C_16(result);
		result &= 0xFFFF;
                COND_SET_FLAG_N_16(result);
                COND_SET_FLAG_Z_16(result);
                condevalVs16(IX, op2);
                break;

            case 0x9D:                  /* JSR dir */
		addr = get_dir_addr();
		push_sp_word(PC);
		PC = addr;
                break;

            case 0x9E:                  /* LDX dir */
                IX = get_dir_word_val();
                COND_SET_FLAG_N_16(IX);
                COND_SET_FLAG_Z_16(IX);
                CLR_FLAG(VF);
                break;

            case 0x9F:                  /* STX dir */
                CPU_BD_put_mword(get_dir_addr(), IX);
                COND_SET_FLAG_N_16(IX);
                COND_SET_FLAG_Z_16(IX);
                CLR_FLAG(VF);
                break;

/* 0xA0 - indexed mode */
            case 0xA0:                  /* SUBA ind */
                op1 = A;
                op2 = get_indexed_byte_val();
                A = A - op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVs(op1, op2);
                break;

            case 0xA1:                  /* CMPA ind */
                /* Do not modify A */
                op2 = get_indexed_byte_val();
                result = A - op2;
                COND_SET_FLAG_C(result);
                result &= 0xFF;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                condevalVs(A, op2);
                break;

            case 0xA2:                  /* SBCA ind */
                op1 = A;
                op2 = get_indexed_byte_val() + get_flag(CF);
                A = A - op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVs(op1, op2);
                break;

            case 0xA3:                  /* SUBD ind */
		D = (A << 8) | (B & BYTEMASK);
                op2 = get_indexed_word_val();
		result = D - op2;
                COND_SET_FLAG_C_16(result);
		result &= 0xFFFF;
		A = result >> 8;
		B = result & BYTEMASK;
                COND_SET_FLAG_N_16(result);
                COND_SET_FLAG_Z_16(result);
                condevalVs16(D, op2);
                break;

            case 0xA4:                  /* ANDA ind */
                A = (A & get_indexed_byte_val()) & BYTEMASK;
                CLR_FLAG(VF);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                break;

            case 0xA5:                  /* BITA ind */
                /* Do not modify A */
                result = (A & get_indexed_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                CLR_FLAG(VF);
                break;

            case 0xA6:                  /* LDA ind */
                A = get_indexed_byte_val();
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0xA7:                  /* STA ind */
                CPU_BD_put_mbyte(get_indexed_addr(), A);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0xA8:                  /* EORA ind */
                A = (A ^ get_indexed_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0xA9:                  /* ADCA ind */
                op1 = A;
                op2 = get_indexed_byte_val() + get_flag(CF);
                A = A + op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVa(op1, op2);
                break;

            case 0xAA:                  /* ORA ind */
                A = A | get_indexed_byte_val();
		A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0xAB:                  /* ADDA ind */
                op1 = A;
                op2 = get_indexed_byte_val();
                A = A + op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVa(op1, op2);
                break;

            case 0xAC:                  /* CMPX ind */
                /* Do not modify X */
                op2 = get_indexed_word_val();
                result = IX - op2;
                COND_SET_FLAG_C_16(result);
		result &= 0xFFFF;
                COND_SET_FLAG_N_16(result);
                COND_SET_FLAG_Z_16(result);
		condevalVs16(IX, op2);
                break;

            case 0xAD:                  /* JSR ind */
                addr = get_indexed_addr();
                push_sp_word(PC);
                PC = addr;
                break;

            case 0xAE:                  /* LDX ind */
                IX = get_indexed_word_val();
                COND_SET_FLAG_N_16(IX);
                COND_SET_FLAG_Z_16(IX);
                CLR_FLAG(VF);
                break;

            case 0xAF:                  /* STX ind */
                CPU_BD_put_mword(get_indexed_addr(), IX);
                COND_SET_FLAG_N_16(IX);
                COND_SET_FLAG_Z_16(IX);
                CLR_FLAG(VF);
                break;

/* 0xB0 - extended mode */
            case 0xB0:                  /* SUBA ext */
                op1 = A;
                op2 = get_ext_byte_val();
                A = A - op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVs(op1, op2);
                break;

            case 0xB1:                  /* CMPA ext */
                /* Do not modify A */
                op2 = get_ext_byte_val();
                result = A - op2;
                COND_SET_FLAG_C(result);
                result &= 0xFF;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                condevalVs(A, op2);
                break;

            case 0xB2:                  /* SBCA ext */
                op1 = A;
                op2 = get_ext_byte_val() + get_flag(CF);
                A = A - op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVs(op1, op2);
                break;

            case 0xB3:                  /* SUBD ext */
		D = (A << 8) | (B & BYTEMASK);
                op2 = get_ext_word_val();
		result = D - op2;
                COND_SET_FLAG_C_16(result);
		result &= 0xFFFF;
		A = result >> 8;
		B = result & BYTEMASK;
                COND_SET_FLAG_N_16(result);
                COND_SET_FLAG_Z_16(result);
                condevalVs16(D, op2);
                break;

            case 0xB4:                  /* ANDA ext */
                A = (A & get_ext_byte_val()) & BYTEMASK;
                CLR_FLAG(VF);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                break;

            case 0xB5:                  /* BITA ext */
                /* Do not modify A */
                result = (A & get_ext_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                CLR_FLAG(VF);
                break;

            case 0xB6:                  /* LDA ext */
                A = get_ext_byte_val();
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0xB7:                  /* STA ext */
                CPU_BD_put_mbyte(get_ext_addr(), A);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0xB8:                  /* EORA ext */
                A = (A ^ get_ext_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0xB9:                  /* ADCA ext */
                op1 = A;
                op2 = get_ext_byte_val() + get_flag(CF);
                A = A + op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVa(op1, op2);
                break;

            case 0xBA:                  /* ORA ext */
                A = A | get_ext_byte_val();
		A &= 0xFF;
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                CLR_FLAG(VF);
                break;

            case 0xBB:                  /* ADDA ext */
                op1 = A;
                op2 = get_ext_byte_val();
                A = A + op2;
                COND_SET_FLAG_C(A);
                A &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(A);
                COND_SET_FLAG_Z(A);
                condevalVa(op1, op2);
                break;

            case 0xBC:                  /* CMPX ext */
                /* Do not modify X */
                op2 = get_ext_word_val();
                result = IX - op2;
                COND_SET_FLAG_C_16(result);
		result &= 0xFFFF;
                COND_SET_FLAG_N_16(result);
                COND_SET_FLAG_Z_16(result);
		condevalVs16(IX, op2);
                break;

            case 0xBD:                  /* JSR ext */
                addr = get_ext_addr();
                push_sp_word(PC);
                PC = addr;
                break;

            case 0xBE:                  /* LDX ext */
                IX = get_ext_word_val();
                COND_SET_FLAG_N_16(IX);
                COND_SET_FLAG_Z_16(IX);
                CLR_FLAG(VF);
                break;

            case 0xBF:                  /* STX ext */
                CPU_BD_put_mword(get_ext_addr(), IX);
                COND_SET_FLAG_N_16(IX);
                COND_SET_FLAG_Z_16(IX);
                CLR_FLAG(VF);
                break;

/* 0xC0 - immediate mode */
            case 0xC0:                  /* SUBB imm */
                op1 = B;
                op2 = get_imm_byte_val();
                B = B - op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVs(op1, op2);
                break;

            case 0xC1:                  /* CMPB imm */
                /* Do not modify B */
                op2 = get_imm_byte_val();
                result = B - op2;
                COND_SET_FLAG_C(result);
                result &= 0xFF;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                condevalVs(B, op2);
                break;

            case 0xC2:                  /* SBCB imm */
                op1 = B;
                op2 = get_imm_byte_val() + get_flag(CF);
                B = B - op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVs(op1, op2);
                break;

            case 0xC3:                  /* ADDD imm */
		op1 = (A << 8) | (B & BYTEMASK);
		op2 = get_imm_word_val();
                D = op1 + op2;
                COND_SET_FLAG_C_16(D);
		D &= 0xFFFF;
		A = D >> 8;
		B = D & BYTEMASK;
                COND_SET_FLAG_N_16(D);
                COND_SET_FLAG_Z_16(D);
                condevalVa16(op1, op2);
                break;

            case 0xC4:                  /* ANDB imm */
                B = (B & get_imm_byte_val()) & BYTEMASK;
                CLR_FLAG(VF);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                break;

            case 0xC5:                  /* BITB imm */
                /* Do not modify B */
                result = (B & get_imm_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                CLR_FLAG(VF);
                break;

            case 0xC6:                  /* LDB imm */
                B = get_imm_byte_val();
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xC8:                  /* EORB imm */
                B = (B ^ get_imm_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xC9:                  /* ADCB imm */
		op1 = B;
                op2 = get_imm_byte_val() + get_flag(CF);
                B = B + op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVa(op1, op2);
                break;

            case 0xCA:                  /* ORB imm */
                B = B | get_imm_byte_val();
		B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xCB:                  /* ADDB imm */
                op1 = B;
                op2 = get_imm_byte_val();
                B = B + op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVa(op1, op2);
                break;

            case 0xCC:                  /* LDD imm */
                D = get_imm_word_val();
		B = D & BYTEMASK;
		A = D >> 8;
                COND_SET_FLAG_N_16(D);
                COND_SET_FLAG_Z_16(D);
                CLR_FLAG(VF);
                break;

            case 0xCE:                  /* LDU imm */
                UP = get_imm_word_val();
                COND_SET_FLAG_N_16(UP);
                COND_SET_FLAG_Z_16(UP);
                CLR_FLAG(VF);
                break;

/* 0xD0 - direct modes */
            case 0xD0:                  /* SUBB dir */
                op1 = B;
                op2 = get_dir_byte_val();
                B = B - op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVs(op1, op2);
                break;

            case 0xD1:                  /* CMPB dir */
                /* Do not modify B */
                op2 = get_dir_byte_val();
                result = B - op2;
                COND_SET_FLAG_C(result);
		result &= 0xFF;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                condevalVs(B, op2);
                break;

            case 0xD2:                  /* SBCB dir */
		op1 = B;
                op2 = get_dir_byte_val() + get_flag(CF);
                B = B - op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVs(op1, op2);
                break;

            case 0xD3:                  /* ADDD dir */
		op1 = (A << 8) | (B & BYTEMASK);
                op2 = get_dir_word_val();
                D = op1 + op2;
                COND_SET_FLAG_C_16(D);
		D &= 0xFFFF;
		A = D >> 8;
		B = D & BYTEMASK;
                COND_SET_FLAG_N_16(D);
                COND_SET_FLAG_Z_16(D);
                condevalVa16(op1, op2);
                break;

            case 0xD4:                  /* ANDB dir */
                B = (B & get_dir_byte_val()) & BYTEMASK;
                CLR_FLAG(VF);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                break;

            case 0xD5:                  /* BITB dir */
                /* Do not modify B */
                result = (B & get_dir_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                CLR_FLAG(VF);
                break;

            case 0xD6:                  /* LDB dir */
                B = get_dir_byte_val();
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xD7:                  /* STB dir */
                CPU_BD_put_mbyte(get_dir_addr(), B);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xD8:                  /* EORB dir */
                B = (B ^ get_dir_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xD9:                  /* ADCB dir */
                op1 = B;
                op2 = get_dir_byte_val() + get_flag(CF);
                B = B + op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVa(op1, op2);
                break;

            case 0xDA:                  /* ORB dir */
                B = B | get_dir_byte_val();
		B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xDB:                  /* ADDB dir */
                op1 = B;
                op2 = get_dir_byte_val();
                B = B + op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVa(op1, op2);
                break;

            case 0xDC:                  /* LDD dir */
                D = get_dir_word_val();
		B = D & BYTEMASK;
		A = D >> 8;
                COND_SET_FLAG_N_16(D);
                COND_SET_FLAG_Z_16(D);
                CLR_FLAG(VF);
                break;

            case 0xDD:                  /* STD dir */
		D = (A << 8) | (B & BYTEMASK);
                CPU_BD_put_mword(get_dir_addr(), D);
                COND_SET_FLAG_N_16(D);
                COND_SET_FLAG_Z_16(D);
                CLR_FLAG(VF);
                break;

            case 0xDE:                  /* LDU dir */
                UP = get_dir_word_val();
                COND_SET_FLAG_N_16(UP);
                COND_SET_FLAG_Z_16(UP);
                CLR_FLAG(VF);
                break;

            case 0xDF:                  /* STU dir */
                CPU_BD_put_mword(get_dir_addr(), UP);
                COND_SET_FLAG_N_16(UP);
                COND_SET_FLAG_Z_16(UP);
                CLR_FLAG(VF);
                break;

/* 0xE0 - indexed mode */
            case 0xE0:                  /* SUBB ind */
                op1 = B;
                op2 = get_indexed_byte_val();
                B = B - op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVs(op1, op2);
                break;

            case 0xE1:                  /* CMPB ind */
                /* do not modify B */
                op2 = get_indexed_byte_val();
                result = B - op2;
                COND_SET_FLAG_C(result);
                result &= 0xFF;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                condevalVs(B, op2);
                break;

            case 0xE2:                  /* SBCB ind */
                op1 = B;
                op2 = get_indexed_byte_val() + get_flag(CF);
                B = B - op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVs(op1, op2);
                break;

            case 0xE3:                  /* ADDD ind */
		op1 = (A << 8) | (B & BYTEMASK);
                op2 = get_indexed_word_val();
		D = op1 + op2;
                COND_SET_FLAG_C_16(D);
		D &= 0xFFFF;
		A = D >> 8;
		B = D & BYTEMASK;
                COND_SET_FLAG_N_16(D);
                COND_SET_FLAG_Z_16(D);
                condevalVa16(op1, op2);
                break;

            case 0xE4:                  /* ANDB ind */
                B = (B & get_indexed_byte_val()) & BYTEMASK;
                CLR_FLAG(VF);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                break;

            case 0xE5:                  /* BITB ind */
                /* Do not modify B */
                result = (B & get_indexed_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                CLR_FLAG(VF);
                break;

            case 0xE6:                  /* LDB ind */
                B = get_indexed_byte_val();
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xE7:                  /* STB ind */
                CPU_BD_put_mbyte(get_indexed_addr(), B);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xE8:                  /* EORB ind */
                B = (B ^ get_indexed_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xE9:                  /* ADCB ind */
                op1 = B;
                op2 = get_indexed_byte_val() + get_flag(CF);
                B = B + op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVa(op1, op2);
                break;

            case 0xEA:                  /* ORB ind */
                B = B | get_indexed_byte_val();
		B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xEB:                  /* ADDB ind */
                op1 = B;
                op2 = get_indexed_byte_val();
                B = B + op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVa(op1, op2);
                break;

            case 0xEC:                  /* LDD ind */
                D = get_indexed_word_val();
		B = D & BYTEMASK;
		A = D >> 8;
                COND_SET_FLAG_N_16(D);
                COND_SET_FLAG_Z_16(D);
                CLR_FLAG(VF);
                break;

            case 0xED:                  /* STD ind */
		D = (A << 8) | (B & BYTEMASK);
                CPU_BD_put_mword(get_indexed_addr(), D);
                COND_SET_FLAG_N_16(D);
                COND_SET_FLAG_Z_16(D);
                CLR_FLAG(VF);
                break;

            case 0xEE:                  /* LDU ind */
                UP = get_indexed_word_val();
                COND_SET_FLAG_N_16(UP);
                COND_SET_FLAG_Z_16(UP);
                CLR_FLAG(VF);
                break;

            case 0xEF:                  /* STU ind */
                CPU_BD_put_mword(get_indexed_addr(), UP);
                COND_SET_FLAG_N_16(UP);
                COND_SET_FLAG_Z_16(UP);
                CLR_FLAG(VF);
                break;

/* 0xF0 - extended mode */
            case 0xF0:                  /* SUBB ext */
                op1 = B;
                op2 = get_ext_byte_val();
                B = B - op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVs(op1, op2);
                break;

            case 0xF1:                  /* CMPB ext */
                /* Do not modify B */
                op2 = get_ext_byte_val();
                result = B - op2;
                COND_SET_FLAG_C(result);
                result &= 0xFF;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                condevalVs(B, op2);
                break;

            case 0xF2:                  /* SBCB ext */
		op1 = B;
                op2 = get_ext_byte_val() + get_flag(CF);
                B = B - op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVs(op1, op2);
                break;

            case 0xF3:                  /* ADDD ext */
		op1 = (A << 8) | (B & BYTEMASK);
                op2 = get_ext_word_val();
		D = op1 + op2;
                COND_SET_FLAG_C_16(D);
		D &= 0xFFFF;
		A = D >> 8;
		B = D & BYTEMASK;
                COND_SET_FLAG_N_16(D);
                COND_SET_FLAG_Z_16(D);
                condevalVa16(op1, op2);
                break;

            case 0xF4:                  /* ANDB ext */
                B = (B & get_ext_byte_val()) & BYTEMASK;
                CLR_FLAG(VF);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                break;

            case 0xF5:                  /* BITB ext */
                /* Do not modify B */
                result = (B & get_ext_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(result);
                COND_SET_FLAG_Z(result);
                CLR_FLAG(VF);
                break;

            case 0xF6:                  /* LDB ext */
                B = get_ext_byte_val();
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xF7:                  /* STB ext */
                CPU_BD_put_mbyte(get_ext_addr(), B);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xF8:                  /* EORB ext */
                B = (B ^ get_ext_byte_val()) & BYTEMASK;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xF9:                  /* ADCB ext */
                op1 = B;
                op2 = get_ext_byte_val() + get_flag(CF);
                B = B + op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVa(op1, op2);
                break;

            case 0xFA:                  /* ORB ext */
                B = B | get_ext_byte_val();
		B &= 0xFF;
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                CLR_FLAG(VF);
                break;

            case 0xFB:                  /* ADDB ext */
                op1 = B;
                op2 = get_ext_byte_val();
                B = B + op2;
                COND_SET_FLAG_C(B);
                B &= 0xFF;
                condevalHa(op1, op2);
                COND_SET_FLAG_N(B);
                COND_SET_FLAG_Z(B);
                condevalVa(op1, op2);
                break;

            case 0xFC:                  /* LDD ext */
                D = get_ext_word_val();
		B = D & BYTEMASK;
		A = D >> 8;
                COND_SET_FLAG_N_16(D);
                COND_SET_FLAG_Z_16(D);
                CLR_FLAG(VF);
                break;

            case 0xFD:                  /* STD ext */
		D = (A << 8) | (B & BYTEMASK);
                CPU_BD_put_mword(get_ext_addr(), D);
                COND_SET_FLAG_N_16(D);
                COND_SET_FLAG_Z_16(D);
                CLR_FLAG(VF);
                break;

            case 0xFE:                  /* LDU ext */
                UP = get_ext_word_val();
                COND_SET_FLAG_N_16(UP);
                COND_SET_FLAG_Z_16(UP);
                CLR_FLAG(VF);
                break;

            case 0xFF:                  /* STU ext */
                CPU_BD_put_mword(get_ext_addr(), UP);
                COND_SET_FLAG_N_16(UP);
                COND_SET_FLAG_Z_16(UP);
                CLR_FLAG(VF);
                break;

            default:                    /* Unassigned opcode */
                if (m6809_unit.flags & UNIT_OPSTOP) {
                    reason = STOP_OPCODE;
                    PC--;
                }
                break;
        }
    }
    /* Simulation halted - lets dump all the registers! */
    dump_regs();
    saved_PC = PC;
    return reason;
}

/* dump the working registers */

void dump_regs(void)
{
   printf("\r\nPC=%04X SP=%04X UP=%04X IX=%04X IY=%04X ", PC, SP, UP, IX, IY);
    printf("A=%02X B=%02X DP=%02X CCR=%02X", A, B, DP, CCR);
}

void dump_regs1(void)
{
    printf("PC=%04X SP=%04X UP=%04X IX=%04X IY=%04X ", PC, SP, UP, IX, IY);
    printf("A=%02X B=%02X DP=%02X CCR=%02X\n", A, B, DP, CCR);
}

/* fetch an instruction or byte */
int32 fetch_byte(int32 flag)
{
    int32 val;

    val = CPU_BD_get_mbyte(PC);   /* fetch byte */
    if (flag == 0) {
        /* opcode fetch */
        sim_debug(DEBUG_asm, &m6809_dev, "%04X: %s\n", PC, opcode[val]);
    } else {
        /* byte operand fetch */
        sim_debug(DEBUG_asm, &m6809_dev, "%04X: 0x%02XH\n", PC, val);
    }
    PC = (PC + 1) & ADDRMASK;           /* increment PC */
    return val;
}

/* fetch a word - big endian */
int32 fetch_word()
{
    int32 val;
    int32 temp_pc = PC;

    val = CPU_BD_get_mbyte(PC) << 8;     /* fetch high byte */
    PC = (PC + 1) & ADDRMASK;
    val = val | CPU_BD_get_mbyte(PC); /* fetch low byte */
    PC = (PC + 1) & ADDRMASK;

    /* 2-byte operand fetch */
    sim_debug(DEBUG_asm, &m6809_dev, "%04X: 0x%04XH\n", temp_pc, val);
    return val;
}

/* push a byte using the hardware stack pointer (SP) */
void push_sp_byte(uint8 val)
{
    SP = (SP - 1) & ADDRMASK;
    CPU_BD_put_mbyte(SP, val & BYTEMASK);
}

/* push a byte using the user stack pointer (UP) */
void push_up_byte(uint8 val)
{
    UP = (UP - 1) & ADDRMASK;
    CPU_BD_put_mbyte(UP, val & BYTEMASK);
}

/* push a word using the hardware stack pointer (SP) */
void push_sp_word(uint16 val)
{
    push_sp_byte(val & BYTEMASK);
    push_sp_byte(val >> 8);
}

/* push a word using the user stack pointer (UP) */
void push_up_word(uint16 val)
{
    push_up_byte(val & BYTEMASK);
    push_up_byte(val >> 8);
}

/* pop a byte using the hardware stack pointer (SP) */
uint8 pop_sp_byte(void)
{
    register uint8 res;

    res = CPU_BD_get_mbyte(SP);
    SP = (SP + 1) & ADDRMASK;
    return res;
}

/* pop a byte using the user stack pointer (UP) */
uint8 pop_up_byte(void)
{
    register uint8 res;

    res = CPU_BD_get_mbyte(UP);
    UP = (UP + 1) & ADDRMASK;
    return res;
}

/* pop a word using the hardware stack pointer (SP) */
uint16 pop_sp_word(void)
{
    register uint16 res;

    res = pop_sp_byte() << 8;
    res |= pop_sp_byte();
    return res;
}

/* pop a word using the user stack pointer (UP) */
uint16 pop_up_word(void)
{
    register uint16 res;

    res = pop_up_byte() << 8;
    res |= pop_up_byte();
    return res;
}

/* this routine does the jump to relative offset if the condition is met.  Otherwise, execution continues at the current PC. */
void go_rel(int32 cond)
{
    int32 temp;

    temp = get_rel_addr();
    if (cond != 0) {
        PC += temp;
        PC &= ADDRMASK;
    }
}

/* this routine does the jump to long relative offset if the condition is met.  Otherwise, execution continues at the current PC. */
void go_long_rel(int32 cond)
{
    int32 temp;

    temp = get_long_rel_addr();
    if (cond != 0) {
        PC += temp;
        PC &= ADDRMASK;
    }
}

/* returns the relative offset sign-extended */
int32 get_rel_addr(void)
{
    int32 temp;

    temp = fetch_byte(1);
    if (temp & 0x80)
        temp |= 0xFF00;
    return(temp & ADDRMASK);
}

/* returns the long relative offset sign-extended */
int32 get_long_rel_addr(void)
{
    return(fetch_word());
}

/* returns the byte value at the direct address pointed to by PC */
int32 get_dir_byte_val(void)
{
    return(CPU_BD_get_mbyte(get_dir_addr()));
}

/* returns the word value at the direct address pointed to by PC */
int32 get_dir_word_val(void)
{
    return(CPU_BD_get_mword(get_dir_addr()));
}

/* returns the immediate byte value pointed to by PC */
int32 get_imm_byte_val(void)
{
    return(fetch_byte(1));
}

/* returns the immediate word value pointed to by PC */
int32 get_imm_word_val(void)
{
    return(fetch_word());
}

/* returns the direct address pointed to by PC */
/* use the Direct Page register as the high byte of the address */
int32 get_dir_addr(void)
{
    int32 temp;

    temp = (DP << 8) + fetch_byte(1);
    return(temp & ADDRMASK);
}

/* returns the byte value at the indexed address pointed to by PC */
int32 get_indexed_byte_val(void)
{
    return(CPU_BD_get_mbyte(get_indexed_addr()));
}

/* returns the word value at the indexed address pointed to by PC */
int32 get_indexed_word_val(void)
{
    return(CPU_BD_get_mword(get_indexed_addr()));
}

/* returns the indexed address. Note this also handles the indirect indexed mode */
int32 get_indexed_addr(void)
{
    int32 temp;
    int32 offset;
    uint8 post_byte;

    /* fetch the index mode post-byte */
    post_byte = fetch_byte(1);
   
    if ((post_byte & 0x80) == 0) {
        /* R +- 4-bit offset (non indirect only) */
        /* read register value */
        switch (post_byte & 0x60) {
            case 0b00000000: temp = IX; break;
            case 0b00100000: temp = IY; break;
            case 0b01000000: temp = UP; break;
            case 0b01100000: temp = SP; break;
            default: break;
        }
        /* add 4 bit signed offset */
        if (post_byte & 0x10) {
           temp += (post_byte | 0xFFF0);
	} else {
           temp += (post_byte & 0x0F);
	}
    } else {
	switch ( post_byte & 0x0F ) {
	    case 0b0000:
                /* .R+ post increment by 1 (non indirect) */
                switch ( post_byte & 0x60 ) {
                    case 0b00000000: temp = IX++; IX &= 0xFFFF; break;
                    case 0b00100000: temp = IY++; IY &= 0xFFFF; break;
                    case 0b01000000: temp = UP++; UP &= 0xFFFF; break;
                    case 0b01100000: temp = SP++; SP &= 0xFFFF; break;
                    default: break;
                };
		break;
            case 0b0001:
                /* .R+ post increment by 2 (non indirect) */
                switch ( post_byte & 0x60 ) {
                    case 0b00000000: temp = IX; IX=IX+2; IX &= 0xFFFF; break;
                    case 0b00100000: temp = IY; IY=IY+2; IY &= 0xFFFF; break;
                    case 0b01000000: temp = UP; UP=UP+2; UP &= 0xFFFF; break;
                    case 0b01100000: temp = SP; SP=SP+2; SP &= 0xFFFF; break;
                    default: break;
                };
		break;
            case 0b0010:
                /* .-R pre decrement by 1 (non indirect) */
                switch ( post_byte & 0x60 ) {
                    case 0b00000000: temp = --IX; IX &= 0xFFFF; break;
                    case 0b00100000: temp = --IY; IY &= 0xFFFF; break;
                    case 0b01000000: temp = --UP; UP &= 0xFFFF; break;
                    case 0b01100000: temp = --SP; SP &= 0xFFFF; break;
                    default: break;
                };
		break;
            case 0b0011:
                /* .--R pre decrement by 2 (non indirect) */
                switch ( post_byte & 0x60 ) {
                    case 0b00000000: IX-=2; temp = IX; IX &= 0xFFFF; break;
                    case 0b00100000: IY-=2; temp = IY; IY &= 0xFFFF; break;
                    case 0b01000000: UP-=2; temp = UP; UP &= 0xFFFF; break;
                    case 0b01100000: SP-=2; temp = SP; SP &= 0xFFFF; break;
                    default: break;
                };
		break;
            case 0b0100:
                /* R+0 zero offset (non indirect) */
                switch ( post_byte & 0x60 ) {
                    case 0b00000000: temp = IX; break;
                    case 0b00100000: temp = IY; break;
                    case 0b01000000: temp = UP; break;
                    case 0b01100000: temp = SP; break;
                    default: break;
                };
		break;
            case 0b0101:
                /* R+-ACCB (non indirect)*/
                switch ( post_byte & 0x60 ) {
                    case 0b00000000: temp = IX; break;
                    case 0b00100000: temp = IY; break;
                    case 0b01000000: temp = UP; break;
                    case 0b01100000: temp = SP; break;
                    default: break;
                };
		if (B & 0x80) {
                    offset = B | 0xFF80;
		} else {
                    offset = B;
                }
                temp += offset;
		break;
            case 0b0110:
                /* R+-ACCA (non indirect)*/
                switch ( post_byte & 0x60 ) {
                    case 0b00000000: temp = IX; break;
                    case 0b00100000: temp = IY; break;
                    case 0b01000000: temp = UP; break;
                    case 0b01100000: temp = SP; break;
                    default: break;
                };
		if (A & 0x80) {
                    offset = A | 0xFF80;
		} else {
                    offset = A;
                }
		temp += offset;
                break;
            case 0b1000:
                /* R+- 8-bit offset */
                switch ( post_byte & 0x60 ) {
                    case 0b00000000: temp = IX; break;
                    case 0b00100000: temp = IY; break;
                    case 0b01000000: temp = UP; break;
                    case 0b01100000: temp = SP; break;
                    default: break;
                };
                /* need to fetch 8-bit operand */
		offset = fetch_byte(1);
                /* add 7 bit signed offset */
                if (offset & 0x80) {
                    temp += offset | 0xFF80;
	        } else {
                    temp += offset;
                }
                break;
            case 0b1001:
                /* R+- 16-bit offset */
                switch ( post_byte & 0x60 ) {
                    case 0b00000000: temp = IX; break;
                    case 0b00100000: temp = IY; break;
                    case 0b01000000: temp = UP; break;
                    case 0b01100000: temp = SP; break;
                    default: break;
                };
                /* need to fetch 16-bit operand */
		offset = fetch_word();
                /* add 16 bit signed offset */
                /* calculation is the same for negative or positive offset! */
                temp += offset;
                break;
            case 0b1011:
                /* R+- ACCD */
		D = (A << 8) + (B & BYTEMASK);
                switch ( post_byte & 0x60 ) {
                    case 0b00000000: temp = IX + D; break;
                    case 0b00100000: temp = IY + D; break;
                    case 0b01000000: temp = UP + D; break;
                    case 0b01100000: temp = SP + D; break;
                    default: break;
                };
		break;
            case 0b1100:
                /* PC+- 7-bit offset (non indirect) */
                /* need to fetch 8-bit operand */
		offset = fetch_byte(1);
                // PC value updated after fetch!!!

                /* add 7 bit signed offset */
                if (offset & 0x80) {
                    temp = PC + (offset | 0xFF80);
                } else {
                    temp = PC + offset;
                }
		break;
            case 0b1101:
                /* PC+- 15-bit offset (non indirect)*/
                /* need to fetch 16-bit operand */
		offset = fetch_word();
                // PC value updated after fetch!!!

                /* add 15 bit signed offset */
                temp = PC + offset;
		break;
            case 0b1111:
		// Extended indirect - fetch 16-bit address
		temp = fetch_word();
		break;
        }
        switch ( post_byte & 0x1F ) {
            /* perform the indirection - 11 valid post-byte opcodes */
            case 0b10001:
            case 0b10011:
            case 0b10100:
            case 0b10101:
            case 0b10110:
            case 0b11000:
            case 0b11001:
            case 0b11011:
            case 0b11100:
            case 0b11101:
            case 0b11111:
                temp = temp & 0xFFFF;
                temp = CPU_BD_get_mword(temp);
                break;
            default: break;
        }
    }
    /* make sure to truncate to 16-bit value */
    return(temp & 0xFFFF);
}

/* returns the value at the extended address pointed to by PC */
int32 get_ext_byte_val(void)
{
    return CPU_BD_get_mbyte(get_ext_addr());
}

/* returns the value at the extended address pointed to by PC */
int32 get_ext_word_val(void)
{
    return CPU_BD_get_mword(get_ext_addr());
}

/* returns the extended address pointed to by PC or immediate word */
int32 get_ext_addr(void)
{
    int32 temp;

    temp = fetch_word();
    return temp;
}

/* return 1 for flag set or 0 for flag clear */
inline int32 get_flag(int32 flg)
{
    if (CCR & flg) {
        return 1;
    }
    else {
        return 0;
    }
}

/* test and set V for 8-addition */
void condevalVa(int32 op1, int32 op2)
{
    int32 temp;

    /* op1 + op2 */
    // If 2 Two's Complement numbers are added, and they both have the same sign (both positive or both negative),
    // then overflow occurs if and only if the result has the opposite sign.
    // Overflow never occurs when adding operands with different signs.
    temp = op1 + op2;
    if ( ((op1 & 0x80) == (op2 & 0x80)) && ((temp & 0x80) != (op1 & 0x80)) ) {
        SET_FLAG(VF);
    } else {
        CLR_FLAG(VF);
    }
}

/* test and set V for 16-bit addition */
void condevalVa16(int32 op1, int32 op2)
{
    int32 temp;

    /* op1 + op2 */
    // If 2 Two's Complement numbers are added, and they both have the same sign (both positive or both negative),
    // then overflow occurs if and only if the result has the opposite sign.
    // Overflow never occurs when adding operands with different signs.
    temp = op1 + op2;
    if ( ((op1 & 0x8000) == (op2 & 0x8000)) && ((temp & 0x8000) != (op1 & 0x8000)) ) {
        SET_FLAG(VF);
    } else {
        CLR_FLAG(VF);
    }
}

/* test and set V for 8-bit subtraction */
void condevalVs(int32 op1, int32 op2)
{
    int32 temp;

    /* op1 - op2 */
    // If 2 Two's Complement numbers are subtracted, and their signs are different,
    // then overflow occurs if and only if the result has the same sign as the subtrahend (op2).
    temp = op1 - op2;
    if ( ((op1 & 0x80) != (op2 & 0x80)) && ((temp & 0x80) == (op2 & 0x80)) ) {
        SET_FLAG(VF);
    } else {
        CLR_FLAG(VF);
    }
}

/* test and set V for 16-bit subtraction */
void condevalVs16(int32 op1, int32 op2)
{
    int32 temp;

    /* op1 - op2 */
    // If 2 Two's Complement numbers are subtracted, and their signs are different,
    // then overflow occurs if and only if the result has the same sign as the subtrahend (op2).
    temp = op1 - op2;
    if ( ((op1 & 0x8000) != (op2 & 0x8000)) && ((temp & 0x8000) == (op2 & 0x8000)) ) {
        SET_FLAG(VF);
    } else {
        CLR_FLAG(VF);
    }
}

/* test and set H for addition (8-bit only) */
void condevalHa(int32 op1, int32 op2)
{
    if (((op1 & 0x0F) + (op2 & 0x0F)) > 0x0F) 
        SET_FLAG(HF);
    else 
        CLR_FLAG(HF);
}

/* calls from the simulator */

/* Boot routine */
t_stat m6809_boot(int32 unit_num, DEVICE *dptr)
{
    sim_debug(DEBUG_flow, &m6809_dev, "Call to m6809_boot()\n");

    /* retrieve the reset vector at $FFFE */
    saved_PC = CPU_BD_get_mword(0xFFFE);
    if (saved_PC == 0xFFFF) {
        ; // No BOOT ROM detected!
    }
    return SCPE_OK;
}

/* Reset routine */
t_stat m6809_reset (DEVICE *dptr)
{
    sim_debug(DEBUG_flow, &m6809_dev, "Call to m6809_reset()\n");

    CCR = EF | FF | IF;
    DP = 0;
    int_req = 0;
    sim_brk_types = sim_brk_dflt = SWMASK ('E');

    /* retrieve the reset vector at $FFFE */
    saved_PC = CPU_BD_get_mword(0xFFFE);
    if (saved_PC == 0xFFFF) {
        ; // No BOOT ROM detected!
    }
    return SCPE_OK;
}

/* examine routine */
t_stat m6809_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches)
{
    sim_debug(DEBUG_flow, &m6809_dev, "Call to m6809_examine()\n");

    if (addr > ADDRMASK) {
        /* exceed 16-bit address space */
        return SCPE_NXM;
    } else {
        if (eval_array != NULL) {
            *eval_array = CPU_BD_get_mbyte(addr);
        }
        return SCPE_OK;
    }
}

/* deposit routine */
t_stat m6809_deposit(t_value value, t_addr addr, UNIT *uptr, int32 switches)
{
    sim_debug(DEBUG_flow, &m6809_dev, "Call to m6809_deposit()\n");

    if (addr > ADDRMASK) {
        /* exceed 16-bit address space */
        return SCPE_NXM;
    } else {
        CPU_BD_put_mbyte(addr, value);
        return SCPE_OK;
    }
}


/* This is the dumper/loader. This command uses the -h to signify a
    hex dump/load vice a binary one.  If no address is given to load, it
    takes the address from the hex record or the current PC for binary.
*/

t_stat sim_load (FILE *fileref, CONST char *cptr, CONST char *fnam, int flag)
{
    int32 i, addr = 0, cnt = 0;

    sim_debug(DEBUG_flow, &m6809_dev, "Call to sim_load()\n");

    if ((*cptr != 0) || (flag != 0)) return SCPE_ARG;
    addr = saved_PC;
    while ((i = getc (fileref)) != EOF) {
        CPU_BD_put_mbyte(addr, i);
        addr++;
        cnt++;
    }                                   // end while
    printf ("%d Bytes loaded.\n", cnt);
    return (SCPE_OK);
}

/* Symbolic output

    Inputs:
        *of     =   output stream
        addr    =   current PC
        *val    =   pointer to values
        *uptr   =   pointer to unit
        sw      =   switches
    Outputs:
        status  =   error code
        for M6809
*/
t_stat fprint_sym (FILE *of, t_addr addr, t_value *val, UNIT *uptr, int32 sw)
{
    int32 i, inst;

    sim_debug(DEBUG_flow, &m6809_dev, "Call to fprint_sym()\n");

    if (sw & SWMASK ('D')) {            // dump memory
        for (i=0; i<16; i++)
            fprintf(of, "%02X ", val[i]);
        fprintf(of, "  ");
        for (i=0; i<16; i++)
            if (isprint(val[i]))
                fprintf(of, "%c", val[i]);
            else
                fprintf(of, ".");
        return -15;
    } else if (sw & SWMASK ('M')) {     // dump instruction mnemonic
        inst = val[0];
        if (!oplen[inst]) {             // invalid opcode
            fprintf(of, "%02X", inst);
            return 0;
        }

        /* lookup mnemonic in table */
        fprintf (of, "%s", opcode[inst]); // mnemonic
        if (strlen(opcode[inst]) == 3)
            fprintf(of, " ");

        /* display some potential operands - this is not exact! */
        switch (oplen[inst]) {
            case 0:
            case 1:
                break;
            case 2:
                fprintf(of, " $%02X", val[1]);
                break;
            case 3:
                fprintf(of, " $%02X $%02X", val[1], val[2]);
                break;
            case 4:
                fprintf(of, " $%02X $%02X $%02x", val[1], val[2], val[3]);
                break;
        }
        return (-(oplen[inst] - 1));
    } else {
        return SCPE_ARG;
    }
}

/* Symbolic input

    Inputs:
        *cptr   =   pointer to input string
        addr    =   current PC
        *uptr   =   pointer to unit
        *val    =   pointer to output values
        sw      =   switches
    Outputs:
        status  =   error status
*/
t_stat parse_sym (CONST char *cptr, t_addr addr, UNIT *uptr, t_value *val, int32 sw)
{
    return (1);
}

/* end of m6809.c */
