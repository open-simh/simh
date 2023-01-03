/* rtx2001a_decode.c: RTX2001A simulator definitions

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

   Instruction decoding

   See SIMM2000/DECODE.C
 */
#include "rtx2001a_registers.h"
#include "rtx2001a_defs.h"
#include "rtx2001a_debug.h"
#include "rtx2001a_decode.h"

/**
 * Driver for instruction operation cracking.
 * Input: 16-bit RTX instruction
 * Output: enum for operation to be performed (exclusive of alu/shift op).
 */
t_stat decode(t_value instruction, machine_op *opcode)
{
    if (instruction & 0x8000)
    {                                 /* non-call instructions */
        switch (instruction & 0xF000) /* 3 bits of top 4 */
        {
        case 0x8000: /* Branch */
        case 0x9000: /* Branch */
            return (decode_branch(instruction, opcode));

        case 0xA000: /* ALU */
            if (instruction & 0x0010)
            {
                return (decode_step_math(instruction, opcode));
            }
            return (decode_alu(instruction, opcode));

        case 0xB000: /* G@/G! or short_lit */
            if (instruction & 0x0040)
            {
                return (decode_short_lit(instruction, opcode));
            }
            return (decode_gbus(instruction, opcode));

        case 0xC000: /* U@/U! */
            return (decode_user(instruction, opcode));

        case 0xD000: /* long literal */
            return (decode_long_lit(instruction, opcode));

        case 0xE000: /* word memory access*/
        case 0xF000: /* byte memory access*/
            return (decode_memory(instruction, opcode));
        }
    }
    else
    { /* call instruction */
        *opcode = OP_CALL;
        return SCPE_OK;
    }
    return STOP_UNK; // Defensive, no test
}

/** Decode given that the instruction is known to be ALU class,
 *  returning an enum corresponding to the instruction type
 *
 * 1010 xxxx xxx0 xxxx
 */
t_stat decode_alu(t_value instruction, machine_op *opcode)
{
    switch (instruction & 0x00C0)
    {
    case 0x0000: // 1010 xxxx 00x0 xxxx
        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = OP_SHIFT; // 1010 000x 00x0 xxxx
            return SCPE_OK;
        }
        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_DROP_DUP; // 1010 111x 00x0 xxxx
            return SCPE_OK;
        }
        *opcode = OP_UNDER_ALU; // 1010 cccc 00x0 xxxx
        return SCPE_OK;

    case 0x0040: // 1010 xxxx 01x0 xxxx
        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = OP_NIP; // 1010 000x 01x0 xxxx
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_DROP; // 1010 111x 01x0 xxxx
            return SCPE_OK;
        }

        *opcode = OP_ALU; // 1010 cccc 01x0 xxxx
        return SCPE_OK;

    case 0x0080: // 1010 xxxx 10x0 xxxx
        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = OP_NIP_DUP; // 1010 000x 10x0 xxxx
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_SWAP; // 1010 111x 10x0 xxxx
            return SCPE_OK;
        }

        *opcode = OP_TUCK_ALU; // 1010 cccc 10x0 xxxx
        return SCPE_OK;

    case 0x00C0: // 1010 xxxx 11x0 xxxx
        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = OP_DUP; // 1010 000x 11x0 xxxx
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_OVER; // 1010 111x 11x0 xxxx
            return SCPE_OK;
        }

        *opcode = OP_DDUP_ALU; // 1010 cccc 11x0 xxxx
        return SCPE_OK;
    }
    return STOP_UNK; // defensive bug check, not tested
}

/**
 * Decode given that the instruction is known to be BRANCH class,
 * returning an enum corresponding to the instruction type
 *
 * 100x xbba aaaa aaaa
 */
t_stat decode_branch(t_value instruction, machine_op *opcode)
{
    *opcode = NIL;
    switch (instruction & 0xF800) // which type of branch instruction
    {
    case 0x8000:
        *opcode = OP_QDUP_0BRANCH;
        return SCPE_OK;
    case 0x8800:
        *opcode = OP_0BRANCH;
        return SCPE_OK;
    case 0x9000:
        *opcode = OP_BRANCH;
        return SCPE_OK;
    case 0x9800:
        *opcode = OP_NEXT;
        return SCPE_OK;
    }
    return STOP_UNK; // defensive bug check, not tested
}

/**
 *  Decode given that the instruction is known to be SHORT LIT class,
 *  returning an enum corresponding to the instruction type
 *
 * 1011 xxxx x1xx xxxx  --- short_lit
 */
