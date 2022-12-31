/* rtx2001a_defs.h: RTX2001A simulator definitions

   Copyright (c) 2020, Systasis Computer Systems, Inc.

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

#include "hp3k_defs.h"

extern t_bool build_dev_tab(void);
extern DEVICE cpu_dev;
extern int32 sim_emax; // contains the maximum number of words needed to hold the largest instruction or data item in the VM.  Examine and deposit will process up to sim_emax words.
extern t_stat cpu_reset(DEVICE *);
extern t_stat do_cmd_label(int32 flag, CONST char *fcptr, CONST char *label);
extern t_stat cpu_boot(t_value unit_num, DEVICE *dptr);
extern jmp_buf save_env;
extern jmp_buf bkpt_env; // breakpoint handler

#define STOP_UNK 0         // Unknown Error
#define STOP_HALT 1        // HALT
#define STOP_IBKPT 2       // breakpoint
#define STOP_INVOPCOD 3    // invalid instruction
#define STOP_ILLASICADDR 4 // Illegal ASIC bus address

#define ABORT(val) longjmp(save_env, (val))

#endif /* RTX2001A_DEFS_H_ */
