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

   10-Oct-22   SYS      Initial version.

   See RTX2000/EXECUTE.C

   Instruction execution
*/
#include "rtx2001a_defs.h"
#include "rtx2001a_debug.h"
#include "rtx2001a_ab.h"
#include "rtx2001a_psb.h"
#include "rtx2001a_mb.h"
#include "rtx2001a_registers.h"
#include "rtx2001a_execute.h"
#include "rtx2001a_decode.h"

/* disassembler constants for alu functions */
char *alu_names[] = {
    /* 0 */ "", /* 1 */ "not ",
    /* 2 */ "and ", /* 3 */ "nor ",
    /* 4 */ "swap- ", /* 5 */ "swap-c ",
    /* 6 */ "or ", /* 7 */ "nand ",
    /* 8 */ "+ ", /* 9 */ "+c ",
    /* a */ "xor ", /* b */ "xnor ",
    /* c */ "- ", /* d */ "-c ",
    /* e */ "", /* f */ "not "};

char *swap_alu_names[] = {
    /* 0 */ "", /* 1 */ "not ",
    /* 2 */ "and ", /* 3 */ "nor ",
    /* 4 */ "- ", /* 5 */ "-c ",
    /* 6 */ "or ", /* 7 */ "nand ",
    /* 8 */ "+ ", /* 9 */ "+c ",
    /* a */ "xor ", /* b */ "xnor ",
    /* c */ "swap- ", /* d */ "swap-c ",
    /* e */ "", /* f */ "not "};

/* disassembler constants for shifter functions */
char *shift_names[] = {
    /* 0 */ "", /* 1 */ "0< ",
    /* 2 */ "2* ", /* 3 */ "2*c ",
    /* 4 */ "cU2/ ", /* 5 */ "c2/ ",
    /* 6 */ "U2/ ", /* 7 */ "2/ ",
    /* 8 */ "N2* ", /* 9 */ "N2*c ",
    /* A */ "D2* ", /* B */ "D2*c ",
    /* C */ "cUD2/ ", /* D */ "cD2/ ",
    /* E */ "UD2/ ", /* F */ "D2/ "};

/* disassembler constants for memory size specifiers */
char *memory_names[] = {
    /* 0 */ "", /* 1 */ "C"};

void bad_insn()
{
    sim_printf("%s PC=0x%X IR=0x%X\n", sim_stop_messages[STOP_INVOPCOD], asic_file[PC], IR);
    // print_instruction(IR, cpr.pr, asic_file[PC]);
    ABORT(STOP_IBKPT);
    // longjmp(bkpt_env, STOP_IBKPT);
}

void do_exit()
{
    cpr.pr = ipr.pr & 0x0F;
    sim_debug(DBG_ASB_W, &cpu_dev, "CPR=0x%X\n", cpr.pr);
    set_DPRSEL(ipr.pr & 0x10);
    asic_file[PC] = asic_file[I];
    sim_debug(DBG_ASB_W, &cpu_dev, "PC=0x%X\n", asic_file[PC]);
    rs_pop();
}

void do_call()
{
    sim_debug(DBG_ASB_R, &cpu_dev, "PC=0x%04X\n", asic_file[PC]);
    rs_push(cpr.pr, asic_file[PC]);
    asic_file[PC] = (IR << 1) & D16_UMAX;
    sim_debug(DBG_ASB_W, &cpu_dev, "PC=0x%04X\n", asic_file[PC]);
    CLOCKS(1);
}

void do_qdup_0branch()
{
    if (!TOP)
    {
        sim_debug(DBG_ASB_R, &cpu_dev, "PC=0x%X\n", asic_file[PC]);
        asic_file[PC] = target_addr(IR, asic_file[PC]);
        sim_debug(DBG_ASB_W, &cpu_dev, "PC=0x%X\n", asic_file[PC]);
        ps_pop();
    }
    CLOCKS(1);
}

/* void do_0branch()
{
    sim_debug(DBG_ASB_R, &cpu_dev, "PC=0x%X IR=0x%X\n", asic_file[PC], IR);
    asic_file[PC] = target_addr(IR, asic_file[PC]);
    sim_debug(DBG_ASB_W, &cpu_dev, "PC=0x%X\n", asic_file[PC]);

    if (!TOP)
    {
        ps_pop();
    }
    CLOCKS(1);
}
*/
void do_0branch()
{
    if (0 == TOP)
    {
        sim_debug(DBG_ASB_R, &cpu_dev, "PC=0x%X IR=0x%X\n", asic_file[PC], IR);
        asic_file[PC] = target_addr(IR, asic_file[PC]);
        sim_debug(DBG_ASB_W, &cpu_dev, "PC=0x%X\n", asic_file[PC]);
    }

    ps_pop();
    CLOCKS(1);
}

