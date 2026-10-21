/* vax_sgec.h: DEC SGEC (DC541) Ethernet controller

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

   The model that uses the SGEC maps its sixteen CSRs (register n at
   base + 4n) to sgec_rd and sgec_wr, answers the interrupt acknowledge at
   the SGEC's level with sgec_vector (), and defines in its _defs.h:

   SGEC_SET_INT         the SGEC's interrupt request is asserted
   SGEC_CLR_INT         ... and removed
   SGEC_GET_MAC(m)      copy the station address (ETH_MAC) into m
   SGEC_SET_MAC(m)      set the station address from m

   The request is a level: it stays asserted while CSR5 has an interrupt
   status bit set and CSR6<IE> is set.  The DMA goes to physical memory
   (descriptors, buffers and page tables), checked with ADDR_IS_MEM.  The
   station address belongs to the board (its network address ROM), not to
   the SGEC, which learns it from the setup frames software gives it; the
   two MAC hooks let SET SGEC MAC= change the board's ROM and let the
   attach check the network for a duplicate.
*/

#ifndef VAX_SGEC_H_
#define VAX_SGEC_H_ 1

#define SGEC_NCSR       16                              /* CSRs */
#define SGEC_CSRSIZE    (SGEC_NCSR << 2)

extern DEVICE sgec_dev;

int32 sgec_rd (int32 pa);
void sgec_wr (int32 pa, int32 val, int32 lnt);
int32 sgec_vector (void);

#endif
