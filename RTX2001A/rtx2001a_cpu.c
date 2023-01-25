/* rtx2001a_cpu.c: RTX2001A CPU simulator

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

#include "setjmp.h"
#include "rtx2001a_debug.h"
#include "rtx2001a_defs.h"
#include "rtx2001a_ab.h"
#include "rtx2001a_mb.h"
#include "rtx2001a_psb.h"
#include "rtx2001a_rsb.h"
#include "rtx2001a_registers.h"
#include "rtx2001a_execute.h"
#include "rtx2001a_decode.h"

/* Function declaration. */
t_stat cpu_ex(t_value *vptr, t_addr ea, UNIT *uptr, int32 sw);
t_stat cpu_dep(t_value val, t_addr ea, UNIT *uptr, int32 sw);
t_stat cpu_reset(DEVICE *dptr);

UNIT cpu_unit = {UDATA(NULL, UNIT_FIX + UNIT_BINK, RAM_SIZE)};

MTAB cpu_mod[] = {
    {0}};

DEVICE cpu_dev = { // RTX2001A device structure
    "CPU",         /* name */
    &cpu_unit,     /* units */
    &cpu_reg[0],   /* registers */
    &cpu_mod[0],   /* modifiers */
    0,             /* #units */
    16,            /* address radix */
    16,            /* address width */
    1,             /* addr increment */
    16,            /* data radix */
    16,            /* data width */
    &cpu_ex,       /* examine routine */
    &cpu_dep,      /* deposit routine */
    &cpu_reset,    /* reset routine */
    NULL,          /* boot routine */
    NULL,          /* attach routine */
    NULL,          /* detach routine */
    NULL,          /* context */
    DEV_DEBUG,     /* enable device debug */
    /* debug control */
    SWMASK('E') | DBG_ASB_W | DBG_ASB_R | DBG_PSB_W | DBG_PSB_R | DBG_MEB_W | DBG_MEB_R | DBG_RSB_W | DBG_RSB_R,
    &cpu_deb[0], /* debug flags */
    NULL, NULL, NULL, NULL, NULL, NULL};

/**
 * 3.8	Other Data Structures, simh.doc, pg 33
 * contains the maximum number of words needed to hold the largest instruction or data item in the VM.
 * Examine and deposit will process up to sim_emax words.
 * "The number of words in the val array is given by global variable sim_emax.", simh.doc, pg 35
 */
int32 sim_emax = 1;

RTX_WORD TOP = 0;
RTX_WORD NEXT = 0;
RTX_WORD IR = 0;
RTX_WORD clocks = 0;
REG *sim_PC = &cpu_reg[7];
/*
** second cycle context
*/
t_value second_cycle = 0;
t_value stream_mode = 0;
t_value STREAM = 0;
RTX_WORD temp_ir = 0;
// RTX_WORD temp_pc = 0;

RTX_WORD ps[PS_MAX + 1] = {0};

RTX_WORD asic_file[ASIC_MAX] = {0};

RTX_WORD gdata[GDATA_SIZE] = {0};

ReturnStack rs[RS_MAX + 1] = {0};

jmp_buf save_env; // abort handler
jmp_buf trap_env; // trap/breakpoint handler

t_value rtx2001a_pc_value(void)
{
  return (t_value)asic_file[PC];
}

/*
** See SSEM/ssem_cpu.c
*/
t_stat sim_instr(void)
{
  t_stat reason = 0;

  sim_cancel_step(); /* defang SCP step. See SSEM */

  /* Abort handling

     If an abort occurs in memory management or memory access, the lower
     level routine executes a longjmp to this area OUTSIDE the main
     simulation loop.

     All variables used in setjmp processing, or assumed to be valid
     after setjmp, must be volatile or global.
  */
  int32 abortval = setjmp(save_env); /* set abort hdlr */
  if (0 != abortval)
  {
    return abortval;
  }

  /* Main instruction fetch/decode loop */
  do
  {

    if (sim_interval <= 0)
    { /* check clock queue */
#if !UNIX_PLATFORM
      if ((reason = sim_poll_kbd()) == SCPE_STOP)
      { /* poll on platforms without reliable signalling */
        break;
      }
#endif
      /*
       ** process timer interrupt. See pg. 14
       ** maybe needed outside of sim_interval check
       */
      //   if (sim_is_active())
      //   {
      if (SCPE_OK != (reason = sim_process_event()))
        break;
      //   }
    }

    if (sim_brk_summ && /* breakpoint? */
        sim_brk_test(asic_file[PC], SWMASK('E')))
    {
      reason = STOP_IBKPT; /* stop simulation */
      break;
    }

    /*
    ** RTX 2000 and RTX 2001A Extended Cycle Operation
    ** See pg. 67, PgmrsRefMnl
    */

    reason = setjmp(trap_env); /* set break hdlr */
    if (0 == reason)
    {
      if (!STREAM && second_cycle)
      {
        if (SCPE_OK != (reason = execute(temp_ir)))
        {
          return (reason);
        }
      }
      else
      {
        if (SCPE_OK != (reason = execute(IR)))
        {
          return (reason);
        }
      }

      if (stream_mode)
      {
        if (0 == asic_file[I])
        {
          STREAM = 0;
          stream_mode = 0;
          rs_pop();
          if (!second_cycle)
          {
            _long_fetch(cpr.fields.pr, asic_file[PC], &IR);
            temp_ir = IR; // Save context. Could be a two cycle instruction
            INC_PC;
          }
        }
        else
        {
          asic_file[I] = (asic_file[I] - 1) & D16_MASK;
        }
      }
      else
      {
        if (STREAM)
        { /* enter streamed mode for following instruction */
          stream_mode = TRUE;
        }
        if (!second_cycle)
        {
          _long_fetch(cpr.fields.pr, asic_file[PC], &IR);
          temp_ir = IR; // Save context. Could be a two cycle instruction
          INC_PC;
        }
      }

      sim_interval -= (sim_interval <= 0) ? 0 : 1;

      if (sim_step && (--sim_step <= 0))
      {
        reason = SCPE_STOP;
        break;
      }
    }

  } while (reason == 0); /* loop until halted */

  return reason;
}

