/* rtx2001a_debug.h: RTX2001A CPU simulator

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

    6-Sep-22   SYS      Initial version
*/
#ifndef RTX2001A_DEBUG_H_
#define RTX2001A_DEBUG_H_ 0

#include "rtx2001a_defs.h"

/* Debug */
#define DBG_CPU SWMASK('E')
#define DBG_PSB_R SWMASK('Y') // parameter stack bus read
#define DBG_PSB_W SWMASK('X') // parameter stack bus write
#define DBG_RSB_R SWMASK('V') // return stack bus read
#define DBG_RSB_W SWMASK('U') // return stack bus write
#define DBG_MEB_R SWMASK('S') // memory bus read
#define DBG_MEB_W SWMASK('R') // memory bus write
#define DBG_ASB_R SWMASK('P') // ASIC bus read
#define DBG_ASB_W SWMASK('O') // ASIC bus write

#define STOP_HALT 1
#define STOP_IBKPT 2
#define STOP_ACCESS 3

static BRKTYPTAB cpu_breakpoints[] = {
    BRKTYPE('E', "CPU breakpoint"),
    {DBG_PSB_R, "Read from parameter stack bus"},
    {DBG_PSB_W, "Write to parameter stack bus"},
    {DBG_RSB_R, "Read from return stack bus"},
    {DBG_RSB_W, "Write to return stack bus"},
    {DBG_MEB_R, "Read from memory bus"},
    {DBG_MEB_W, "Write to memory bus"},
    {DBG_ASB_R, "Read from ASIC bus"},
    {DBG_ASB_W, "Write to ASIC bus"},
    {0}};

static DEBTAB cpu_deb[] = {
    {"CPU", DBG_CPU, "CPU"},
    {"MEBR", DBG_MEB_R, "Read from Memory bus"},
    {"MEBW", DBG_MEB_W, "Write to Memory bus"},
    {"ASBR", DBG_ASB_R, "Read from ASIC bus"},
    {"ASBW", DBG_ASB_W, "Write to ASIC bus"},
    {"PSBR", DBG_PSB_R, "Read from Parameter Stack bus"},
    {"PSBW", DBG_PSB_W, "Write to Parameter Stack bus"},
    {"RSBR", DBG_RSB_R, "Read from Return Stack bus"},
    {"RSBW", DBG_RSB_W, "Write to Return Stack bus"},
    {0}};

#define DEBUG_SET(s, b)                                                                     \
    {                                                                                       \
        struct                                                                              \
        {                                                                                   \
            RTX_WORD o;                                                                     \
        } o;                                                                                \
        o.o = s;                                                                            \
        s = (value & D16_MASK);                                                             \
        *val = s;                                                                           \
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, (&cpu_reg[++cell])->name, b, o.o, *val, 1); \
    }

// #define DEBUG_LOG(mask, verb, name, value)                            \
//     if (sim_brk_summ & mask)                                          \
//     {                                                                 \
//         sim_debug(verb, &cpu_dev, "%s: 0x%X\n", (char *)name, value); \
//     }

#endif