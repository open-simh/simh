/* stub_cpu.c: Stub CPU simulator

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
*/

#include "stub_defs.h"


/* Debug */
#define DBG_CPU         0001

/* CPU state. */
static uint16 PC;

/* Function declaration. */
static t_stat cpu_ex (t_value *vptr, t_addr ea, UNIT *uptr, int32 sw);
static t_stat cpu_dep (t_value val, t_addr ea, UNIT *uptr, int32 sw);
static t_stat cpu_reset (DEVICE *dptr);

static UNIT cpu_unit = { UDATA (NULL, UNIT_FIX + UNIT_BINK, 020000) };

REG cpu_reg[] = {
  { ORDATAD (PC, PC, 13, "Program Counter") },
  { NULL }
};

static MTAB cpu_mod[] = {
  { 0 }
};

static DEBTAB cpu_deb[] = {
  { "CPU", DBG_CPU },
  { NULL, 0 }
};

DEVICE cpu_dev = {
  "CPU", &cpu_unit, cpu_reg, cpu_mod,
  0, 8, 16, 1, 8, 16,
  &cpu_ex, &cpu_dep, &cpu_reset,
  NULL, NULL, NULL, NULL, DEV_DEBUG, 0, cpu_deb,
  NULL, NULL, NULL, NULL, NULL, NULL
};

t_stat sim_instr (void)
{
  t_stat reason;

  if ((reason = build_dev_tab ()) != SCPE_OK)
    return reason;

  for (;;) {
    AIO_CHECK_EVENT;
    if (sim_interval <= 0) {
      if ((reason = sim_process_event()) != SCPE_OK)
        return reason;
    }

    if (sim_brk_summ && sim_brk_test(PC, SWMASK('E')))
      return STOP_IBKPT;

    if (sim_step != 0) {
      if (--sim_step == 0)
        return SCPE_STEP;
    }
  }

  return SCPE_OK;
}

static t_stat cpu_ex (t_value *vptr, t_addr ea, UNIT *uptr, int32 sw)
{
  if (vptr == NULL)
    return SCPE_ARG;
  if (ea >= 040000)
    return SCPE_NXM;
  *vptr = M[ea];
  return SCPE_OK;
}

static t_stat cpu_dep (t_value val, t_addr ea, UNIT *uptr, int32 sw)
{
  if (ea >= 040000)
    return SCPE_NXM;
  M[ea] = val & 0177777;
  return SCPE_OK;
}

static t_bool pc_is_a_subroutine_call (t_addr **ret_addrs)
{
  return FALSE;
}

static t_stat
cpu_reset (DEVICE *dptr)
{
  sim_brk_types = SWMASK('D') | SWMASK('E');
  sim_brk_dflt = SWMASK ('E');
  sim_vm_is_subroutine_call = &pc_is_a_subroutine_call;
  return SCPE_OK;
}
