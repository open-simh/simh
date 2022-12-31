/* rtx2001a_sys.c: RTX2001A simulator interface

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

#include "rtx2001a_defs.h"
#include "rtx2001a_registers.h"
#include "rtx2001a_psb.h"
#include "rtx2001a_rsb.h"
#include "rtx2001a_debug.h"

char sim_name[] = "RTX2001A";

DEVICE *sim_devices[] = {
    &cpu_dev,
    NULL};

const char *sim_stop_messages[SCPE_BASE] = {
    "Unknown error",
    "HALT instruction",
    "Breakpoint",
    "Invalid opcode",
};

t_stat sim_load(FILE *fileref, CONST char *cptr, CONST char *fnam, int flag)
{
  return SCPE_OK;
}

t_bool build_dev_tab(void)
{
  return SCPE_OK;
}
/**
 * These cases are differentiated by the presence of special flags in the switch parameter.
 * For a simulation stop, the “M” switch and the SIM_SW_STOP switch are passed.
 * For examining registers, the SIM_SW_REG switch is passed.
 * The user-defined flags and register radix are passed in the addr parameter.
 * Register radix is taken from the radix specified in the register definition, or overridden by –d, -o, or –x switches in the command.
 * For examining memory and the EVAL command, no special switch flags are passed.
 */
t_stat fprint_sym(FILE *of, t_addr addr, t_value *val, UNIT *uptr, int32 sw)
{
  t_value cell = (addr & REG_UFMASK) >> 16;
  // if (_RS == cell)
  // {

  //   RTX_WORD _rsp = spr.fields.rsp;
  //   if (0 == _rsp)
  //   {
  //     fprintf(of, "empty\n");
  //     return SCPE_OK;
  //   }
  //   do
  //   {
  //     fprintf(of, " rs[%d]>> %d:0x%X\n", _rsp, rs[_rsp].fields.ipr, rs[_rsp].fields.i);
  //     _rsp -= 1;
  //   } while (_rsp > 0);
  /*
  void display_rs()
{ double_stack *ptr;
ptr = &return_stack[rp];
printf(" rs>> ");
if (rp == 0 )
  { printf("empty ");  return; }

printf("%01X%04X ",IPR, INDEX);

while (ptr != &return_stack[1] )
  { printf("%01X:%04X",(*ptr).high, (*ptr).low);
    ptr-- ;
  }
}

  void display_short_rs()
{  if (rp == 0 )
  { printf("rs(0)>> empty ");  return; }

printf(" rs(%d)>> ", rp-1);

printf("%01X%04X ",IPR, INDEX);
if (rp != MAX_STACK )  printf(" ...");
}


*/
  // return SCPE_OK;
  // }

  if (sw & SIM_SW_REG)
  {
    BITFIELD *bits = NULL;
    t_value value = *val;
    switch (cell)
    {
    case _CR:
      bits = sim_is_running ? cr_bits : NULL;
      break;
    case _SUR:
      bits = sim_is_running ? sur_bits : NULL;
      break;
    case _IVR: // _SVR
      bits = sim_is_running ? (svr.pr == value ? svr_bits : ivr_bits) : NULL;
      break;
    case _IPR:
    case _DPR:
    case _UPR:
    case _CPR:
      bits = sim_is_running ? pr_bits : NULL;
      break;
    case _IBC:
      bits = sim_is_running ? ibc_bits : NULL;
      break;
    case _UBR:
      bits = sim_is_running ? ubr_bits : NULL;
      break;
    case RSP:
      value = spr.fields.rsp;
      break;
    case PSP:
      value = spr.fields.psp;
      break;
    }
    fprintf(of, sim_is_running ? "0x%X\n" : "0x%X", value);
    if (bits)
    {
      sim_debug_bits(DBG_ASB_R, &cpu_dev, bits, value, value, 1);
    }
    else
    {
      if (sim_is_running)
        fprintf(of, "0x%X", value);
    }
  }
  else
  {
    fprintf(of, "0x%X", *val);
  }

  return SCPE_OK;
}

t_stat parse_sym(CONST char *cptr, t_addr addr, UNIT *uptr, t_value *val, int32 sw)
{
  RTX_WORD value = strtoul(cptr, NULL, 16);
  if (sw & SIM_SW_REG)
  {
    RTX_WORD cell = (addr & REG_UFMASK) >> 16;
    char name[] = "";
    switch (cell)
    {
    case _CR:
      DEBUG_SET(cr.pr, cr_bits);
      break;
    case _SUR:
      DEBUG_SET(sur.pr, sur_bits);
      break;
    case _SVR:
      DEBUG_SET(svr.pr, svr_bits);
      break;
    case _IPR:
      DEBUG_SET(ipr.pr, pr_bits);
      break;
    case _DPR:
      DEBUG_SET(dpr.pr, pr_bits);
      break;
    case _UPR:
      DEBUG_SET(upr.pr, pr_bits);
      break;
    case _CPR:
      DEBUG_SET(cpr.pr, pr_bits);
      break;
    case _IBC:
      DEBUG_SET(ibc.pr, ibc_bits);
      break;
    case _UBR:
      DEBUG_SET(ubr.pr, ubr_bits);
      break;
    case PSP:
      spr.fields.psp = (value & D16_MASK);
      *val = spr.pr;
      sim_debug(DBG_ASB_W, &cpu_dev, "%s: 0x%X\n", (&cpu_reg[_SPR])->name, *val);
      break;
    case RSP:
      spr.fields.rsp = (value & D16_MASK);
      *val = spr.pr;
      sim_debug(DBG_ASB_W, &cpu_dev, "%s: 0x%X\n", (&cpu_reg[_SPR])->name, *val);
      break;
    default:
    {
      *val = (value & D16_MASK);
      sim_debug(DBG_ASB_W, &cpu_dev, "%s: 0x%X\n", (&cpu_reg[cell])->name, *val);
    }
    }
  }
  else
  {
    *val = (value & D16_MASK);
  }
  return SCPE_OK;
}