/* vax_tca.h: TURBOchannel adapter (TCA) of the VAXstation 4000 Model 90 and 96


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

   The adapter answers in two places in I/O space, which the model that
   carries it defines (the VAXstation 4000 Model 90 family: TCA_BASE
   3600.0000 and TCA_CSR 3680.0000) and routes to the functions below.
   Only the part that the console's use fixes is modelled, see vax_tca.c. */

#ifndef VAX_TCA_H_
#define VAX_TCA_H_      0

#include "sim_defs.h"

#define TCA_WINSIZE     0x00100000                      /* the largest option ROM the
                                                           header's size byte can ask
                                                           for: 255 x 4 KB, rounded up */
#define TCA_CSRSIZE     0x00000004                      /* one longword */

extern DEVICE tca_dev;

int32 tca_rd (int32 pa);
void tca_wr (int32 pa, int32 val, int32 lnt);
int32 tca_csr_rd (int32 pa);
void tca_csr_wr (int32 pa, int32 val, int32 lnt);
t_bool tca_present (void);

#endif
