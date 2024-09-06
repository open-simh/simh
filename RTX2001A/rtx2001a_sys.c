/* rtx2001a_sys.c: RTX2001A simulator interface

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

#include "rtx2001a_defs.h"
#include "rtx2001a_registers.h"
#include "rtx2001a_psb.h"
#include "rtx2001a_rsb.h"
#include "rtx2001a_debug.h"
#include "rtx2001a_mb.h"

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

    case RS:
    {
      fprintf(of, "RSP=%d ", spr.fields.rsp);
      if (0 == spr.fields.rsp)
      {
        fprintf(of, "empty ");
        return SCPE_OK;
      }

      fprintf(of, "%d:0x%04X ", ipr.fields.pr, asic_file[I]);

      ReturnStack *ptr = &rs[spr.fields.rsp];
      while (ptr != &rs[1])
      {
        fprintf(of, "%d:0x%04X ", ptr->fields.ipr, ptr->fields.i);
        ptr--;
      }
      return SCPE_OK;
      break;
    }

    case PS:
    {
      fprintf(of, "PSP=%d ", spr.fields.psp);
      if (0 == spr.fields.psp)
      {
        fprintf(of, "empty ");
        return SCPE_OK;
      }

      fprintf(of, "0x%X ", TOP);
      if (1 == spr.fields.psp)
        return SCPE_OK;

      fprintf(of, "0x%X ", NEXT);

      RTX_WORD *ptr = &ps[spr.fields.psp];
      while (ptr != &ps[2])
      {
        printf("0x%X ", *ptr & D16_MASK);
        ptr--;
      }

      return SCPE_OK;
    }

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

void byte_to_int(char *byte, int *value)
{
  if (*byte >= '0' && *byte <= '9')
  {
    *value = (*byte - (int)'0') << 4;
  }
  if (*byte >= 'A' && *byte <= 'F')
  {
    *value = (*byte + 10 - (int)'A') << 4;
  }
  if (*byte >= 'a' && *byte <= 'f')
  {
    *value = (*byte + 10 - (int)'a') << 4;
  }
  byte += 1;
  if (*byte >= '0' && *byte <= '9')
  {
    *value |= (*byte - (int)'0');
  }
  if (*byte >= 'A' && *byte <= 'F')
  {
    *value |= (*byte + 10 - (int)'A');
  }
  if (*byte >= 'a' && *byte <= 'f')
  {
    *value |= (*byte + 10 - (int)'a');
  }
}

void word_to_int(char *word, int *value)
{
  int hi = 0;
  int lo = 0;
  byte_to_int(word, &hi);
  word += 2;
  byte_to_int(word, &lo);
  *value = hi << 8 | lo;
}

/*
If flag = 0, load data from binary file fptr.
If flag = 1, dump data to binary file fptr.
For either command, buf contains any VM-specific arguments, and fnam contains the file name.
*/
t_stat sim_load(FILE *fptr, const char *buf, const char *fnam, t_bool flag)
{
  struct
  {
    char leader;
    char len[2];
  } header;

  struct
  {
    char address[4];
    char type[2];
    char body[255];
  } cargo;

  int len;
  int address;
  int bytesRead = fread(&header, sizeof(header), 1, fptr);
  while (!feof(fptr))
  {
    if (':' != header.leader)
    {
      sim_messagef(SCPE_INCOMP, "input file not in proper .HEX format, ");
      return SCPE_INCOMP;
    }

    byte_to_int(header.len, &len);

    if (0 == len)
    {
      break;
    }

    memset(cargo.address, '\0', sizeof(cargo.address));
    memset(cargo.type, '\0', sizeof(cargo.type));
    memset(cargo.body, '\0', sizeof(cargo.body));
    bytesRead = fread(&cargo, (len * 2) + 10, 1, fptr);
    if (feof(fptr))
    {
      sim_messagef(SCPE_INCOMP, "unexpected EOF, ");
      return SCPE_INCOMP;
    }

    word_to_int(cargo.address, &address);
    int value;
    for (int i = 0; i < len * 2; i++)
    {
      byte_to_int(&cargo.body[i], &value);
      byte_store(address, value);
      sim_printf("dep 0x%X %c", address, cargo.body[i]);
      byte_to_int(&cargo.body[++i], &value);
      byte_store(++address, value);
      sim_printf("%c\n", cargo.body[i]);
    }

    header.leader = '\0';
    memset(header.len, '\0', sizeof(header.len));
    bytesRead = fread(&header, sizeof(header), 1, fptr);
  }

  if (':' != header.leader)
  {
    sim_messagef(SCPE_INCOMP, "input file not in proper .HEX format, ");
    return SCPE_INCOMP;
  }

  return SCPE_OK;
}