t_stat decode_short_lit(t_value instruction, machine_op *opcode)
{
    switch (instruction & 0x00C0)
    {
    case 0x0040: // 1011 xxxx 01xx xxxx
        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = OP_RESERVED; // 1011 000x 01xx xxxx
            return SCPE_OK;
        }
        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_SHORT_LIT; // 1011 111x 01xd dddd
            return SCPE_OK;
        }
        *opcode = OP_SHORT_LIT_OVER_ALU; // 1011 cccc 01xd dddd
        return SCPE_OK;

    case 0x00C0: // 1011 xxxx 11xx xxxx
        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = OP_RESERVED; // 1011 000x 11xx xxxx
            return SCPE_OK;
        }
        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_DROP_SHORT_LIT; // 1011 111x 11xd dddd
            return SCPE_OK;
        }
        *opcode = OP_SHORT_LIT_SWAP_ALU; // 1011 cccc 11xd dddd
        return SCPE_OK;
    }
    return STOP_UNK; // defensive bug check, not tested
}

/**
 * Decode given that the instruction is known to be GBUS class,
 *  returning an enum corresponding to the instruction type
 *
 *  1011 xxxx xxxx xxxx
 */
t_stat decode_gbus(t_value instruction, machine_op *opcode)
{
    int result = 0;

    if ((instruction & 0xFF40) == 0xB000)
    {
        decode_special(instruction, opcode);
        if (OP_RESERVED != *opcode)
        {
            return SCPE_OK;
        }
    }

    // Prevent operations on certain ASIC addresses
    int middle_flag = ((instruction & 0x001F) > 0x07 && (instruction & 0x001F) < 0x18) ? TRUE : FALSE;
    *opcode = OP_RESERVED;

    switch (instruction & 0x00C0)
    {            // case 0x0040 is a short lit instruction
    case 0x0000: /* 1011 xxxx 00xx xxxx */
        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = middle_flag ? OP_RESERVED : OP_GFETCH_DROP; /* 1011 000x 00xg gggg */
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_GFETCH; /* 1011 111x 00xg gggg */
            return SCPE_OK;
        }

        *opcode = middle_flag ? OP_RESERVED : OP_GFETCH_OVER_ALU; /* 1011 cccc 00xg gggg */
        return SCPE_OK;

    case 0x0080: /* 1011 xxxx 10xx xxxx */
        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = middle_flag ? OP_RESERVED : OP_DUP_GSTORE; /* 1011 000x 10xg gggg */
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_GSTORE; /* 1011 111x 10xg gggg */
            return SCPE_OK;
        }

        // Allow ALU operations
        *opcode = (middle_flag && ((instruction & 0x1F) != 0x11) && ((instruction & 0x1F) != 0x09)) ? OP_RESERVED : OP_GFETCH_SWAP_ALU; /* 1011 cccc 10xg gggg */
        return SCPE_OK;
    }
    ABORT(STOP_UNK);
}

/**
 * Decode given that the instruction is known to be LONG LITERAL class,
 *  returning an enum corresponding to the instruction type
 *
 * 1101 xxxx xxxx xxxx  --- long literal
 */
t_stat decode_long_lit(t_value instruction, machine_op *opcode)
{
    if (instruction & 0x001F)
    {                          // See DECODE.C:294 wrong bit pattern for $D01F
        *opcode = OP_RESERVED; // 1101 xxxx xxx1 yyyy
        return SCPE_OK;
    }

    switch (instruction & 0x00C0)
    {
    case 0x0000: // 1101 xxxx 00xx xxxx
        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = OP_LIT_SWAP; /* 1101 000x 00xx xxxx */
            return SCPE_OK;
        }
        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_LIT; /* 1101 111x 00xx xxxx */
            return SCPE_OK;
        }
        *opcode = OP_LIT_OVER_ALU; /* 1101 cccc 00xx xxxx */
        return SCPE_OK;

    case 0x0080: /* 1101 xxxx 10xx xxxx */
        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = OP_RESERVED; /* 1101 000x 10xx xxxx */
            return SCPE_OK;
        }
        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_DROP_LIT; /* 1101 111x 10xx xxxx */
            return SCPE_OK;
        }
        *opcode = OP_LIT_SWAP_ALU; /* 1101 cccc 10xx xxxx */
        return SCPE_OK;
    }
    *opcode = OP_RESERVED; /* 1101 xxxx x1xx xxxx */
    return SCPE_OK;
}

