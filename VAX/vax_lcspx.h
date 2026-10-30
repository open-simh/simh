/* vax_lcspx.h: DEC LCSPX graphics option

   Copyright (c) 2026, Douglas P. Fields, Jr.

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
   THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of the author shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from the author.

   The LCSPX occupies a 32 MB window of the system's I/O space; the model
   that carries it defines LCSPX_BASE (the VAXstation 4000 Model 90:
   3800.0000) and routes every reference in the window to lcspx_rd and
   lcspx_wr.  The offsets below are from the base. */

#ifndef VAX_LCSPX_H_
#define VAX_LCSPX_H_    0

#include "sim_defs.h"

#define LCSPX_WINSIZE   0x02000000                      /* the whole window */

#define LCSPX_FB        0x00000000                      /* frame buffer, one byte per pixel */
#define LCSPX_FBSIZE    0x00400000                      /* 4 MB: "8 Plane 4Mpixel FB" */
#define LCSPX_CMD       0x01000000                      /* command windows into the TBC FIFOs */
#define LCSPX_CMDSTEP   0x00400000                      /* one per FIFO */
#define LCSPX_CMDSIZE   0x00300000
#define LCSPX_SP        0x01300000                      /* scan processor (pixel engine) */
#define LCSPX_SPSIZE    0x00100000                      /* 512 KB, and a diagnostic view */
#define LCSPX_SP2       0x01700000                      /* the scan processor again */
#define LCSPX_TBC       0x01B00000                      /* TBC: timing and bus controller */
#define LCSPX_TBCSIZE   0x00000400
#define LCSPX_BT        0x01B10000                      /* Bt459 RAMDAC: four ports */
#define LCSPX_FIFO      0x01B20000                      /* TBC FIFO memory */
#define LCSPX_FIFOSIZE  0x00020000
#define LCSPX_BTSIZE    0x00010000
#define LCSPX_ROM       0x01BC0000                      /* option ROM */
#define LCSPX_ROMSIZE   0x00040000                      /* 16 bits wide, longword stride */

extern DEVICE lcspx_dev;

int32 lcspx_rd (int32 pa);
void lcspx_wr (int32 pa, int32 val, int32 lnt);
t_bool lcspx_present (void);

#endif
