/* rtx2001a_execute.h: RTX2001A simulator definitions

   Copyright (c) 2022, Systasis Computer Systems, Inc.

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
   SYSTASIS COMPUTER SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Systasis Computer Systems shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Systasis Computer Systems.

   10-Oct-22   SYS      New simulator.

   See RTX2000/EXECUTE.H

   Instruction execution
*/

#ifndef RTX2001A_EXECUTE_H_
#define RTX2001A_EXECUTE_H_ 0

#include "rtx2001a_defs.h"
#include "rtx2001a_decode.h"

/**
 *  Note: you *MUST* change the initialization statement for these arrays
 * if NUMBER_OF_ROUTINES is changed !!!!!
 */
#define NUMBER_OF_ROUTINES 128
extern void (*dispatch_vector_1[NUMBER_OF_ROUTINES])();
extern void (*dispatch_vector_2[NUMBER_OF_ROUTINES])();

extern void init_dispatch_vectors(); // Function dispatch vector

extern t_stat execute(t_value instruction);
extern void print_instruction(t_value instruction, t_value page, t_addr address);
extern t_stat alu(t_value opa, t_value opb, t_value *result);
extern t_stat shift(void);

extern void do_0branch();
extern void do_call();
extern void do_exit();
#define do_fetch fetch_swap
extern void do_fetch_2();
#define do_fetch_over_alu fetch_swap
extern void do_fetch_over_alu_2();
extern void do_fetch_swap_2();
#define do_fetch_swap_alu fetch_swap
extern void do_fetch_swap_alu_2();
#define do_lit D_swap
#define do_lit_swap_alu D_swap
extern void do_list_2();
extern void do_qdup_0branch();
extern void do_store_lit_2();
extern void bad_insn(void);
extern void D_swap();

extern RTX_WORD clocks;
extern t_value STREAM;
extern t_value stream_mode;
extern t_value second_cycle;

#define CLOCKS(arg) clocks += arg

#define EXIT (IR & 0x0020)

#define TEST_EXIT      \
    {                  \
        if (EXIT)      \
            do_exit(); \
    }

// #define CARRY(a, b) (((uint16_t)a >= (uint16_t)(-b)) ? 1 : 0)
// #define BORROW(a, b) (((uint16_t)a < (uint16_t)b) ? 1 : 0)
#define BORROW(a, b, c) CARRY(a, ~b, c)

#define invert()                     \
    {                                \
        if (IR & 0x0100)             \
            TOP = (~TOP) & D16_UMAX; \
    }

/* MACROS FOR SIMPLIFIED PRINTING */

/* print out a shift operation */
#define SHIFT (shift_names[instruction & 0xF])

/* print out the invert operation */
#define INVERT (alu_names[(instruction >> 8) & 0x1])

/* print out an alu operation */
#define ALU (alu_names[(instruction >> 8) & 0xF])
#define SWAP_ALU (swap_alu_names[(instruction >> 8) & 0xF])

/* print out a 3-bit alu operation */
#define ALU3 (alu_names[(instruction >> 8) & 0xE])
#define SWAP_ALU3 (swap_alu_names[(instruction >> 8) & 0xE])

/* extract the bit that determines word vs. byte */
#define MEM (memory_names[(instruction >> 12) & 0x1])

#endif
