/* rtx2001a_rsb.c: RTX2001A CPU simulator

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

    9-Oct-22   SYS      Initial version

    Parameter Stack Bus support
*/
#include "rtx2001a_defs.h"
#include "rtx2001a_debug.h"
#include "rtx2001a_registers.h"
#include "rtx2001a_rsb.h"
#include "rtx2001a_ab.h"

/**
** return stack push with 2 inputs
** @TODO: Implement stack over/under flow
*/
void rs_push(t_addr page, t_value data)
{
    ReturnStack ors;
    SPR ospr;
    ors.pr = rs[(spr.fields.rsp)].pr;
    ospr.pr = spr.pr;

    (spr.fields.rsp) = (++(spr.fields.rsp)) & STACK_MASK;
    sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "SPR", spr_bits, ospr.pr, spr.pr, 1);

    rs[(spr.fields.rsp)].fields.ipr = ipr.fields.pr;
    rs[(spr.fields.rsp)].fields.i = asic_file[I];
    // sim_debug_bits_hdr(DBG_RSB_W, &cpu_dev, "RSB", rs_bits, ors.pr, rs[(spr.fields.rsp)].pr, 1);

    asic_file[I] = data;
    set_IPR(page);
    sim_debug(DBG_ASB_W, &cpu_dev, "rs[%d]=%d:0x%X (%d:0x%X)\n", spr.fields.rsp, ipr.fields.pr, asic_file[I], rs[(spr.fields.rsp)].fields.ipr, rs[(spr.fields.rsp)].fields.i);
    // IDK why this is here. See STATE.C:145
    // if (DPRSEL)
    // {
    //     IPR oipr;
    //     oipr.pr = ipr.pr;
    //     ipr.pr &= 0x10;
    //     sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "IPR", ipr_bits, oipr.pr, ipr.pr, 1);
    // }
}

/* return stack pop, return element popped */
void rs_pop()
{
    ReturnStack orsp;
    SPR ospr;
    orsp.pr = rs[(spr.fields.rsp)].pr;
    ospr.pr = spr.pr;

    sim_debug_bits_hdr(DBG_RSB_R, &cpu_dev, "RSB", rs_bits, orsp.pr, rs[(spr.fields.rsp)].pr, 1);

    /* don't use set_IPR; bit 4 comes from RS */
    ipr.fields.pr = rs[(spr.fields.rsp)].fields.ipr & 0x1F;
    asic_file[I] = rs[(spr.fields.rsp)].fields.i & D16_MASK;
    sim_debug(DBG_ASB_W, &cpu_dev, "I=0x%X IPR=%d\n", asic_file[I], ipr.fields.pr);

    (spr.fields.rsp) = (--(spr.fields.rsp)) & STACK_MASK;
    sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "SPR", spr_bits, ospr.pr, spr.pr, 1);
}
