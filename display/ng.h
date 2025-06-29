/*
 * Copyright (c) 2025 Lars Brinkhoff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization
 * from the authors.
 */

#if defined(__cplusplus)
extern "C" {
#endif
#ifndef SIM_DEFS_H_
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
#endif /* SIM_DEFS_H_ */

/*
 * The PIX_SCALE #define establishes the initial default display scale
 * factor; to change from the default scale factor, set type34_scale
 * before calling type34_init().
 */
#ifndef PIX_SCALE
#define PIX_SCALE RES_FULL
#endif
extern int32 type34_type;
extern int32 type34_scale;

extern int32 type34_get_csr(void);
extern int32 type34_get_reloc(void);
extern void type34_set_csr(uint16);
extern void type34_set_reloc(uint16);

extern int  type34_init(void *, int);
extern int  type34_cycle(int, int);

extern int  type34_fetch(uint32, uint16 *);       /* get a display-file word */
extern int  type34_store(uint32, uint16);

#if defined(__cplusplus)
}
#endif