/*
** Support routines
*/
t_stat shift()
{
    /* compute shift function */
    switch (IR & 0x000F)
    {
    case 0x0000: // nop
        break;

    case 0x0001: // 0<
        TOP = (TOP & 0x8000) ? 0xFFFF : 0x0000;
        break;

    case 0x0002: // 2*
    {
        CR ocr;
        ocr.pr = cr.pr;
        CY = (TOP & 0x8000) ? 1 : 0;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 0);
        TOP = (TOP << 1) & 0xFFFE;
    }
    break;

    case 0x0003: /* 2*c */
    {
        CR ocr;
        ocr.pr = cr.pr;
        t_value temp = (TOP << 1) & 0xFFFE;
        if (CY)
        {
            temp = temp | 1;
        }
        CY = (TOP & 0x8000) ? 1 : 0;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 0);
        TOP = temp;
    }
    break;

    case 0x0004: // cU2/
    {
        CR ocr;
        ocr.pr = cr.pr;
        TOP = (TOP >> 1) & 0x7FFF;
        if (0 != CY)
        {
            TOP = TOP | 0x8000;
        }
        CY = 0;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 0);
    }
    break;

    case 0x0005: // c2/
    {
        CR ocr;
        ocr.pr = cr.pr;
        t_value temp = (TOP >> 1) & 0x7FFF;
        if (CY)
        {
            temp = temp | 0x8000;
        }
        CY = (TOP & 1) ? 1 : 0;
        TOP = temp;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 0);
    }
    break;

    case 0x0006: // U2/
    {
        CR ocr;
        ocr.pr = cr.pr;
        TOP = (TOP >> 1) & 0x7FFF;
        CY = 1;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 0);
    }
    break;

    case 0x0007: // 2/
    {
        CR ocr;
        ocr.pr = cr.pr;
        t_value temp = (TOP >> 1) & 0x7FFF;
        if (TOP & 0x8000)
        {
            temp |= 0x8000;
        }
        CY = (TOP & 0x8000) ? 1 : 0;
        TOP = temp;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 0);
    }
    break;

    case 0x0008: // N2*
        NEXT = (NEXT << 1) & 0xFFFE;
        break;

    case 0x0009: // N2*c
        NEXT = (NEXT << 1) & 0xFFFE;
        if (CY)
        {
            NEXT |= 1;
        }
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "CR", cr_bits, cr.pr, cr.pr, 0);
        break;

    case 0x000A: // D2*
    {
        CR ocr;
        ocr.pr = cr.pr;
        CY = (TOP & 0x8000) ? 1 : 0;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 1);
        TOP = (TOP << 1) & 0xFFFE;
        if (NEXT & 0x8000)
            TOP |= 1;
        NEXT = (NEXT << 1) & 0xFFFE;
    }
    break;

    case 0x000B: // D2*c
    {
        CR ocr;
        ocr.pr = cr.pr;
        t_value temp = (NEXT & 0x8000) ? 1 : 0;
        t_value tempa = (TOP & 0x8000) ? 1 : 0;
        NEXT = (NEXT << 1) & 0xFFFE;
        if (CY)
        {
            NEXT |= 1;
        }
        TOP = (TOP << 1) & 0xFFFE;
        if (temp)
            TOP |= 1;
        CY = tempa;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 0);
    }
    break;

    case 0x000C: // cUD2/
    {
        CR ocr;
        ocr.pr = cr.pr;
        t_value temp = (TOP & 0x0001) ? 1 : 0;
        TOP = (TOP >> 1) & 0x7FFF;
        if (CY)
            TOP |= 0x8000;
        NEXT = (NEXT >> 1) & 0x7FFF;
        if (temp)
            NEXT |= 0x8000;
        CY = 1;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 0);
    }
    break;

    case 0x000D: // cD2/
    {
        CR ocr;
        ocr.pr = cr.pr;
        t_value temp = (TOP & 0x0001) ? 1 : 0;
        t_value tempa = (NEXT & 0x0001) ? 1 : 0;
        TOP = (TOP >> 1) & 0x7FFF;
        if (CY)
            TOP |= 0x8000;
        NEXT = (NEXT >> 1) & 0x7FFF;
        if (temp)
            NEXT |= 0x8000;
        CY = tempa;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 0);
    }
    break;

    case 0x000E: // UD2/
    {
        CR ocr;
        ocr.pr = cr.pr;
        t_value temp = (TOP & 0x0001) ? 1 : 0;
        TOP = (TOP >> 1) & 0x7FFF;
        NEXT = (NEXT >> 1) & 0x7FFF;
        if (temp)
            NEXT |= 0x8000;
        CY = 1;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 0);
    }
    break;

    case 0x000F: // D2/
    {
        CR ocr;
        ocr.pr = cr.pr;
        t_value temp = (TOP & 0x8000) ? 1 : 0;
        t_value tempa = (TOP & 0x0001) ? 1 : 0;
        TOP = (TOP >> 1) & 0x7FFF;
        TOP |= temp;
        CY = temp;
        NEXT = (NEXT >> 1) & 0x7FFF;
        if (tempa)
        {
            NEXT |= 0x8000;
        }
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 0);
    }
    break;

    default:
        return STOP_UNK; // Defensive, no test
    }

    sim_debug(DBG_ASB_W, &cpu_dev, " TOP=0x%X NEXT=0x%X\n", TOP, NEXT);
    return SCPE_OK;
}

void do_alu()
{
    t_stat result = alu(TOP, NEXT, &NEXT);
    if (SCPE_OK != result)
    {
        ABORT(result);
    }

    ps_pop();
    invert();

    if (SCPE_OK != (result = shift()))
    {
        ABORT(result);
    }

    TEST_EXIT;
    CLOCKS(1);
    sim_debug(DBG_ASB_W, &cpu_dev, " TOP=0x%X NEXT=0x%X\n", TOP, NEXT);
}

void do_asic_stream_mac()
{
    bad_insn();
    TEST_EXIT;
    CLOCKS(1);
}

void do_branch()
{
    sim_debug(DBG_ASB_R, &cpu_dev, "PC=0x%X IR=0x%X\n", asic_file[PC], IR);
    asic_file[PC] = target_addr(IR, asic_file[PC]);
    sim_debug(DBG_ASB_W, &cpu_dev, "PC=0x%X\n", asic_file[PC]);
    CLOCKS(1);
}

void do_drop()
{
    ps_pop();
    invert();
    t_stat result = shift();
    if (SCPE_OK != result)
        ABORT(result);
    TEST_EXIT;
    CLOCKS(1);
}

void do_dup()
{
    ps_push(TOP);
    invert();
    t_stat result = shift();
    if (SCPE_OK != result)
        ABORT(result);
    TEST_EXIT;
    CLOCKS(1);
}

void do_ddup_alu()
{
    t_stat result = SCPE_LOST;
    ps_push(NEXT);
    if (SCPE_OK != (result = alu(NEXT, TOP, &TOP)))
    {
        ABORT(result);
    }
    invert();
    shift();
    TEST_EXIT;
    CLOCKS(1);
}

void do_ddup_store_with_alu()
{
    t_stat result = SCPE_LOST;
    store(TOP, NEXT);
    if (SCPE_OK != (result = alu(TOP, SHORT_LIT, &TOP)))
    {
        ABORT(result);
    }

    TEST_EXIT;
    CLOCKS(1);
    second_cycle = TRUE;
}

void do_ddup_store_with_alu_2()
{
    CLOCKS(1);
}

void do_fetch_2() /* SWAP {inv} */
{
    int temp = TOP;
    TOP = NEXT;
    NEXT = temp;
    invert();
    CLOCKS(1);
}

/**
 ** @ swap
 */
void fetch_swap()
{
    RTX_WORD temp = NEXT;
    fetch(TOP, &NEXT);
    TOP = temp;
    TEST_EXIT;
    CLOCKS(1);
    second_cycle = TRUE;
}

void do_fetch_swap_alu_2() /* alu */
{
    t_stat result = SCPE_LOST;
    if (SCPE_OK != (result = alu(TOP, NEXT, &NEXT)))
    {
        ABORT(result);
    }

    ps_pop();
    CLOCKS(1);
}

