/* linc_crt.c: LINC CRT display

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
#include "display/display.h"

/* Function declaration. */
static t_stat crt_svc(UNIT *uptr);
static t_stat crt_reset(DEVICE *dptr);

#define CRT_UNITS  2

static VID_DISPLAY *crt_window[CRT_UNITS] = { NULL, NULL };
static uint32 fade[CRT_UNITS][512 * 512];
static uint32 dot[CRT_UNITS][7 * 7];
t_stat crt_svc_stat;


/* Debug */
#define DBG             0001
#define DBG_DOT         0002

#define UNIT_V_ORANGE   (UNIT_V_UF + 0)
#define UNIT_ORANGE     (1 << UNIT_V_ORANGE)
#define UNIT_V_GREEN    (UNIT_V_UF + 1)
#define UNIT_GREEN      (1 << UNIT_V_GREEN)
#define UNIT_PHOSPHOR   (UNIT_ORANGE | UNIT_GREEN)

#define UNIT_V_CH0      (UNIT_V_UF + 2)
#define UNIT_CH0        (1 << UNIT_V_CH0)
#define UNIT_V_CH1      (UNIT_V_UF + 3)
#define UNIT_CH1        (1 << UNIT_V_CH1)
#define UNIT_BOTH       (UNIT_CH0 | UNIT_CH1)
#define UNIT_CHANNEL    (UNIT_CH0 | UNIT_CH1)

static UNIT crt_unit[CRT_UNITS] = {
  { UDATA(&crt_svc, UNIT_IDLE|UNIT_ORANGE|UNIT_CH0|UNIT_DISABLE, 0) },
  { UDATA(NULL,     UNIT_IDLE|UNIT_GREEN|UNIT_CH1|UNIT_DISABLE|UNIT_DIS, 0) }
};

static MTAB crt_mod[] = {
  { UNIT_PHOSPHOR, UNIT_ORANGE, "ORANGE", "ORANGE", NULL, NULL, "Orange phosphor" },
  { UNIT_PHOSPHOR, UNIT_GREEN,  "GREEN",  "GREEN",  NULL, NULL, "Green phosphor" },
  { UNIT_CHANNEL,  UNIT_CH0,    "CH0",    "CH0",    NULL, NULL, "Channel 0" },
  { UNIT_CHANNEL,  UNIT_CH1,    "CH1",    "CH1",    NULL, NULL, "Channel 1" },
  { UNIT_CHANNEL,  UNIT_BOTH,   "BOTH",   "BOTH",   NULL, NULL, "Both channels" },
  { 0 }
};

static DEBTAB crt_deb[] = {
  { "DBG",  DBG },
  { "DOT",  DBG_DOT },
  { "VVID", SIM_VID_DBG_VIDEO },
  { "KVID", SIM_VID_DBG_KEY },
  { NULL, 0 }
};

DEVICE crt_dev = {
  "CRT", crt_unit, NULL, crt_mod,
  CRT_UNITS, 8, 12, 1, 8, 12,
  NULL, NULL, &crt_reset,
  NULL, NULL, NULL,
  NULL, DEV_DISABLE | DEV_DEBUG, 0, crt_deb,
  NULL, NULL, NULL, NULL, NULL, NULL
};

static t_stat
crt_svc(UNIT *uptr)
{
  SIM_KEY_EVENT ev;
  int i;

  for (i = 0; i < CRT_UNITS; i++) {
    if (crt_window[i] != NULL) {
      if (crt_unit[i].flags & UNIT_DIS) {
        vid_close_window(crt_window[i]);
        crt_window[i] = NULL;
        continue;
      }
      vid_refresh_window(crt_window[i]);
      vid_draw_window(crt_window[i], 0, 0, 512, 512, fade[i]);
    } else if ((crt_unit[i].flags & UNIT_DIS) == 0) {
      crt_reset(uptr->dptr);
    }
  }

  while (vid_poll_kb(&ev) == SCPE_OK)
    kbd_event(&ev);

  sim_activate_after(uptr, 50000);
  return crt_svc_stat;
}

