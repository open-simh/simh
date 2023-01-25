/* rtx2001a_defs.h: RTX2001A simulator definitions

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

   12-Aug-22   SYS      New simulator.
*/

#ifndef RTX2001A_DEFS_H_
#define RTX2001A_DEFS_H_ 0

#include "setjmp.h"

#include "sim_defs.h"

/* Architectural constants.
**
** See hp3000_defs.h:356
*/
typedef uint32 RTX_WORD; /* RTX2001A 16-bit data word representation */

#define R_MASK 0177777u /* 16-bit register mask */

#define D4_WIDTH 4    /* 4-bit data bit width */
#define D4_MASK 0017u /* 4-bit data mask */

#define D6_WIDTH 6   /* 6-bit data bit width */
#define D6_MASK 077u /* 6-bit data mask */

#define D8_WIDTH 8    /* 8-bit data bit width */
#define D8_MASK 0377u /* 8-bit data mask */
#define D8_UMAX 0377u /* 8-bit unsigned maximum value */
#define D8_SMAX 0177u /* 8-bit signed maximum value */
#define D8_SMIN 0200u /* 8-bit signed minimum value */
#define D8_SIGN 0200u /* 8-bit sign */

#define D16_WIDTH 16      /* 16-bit data bit width */
#define D16_MASK 0177777u /* 16-bit data mask */
#define D16_UMAX 0177777u /* 16-bit unsigned maximum value */
#define D16_SMAX 0077777u /* 16-bit signed maximum value */
#define D16_SMIN 0100000u /* 16-bit signed minimum value */
#define D16_SIGN 0100000u /* 16-bit sign */

#define D32_WIDTH 32           /* 32-bit data bit width */
#define D32_MASK 037777777777u /* 32-bit data mask */
#define D32_UMAX 037777777777u /* 32-bit unsigned maximum value */
#define D32_SMAX 017777777777u /* 32-bit signed maximum value */
#define D32_SMIN 020000000000u /* 32-bit signed minimum value */
#define D32_SIGN 020000000000u /* 32-bit sign */

#define D48_WIDTH 48                 /* 48-bit data bit width */
#define D48_MASK 07777777777777777uL /* 48-bit data mask */
#define D48_UMAX 07777777777777777uL /* 48-bit unsigned maximum value */
#define D48_SMAX 03777777777777777uL /* 48-bit signed maximum value */
#define D48_SMIN 04000000000000000uL /* 48-bit signed minimum value */
#define D48_SIGN 04000000000000000uL /* 48-bit sign */

#define D64_WIDTH 64                       /* 64-bit data bit width */
#define D64_MASK 01777777777777777777777uL /* 64-bit data mask */
#define D64_UMAX 01777777777777777777777uL /* 64-bit unsigned maximum value */
#define D64_SMAX 00777777777777777777777uL /* 64-bit signed maximum value */
#define D64_SMIN 01000000000000000000000uL /* 64-bit signed minimum value */
#define D64_SIGN 01000000000000000000000uL /* 64-bit sign */

#define S16_OVFL_MASK ((uint32)D16_UMAX << D16_WIDTH | \
                       D16_SIGN) /* 16-bit signed overflow mask */

#define S32_OVFL_MASK ((t_uint64)D32_UMAX << D32_WIDTH | \
                       D32_SIGN) /* 32-bit signed overflow mask */

/* Portable conversions.
**
** See hp3000_defs.h:555
*/
#define SEXT8(x) (int32)((x)&D8_SIGN ? (x) | ~D8_MASK : (x))
#define SEXT16(x) (int32)((x)&D16_SIGN ? (x) | ~D16_MASK : (x))

#define NEG8(x) ((~(x) + 1) & D8_MASK)
#define NEG16(x) ((~(x) + 1) & D16_MASK)
#define NEG32(x) ((~(x) + 1) & D32_MASK)

#define INT16(u) ((u) > D16_SMAX ? (-(int16)(D16_UMAX - (u)) - 1) : (int16)(u))
#define INT32(u) ((u) > D32_SMAX ? (-(int32)(D32_UMAX - (u)) - 1) : (int32)(u))

/* Half-byte accessors */

#define UPPER_HALF(b) ((b) >> D4_WIDTH & D4_MASK)
#define LOWER_HALF(b) ((b)&D4_MASK)

/* Byte accessors.
**
** See hp3000_defs.h:601
*/
typedef enum
{
   upper, /* upper byte selected */
   lower  /* lower byte selected */
} BYTE_SELECTOR;

#define UPPER_BYTE(w) (uint8)((w) >> D8_WIDTH & D8_MASK)
#define LOWER_BYTE(w) (uint8)((w)&D8_MASK)
#define TO_WORD(u, l) (RTX_WORD)(((u)&D8_MASK) << D8_WIDTH | (l)&D8_MASK)

#define REPLACE_UPPER(w, b) ((w)&D8_MASK | ((b)&D8_MASK) << D8_WIDTH)
#define REPLACE_LOWER(w, b) ((w)&D8_MASK << D8_WIDTH | (b)&D8_MASK)

/* Double-word accessors */

#define UPPER_WORD(d) (RTX_WORD)((d) >> D16_WIDTH & D16_MASK)
#define LOWER_WORD(d) (RTX_WORD)((d)&D16_MASK)

#define TO_DWORD(u, l) ((RTX_WORD)(u) << D16_WIDTH | (l))

extern t_bool build_dev_tab(void);
extern DEVICE cpu_dev;
extern int32 sim_emax; // contains the maximum number of words needed to hold the largest instruction or data item in the VM.  Examine and deposit will process up to sim_emax words.
extern t_stat cpu_reset(DEVICE *);
extern t_stat do_cmd_label(int32 flag, CONST char *fcptr, CONST char *label);
extern t_stat cpu_boot(t_value unit_num, DEVICE *dptr);
extern jmp_buf save_env;
extern jmp_buf trap_env; // breakpoint handler

#define STOP_UNK 0         // Unknown Error
#define STOP_HALT 1        // HALT
#define STOP_IBKPT 2       // breakpoint
#define STOP_INVOPCOD 3    // invalid instruction
#define STOP_ILLASICADDR 4 // Illegal ASIC bus address

#define ABORT(val) longjmp(save_env, (val))
#define TRAP(val) longjmp(trap_env, (val))

#endif /* RTX2001A_DEFS_H_ */
