/* rtx2001a_decode.h: RTX2001A simulator definitions

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

   See SIMM2000/DECODE.H

   This file contains the enum definition for result of instruction cracking operations, and prototypes for the decoding subroutine.
 */
#ifndef RTX2001A_DECODE_H_
#define RTX2001A_DECODE_H_ 0

#include "rtx2001a_defs.h"

#define NIL 0

typedef enum machine_op
{
  OP_CALL,
  OP_0BRANCH,
  OP_ALU,
  OP_ASIC_STREAM_MAC,
  OP_BRANCH,
  OP_BS_ONE_TICK,
  OP_BS_TICK,
  OP_BUSLASH_TICK,
  OP_BUSTAR_TICK,
  OP_BUSTAR_TICK_TICK,
  OP_CLEAR_ACC,
  OP_CLEAR_SOFTINT,
  OP_C_TICK,
  OP_DDUP_ALU,
  OP_DDUP_STORE,
  OP_DDUP_STORE_WITH_ALU,
  OP_DEC_RX,
  OP_DROP,
  OP_DROP_DUP,
  OP_DROP_LIT,
  OP_DROP_SHORT_LIT,
  OP_DSLL,
  OP_DSRA,
  OP_DSRL,
  OP_DUP,
  OP_DUP_FETCH_SWAP,
  OP_DUP_GSTORE,
  OP_DUP_USTORE,
  OP_FETCH,
  OP_FETCH_LIT,
  OP_FETCH_OVER_ALU,
  OP_FETCH_SWAP,
  OP_FETCH_SWAP_ALU,
  OP_FETCH_WITH_ALU,
  OP_GFETCH,
  OP_GFETCH_DROP,
  OP_GFETCH_OVER_ALU,
  OP_GFETCH_SWAP_ALU,
  OP_GSTORE,
  OP_INC_RX,
  OP_LIT,
  OP_LIT_OVER_ALU,
  OP_LIT_SWAP,
  OP_LIT_SWAP_ALU,
  OP_MAC,
  OP_MIXED_MAC,
  OP_MIXED_MULT,
  OP_MULT,
  OP_MULT_SUB,
  OP_NEXT,
  OP_NIP,
  OP_NIP_DUP,
  OP_NIP_DUP_FETCH_SWAP,
  OP_NIP_FETCH_LIT,
  OP_NIP_FETCH_WITH_ALU,
  OP_NORMALIZE,
  OP_OVER,
  OP_QDUP_0BRANCH,
  OP_RDR,
  OP_RESERVED,
  OP_RESERVED_STEP_MATH,
  OP_RTR,
  OP_R_TICK,
  OP_SELECT_CPR,
  OP_SELECT_DPR,
  OP_SET_SOFTINT,
  OP_SHIFT,
  OP_SHIFT_MAC_RIGHT,
  OP_SHORT_LIT,
  OP_SHORT_LIT_OVER_ALU,
  OP_SHORT_LIT_SWAP_ALU,
  OP_STAR_TICK,
  OP_STAR_TICK_TICK,
  OP_STORE,
  OP_STORE_LIT,
  OP_STREAM_MAC,
  OP_SWAP,
  OP_S_ONE_TICK,
  OP_S_TICK,
  OP_S_TICK_TICK,
  OP_TUCK_ALU,
  OP_TUCK_STORE,
  OP_TUCK_STORE_WITH_ALU,
  OP_TWO_STAR_TICK,
  OP_UFETCH,
  OP_UFETCH_OVER_ALU,
  OP_UFETCH_SWAP,
  OP_UFETCH_SWAP_ALU,
  OP_UMAC,
  OP_UMULT,
  OP_UNDER_ALU,
  OP_UNDER_STORE,
  OP_UNDER_STORE_LIT,
  OP_USLASH_ONE_TICK,
  OP_USLASH_ONE_TICK_TICK,
  OP_USLASH_TICK,
  OP_USLASH_TICK_TICK,
  OP_USTAR_TICK,
  OP_USTAR_TICK_TICK,
  OP_USTORE,
  OP_ZERO_EQUAL,
  DUMMY_LAST /* put here as a bounds check for initialization */
} machine_op;

/* Driver for instruction operation cracking.
 *   Input: 16-bit RTX instruction
 *   Output: enum for operation to be performed (exclusive of alu/shift op).
 */
t_stat decode(t_value instruction, machine_op *opcode);
t_stat decode_branch(t_value instruction, machine_op *opcode);
t_stat decode_step_math(t_value instruction, machine_op *opcode);
t_stat decode_alu(t_value instruction, machine_op *opcode);
t_stat decode_short_lit(t_value instruction, machine_op *opcode);
t_stat decode_gbus(t_value instruction, machine_op *opcode);
t_stat decode_user(t_value instruction, machine_op *opcode);
t_stat decode_long_lit(t_value instruction, machine_op *opcode);
t_stat decode_memory(t_value instruction, machine_op *opcode);
t_stat decode_special(t_value instruction, machine_op *opcode);

/* Compute branch target address
 *   Input: instruction = 16-bit RTX instruction
 *          address = 16-bit RTX address
 *  Output: 16-bit RTX branch target address
 */
extern t_addr target_addr(t_value instruction, t_addr address);
#endif