static void crt_quit(VID_DISPLAY *vptr)
{
  int u;

  /* Disable the unit that was quit. */
  for (u = 0; u < CRT_UNITS; u++) {
    if (vptr != crt_window[u])
      continue;
    crt_unit[u].flags |= UNIT_DIS;
  }

  /* Check if there are any open windows left. */
  for (u = 0; u < CRT_UNITS; u++) {
    if ((crt_unit[u].flags & UNIT_DIS) == 0)
      return;
  }

  /* No windows, stop. */
  crt_svc_stat = SCPE_STOP;
}

static t_stat
crt_reset(DEVICE *dptr)
{
  char title[100];
  t_stat stat;
  uint8 r, g, b, a;
  int i, j, u;

  if ((dptr->flags & DEV_DIS) != 0 || (sim_switches & SWMASK('P')) != 0) {
    for (u = 0; u < CRT_UNITS; u++) {
      if (crt_window[u] != NULL)
        vid_close_window(crt_window[u]);
      crt_window[u] = NULL;
    }
    sim_cancel(&crt_unit[0]);
  } else {
    for (u = 0; u < CRT_UNITS; u++) {
      if (crt_unit[u].flags & UNIT_DIS)
        continue;
      if (!sim_is_active(&crt_unit[0]))
        sim_activate(&crt_unit[0], 1);
      if (crt_window[u] != NULL)
        continue;

      snprintf(title, sizeof title, "Scope %d", u);
      stat = vid_open_window(&crt_window[u], dptr, title, 512, 512, 0);
      if (stat != SCPE_OK)
        return stat;
      if (crt_window[u] == NULL)
        return SCPE_OPENERR;
      /* Allow time for window to open and data structures to settle. */
      sim_os_ms_sleep(100);
      stat = vid_set_alpha_mode(crt_window[u], SIM_ALPHA_BLEND);
      if (stat != SCPE_OK) {
        vid_close_window(crt_window[u]);
        crt_window[u] = NULL;
        return stat;
      }
      vid_register_close_callback(crt_window[u], crt_quit);

      if (crt_unit[u].flags & UNIT_ORANGE) {
        r = 250;
        g = 130;
        b = 30;
        a = 20;
      } else if (crt_unit[u].flags & UNIT_GREEN) {
        r = 30;
        g = 250;
        b = 100;
        a = 95;
      } else
        return SCPE_OPENERR;

      fade[u][0] = vid_map_rgba_window(crt_window[u], 0, 0, 0, a);
      for (i = 1; i < 512 * 512; i++)
        fade[u][i] = fade[u][0];

      for (i = 0; i < 7; i++) {
        for (j = 0; j < 7; j++) {
          int dx = i - 3, dy = j - 3;
          int r2 = dx*dx + dy*dy;
          double focus = 0.7;
          double alpha = 0xFF*exp(-focus*r2) + .49;
          dot[u][i + 7*j] = vid_map_rgba_window(crt_window[u], r, g, b, (uint8)alpha);
        }
      }
    }
  }

  return SCPE_OK;
}

void
crt_point(uint16 u, uint16 x, uint16 y)
{
  unsigned mask = 1 << (UNIT_V_CH0 + u);
  int i;
  sim_debug(DBG, &crt_dev, "Display %d; point %o,%o\n", u, x, y);
  for (i = 0; i < CRT_UNITS; i++) {
    if (crt_window[i] == NULL)
      continue;
    if (crt_unit[i].flags & mask) {
      vid_draw_window(crt_window[i], x - 3, 511 - y - 3, 7, 7, dot[i]);
      if (crt_dev.dctrl & DBG_DOT)
        vid_refresh_window(crt_window[i]);
    }
  }
}

void crt_toggle_fullscreen(VID_DISPLAY *vptr)
{
  if (vptr != NULL)
    vid_set_fullscreen_window(vptr, !vid_is_fullscreen_window(vptr));
}