void do_gfetch()
{
    CLOCKS(1);
    t_value temp = 0;
    t_stat result = gfetch(SHORT_LIT, &temp);
    if (SCPE_OK != result)
        ABORT(result);
    ps_push(temp);
    invert();
    TEST_EXIT;
}

/**
 * TODO: ??? what happens for short_lit = 16, short_lit = 17???
 */
void do_gfetch_drop()
{
    t_stat result = SCPE_LOST;
    t_value value;
    // See EXECUTE.C:559 Does not write TOP
    if (SCPE_OK != (result = gfetch(SHORT_LIT, &value)))
    {
        ABORT(result);
    }
    invert();
    TEST_EXIT;
    CLOCKS(1);
}

void do_fetch_over_alu_2() /* TUCK alu */
{
    t_stat result = SCPE_LOST;
    t_value temp = TOP;
    if (SCPE_OK != (result = alu(TOP, NEXT, &TOP)))
    {
        ABORT(result);
    }
    NEXT = temp;
    CLOCKS(1);
}

void do_fetch_swap_2()
{
    invert();
    CLOCKS(1);
}

void do_gfetch_swap_alu()
{ /* ??? what happens for short_lit = 16, short_lit = 17??? */
    t_value temp;
    t_stat result = gfetch(SHORT_LIT, &temp);
    if (SCPE_OK != result)
        ABORT(result);
    result = alu(TOP, temp, &TOP);
    if (SCPE_OK != result)
        ABORT(result);
    TEST_EXIT;
    CLOCKS(1);
}

void do_fetch_with_alu()
{
    t_stat result = SCPE_LOST;
    ps_push(TOP);
    fetch(TOP, &NEXT);
    if (SCPE_OK != (result = alu(TOP, SHORT_LIT, &TOP)))
    {
        ABORT(result);
    }

    TEST_EXIT;
    CLOCKS(1);
    second_cycle = TRUE;
}

void do_fetch_with_alu_2()
{
    CLOCKS(1);
}

void do_gstore()
{
    if (SHORT_LIT != 7)
        TEST_EXIT; /* important that TEST_EXIT come before gstore! */

    t_stat result = gstore(SHORT_LIT, TOP);
    if (SCPE_OK != result)
        ABORT(result);
    if (SHORT_LIT != 9)
        ps_pop();
    invert();
    CLOCKS(1);
}

void do_lit_2() /* swap {inv} */
{
    t_value temp;
    temp = NEXT;
    NEXT = TOP;
    TOP = temp;
    invert();
    CLOCKS(1);
}

void do_lit_swap_alu_2()
{
    t_stat result = alu(TOP, NEXT, &NEXT);
    if (SCPE_OK != result)
    {
        ABORT(result);
    }

    ps_pop();
    CLOCKS(1);
}

void do_next()
{
    if (0 != asic_file[I])
    { /* loop */
        asic_file[PC] = target_addr(IR, asic_file[PC]);
        asic_file[I] = (asic_file[I] - 1) & D16_UMAX;
        sim_debug(DBG_ASB_W, &cpu_dev, "I=0x%X PC=0x%X\n", asic_file[I], asic_file[PC]);
    }
    else
    { /* fall through */
        rs_pop();
    }
    CLOCKS(1);
}

void do_nip()
{
    NEXT = TOP;
    ps_pop();
    invert();
    shift();
    TEST_EXIT;
    CLOCKS(1);
}

void do_over()
{
    ps_push(NEXT);
    invert();
    t_stat result = shift();
    if (SCPE_OK != result)
        ABORT(result);
    TEST_EXIT;
    CLOCKS(1);
}

void do_shift()
{
    t_stat result;
    invert();
    if (SCPE_OK != (result = shift()))
    {
        ABORT(result);
    }
    TEST_EXIT;
    CLOCKS(1);
}

void do_short_lit_swap_alu()
{
    t_stat result;
    if (SCPE_OK != (result = alu(TOP, SHORT_LIT, &TOP)))
    {
        ABORT(result);
    }
    TEST_EXIT;
    CLOCKS(1);
    sim_debug(DBG_ASB_W, &cpu_dev, " TOP=0x%X NEXT=0x%X\n", TOP, NEXT);
}

void do_store()
{
    store(TOP, NEXT);
    ps_pop();
    TEST_EXIT;
    CLOCKS(1);
    second_cycle = TRUE;
}

void do_store_2()
{
    ps_pop();
    invert();
    CLOCKS(1);
}

void do_store_lit()
{
    store(TOP, NEXT);
    ps_pop();
    TOP = SHORT_LIT;
    TEST_EXIT;
    CLOCKS(1);
    second_cycle = TRUE;
}

void do_store_lit_2()
{
    CLOCKS(1);
}

void do_swap()
{
    t_value temp = TOP;
    t_stat result;
    TOP = NEXT;
    NEXT = temp;
    invert();

    if (SCPE_OK != (result = shift()))
    {
        ABORT(result);
    }

    TEST_EXIT;
    CLOCKS(1);
}

void do_tuck_alu()
{
    t_value temp = NEXT;
    NEXT = TOP;
    t_stat result = alu(TOP, temp, &TOP);
    if (SCPE_OK != result)
        ABORT(result);
    result = shift();
    if (SCPE_OK != result)
        ABORT(result);
    TEST_EXIT;
    CLOCKS(1);
}

void do_tuck_store_with_alu()
{
    store(TOP, NEXT);
    NEXT = TOP;
    ps_pop();
    t_stat result = alu(TOP, SHORT_LIT, &TOP);
    if (SCPE_OK != result)
        ABORT(result);
    TEST_EXIT;
    CLOCKS(1);
    second_cycle = TRUE;
}

void do_tuck_store_with_alu_2()
{
    CLOCKS(1);
}

/**
 * D swap
 */
void D_swap()
{
    ps_push(TOP);
    _long_fetch(cpr.fields.pr, asic_file[PC], &NEXT);
    INC_PC;
    TEST_EXIT;
    CLOCKS(1);
    second_cycle = TRUE;
}

void do_short_lit()
{
    ps_push(SHORT_LIT);
    invert();
    TEST_EXIT;
    CLOCKS(1);
}

