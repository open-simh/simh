/* rtx2001a_psb.c: RTX2001A CPU simulator

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
#include "rtx2001a_psb.h"
#include "rtx2001a_execute.h"

/**
 ** parameter stack pop
 */
void ps_pop()
{
    SPR ospr;
    ospr.pr = spr.pr;

    TOP = NEXT;
    NEXT = ps[(spr.fields.psp)];
    sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "SPR", spr_bits, spr.pr, spr.pr, 1);
    sim_debug(DBG_PSB_R, &cpu_dev, "PS[%d]=0x%X\n", spr.fields.psp, ps[spr.fields.psp] & D16_MASK);

    (spr.fields.psp) = (--(spr.fields.psp)) & STACK_MASK;
    sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "SPR", spr_bits, ospr.pr, spr.pr, 1);
}

/**
 ** parameter stack push
 */
void ps_push(t_value data)
{
    SPR ospr;
    ospr.pr = spr.pr;
    sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "SPR", spr_bits, spr.pr, spr.pr, 1);
    (spr.fields.psp) = (++(spr.fields.psp)) & STACK_MASK;
    sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "SPR", spr_bits, ospr.pr, spr.pr, 1);

    ps[(spr.fields.psp)] = NEXT;
    NEXT = TOP;
    TOP = data;
    sim_debug(DBG_PSB_W, &cpu_dev, "PS[%d]=0x%X\n", spr.fields.psp, ps[spr.fields.psp] & D16_MASK);
}