/**
 * Check for "special" instructions given that the instruction
 * is known to be GBUS class, returning an enum corresponding
 * to the instruction type if found, otherwise returning 0.
 */
t_stat decode_special(t_value instruction, machine_op *opcode)
{
    switch (instruction & 0xFFDF)
    {
    case 0xB00D:
        *opcode = OP_SELECT_CPR;
        return SCPE_OK;
    case 0xB010:
        *opcode = OP_CLEAR_SOFTINT;
        return SCPE_OK;
    case 0xB08D:
        *opcode = OP_SELECT_DPR;
        return SCPE_OK;
    case 0xB090:
        *opcode = OP_SET_SOFTINT;
        return SCPE_OK;
    case 0xB016:
        *opcode = OP_DEC_RX;
        return SCPE_OK;
    case 0xB096:
        *opcode = OP_INC_RX;
        return SCPE_OK;
    }
    *opcode = OP_RESERVED;
    return SCPE_OK;
}

/**
 * Decode given that the instruction is known to be MEMORY class,
 * returning an enum corresponding to the instruction type
 *
 * 111x xxxx xxxx xxxx  --- memory access
 */
t_stat decode_memory(t_value instruction, machine_op *opcode)
{
    switch (instruction & 0x01C0)
    {
    case 0x0000: // 111x xxx0 00xx xxxx
    case 0x0100: // 111x xxx1 00xx xxxx
        if (instruction & 0x001F)
        {
            *opcode = OP_RESERVED; // low 5 bits not 0
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = OP_FETCH_SWAP; // 111x 000x 00xx xxxx
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_FETCH; // 111x 111x 00xx xxxx
            return SCPE_OK;
        }

        *opcode = OP_FETCH_OVER_ALU; // 111x cccc 00xx xxxx
        return SCPE_OK;

    case 0x0040: // 111x xxx0 01xx xxxx
        if ((instruction & 0x0E00) == 0x0000)
        {
            if (instruction & 0x001F)
            {
                *opcode = OP_RESERVED; // low 5 bits not 0
                return SCPE_OK;
            }
            *opcode = OP_NIP_DUP_FETCH_SWAP; // 111x 0000 01xx xxxx
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_NIP_FETCH_LIT; // 111x 1110 01xd dddd
            return SCPE_OK;
        }

        // NIP DUP @ SWAP d SWAP ALU
        *opcode = OP_NIP_FETCH_WITH_ALU; // 111x aaa0 01xx xxxx
        return SCPE_OK;

    case 0x0140: // 111x xxx1 01xx xxxx

        if ((instruction & 0x0E00) == 0x0000)
        {
            if (instruction & 0x001F)
            {
                *opcode = OP_RESERVED; // low 5 bits not 0
                return SCPE_OK;
            }
            *opcode = OP_DUP_FETCH_SWAP; // 111x 0001 01xx xxxx
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_FETCH_LIT; // 111x 1111 01xd dddd
            return SCPE_OK;
        }

        // DUP @ SWAP d SWAP ALU
        *opcode = OP_FETCH_WITH_ALU; // 111x aaa1 01xx xxxx
        return SCPE_OK;

    case 0x0080: // 111x xxx0 10xx xxxx
    case 0x0180: // 111x xxx1 10xx xxxx
        if (instruction & 0x001F)
        {
            *opcode = OP_RESERVED; // low 5 bits not 0
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = OP_UNDER_STORE; // 111x 000x 10xx xxxx
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_STORE; // 111x 111x 10xx xxxx
            return SCPE_OK;
        }

        *opcode = OP_FETCH_SWAP_ALU; // 111x cccc 10xx xxxx
        return SCPE_OK;

    case 0x00C0: // 111x xxx0 11xx xxxx
        if ((instruction & 0x0E00) == 0x0000)
        {
            if (instruction & 0x001F)
            {
                *opcode = OP_RESERVED; // low 5 bits not 0
                return SCPE_OK;
            }
            *opcode = OP_DDUP_STORE; // 111x 0000 11xx xxxx
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_UNDER_STORE_LIT; // 111x 1110 11xd dddd
            return SCPE_OK;
        }

        // DDUP ! d SWAP alu
        *opcode = OP_DDUP_STORE_WITH_ALU; // 111x aaa0 11xx xxxx
        return SCPE_OK;

    case 0x01C0: // 111x xxx1 11xx xxxx
        if ((instruction & 0x0E00) == 0x0000)
        {
            if (instruction & 0x001F)
            {
                *opcode = OP_RESERVED; // low 5 bits not 0
                return SCPE_OK;
            }
            *opcode = OP_TUCK_STORE; // 111x 0001 11xx xxxx
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_STORE_LIT; // 111x 1111 11xd dddd
            return SCPE_OK;
        }

        // TUCK ! d SWAP alu
        *opcode = OP_TUCK_STORE_WITH_ALU; // 111x aaa1 11xx xxxx
        return SCPE_OK;
    }

    return STOP_HALT; // Defensive, no test case
}