void do_uslash_one_tick()
{
    /* really sleazy definition to make things work for now ???? */
    /* undoes the preceeding d2* */
    NEXT = (NEXT >> 1) & 0x7FFF;
    if (TOP & 1)
        NEXT = NEXT | 0x8000;
    TOP = (TOP >> 1) & 0x7FFF;
    if (CY)
        TOP = TOP | 0x8000;
}

void do_uslash_tick()
{
    /* really sleazy definition to make things work for now ???? */
    /* do nothing -- save the division up for the end! */
    TEST_EXIT;
    CLOCKS(1);
}

void do_uslash_tick_tick()
{
    /* really sleazy definition to make things work for now ???? */
    /* do all the division at once */
    t_uint64 dividend;
    RTX_WORD divisor, quotient, remainder;
    dividend = ((t_uint64)TOP << 16) & 0xFFFF0000;
    dividend = dividend | (t_uint64)(NEXT & 0xFFFF);
    divisor = asic_file[MD];
    quotient = dividend / divisor;
    dividend = dividend - ((t_uint64)quotient * (t_uint64)divisor);
    remainder = dividend;
    TOP = remainder;
    NEXT = quotient;
    TEST_EXIT;
    CLOCKS(1);
}

/**
 *  ??? the below definitions might speed things up,  from Mitch Bradley
 * ??? the below definitions might speed things up,  from Mitch Bradley
 * #define CARRY(a,b)   ((unsigned)a >= (unsigned)(-b))
 * #define BORROW(a,b)  ((unsigned)a <  (unsigned)b)
 * 16-bit versions that promote to longs
 */
RTX_WORD CARRY(RTX_WORD a, RTX_WORD b, RTX_WORD c)
{
    t_value sum, temp; /* sum=a+b+c */
    /* the additions below use high 16 bits of the long to catch carries */
    temp = a;
    sum = temp & 0xFFFF;
    temp = b;
    sum = sum + (temp & 0xFFFF);
    temp = c;
    sum = sum + (temp & 0xFFFF);
    if (sum & 0x10000)
        return (1);
    return (0);
}

/**
 * All ALU operations are performed between the contents of the TOP Register
 * and another operand which is determined by the instruction.
 * The results of the operation are loaded into TOP. The ALU function to be performed
 * is encoded as a field in the instruction and is shown in the opcode formats
 * as either "cccc" or "aaa".
 *
 * See Table 7.3, Prgmrs Ref Man
 */
t_stat alu(t_value opa, t_value opb, t_value *result)
{
    switch (IR & 0x0F00)
    {
    case 0x0200: // T AND Y
        *result = (opa & opb) & D16_MASK;
        return SCPE_OK;
    case 0x0300: // T NOR Y
        *result = (~(opa | opb)) & D16_MASK;
        return SCPE_OK;
    case 0x0400: // T - Y
        CY = BORROW(opa, opb, 1);
        *result = (opa - opb) & D16_MASK;
        return SCPE_OK;
    case 0x0500: // T - Y with borrow
        CY = BORROW(opa, opb, CY);
        *result = (opa + ~opb + CY) & D16_MASK;
        return SCPE_OK;
    case 0x0600: // T OR Y
        *result = (opa | opb) & D16_MASK;
        return SCPE_OK;
    case 0x0700: // T NAND Y
        *result = (~(opa & opb)) & D16_MASK;
        return SCPE_OK;
    case 0x0800: // T + Y
        CY = CARRY(opa, opb, 0);
        *result = (opa + opb) & D16_MASK;
        return SCPE_OK;
    case 0x0900: // T + Y with carry
        CY = CARRY(opa, opb, CY);
        *result = (opa + opb + CY) & D16_MASK;
        return SCPE_OK;
    case 0x0A00: // T XOR Y
        *result = (opa ^ opb) & D16_MASK;
        return SCPE_OK;
    case 0x0B00: // T XNOR Y
        *result = (~(opa ^ opb)) & D16_MASK;
        return SCPE_OK;
    case 0x0C00: // Y - T
        CY = BORROW(opb, opa, 1);
        *result = (opb - opa) & D16_MASK;
        return SCPE_OK;
    case 0x0D00: //  Y - T with borrow
        CY = BORROW(opb, opa, CY);
        *result = (opb + ~opa + CY) & D16_MASK;
        return SCPE_OK;
    }
    return STOP_UNK; // Defensive, no test
}

/* Compute branch target address
 *   Input: instruction = 16-bit RTX instruction
 *          address = 16-bit RTX address
 *  Output: 16-bit RTX branch target address
 */
t_addr target_addr(RTX_WORD instruction, t_addr address)
{
    /**
     * The RTX Branch Instruction treats each Code Memory page as 64 "blocks" of 512 words each.
     * Bits 15-10 of the Program Counter determine the block number;
     * Bits 9-0 determine the word offset within the block.
     */
    t_addr next_addr = ((address + 1) & 0xFC00) | ((instruction << 1) & 0x03FE);
    sim_debug(DBG_CPU, &cpu_dev, "NEXT_ADDR=0x%X BK=%d\n", next_addr, instruction & 0x0600);

    // See TABLE 7.2: BLOCK BRANCHING ASSIGNMENTS, Pgmrs Ref Man
    switch (instruction & 0x0600)
    {
    case 0x0000: /* same memory block */
        return (next_addr);
    case 0x0200: /* next memory block */
        return (next_addr + 0x0400) & D16_UMAX;
    case 0x0400: /* memory block 0 */
        return (next_addr & 0x03FE);
    case 0x0600: /* previous memory block */
        return (next_addr - 0x0400) & D16_UMAX;
    }
    sim_debug(DBG_CPU, &cpu_dev, "case statement error in target_addr, IR=0x%X, addr=0x%X", instruction, address);
    return 0;
}
/* dispatch table for 1st clock cycle */
void (*dispatch_vector_1[NUMBER_OF_ROUTINES])() =
    {
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 16 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 32 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 48 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 64 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 80 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 96 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 112 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn /* 128 */
};

/* dispatch table for 2nd clock cycle of 2-cycle instructions */
void (*dispatch_vector_2[NUMBER_OF_ROUTINES])() =
    {
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 16 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 32 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 48 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 64 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 80 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 96 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn, /* 112 */
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn,
        bad_insn, bad_insn, bad_insn, bad_insn /* 128 */
};

