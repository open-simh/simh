/* rtx2001a_mb.c: RTX2001A CPU simulator

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

    6-Sep-22   SYS      From RTX_2000 simulator by Phil Koopman Jr.

    Memory Bus support
*/
#include "rtx2001a_defs.h"
#include "rtx2001a_debug.h"
#include "rtx2001a_registers.h"
#include "rtx2001a_mb.h"

RTX_WORD ram[RAM_SIZE] = {0};

void _read(t_addr addr, t_value *value)
{
    *value = ram[addr] & D16_MASK;
    if (sim_is_running)
    {
        sim_debug(DBG_MEB_R, &cpu_dev, "0x%X 0x%X\n", addr, *value);
    }
}

void _write(t_addr addr, t_value value)
{
    ram[addr] = value;
    sim_debug(DBG_MEB_W, &cpu_dev, "0x%X 0x%X\n", addr, value);
}

// See rtx2000_simulator/STATE.C:208
void _long_fetch(t_addr seg, t_addr address, t_value *value)
{
    t_addr addr = 0;
    LEA(seg, address, addr);
    if ((addr >= 0) && (addr < RAM_BYTES))
    {
        _read(addr >> 1, value);
        return;
    }

    sim_debug(DBG_MEB_R, &cpu_dev, "nxm: PC=0x%X seg:addr=%d:0x%X addr=0x%X\n", asic_file[PC], seg, address, addr);
    ABORT(SCPE_NXM);
}

// See rtx2000_simulator/STATE.C:195
void _long_store(t_addr seg, t_addr address, t_value data)
{
    t_addr addr = 0;
    LEA(seg, address, addr);
    if ((addr >= 0) && (addr < RAM_BYTES))
    {
        _write(addr >> 1, data & D16_MASK);
        return;
    }

    sim_debug(DBG_MEB_W, &cpu_dev, "nxm: PC=0x%X seg:addr=%d:0x%X addr=0x%X\n", asic_file[PC], seg, address, addr);
    ABORT(SCPE_NXM);
}

// See rtx2000_simulator/STATE.C:234
void byte_fetch(t_addr address, t_value *value)
{
    if ((address & 1) ^ BO)
    {
        dp_fetch(address, value);
        *value &= 0x00FF;
    }
    else
    {
        dp_fetch(address, value);
        *value >>= 8;
        *value &= 0x00FF;
    }
}

// See rtx2000_simulator/STATE.C:225
void byte_store(t_addr address, t_value data)
{
    uint32 temp = 0;
    union
    {
        struct
        {
            uint8 a;
            uint8 b;
            uint16 mbz;
        };
        RTX_WORD c;
    } value;
    value.mbz = 0;

    // Watch out for doubled wait states!!!
    dp_fetch(address, &temp);

    if ((address & 1) ^ BO)
    {
        value.a = data & 0x00FF;
        value.b = temp >> 8;
    }
    else
    {
        value.a = temp & 0x00FF;
        value.b = data & 0x00FF;
        // RTX_WORD value = ((data << 8) & 0xFF00) | (temp & 0xFF);
        // dp_store(address, value);
    }
    dp_store(address, value.c);
}

/*
 * See rtx2000_simulator/STATE.C:240
 * Classes 14 and 15: Data Memory Access
 *   The instruction formats are identical for both word and byte access.
 *   The "s" bit (bit 12) of the instruction dictates the size of the operand
 *   (s =0 for 16-bit word, s = 1 for 8-bit byte).
 */
void fetch(t_addr address, t_value *value)
{
    uint32 tempa = 0;
    uint32 tempb = 0;

    // if ((sim_brk_summ & DBG_MEB_R) && sim_brk_test(address, DBG_MEB_R))
    // {
    //     // longjmp(bkpt_env, STOP_IBKPT);
    //     printf("!\n");
    // }
    if (IR & 0x1000)
    {
        byte_fetch(address, value);
    }
    else if ((address & 1) ^ BO)
    {
        dp_fetch(address, &tempa);
        tempb = (tempa >> 8) & 0xFF;
        tempa = (tempa << 8) & 0xFF00;
        *value = tempa | tempb;
    }
    else
    {
        dp_fetch(address, &tempa);
        *value = tempa & D16_MASK;
    }
}

/*
 * See rtx2000_simulator/STATE.C:255
 * Classes 14 and 15: Data Memory Access
 *   The instruction formats are identical for both word and byte access.
 *   The "s" bit (bit 12) of the instruction dictates the size of the operand
 *   (s =0 for 16-bit word, s = 1 for 8-bit byte).
 */
void store(t_addr address, t_value data)
{
    uint32 tempa = 0;
    uint32 tempb = 0;

    if ((sim_brk_summ & DBG_MEB_W) && sim_brk_test(address, DBG_MEB_W))
    {
        // longjmp(bkpt_env, STOP_IBKPT);
        printf("!\n");
    }

    if (IR & 0x1000)
    {
        byte_store(address, (data & D16_MASK));
    }
    else
    {
        if ((address & 1) ^ BO)
        {
            tempb = (data >> 8) & 0xFF;
            tempa = (data << 8) & 0xFF00;
            dp_store(address, (tempa | tempb));
        }
        else
        {
            dp_store(address, (data & D16_MASK));
        }
    }
}

void ustore(t_addr offset, t_value data)
{
    // See RTX 2001A Datasheet; Fig 13, pg 10
    // uint32 addr = (ubr.ubr & 0xFFFE) | ((offset << 1) & 0x003E);
    t_addr addr = ubr.fields.page | ((offset << 1) & 0x003E);
    sim_debug(DBG_ASB_R, &cpu_dev, "data=0x%X UBR=0x%X offset=0x%X addr=0x%X\n", data, ubr.pr, offset, addr);
    // sim_debug(DBG_ASB_R, &cpu_dev, "PC=0x%X UPR=0x%X addr=0x%X\n", asic_file[PC], upr.pr, addr);
    _write(addr, data);
}

void ufetch(t_addr offset, t_value *data)
{
    // See RTX 2001A Datasheet; Fig 13, pg 10
    t_addr addr = ubr.fields.page | ((offset << 1) & 0x003E);
    // sim_debug(DBG_ASB_R, &cpu_dev, "PC=0x%X UPR=0x%X addr=0x%X\n", asic_file[PC], upr.pr, addr);
    _read(addr, data);
    sim_debug(DBG_ASB_R, &cpu_dev, "data=0x%X UBR=0x%X offset=0x%X addr=0x%X\n", *data, ubr.pr, offset, addr);
}
