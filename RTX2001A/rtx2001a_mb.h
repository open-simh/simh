/* rtx2001a_mb.h: RTX2001A CPU simulator

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

    6-Sep-22   SYS      From RTX_2000 simulator by Phil Koopman Jr.

    Memory Bus support
*/
#ifndef RTX2001A_MB_H_
#define RTX2001A_MB_H_ 0

// #include "rtx2001a_defs.h"
#include "rtx2001a_debug.h"
#include "rtx2001a_registers.h"

// See rtx2000_simulator/STATE.H:208
#define RAM_SIZE 0x10000 /* in words */
#define RAM_BYTES 0x100000
#define RAM_ORG 0x0000

// extract the in-line 16-bit lit
#define LIT _long_fetch(page, address, value)

extern RTX_WORD ram[RAM_SIZE];

extern void _read(t_addr, t_value *);
extern void _write(t_addr, t_value);
extern void _long_fetch(t_addr, t_addr, t_value *);
extern void _long_store(t_addr, t_addr, t_value);
extern void byte_fetch(t_addr, t_value *);
extern void byte_store(t_addr, t_value);
extern void fetch(t_addr, t_value *);
extern void store(t_addr, t_value);
extern void ufetch(t_addr, t_value *);
extern void ustore(t_addr, t_value);

#define DATA_PAGE (DPRSEL ? DPR : CPR) /* correct page for access */

#define SEGADDR(prefix) \
    sim_is_running ? prefix "seg:addr=%d:0x%X addr=0x%X\n" : ""

#define LEA(seg, address, addr) \
    addr = ((seg) ? (((seg << 16) & 0xF0000) | (address & D16_MASK)) : (address & D16_MASK)) - RAM_ORG;

#define long_fetch(segment, address, value) _long_fetch(segment, address, value)

#define long_store(segment, address, data) _long_store(segment, address, data)

#define dp_store(address, data) _long_store(dpr.pr, address, data)

#define dp_fetch(address, value) _long_fetch(dpr.pr, address, value)

#define cp_store(address, data) _long_store(cpr.pr, address, data)

#define cp_fetch(address, value) _long_fetch(cpr.pr, address, value)

#endif