t_stat execute(t_value instruction)
{
    machine_op opcode = 0;
    t_stat status;
    if (SCPE_OK == (status = decode(instruction, &opcode)))
    {
        print_instruction(instruction, cpr.pr, asic_file[PC]);
        if (!STREAM && second_cycle)
        {
            dispatch_vector_2[opcode]();
            second_cycle = 0;
        }
        else
        {
            dispatch_vector_1[opcode]();
        }
    }
    return status;
}

void dump_header(char *dest, t_value offset)
{
    t_value temp = 0;
    t_value _sim_is_running = sim_is_running;
    sim_is_running = 0;
    byte_fetch(((IR << 1) & D16_MASK) - offset, &temp);
    sim_is_running = _sim_is_running;
    if (temp > 32 && temp < 128)
    {
        *dest = temp;
    }
}

/**
 * print a disassembled instruction
 */
void print_instruction(t_value instruction, t_value page, t_addr address)
{

#define PREFIX \
    ((second_cycle && !STREAM) ? "2nd " : "")

    machine_op opr = 0;
    if (SCPE_OK != decode(instruction, &opr))
    {
        return;
    }
    switch (opr)
    {
    case OP_0BRANCH:
        sim_debug(DBG_CPU, &cpu_dev, "0BRANCH TOP=0x%X 0x%X\n", TOP, target_addr(instruction, address));
        break;

    case OP_ALU:
        sim_debug(DBG_CPU, &cpu_dev, "%s%s\n", ALU, SHIFT);
        break;

    case OP_ASIC_STREAM_MAC:
        sim_debug(DBG_CPU, &cpu_dev, "Asic Stream Mac\n");
        break;

    case OP_BRANCH:
        sim_debug(DBG_CPU, &cpu_dev, "BRANCH %d:0x%X\n", page, target_addr(instruction, address));
        break;

    case OP_BS_ONE_TICK:
        sim_debug(DBG_CPU, &cpu_dev, "BS1'\n");
        break;

    case OP_BS_TICK:
        sim_debug(DBG_CPU, &cpu_dev, "BS'\n");
        break;

    case OP_BUSLASH_TICK:
        sim_debug(DBG_CPU, &cpu_dev, "BU/'\n");
        break;

    case OP_BUSTAR_TICK:
        sim_debug(DBG_CPU, &cpu_dev, "BU*'\n");
        break;

    case OP_BUSTAR_TICK_TICK:
        sim_debug(DBG_CPU, &cpu_dev, "BU*''\n");
        break;

    case OP_CALL:
    {
        char buffer[64];
        memset(buffer, '\0', sizeof(buffer));
        sprintf(buffer, "CALL %d:0x%X ", page, ((instruction << 1) & D16_MASK));
        t_value i = strlen(buffer);
        for (int j = 9; j > 0; j--, i++)
        {
            dump_header(&buffer[i], j);
        }
        sim_debug(DBG_CPU, &cpu_dev, "%s\n", buffer);
    }
    break;

    case OP_DDUP_STORE:
        sim_debug(DBG_CPU, &cpu_dev, "%sDDUP %s!\n", PREFIX, MEM);
        break;

    case OP_DDUP_STORE_WITH_ALU:
        sim_debug(DBG_CPU, &cpu_dev, "%sDDUP %s! 0x%X %s\n", PREFIX, MEM, SHORT_LIT, SWAP_ALU3);
        break;

    case OP_DROP:
        sim_debug(DBG_CPU, &cpu_dev, "DROP %s%s\n", INVERT, SHIFT);
        break;

    case OP_DROP_LIT:
    {
        t_value value;
        _long_fetch(page, address, &value);
        sim_debug(DBG_CPU, &cpu_dev, "%sDROP LIT %d %s\n", PREFIX, value, INVERT);
    }
    break;

    case OP_DDUP_ALU:
        sim_debug(DBG_CPU, &cpu_dev, "DDUP %s%s\n", ALU, SHIFT);
        break;

    case OP_DUP:
        sim_debug(DBG_CPU, &cpu_dev, "DUP %s%s\n", INVERT, SHIFT);
        break;

    case OP_DUP_FETCH_SWAP:
        sim_debug(DBG_CPU, &cpu_dev, "%sDUP %s@ SWAP\n", PREFIX, MEM);
        break;

    case OP_DUP_USTORE:
        sim_debug(DBG_CPU, &cpu_dev, "%sDUP %X U! %s\n", PREFIX, SHORT_LIT, INVERT);
        break;

    case OP_GFETCH_DROP:
        sim_debug(DBG_CPU, &cpu_dev, "0x%X G@ DROP %s ", SHORT_LIT, INVERT);
        if (SHORT_LIT == 1)
            sim_debug(DBG_CPU, &cpu_dev, "(R> DROP)\n");
        else
            sim_debug(DBG_CPU, &cpu_dev, "\n");
        break;

    case OP_FETCH:
        sim_debug(DBG_CPU, &cpu_dev, "%s%s@ %s\n", PREFIX, MEM, INVERT);
        break;

    case OP_FETCH_LIT:
        sim_debug(DBG_CPU, &cpu_dev, "%s%s@ 0x%X\n", PREFIX, MEM, SHORT_LIT);
        break;

    case OP_FETCH_SWAP:
        sim_debug(DBG_CPU, &cpu_dev, "%s%s@ SWAP %s\n", PREFIX, MEM, INVERT);
        break;

    case OP_FETCH_OVER_ALU:
        sim_debug(DBG_CPU, &cpu_dev, "%s%s@ OVER %s\n", PREFIX, MEM, ALU);
        break;

    case OP_FETCH_SWAP_ALU:
        sim_debug(DBG_CPU, &cpu_dev, "%s%s@ %s\n", PREFIX, MEM, SWAP_ALU);
        break;

    case OP_FETCH_WITH_ALU:
        sim_debug(DBG_CPU, &cpu_dev, "%s0x%X %s@_%s\n", PREFIX, SHORT_LIT, MEM, ALU3);
        break;

    case OP_GFETCH:
        sim_debug(DBG_CPU, &cpu_dev, "0x%X G@ %s ", SHORT_LIT, INVERT);
        if (SHORT_LIT == 1)
        {
            sim_debug(DBG_CPU, &cpu_dev, "(R>)\n");
        }
        else
        {
            sim_debug(DBG_CPU, &cpu_dev, "\n");
        };
        break;

    case OP_QDUP_0BRANCH:
        sim_debug(DBG_CPU, &cpu_dev, "?DUP 0BRANCH %d:0x%X\n", page, target_addr(instruction, address));
        break;

    case OP_GFETCH_SWAP_ALU:
        sim_debug(DBG_CPU, &cpu_dev, "0x%X G@ %s\n", SHORT_LIT, SWAP_ALU);
        break;

    case OP_GSTORE:
        sim_debug(DBG_CPU, &cpu_dev, "0x%X G! %s ", SHORT_LIT, INVERT);
        if (SHORT_LIT == 0)
        {
            sim_debug(DBG_CPU, &cpu_dev, "(R> DROP >R)\n");
        }
        else if (SHORT_LIT == 1)
        {
            sim_debug(DBG_CPU, &cpu_dev, "(>R)\n");
        }
        else if (SHORT_LIT == 7 && (instruction & 0x0020))
        {
            sim_debug(DBG_CPU, &cpu_dev, "(>R;)\n");
        }
        else
        {
            sim_debug(DBG_CPU, &cpu_dev, "\n");
        }
        break;

    case OP_LIT:
    {
        if (STREAM | !second_cycle)
        {
            t_value value;
            t_bool _sim_is_running = sim_is_running;
            sim_is_running = 0; // turn off debug for a moment
            _long_fetch(page, address, &value);
            sim_is_running = _sim_is_running;
            sim_debug(DBG_CPU, &cpu_dev, "LIT 0x%X %s\n", value, INVERT);
        }
        else
        {
            sim_debug(DBG_CPU, &cpu_dev, "%sLIT\n", PREFIX);
        }
    }
    break;

    case OP_LIT_OVER_ALU:
    {
        t_value value;
        _long_fetch(page, address, &value);
        sim_debug(DBG_CPU, &cpu_dev, "%sLIT 0x%X OVER %s\n", PREFIX, value, ALU);
    }
    break;

    case OP_LIT_SWAP:
    {
        t_value value;
        _long_fetch(page, address, &value);
        sim_debug(DBG_CPU, &cpu_dev, "%sLIT %d SWAP %s\n", PREFIX, value, INVERT);
    }
    break;

    case OP_LIT_SWAP_ALU:
    {
        t_value value;
        t_bool _sim_is_running = sim_is_running;
        sim_is_running = 0; // turn off debug for a moment
        _long_fetch(page, address, &value);
        sim_is_running = _sim_is_running;
        sim_debug(DBG_CPU, &cpu_dev, "%sLIT %d %s\n", PREFIX, value, SWAP_ALU);
    }
    break;

    case OP_NEXT:
        sim_debug(DBG_CPU, &cpu_dev, "NEXT %d:0x%X ", page, target_addr(instruction, address));
        break;

    case OP_NIP:
        sim_debug(DBG_CPU, &cpu_dev, "NIP %s%s\n", INVERT, SHIFT);
        break;

    case OP_NIP_DUP_FETCH_SWAP:
        sim_debug(DBG_CPU, &cpu_dev, "%sNIP DUP %s@ SWAP\n", PREFIX, MEM);
        break;

    case OP_NIP_FETCH_LIT:
        sim_debug(DBG_CPU, &cpu_dev, "%sNIP %s@ 0x%X\n", PREFIX, MEM, SHORT_LIT);
        break;

    case OP_NIP_FETCH_WITH_ALU:
        sim_debug(DBG_CPU, &cpu_dev, "%sNIP 0x%X %s@_%s\n", PREFIX, SHORT_LIT, MEM, ALU3);
        break;

    case OP_OVER:
        sim_debug(DBG_CPU, &cpu_dev, "OVER %s%s\n", INVERT, SHIFT);
        break;

    case OP_SHIFT:
        if ((instruction & 0x0FFF) == 0)
        {
            sim_debug(DBG_CPU, &cpu_dev, "NOP\n");
        }
        else
        {
            sim_debug(DBG_CPU, &cpu_dev, "%s%s\n", INVERT, SHIFT);
        }
        break;

    case OP_SHORT_LIT:
        sim_debug(DBG_CPU, &cpu_dev, "0x%X %s\n", SHORT_LIT, INVERT);
        break;

    case OP_SHORT_LIT_SWAP_ALU:
        sim_debug(DBG_CPU, &cpu_dev, "0x%X %s\n", SHORT_LIT, SWAP_ALU);
        break;

    case OP_STORE:
        sim_debug(DBG_CPU, &cpu_dev, "%s%s! %s\n", PREFIX, MEM, INVERT);
        break;

    case OP_STORE_LIT:
        sim_debug(DBG_CPU, &cpu_dev, "%s%s! 0x%X\n", PREFIX, MEM, SHORT_LIT);
        break;

    case OP_SWAP:
        sim_debug(DBG_CPU, &cpu_dev, "SWAP %s%s\n", INVERT, SHIFT);
        break;

    case OP_TUCK_ALU:
        sim_debug(DBG_CPU, &cpu_dev, "TUCK %s%s\n", ALU, SHIFT);
        break;

    case OP_TUCK_STORE:
        sim_debug(DBG_CPU, &cpu_dev, "%sTUCK %s!\n", PREFIX, MEM);
        break;

    case OP_TUCK_STORE_WITH_ALU:
        sim_debug(DBG_CPU, &cpu_dev, "%sTUCK %s! 0x%X %s\n", PREFIX, MEM, SHORT_LIT, ALU3);
        break;

    case OP_UFETCH:
        sim_debug(DBG_CPU, &cpu_dev, "%s0x%X U@ %s\n", PREFIX, SHORT_LIT, INVERT);
        break;

    case OP_UFETCH_OVER_ALU:
        sim_debug(DBG_CPU, &cpu_dev, "%s0x%X U@ OVER %s\n", PREFIX, SHORT_LIT, ALU);
        break;

    case OP_UFETCH_SWAP:
        sim_debug(DBG_CPU, &cpu_dev, "%s0x%X U@ SWAP %s\n", PREFIX, SHORT_LIT, INVERT);
        break;

    case OP_UFETCH_SWAP_ALU:
        sim_debug(DBG_CPU, &cpu_dev, "%s0x%X U@ %s", PREFIX, SHORT_LIT, SWAP_ALU);
        break;

    case OP_UNDER_STORE:
        sim_debug(DBG_CPU, &cpu_dev, "%sUNDER %s! %s\n", PREFIX, MEM, INVERT);
        break;

    case OP_UNDER_STORE_LIT:
        sim_debug(DBG_CPU, &cpu_dev, "%sUNDER %s! 0x%X\n", PREFIX, MEM, SHORT_LIT);
        break;

    case OP_USLASH_ONE_TICK:
        sim_debug(DBG_CPU, &cpu_dev, "U/1'\n");
        break;

    case OP_USLASH_TICK:
        sim_debug(DBG_CPU, &cpu_dev, "U/'\n");
        break;

    case OP_USLASH_TICK_TICK:
        sim_debug(DBG_CPU, &cpu_dev, "U/''\n");
        break;

    default:
        bad_insn();
    }
}

