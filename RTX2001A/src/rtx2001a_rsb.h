/* rtx2001a_rsb.h: RTX2001A CPU simulator

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

    6-Sep-22   SYS      Initial version. See also SIM2000/STATE.H:109

    Return Stack Bus

*/
#ifndef RTX2001A_RSB_H_
#define RTX2001A_RSB_H_ 0

#include "hp3k_defs.h"

/*
**
** Stacks on the RTX 2000 and RTX 2010 are each 256 elements deep;
**     stacks on the RTX 2001A are 64 elements deep. See SIM2000/STATE.H:38
*/
struct ReturnStackFields
{
    RTX_WORD ipr : 6;   // 0-5: Current value of IPR
    RTX_WORD i : 15;    // 6-21: Current value of I
};

union Return_Stack
{
    struct ReturnStackFields fields;
    RTX_WORD pr;
};

typedef union Return_Stack ReturnStack;

static BITFIELD rs_bits[] = { // Fig 11, pg 10
    BITF(IPR, 6),             // 0-5: Current value of IPR
    BITF(I, 15),              // 6-21: Current value of I
    ENDBITS};

#define RS_MAX 0x40

extern ReturnStack rs[RS_MAX + 1];

extern void rs_push(t_addr, t_value);
extern void rs_pop();
#endif