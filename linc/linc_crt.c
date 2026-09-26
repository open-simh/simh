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

static VID_DISPLAY *crt_window = NULL;
static uint32 fade[512 * 512];
static uint32 dot[7 * 7];
t_stat crt_svc_stat;


/* Debug */
#define DBG             0001
#define DBG_DOT         0002

static UNIT crt_unit = {
  UDATA(&crt_svc, UNIT_IDLE, 0)
};

static DEBTAB crt_deb[] = {
  { "DBG",  DBG },
  { "DOT",  DBG_DOT },
  { "VVID", SIM_VID_DBG_VIDEO },
  { "KVID", SIM_VID_DBG_KEY },
  { NULL, 0 }
};

DEVICE crt_dev = {
  "CRT", &crt_unit, NULL, NULL,
  1, 8, 12, 1, 8, 12,
  NULL, NULL, &crt_reset,
  NULL, NULL, NULL,
  NULL, DEV_DISABLE | DEV_DEBUG, 0, crt_deb,
  NULL, NULL, NULL, NULL, NULL, NULL
};

static t_stat
crt_svc(UNIT *uptr)
{
  SIM_KEY_EVENT ev;
  if (crt_window != NULL) {
    vid_refresh_window(crt_window);
    vid_draw_window(crt_window, 0, 0, 512, 512, fade);
    if (vid_poll_kb(&ev) == SCPE_OK)
      kbd_event(&ev);
  }
  sim_activate_after(uptr, 50000);
  return crt_svc_stat;
}

static void crt_quit(void)
{
  crt_svc_stat = SCPE_STOP;
}

static t_stat
crt_reset(DEVICE *dptr)
{
  t_stat stat;
  int i, j;

  stat = vid_register_quit_callback (crt_quit);
  if (stat != SCPE_OK)
    return stat;

  if ((dptr->flags & DEV_DIS) != 0 || (sim_switches & SWMASK('P')) != 0) {
    if (crt_window != NULL)
      vid_close_window(crt_window);
    crt_window = NULL;
    sim_cancel(&crt_unit);
  } else if (crt_window == NULL) {
    stat = vid_open_window(&crt_window, dptr, "LINC display", 512, 512, 0);
    if (stat != SCPE_OK)
      return stat;
    if (crt_window == NULL)
      return SCPE_OPENERR;
    /* Allow time for window to open and data structures to settle. */
    sim_os_ms_sleep(100);
    stat = vid_set_alpha_mode(crt_window, SIM_ALPHA_BLEND);
    if (stat != SCPE_OK) {
      vid_close_window(crt_window);
      crt_window = NULL;
      return stat;
    }

    fade[0] = vid_map_rgba_window(crt_window, 0, 0, 0, 40);
    for (i = 1; i < 512 * 512; i++)
      fade[i] = fade[0];

    for (i = 0; i < 7; i++) {
      for (j = 0; j < 7; j++) {
        int dx = i - 3, dy = j - 3;
        int r2 = dx*dx + dy*dy;
        double focus = 0.7;
        double alpha = 0xFF*exp(-focus*r2) + .49;
        uint8 r = 220;
        uint8 g = 130;
        uint8 b = 0;
        dot[i + 7*j] = vid_map_rgba_window(crt_window, r, g, b, (uint8)alpha);
      }
    }

    sim_activate(&crt_unit, 1);
  }

  return SCPE_OK;
}

void
crt_point(uint16 x, uint16 y)
{
  sim_debug(DBG, &crt_dev, "Point %o,%o\n", x, y);
  if (crt_window) {
    vid_draw_window(crt_window, x - 3, 511 - y - 3, 7, 7, dot);
    if (crt_dev.dctrl & DBG_DOT)
      vid_refresh_window(crt_window);
  }
}

void crt_toggle_fullscreen(void)
{
  if (crt_window)
    vid_set_fullscreen_window(crt_window, !vid_is_fullscreen_window(crt_window));
}