/**
 * Decode given that the instruction is known to be USER MEMORY class,
 * returning an enum corresponding to the instruction type
 *
 * 1100 xxxx x0xx xxxx  --- U@/U!
 */
t_stat decode_user(t_value instruction, machine_op *opcode)
{
    switch (instruction & 0x00C0)
    {
    case 0x0000: // 1100 xxxx 00xx xxxx
        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = OP_UFETCH_SWAP; // 1100 000x 00xu uuuu
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_UFETCH; // 1100 111x 00xu uuuu
            return SCPE_OK;
        }

        *opcode = OP_UFETCH_OVER_ALU; // 1100 cccc 00xu uuuu
        return SCPE_OK;

    case 0x0080: // 1100 xxxx 10xx xxxx
        if ((instruction & 0x0E00) == 0x0000)
        {
            *opcode = OP_DUP_USTORE; // 1100 000x 10xu uuuu
            return SCPE_OK;
        }

        if ((instruction & 0x0E00) == 0x0E00)
        {
            *opcode = OP_USTORE; // 1100 111x 10xu uuuu
            return SCPE_OK;
        }

        *opcode = OP_UFETCH_SWAP_ALU; // 1100 cccc 10xu uuuu
        return SCPE_OK;
    }
    return STOP_HALT;
}

/**
 * Decode given that the instruction is known to be STEP MATH class,
 * returning an enum corresponding to the instruction type
 */
t_stat decode_step_math(t_value instruction, machine_op *opcode)
{
    *opcode = OP_RESERVED_STEP_MATH;
    switch (instruction & 0xFFDF)
    { // recognize only fully supported/documented step math operations
    case 0xA012:
        *opcode = OP_TWO_STAR_TICK; // 2*'
        break;
    case 0xA096:
        *opcode = OP_RTR; // RTR
        break;
    case 0xA09E:
        *opcode = OP_RDR; // RDR
        break;
    case 0xA196:
        *opcode = OP_R_TICK; // R'
        break;
    case 0xA412:
        *opcode = OP_BUSLASH_TICK; // BU/'
        break;
    case 0xA418:
        *opcode = OP_USLASH_ONE_TICK_TICK; // U/1''
        break;
    case 0xA41A:
        *opcode = OP_USLASH_ONE_TICK; // U/1'
        break;
    case 0xA458:
        *opcode = OP_USLASH_TICK_TICK; // U/''
        break;
    case 0xA45A:
        *opcode = OP_USLASH_TICK; // U/'
        break;
    case 0xA494:
        *opcode = OP_BUSTAR_TICK_TICK; // BU*''
        break;
    case 0xA49C:
        *opcode = OP_USTAR_TICK_TICK; // U*''
        break;
    case 0xA49D:
        *opcode = OP_STAR_TICK_TICK; // *''
        break;
    case 0xA512:
        *opcode = OP_BS_ONE_TICK; // BS1'
        break;
    case 0xA51A:
        *opcode = OP_S_ONE_TICK; // S1'
        break;
    case 0xA552:
        *opcode = OP_BS_TICK; // BS'
        break;
    case 0xA558:
        *opcode = OP_S_TICK_TICK; // S''
        break;
    case 0xA55A:
        *opcode = OP_S_TICK; // S'
        break;
    case 0xA894:
        *opcode = OP_BUSTAR_TICK; // BU*'
        break;
    case 0xA89C:
        *opcode = OP_USTAR_TICK; // U*'
        break;
    case 0xA89D:
        *opcode = OP_STAR_TICK; // *'
        break;
    case 0xAADE:
        *opcode = OP_C_TICK; // C'
        break;
    default:
        return STOP_INVOPCOD;
    }
    return SCPE_OK;
}
