/* stub_sys.c: Stub simulator interface

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

   21-Apr-20    LB      New simulator.
*/

#include "stub_defs.h"

int32 sim_emax = 1;
char sim_name[] = "Stub";

uint16 M[040000];
REG *sim_PC = &cpu_reg[0];

DEVICE *sim_devices[] = {
  &cpu_dev,
  NULL
};
  
const char *sim_stop_messages[SCPE_BASE] = {
  "Unknown error",
  "HALT instruction",
  "Breakpoint",
  "Invalid access",
};

t_stat
sim_load (FILE *fileref, CONST char *cptr, CONST char *fnam, int flag)
{
  return SCPE_OK;
}

t_bool build_dev_tab (void)
{
  return SCPE_OK;
}

t_stat fprint_sym (FILE *of, t_addr addr, t_value *val,
                   UNIT *uptr, int32 sw)
{
}

t_stat parse_sym (CONST char *cptr, t_addr addr, UNIT *uptr,
                  t_value *val, int32 sw)
{
  return SCPE_OK;
}
