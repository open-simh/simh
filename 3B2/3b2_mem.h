/* 3b2_mem.h: Memory Map Access Routines

   Copyright (c) 2021-2022, Seth J. Morabito

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.

   Except as contained in this notice, the name of the author shall
   not be used in advertising or otherwise to promote the sale, use or
   other dealings in this Software without prior written authorization
   from the author.
*/

#ifndef _3B2_MEM_H_
#define _3B2_MEM_H_

#include "3b2_defs.h"

#define IS_ROM(PA) ((PA) < ROM_SIZE)
#define IS_RAM(PA) (((PA) >= PHYS_MEM_BASE) && ((PA) < (PHYS_MEM_BASE + MEM_SIZE)))
#if defined(REV3)
#define IS_IO(PA) (((PA >= IO_BOTTOM) && (PA < IO_TOP)) ||           \
                ((PA >= CIO_BOTTOM) && (PA < CIO_TOP)) ||         \
                ((PA >= VCACHE_BOTTOM) && (PA < VCACHE_TOP)) ||   \
                ((PA >= BUB_BOTTOM) && (PA < BUB_TOP)))
#else
#define IS_IO(PA) (((PA >= IO_BOTTOM) && (PA < IO_TOP)) || \
                ((PA >= CIO_BOTTOM) && (PA < CIO_TOP)))
#endif

#define MA_BUB3    0x100      /* BUBUS slot 3 master on fault */
#define MA_BUB2    0x200      /* BUBUS slot 2 master on fault */
#define MA_BUB1    0x400      /* BUBUS slot 1 master on fault */
#define MA_CPU_BU  0x2000     /* CPU access BUBUS peripheral */
#define MA_BUB0    0x4000     /* BUBUS slot 0 master on fault */
#define MA_CPU_IO  0x8000     /* CPU accessing I/O peripheral */
#define MA_IO_NLY  0x10000    /* IO Bus Master on fault */
#define MA_IO_BM   0x80000    /* IO Bus Master or BUBUS was master on fault */

#define BUS_PER    0          /* Read or Write is from peripheral */
#define BUS_CPU    1          /* Read or Write is from CPU */

uint32_t pread_w(uint32_t pa, uint8_t src);
void   pwrite_w(uint32_t pa, uint32_t val, uint8_t src);
uint8_t  pread_b(uint32_t pa, uint8_t src);
void   pwrite_b(uint32_t pa, uint8_t val, uint8_t src);
void   pwrite_b_rom(uint32_t pa, uint8_t val);
uint16_t pread_h(uint32_t pa, uint8_t src);
void   pwrite_h(uint32_t pa, uint16_t val, uint8_t src);

uint8_t  read_b(uint32_t va, uint8_t r_acc, uint8_t src);
uint16_t read_h(uint32_t va, uint8_t r_acc, uint8_t src);
uint32_t read_w(uint32_t va, uint8_t r_acc, uint8_t src);
void   write_b(uint32_t va, uint8_t val, uint8_t src);
void   write_h(uint32_t va, uint16_t val, uint8_t src);
void   write_w(uint32_t va, uint32_t val, uint8_t src);

t_stat read_operand(uint32_t va, uint8_t *val);
t_stat examine(uint32_t va, uint8_t *val);
t_stat deposit(uint32_t va, uint8_t val);

#endif /* _3B2_MEM_H_ */
