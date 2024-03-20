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
    SPR ospr;
    ospr.pr = spr.pr;

    (spr.fields.rsp) = (++(spr.fields.rsp)) & STACK_MASK;
    sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "SPR", spr_bits, ospr.pr, spr.pr, 1);

    rs[(spr.fields.rsp)].fields.ipr = ipr.fields.pr;
    rs[(spr.fields.rsp)].fields.i = asic_file[I];

    sim_debug(DBG_RSB_W, &cpu_dev, "RS[%d]=%d:0x%04X\n", spr.fields.rsp, ipr.fields.pr, asic_file[I]);

    asic_file[I] = data;
    sim_debug(DBG_ASB_W, &cpu_dev, "I=0x%04X\n", asic_file[I]);
    set_IPR(page);
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

    /* don't use set_IPR; bit 4 comes from RS */
    sim_debug(DBG_RSB_R, &cpu_dev, "RS[%d]=%d:0x%04X\n", spr.fields.rsp, rs[spr.fields.rsp].fields.ipr, rs[spr.fields.rsp].fields.i);
    ipr.fields.pr = rs[(spr.fields.rsp)].fields.ipr;
    asic_file[I] = rs[(spr.fields.rsp)].fields.i;
    sim_debug(DBG_ASB_W, &cpu_dev, "IPR=%d I=0x%04X\n", ipr.fields.pr, asic_file[I]);

    (spr.fields.rsp) = (--(spr.fields.rsp)) & STACK_MASK;
    sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "SPR", spr_bits, ospr.pr, spr.pr, 1);
}