t_stat cpu_boot(t_value unit_num, DEVICE *dptr)
{
  _long_fetch(cpr.fields.pr, 0, &IR);
  INC_PC;
  return SCPE_OK;
}

t_stat cpu_ex(t_value *vptr, t_addr ea, UNIT *uptr, t_svalue sw)
{
  t_value m = sw & SWMASK('M');
  if (vptr == NULL)
    return SCPE_ARG;
  if (ea >= RAM_SIZE)
    return SCPE_NXM;
  byte_fetch(ea, vptr);
  if (m)
  {
    RTX_WORD temp = 0;
    _long_fetch(cpr.fields.pr, ea, &temp);
    print_instruction(temp, cpr.pr, ea);
  }
  return SCPE_OK;
}

t_stat cpu_dep(t_value val, t_addr ea, UNIT *uptr, t_svalue sw)
{
  if (ea >= RAM_SIZE)
    return SCPE_NXM;
  byte_store(ea, val);
  return SCPE_OK;
}

t_stat cpu_reset(DEVICE *dptr)
{
  // init debug support
  // sim_brk_types = DBG_ASB; // SWMASK('E'); //| DBG_ASB | DBG_PSB | DBG_MEB | DBG_RSB;
  sim_brk_dflt = SWMASK('E');
  sim_brk_type_desc = cpu_breakpoints;
  sim_brk_types = SWMASK('E') | DBG_ASB_W | DBG_ASB_R | DBG_PSB_W | DBG_PSB_R | DBG_MEB_W | DBG_MEB_R | DBG_RSB_W | DBG_RSB_R;
  sim_vm_pc_value = &rtx2001a_pc_value;
  if (bad_insn == dispatch_vector_1[OP_0BRANCH])
  {
    init_dispatch_vectors();
  }

  // init machine
  memset(ram, 0xA5A5, RAM_SIZE * sizeof(RTX_WORD));
  memset(ps, 0xA5A5, PS_MAX + 1);
  memset(rs, 0xA5A5, RS_MAX + 1);
  memset(asic_file, 0, ASIC_MAX * sizeof(RTX_WORD));
  memset(gdata, 0xA5A5, GDATA_SIZE * sizeof(RTX_WORD));
  IR = 0;
  clocks = 0;
  second_cycle = 0;
  STREAM = 0;

  // Initialize ASIC bus
  REG *rptr = NULL;
  for (rptr = dptr->registers; (rptr != NULL) && (rptr->name != NULL); rptr++)
  {
    if (0 == strcmp(rptr->name, "I"))
    {
      asic_file[I] = asic_file[I_ASIC] = asic_file[I_ALU] = D16_UMAX;
    }
    else if (0 == strcmp(rptr->name, "IR"))
    {
      // written by INC_PC
    }
    else if (0 == strcmp(rptr->name, "CR"))
    {
      // Boot=1; Interrupts disabled; Byte Order=0 (BE)
      cr.pr = 0;
      BOOT = SID = 1;
    }
    else if (0 == strcmp(rptr->name, "MD"))
    {
      asic_file[MD] = D16_UMAX;
    }
    else if (0 == strcmp(rptr->name, "IMR"))
    {
      imr.pr = 0;
    }
    else if (0 == strcmp(rptr->name, "SPR"))
    {
      TOP = IR = spr.pr = 0;
      NEXT = 0xFFFF;
    }
    else if (0 == strcmp(rptr->name, "SUR"))
    {
      // Consider 0x0707 See rtx2000_simulator/STATE.C:58
      sur.pr = 0;
      // psf: 1, pss: 1, rsf: 1, rss: 1
      PSF = RSF = PSS = RSS = 1;
    }
    else if (0 == strcmp(rptr->name, "SVR"))
    {
      svr.pr = 0;
      PVL = RVL = 0x3F;
    }
    else if (0 == strcmp(rptr->name, "IVR"))
    {
      ivr.pr = 0x200;
    }
    else if (0 == strcmp(rptr->name, "IPR"))
    {
      ipr.pr = 0;
    }
    else if (0 == strcmp(rptr->name, "DPR"))
    {
      dpr.pr = 0;
    }
    else if (0 == strcmp(rptr->name, "UPR"))
    {
      upr.pr = 0;
    }
    else if (0 == strcmp(rptr->name, "CPR"))
    {
      cpr.pr = 0;
    }
    else if (0 == strcmp(rptr->name, "IBC"))
    {
      ibc.pr = 0;
    }
    else if (0 == strcmp(rptr->name, "UBR"))
    {
      ubr.pr = 0;
    }
  }
  return SCPE_OK;
}
