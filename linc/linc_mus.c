/* linc_mus.c: Audio output to speaker.

   Copyright (c) 2026, Lars Brinkhoff

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
#ifdef LINC_USE_AUDIO
#include <SDL_audio.h>

static t_stat mus_show_devices(FILE *st, UNIT *uptr, int32 val, CONST void *desc);
static t_stat mus_svc(UNIT *uptr);
static t_stat mus_reset(DEVICE *dptr);
static t_stat mus_attach(UNIT *uptr, CONST char *cptr);
static t_stat mus_detach(UNIT *uptr);

/* MUS calibrated timer. */
#define TMR_MUS  0
#define TMR_HZ   100

/* Audio switch.  The comments refer to the pins in the schematics. */
#define UNIT_V_OUTPUT  (UNIT_V_UF + 0)
#define UNIT_OUTPUT    (3 << UNIT_V_OUTPUT)
#define UNIT_S1        (0 << UNIT_V_OUTPUT)  /* 1013 Z25Z */
#define UNIT_Z11       (1 << UNIT_V_OUTPUT)  /* 1014 V4H */
#define UNIT_A11       (2 << UNIT_V_OUTPUT)  /* 1010 W1Z */

/* Debug */
#define DBG_DEVICE      0001 /* Audio device events. */
#define DBG_STAT        0002 /* Statistics every 1000 ms. */
#define DBG_SLEEP       0004 /* Sleep every 50 ms. */
#define DBG_QUEUE       0010 /* Queueing every 10 ms. */
#define DBG_SAMPLE      0020 /* Every sample. */

#define SDL_AUDIO_DEVICE_PLAYBACK  0

#define WANT_FREQUENCY  48000
#define WANT_SAMPLES    1024

#define QUEUE_MS          10  /* Queue 10 ms at a time. */
#define BUFFER_MAX_MS    150  /* Try to keep buffer below this. */
#define BUFFER_MIN_MS    100  /* Try to keep buffer above this. */
#define BUFFER_DROP_MS  1000  /* Drop samples if buffer exceeds this. */

/* Do not sleep longer than the CRT refresh rate, or else the CRT
   device will not be able to update the display smoothly. */
#if BUFFER_MAX_MS - BUFFER_MIN_MS > CRT_REFRESH_MS
#error MUS sleep time is incompatible with CRT refresh rate.
#endif

#define MIN(A, B)  ((A) < (B) ? (A) : (B))
#define MAX(A, B)  ((A) > (B) ? (A) : (B))

/* Default to Z0 since that's used by music software. */
static UNIT mus_unit = { UDATA(&mus_svc, UNIT_Z11 | UNIT_ATTABLE, 0) };

static MTAB mus_mod[] = {
  { MTAB_XTD|MTAB_VDV|MTAB_NMO, 0, "DEVICES", NULL, NULL,
    &mus_show_devices, NULL, "Display attachable audio devices" },
  { UNIT_OUTPUT, UNIT_S1,  "S1",  "S1",  NULL, NULL, "S bit 1" },
  { UNIT_OUTPUT, UNIT_Z11, "Z11", "Z11", NULL, NULL, "Z bit 11" },
  { UNIT_OUTPUT, UNIT_A11, "A11", "A11", NULL, NULL, "A bit 11" },
  { 0 }
};

static DEBTAB mus_deb[] = {
  { "DEVICE",  DBG_DEVICE },
  { "STAT",    DBG_STAT },
  { "SLEEP",   DBG_SLEEP },
  { "QUEUE",   DBG_QUEUE },
  { "SAMPLE",  DBG_SAMPLE },
  { NULL, 0 }
};

DEVICE mus_dev = {
  "MUS", &mus_unit, NULL, mus_mod,
  1, 8, 12, 1, 8, 12,
  NULL, NULL, &mus_reset,
  NULL, &mus_attach, &mus_detach,
  NULL, DEV_DISABLE | DEV_DIS | DEV_DEBUG, 0, mus_deb,
  NULL, NULL, NULL, NULL, NULL, NULL
};

