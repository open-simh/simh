/* tt2500_defs.h: TT2500 simulator definitions

   Copyright (c) 2020, Lars Brinkhoff

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
   LARS BRINKHOFF BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Lars Brinkhoff shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Lars Brinkhoff.

   24-Sep-20    LB      New simulator.
*/

#ifndef TT2500_DEFS_H_
#define TT2500_DEFS_H_  0

#include "sim_defs.h"

#define STOP_HALT       1
#define STOP_IBKPT      2
#define STOP_ACCESS     3

#define FLAG_KB      001
#define FLAG_RSD     002
#define INT_2KHZ     (001 << 4)
#define INT_RRD      (002 << 4)
#define INT_60HZ     (004 << 4)
#define STAR_WRAP    (001 << 8)
#define STAR_MINUS1  (002 << 8)

/* ALU operations. */
#define ALU_A     00000
#define ALU_ANDN  00020
#define ALU_AND   00040
#define ALU_NOR   00060
#define ALU_IOR   02000
#define ALU_XOR   02020
#define ALU_MROT  02040
#define ALU_ROT   04000
#define ALU_DEC   04020
#define ALU_XADD  04040
#define ALU_ADD   04060
#define ALU_SUB   06000
#define ALU_XSUB  06020
#define ALU_INC   06040
#define ALU_ARS   06060

/* Register. */
#define REG_ALATCH  010
#define REG_YCOR    020
#define REG_XCOR    021
#define REG_SCROLL  022
#define REG_XR      023
#define REG_UART    024
#define REG_DSR     025
#define REG_KEY     026

/* DSR TV on bit. */
#define DSR_TVON  0010000

typedef struct {
  uint16_t reg[4];
  uint16_t (*read)(uint16_t reg);
  void (*write)(uint16_t reg, uint16_t data);
} TTDEV;

extern t_bool build_dev_tab (void);
extern void flag_on (uint16_t flag);
extern void flag_off (uint16_t flag);
extern uint16_t cpu_alu (uint16_t insn, uint16_t op, uint16_t adata, uint16_t bdata);
extern void dpy_magic (uint16_t data, uint16_t *r2, uint16_t *r3, uint16_t r4, uint16_t r5);
extern void dpy_chartv (uint16_t data);
extern void dpy_quit_callback (void);
extern void crt_line (uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t i);
extern void tv_line (int row, uint8_t *data, uint8_t *chars);
extern void tv_refresh (void);

extern REG cpu_reg[];
extern uint16_t MEM[], CRM[];
extern uint16_t DSR;
extern uint8_t FONT[];
extern DEVICE cpu_dev, dpy_dev, crt_dev, tv_dev, key_dev, uart_dev;
extern TTDEV *dev_tab[];
extern int C, V, N, Z;
extern int dpy_quit;

extern uint16_t tt2500_rom[];

#endif /* TT2500_DEFS_H_ */
