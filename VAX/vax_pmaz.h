/* vax_pmaz.h: PMAZ-AA SCSI option module in TURBOchannel slot 0 of a VAXstation 4000 Model 96

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

   The module's address space is reached from the TURBOchannel adapter's slot
   0 window (vax_tca.c); its SCSI chip is the second controller of
   vax4xx_rz94.c.  See vax_pmaz.c. */

#ifndef VAX_PMAZ_H_
#define VAX_PMAZ_H_     0

#include "sim_defs.h"

void pmaz_reset (void);
int32 pmaz_rd (uint32 off);
void pmaz_wr (uint32 off, int32 val, int32 lnt);
int32 pmaz_dma_read (uint32 bc, uint8 *buf);
int32 pmaz_dma_write (uint32 bc, uint8 *buf);
void pmaz_dma_done (uint32 n);

#endif