static const char *mus_default = "default audio device";
static char mus_filename[CBUFSIZE];

static int frequency, queue_samples, device_samples;
static SDL_AudioDeviceID mus_audio = 0;
static float buffer_index;
static uint8 queue_buffer[WANT_FREQUENCY];
static int sleep_min = 10000, sleep_max = 0;

static t_stat mus_show_devices(FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
  const char *name;
  int i, n;

  n = SDL_GetNumAudioDevices(SDL_AUDIO_DEVICE_PLAYBACK);
  if (n < 0)
    return sim_messagef(SCPE_UNATT, "Error getting list of audio devices.\n");
  else if (n == 0)
    return sim_messagef(SCPE_OK, "No audio devices available.\n");

  for (i = 0; i < n; i++) {
    name = SDL_GetAudioDeviceName(i, SDL_AUDIO_DEVICE_PLAYBACK);
    fprintf(st, "  %s\n", name);
  }

  return SCPE_OK;
}

static t_stat mus_svc(UNIT *uptr)
{
  static int counter = 0;
  int32 t = sim_rtcn_calb_tick(TMR_MUS);
  Uint32 queued;

  if (++counter >= TMR_HZ) {
    counter = 0;
    queued = SDL_GetQueuedAudioSize(mus_audio);
    sim_debug(DBG_STAT, &mus_dev,
              "Queue %3ums; cycles %d/s; sleep %d-%dms.\n",
              1000 * queued / frequency, TMR_HZ * t, sleep_min, sleep_max);
    sleep_min = 10000;
    sleep_max = 0;
  }

  return sim_activate_after(uptr, 1000000/TMR_HZ);
}

static t_stat mus_reset(DEVICE *dptr)
{
  t_stat stat;

  if (dptr->flags & DEV_DIS) {
    mus_detach(&mus_unit);
  } else {
    if (sim_idle_enab)
      return sim_messagef(SCPE_OPENERR, "The MUS device does not work with idling.\n");
    if (sim_throt_enab())
      return sim_messagef(SCPE_OPENERR, "The MUS device does not work with throttling.\n");

    if (mus_audio == 0) {
      /* Attach the default audio output device. */
      stat = mus_attach(&mus_unit, NULL);
      if (stat != SCPE_OK)
        return stat;
    }

    /* Prime calibrated timer with expected cycle rate. */
    sim_rtcn_init_unit_ticks(&mus_unit, 125000/TMR_HZ, TMR_MUS, TMR_HZ);
  }

  return SCPE_OK;
}

