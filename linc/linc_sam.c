/* linc_sam.c: LINC sampled analog inputs.

   Copyright (c) 2025, Lars Brinkhoff

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

#include "linc_defs.h"
#include "sim_video.h"

/* Function declaration. */
static t_stat sam_svc(UNIT *uptr);
static t_stat sam_reset(DEVICE *dptr);

/* Debug */
#define DBG             0001

#define SSW (*(uint16 *)cpu_reg[10].loc)
#define SAM ((uint16 *)cpu_reg[14].loc)

static UNIT sam_unit = {
  UDATA(&sam_svc, UNIT_IDLE, 0)
};

static DEBTAB sam_deb[] = {
  { "DBG",  DBG },
  { NULL, 0 }
};

DEVICE sam_dev = {
  "SAM", &sam_unit, NULL, NULL,
  1, 8, 12, 1, 8, 12,
  NULL, NULL, &sam_reset,
  NULL, NULL, NULL,
  NULL, DEV_DISABLE | DEV_DEBUG, 0, sam_deb,
  NULL, NULL, NULL, NULL, NULL, NULL
};

static t_stat
sam_svc(UNIT *uptr)
{
  return SCPE_OK;
}

static uint16 ones_complement(int value)
{
  value /= 258;
  if (value < 0)
    return (uint16)(-value & 07777) ^ 07777;
  else
    return (uint16)(value & 07777);
}

static void sam_motion(int which, int axis, int value)
{
  sim_debug(DBG, &sam_dev, "Motion %d, axis %d, value %d\n",
            which, axis, value);
  switch (which) {
  case 0:
    switch (axis) {
    case 0:
      SAM[0] = ones_complement(value);
      break;
    case 1:
      SAM[1] = ones_complement(-value);
      break;
    }
    break;
  case 1:
    switch (axis) {
    case 0:
      SAM[6] = ones_complement(value);
      break;
    case 1:
      SAM[7] = ones_complement(-value);
      break;
    }
    break;
  }
}

static void sam_button(int which, int button, int state)
{
  switch (which) {
  case 0:
    switch (button) {
    case 0:
      SSW = (SSW & 037) | (state << 5);
      break;
    }
    break;
  case 1:
    switch (button) {
    case 0:
      SSW = (SSW & 076) | state;
      break;
    }
    break;
  }
}

static t_stat
sam_reset(DEVICE *dptr)
{
  t_stat stat;

  if ((dptr->flags & DEV_DIS) != 0 || (sim_switches & SWMASK('P')) != 0) {
    sim_cancel(&sam_unit);
    return SCPE_OK;
  }

  if (crt_dev.flags & DEV_DIS)
    return SCPE_OK;

  crt_dev.reset(&crt_dev);
  stat = vid_register_gamepad_motion_callback(sam_motion);
  if (stat == SCPE_NOATT)
    return SCPE_OK; /* Gamepad not available. */
  if (stat != SCPE_OK && stat != SCPE_ALATT)
    return stat;
  stat = vid_register_gamepad_button_callback(sam_button);
  if (stat != SCPE_OK && stat != SCPE_ALATT)
    return stat;
  sim_activate(&sam_unit, 1);
  return SCPE_OK;
}
