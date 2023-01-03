/* rtx2001a_ab.h: RTX2001A CPU simulator

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

    19-Sep-22   SYS      ASIC bus routines
*/
#ifndef RTX2001A_AB_H_
#define RTX2001A_AB_H_ 0

#include "rtx2001a_registers.h"
#include "rtx2001a_defs.h"
#include "rtx2001a_debug.h"
#include "rtx2001a_rsb.h"

#define GDATA_SIZE 0x20

extern RTX_WORD gdata[GDATA_SIZE];

/* extract the 5-bit short-lit/g-bus/user value */
#define SHORT_LIT (IR & 0x1F)

#define INC_PC                                                      \
    {                                                               \
        asic_file[PC] = (asic_file[PC] + 2) & D16_UMAX;             \
        sim_debug(DBG_ASB_W, &cpu_dev, "PC=0x%X\n", asic_file[PC]); \
    }

#define set_IPR(page)                                                                \
    {                                                                                \
        ipr.fields.pr = page;                                                        \
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "IBC", ibc_bits, ibc.pr, ibc.pr, 1); \
        if (DPRSEL)                                                                  \
        {                                                                            \
            ipr.pr |= 0x10;                                                          \
        }                                                                            \
        sim_debug(DBG_ASB_W, &cpu_dev, "IPR=%d\n", ipr.fields.pr);                   \
    }

#define set_DPRSEL(value)                                                             \
    {                                                                                 \
        IBC oibc;                                                                     \
        oibc.pr = ibc.pr;                                                             \
        ibc.fields.dprsel = (value) ? 1 : 0;                                          \
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "IBC", ibc_bits, oibc.pr, ibc.pr, 1); \
    }

extern t_stat gstore(t_addr, t_value);
extern t_stat gfetch(t_addr, t_value *);

#endif