static t_stat mus_attach(UNIT *uptr, CONST char *cptr)
{
  SDL_AudioSpec want, have;
  int error;

  if ((uptr->flags & UNIT_ATT) != 0 && uptr->filename != mus_default)
    return SCPE_ALATT;

  mus_detach(uptr);

  memset(&want, 0, sizeof want);
  want.freq = WANT_FREQUENCY;
  want.samples = WANT_SAMPLES;
  want.format = AUDIO_U8;
  want.channels = 1;

  error = SDL_InitSubSystem(SDL_INIT_AUDIO);
  if (error != 0)
    return sim_messagef(SCPE_OPENERR, "Error initializing audio: %s\n",
                        SDL_GetError());

  mus_audio = SDL_OpenAudioDevice(cptr, 0, &want, &have,
                            SDL_AUDIO_ALLOW_FREQUENCY_CHANGE |
                            SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
  if (mus_audio == 0)
    return sim_messagef(SCPE_OPENERR, "Error opening audio device: %s\n",
                        SDL_GetError());

  if (have.format != want.format)
    return sim_messagef(SCPE_OPENERR, "Did not get correct audio format.\n");

  frequency = have.freq;
  device_samples = have.samples;

  /* Queue 10 ms into the buffer each call. */
  queue_samples = QUEUE_MS * frequency / 1000;

  buffer_index = 0.0;

  uptr->flags |= UNIT_ATT;
  if (cptr == NULL) {
    uptr->filename = (char *)mus_default;
  } else {
    uptr->filename = mus_filename;
    strlcpy(uptr->filename, cptr, CBUFSIZE);
  }

  /* Queue up 100 ms to prime the buffer. */
  memset(queue_buffer, have.silence, 10 * queue_samples);
  SDL_QueueAudio(mus_audio, queue_buffer, 10 * queue_samples);

  sim_debug(DBG_DEVICE, &mus_dev,
            "Ready to play to %s at %d Hz, buffering %d samples.\n",
            uptr->filename, frequency, device_samples);

  return SCPE_OK;
}

static t_stat mus_detach(UNIT *uptr)
{
  if ((uptr->flags & UNIT_ATT) == 0)
    return SCPE_NOATT;

  if (mus_audio != 0) {
    uptr->flags &= ~UNIT_ATT;
    sim_cancel(&mus_unit);
    sim_debug(DBG_DEVICE, &mus_dev, "Closing %s.\n",
              uptr->filename);
    SDL_CloseAudioDevice(mus_audio);
    mus_audio = 0;
  }

  return SCPE_OK;
}

static void mus_throttle(Uint32 queued)
{
  int ms;

  if (1000 * queued < BUFFER_MAX_MS * frequency)
    return;

  /* Buffer exceeds 150 ms, sleep until there is 100 ms left. */
  ms = 1000 * queued / frequency - BUFFER_MIN_MS;
  sleep_min = MIN(ms, sleep_min);
  sleep_max = MAX(ms, sleep_max);
  sim_debug(DBG_SLEEP, &mus_dev, "Sleep %d ms.\n", ms);
  sim_os_ms_sleep(ms);
}

static void mus_queue()
{
  int n;

  if (!sim_is_active(&mus_unit)) {
    sim_activate(&mus_unit, 1);
    SDL_PauseAudioDevice(mus_audio, 0);
    sim_debug(DBG_DEVICE, &mus_dev, "Start playing to %s.\n",
              mus_unit.filename);
  }

  if (SDL_QueueAudio(mus_audio, queue_buffer, queue_samples) != 0) {
    sim_printf("SDL_QueueAudio error: %s\n", SDL_GetError());
    return;
  }

  sim_debug(DBG_QUEUE, &mus_dev, "Queue %d ms to buffer.\n",
            1000 * queue_samples / frequency);
  buffer_index -= queue_samples;

  /* Any overflow at the end is moved to the front. */
  n = buffer_index + .499;
  memcpy(queue_buffer, queue_buffer + queue_samples, n);
}

void mus_sample(uint16 A, uint16 S, uint16 Z, int32 cycles)
{
  Uint32 queued;
  uint16 bit;
  int i, n;

  if (mus_audio == 0)
    return;

  i = buffer_index + .499;
  buffer_index += cycles * frequency / 125000.0;
  n = buffer_index + .499;

  switch (mus_unit.flags & UNIT_OUTPUT) {
  case UNIT_S1:  bit = S & 00002; break;
  case UNIT_Z11: bit = Z & 04000; break;
  case UNIT_A11: bit = A & 04000; break;
  default:       bit = 0;         break;
  }
  bit = bit ? 0xFF : 0x00;
  memset(queue_buffer + i, bit, n - i);
  sim_debug(DBG_SAMPLE, &mus_dev, "Value %u for %d cycles\n", bit & 1, cycles);

  if (buffer_index > queue_samples - 0.5) {
    queued = SDL_GetQueuedAudioSize(mus_audio);
    /* Drop samples if buffer exceeds 1000 ms. */
    if (1000 * queued < BUFFER_DROP_MS * frequency)
      mus_queue();
    mus_throttle(queued);
  }
}

void mus_chime(void)
{
}

#endif /* LINC_USE_AUDIO */