void init_dispatch_vectors()
{
#if DUMMY_LAST >= NUMBER_OF_ROUTINES
#error : Increase NUMBER_OF_ROUTINES array initialization !!
#endif
    dispatch_vector_1[OP_CALL] = do_call;
    dispatch_vector_1[OP_0BRANCH] = do_0branch;
    dispatch_vector_1[OP_ALU] = do_alu;
#if 0
    dispatch_vector_1[OP_ASIC_STREAM_MAC] = do_asic_stream_mac;
#endif
    dispatch_vector_1[OP_BRANCH] = do_branch;
#if 0
    dispatch_vector_1[OP_BS_ONE_TICK] = do_bs_one_tick;
    dispatch_vector_1[OP_BS_TICK] = do_bs_tick;
    dispatch_vector_1[OP_BUSLASH_TICK] = do_buslash_tick;
    dispatch_vector_1[OP_BUSTAR_TICK] = do_bustar_tick;
    dispatch_vector_1[OP_BUSTAR_TICK_TICK] = do_bustar_tick_tick;
    dispatch_vector_1[OP_CLEAR_ACC] = do_clear_acc;
    dispatch_vector_1[OP_CLEAR_SOFTINT] = do_clear_softint;
    dispatch_vector_1[OP_C_TICK] = do_c_tick;
#endif
    dispatch_vector_1[OP_DDUP_ALU] = do_ddup_alu;
#if 0
    dispatch_vector_1[OP_DDUP_STORE] = do_ddup_store;
#endif
    dispatch_vector_1[OP_DDUP_STORE_WITH_ALU] = do_ddup_store_with_alu;
#if 0
    dispatch_vector_1[OP_DEC_RX] = do_dec_rx;
#endif
    dispatch_vector_1[OP_DROP] = do_drop;
#if 0
    dispatch_vector_1[OP_DROP_DUP] = do_drop_dup;
    dispatch_vector_1[OP_DROP_LIT] = do_drop_lit;
    dispatch_vector_1[OP_DROP_SHORT_LIT] = do_drop_short_lit;
    dispatch_vector_1[OP_DSLL] = do_dsll;
    dispatch_vector_1[OP_DSRA] = do_dsra;
    dispatch_vector_1[OP_DSRL] = do_dsrl;
#endif
    dispatch_vector_1[OP_DUP] = do_dup;
#if 0
    dispatch_vector_1[OP_DUP_FETCH_SWAP] = do_dup_fetch_swap;
    dispatch_vector_1[OP_DUP_GSTORE] = do_dup_gstore;
    dispatch_vector_1[OP_DUP_USTORE] = do_dup_ustore;
#endif
    dispatch_vector_1[OP_FETCH] = do_fetch;
#if 0
    dispatch_vector_1[OP_FETCH_LIT] = do_fetch_lit;
#endif
    dispatch_vector_1[OP_FETCH_OVER_ALU] = do_fetch_over_alu;
#if 0
    dispatch_vector_1[OP_FETCH_SWAP] = do_fetch_swap;
#endif
    dispatch_vector_1[OP_FETCH_SWAP_ALU] = do_fetch_swap_alu;
    dispatch_vector_1[OP_FETCH_WITH_ALU] = do_fetch_with_alu;
    dispatch_vector_1[OP_GFETCH] = do_gfetch;
    dispatch_vector_1[OP_GFETCH_DROP] = do_gfetch_drop;
#if 0
    dispatch_vector_1[OP_GFETCH_OVER_ALU] = do_gfetch_over_alu;
#endif
    dispatch_vector_1[OP_GFETCH_SWAP_ALU] = do_gfetch_swap_alu;
    dispatch_vector_1[OP_GSTORE] = do_gstore;
#if 0
    dispatch_vector_1[OP_INC_RX] = do_inc_rx;
#endif
    dispatch_vector_1[OP_LIT] = do_lit;
#if 0
    dispatch_vector_1[OP_LIT_OVER_ALU] = do_lit_over_alu;
    dispatch_vector_1[OP_LIT_SWAP] = do_lit_swap;
#endif
    dispatch_vector_1[OP_LIT_SWAP_ALU] = do_lit_swap_alu;
#if 0
    dispatch_vector_1[OP_MAC] = do_mac;
    dispatch_vector_1[OP_MIXED_MAC] = do_mixed_mac;
    dispatch_vector_1[OP_MIXED_MULT] = do_mixed_mult;
    dispatch_vector_1[OP_MULT] = do_mult;
    dispatch_vector_1[OP_MULT_SUB] = do_mult_sub;
#endif
    dispatch_vector_1[OP_NEXT] = do_next;
    dispatch_vector_1[OP_NIP] = do_nip;
#if 0
    dispatch_vector_1[OP_NIP_DUP] = do_nip_dup;
    dispatch_vector_1[OP_NIP_DUP_FETCH_SWAP] = do_nip_dup_fetch_swap;
    dispatch_vector_1[OP_NIP_FETCH_LIT] = do_nip_fetch_lit;
    dispatch_vector_1[OP_NIP_FETCH_WITH_ALU] = do_nip_fetch_with_alu;
    dispatch_vector_1[OP_NORMALIZE] = do_normalize;
#endif
    dispatch_vector_1[OP_OVER] = do_over;
    dispatch_vector_1[OP_QDUP_0BRANCH] = do_qdup_0branch;
#if 0
    dispatch_vector_1[OP_RDR] = do_rdr;
    dispatch_vector_1[OP_RESERVED] = do_reserved;
    dispatch_vector_1[OP_RESERVED_STEP_MATH] = do_reserved_step_math;
    dispatch_vector_1[OP_RTR] = do_rtr;
    dispatch_vector_1[OP_R_TICK] = do_r_tick;
    dispatch_vector_1[OP_SELECT_CPR] = do_select_cpr;
    dispatch_vector_1[OP_SELECT_DPR] = do_select_dpr;
    dispatch_vector_1[OP_SET_SOFTINT] = do_set_softint;
#endif
    dispatch_vector_1[OP_SHIFT] = do_shift;
#if 0
    dispatch_vector_1[OP_SHIFT_MAC_RIGHT] = do_shift_mac_right;
#endif
    dispatch_vector_1[OP_SHORT_LIT] = do_short_lit;
#if 0
    dispatch_vector_1[OP_SHORT_LIT_OVER_ALU] = do_short_lit_over_alu;
#endif
    dispatch_vector_1[OP_SHORT_LIT_SWAP_ALU] = do_short_lit_swap_alu;
#if 0
    dispatch_vector_1[OP_STAR_TICK] = do_star_tick;
    dispatch_vector_1[OP_STAR_TICK_TICK] = do_star_tick_tick;
#endif
    dispatch_vector_1[OP_STORE] = do_store;
    dispatch_vector_1[OP_STORE_LIT] = do_store_lit;
#if 0
    dispatch_vector_1[OP_STREAM_MAC] = do_stream_mac;
#endif
    dispatch_vector_1[OP_SWAP] = do_swap;
    dispatch_vector_1[OP_SWAP] = do_swap;
#if 0
    dispatch_vector_1[OP_S_ONE_TICK] = do_s_one_tick;
    dispatch_vector_1[OP_S_TICK] = do_s_tick;
    dispatch_vector_1[OP_S_TICK_TICK] = do_s_tick_tick;
#endif
    dispatch_vector_1[OP_TUCK_ALU] = do_tuck_alu;
#if 0
    dispatch_vector_1[OP_TUCK_STORE] = do_tuck_store;
#endif
    dispatch_vector_1[OP_TUCK_STORE_WITH_ALU] = do_tuck_store_with_alu;
#if 0
    dispatch_vector_1[OP_TWO_STAR_TICK] = do_two_star_tick;
    dispatch_vector_1[OP_UFETCH] = do_ufetch;
    dispatch_vector_1[OP_UFETCH_OVER_ALU] = do_ufetch_over_alu;
    dispatch_vector_1[OP_UFETCH_SWAP] = do_ufetch_swap;
    dispatch_vector_1[OP_UFETCH_SWAP_ALU] = do_ufetch_swap_alu;
    dispatch_vector_1[OP_UMAC] = do_umac;
    dispatch_vector_1[OP_UMULT] = do_umult;
    dispatch_vector_1[OP_UNDER_ALU] = do_under_alu;
    dispatch_vector_1[OP_UNDER_STORE] = do_under_store;
    dispatch_vector_1[OP_UNDER_STORE_LIT] = do_under_store_lit;
#endif
    dispatch_vector_1[OP_USLASH_ONE_TICK] = do_uslash_one_tick;
#if 0
    dispatch_vector_1[OP_USLASH_ONE_TICK_TICK] = do_uslash_one_tick_tick;
#endif
    dispatch_vector_1[OP_USLASH_TICK] = do_uslash_tick;
    dispatch_vector_1[OP_USLASH_TICK_TICK] = do_uslash_tick_tick;
#if 0
    dispatch_vector_1[OP_USTAR_TICK] = do_ustar_tick;
    dispatch_vector_1[OP_USTAR_TICK_TICK] = do_ustar_tick_tick;
    dispatch_vector_1[OP_USTORE] = do_ustore;
    dispatch_vector_1[OP_ZERO_EQUAL] = do_zero_equal;
#endif

/* ---------------- second clock routines */
#if 0
    dispatch_vector_2[OP_DDUP_STORE] = do_ddup_store_2;
#endif
    dispatch_vector_2[OP_DDUP_STORE_WITH_ALU] = do_ddup_store_with_alu_2;
#if 0
    dispatch_vector_2[OP_DROP_LIT] = do_drop_lit_2;
    dispatch_vector_2[OP_DUP_FETCH_SWAP] = do_dup_fetch_swap_2;
    dispatch_vector_2[OP_DUP_USTORE] = do_dup_ustore_2;
#endif
    dispatch_vector_2[OP_FETCH] = do_fetch_2;
#if 0
    dispatch_vector_2[OP_FETCH_LIT] = do_fetch_lit_2;
#endif
    dispatch_vector_2[OP_FETCH_OVER_ALU] = do_fetch_over_alu_2;
    dispatch_vector_2[OP_FETCH_SWAP] = do_fetch_swap_2;
    dispatch_vector_2[OP_FETCH_SWAP_ALU] = do_fetch_swap_alu_2;
    dispatch_vector_2[OP_FETCH_WITH_ALU] = do_fetch_with_alu_2;
    dispatch_vector_2[OP_LIT] = do_lit_2;
#if 0
    dispatch_vector_2[OP_LIT_OVER_ALU] = do_lit_over_alu_2;
    dispatch_vector_2[OP_LIT_SWAP] = do_lit_swap_2;
#endif
    dispatch_vector_2[OP_LIT_SWAP_ALU] = do_lit_swap_alu_2;
#if 0
    dispatch_vector_2[OP_NIP_DUP_FETCH_SWAP] = do_nip_dup_fetch_swap_2;
    dispatch_vector_2[OP_NIP_FETCH_LIT] = do_nip_fetch_lit_2;
    dispatch_vector_2[OP_NIP_FETCH_WITH_ALU] = do_nip_fetch_with_alu_2;
#endif
    dispatch_vector_2[OP_STORE] = do_store_2;
    dispatch_vector_2[OP_STORE_LIT] = do_store_lit_2;
#if 0
    dispatch_vector_2[OP_TUCK_STORE] = do_tuck_store_2;
#endif
    dispatch_vector_2[OP_TUCK_STORE_WITH_ALU] = do_tuck_store_with_alu_2;
#if 0
    dispatch_vector_2[OP_UFETCH] = do_ufetch_2;
    dispatch_vector_2[OP_UFETCH_OVER_ALU] = do_ufetch_over_alu_2;
    dispatch_vector_2[OP_UFETCH_SWAP] = do_ufetch_swap_2;
    dispatch_vector_2[OP_UFETCH_SWAP_ALU] = do_ufetch_swap_alu_2;
    dispatch_vector_2[OP_UNDER_STORE] = do_under_store_2;
    dispatch_vector_2[OP_UNDER_STORE_LIT] = do_under_store_lit_2;
    dispatch_vector_2[OP_USTORE] = do_ustore_2;
#endif
}
