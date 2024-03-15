/* sim_video.c: Bitmap video output

   Copyright (c) 2011-2013, Matt Burke

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
   THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of the author shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from the author.

   08-Nov-2013  MB      Added globals for current mouse status
   11-Jun-2013  MB      First version
*/

#include "sim_video.h"
#include "scp.h"

#if defined(HAVE_LIBPNG) && defined(USE_SIM_VIDEO) && defined(HAVE_LIBSDL)
#include <png.h>
#endif

#if !defined(UNUSED_ARG)
/* Squelch compiler warnings about unused formal arguments. */
#define UNUSED_ARG(arg) (void)(arg)
#endif

static t_stat realloc_array(void **arr, size_t from, size_t to, size_t elem_size, const char *where);

int32 vid_cursor_x;
int32 vid_cursor_y;
t_bool vid_mouse_b1 = FALSE;
t_bool vid_mouse_b2 = FALSE;
t_bool vid_mouse_b3 = FALSE;

static VID_QUIT_CALLBACK *vid_quit_callbacks = NULL;
static size_t n_vid_quit_callbacks = 0;
static size_t alloc_vid_quit_callbacks = 0;

static VID_GAMEPAD_CALLBACK motion_callback[10];
static VID_GAMEPAD_CALLBACK button_callback[10];

t_stat vid_register_quit_callback (VID_QUIT_CALLBACK callback)
{
    t_stat retval = SCPE_OK;
    size_t i;

    for (i = 0; i < n_vid_quit_callbacks && vid_quit_callbacks[i] != callback; ++i)
        /* NOP */;

    if (i >= n_vid_quit_callbacks) {
        if (n_vid_quit_callbacks == alloc_vid_quit_callbacks) {
            retval = realloc_array((void **) &vid_quit_callbacks, alloc_vid_quit_callbacks,
                                alloc_vid_quit_callbacks + 4, sizeof(VID_QUIT_CALLBACK),
                                "vid_register_quit_callback");
            if (retval == SCPE_OK)
                alloc_vid_quit_callbacks += 4;
        }

        if (retval == SCPE_OK) {
            vid_quit_callbacks[n_vid_quit_callbacks++] = callback;
        }
    }

    return retval;
}

static t_stat register_callback (void **array, int n, void *callback)
{
    int i, j = -1;

    for (i = 0; i < n; i++) {
        if (array[i] == callback)
            return SCPE_ALATT;
        if (array[i] == NULL)
            j = i;
        }

    if (j != -1) {
        array[j] = callback;
        return SCPE_OK;
        }

    return SCPE_NXM;
}

t_stat vid_register_gamepad_motion_callback (VID_GAMEPAD_CALLBACK callback)
{
    int n = sizeof (motion_callback) / sizeof (callback);
    return register_callback ((void **)motion_callback, n, (void *)callback);
}

t_stat vid_register_gamepad_button_callback (VID_GAMEPAD_CALLBACK callback)
{
    int n = sizeof (button_callback) / sizeof (callback);
    return register_callback ((void **)button_callback, n, (void *)callback);
}

static t_stat realloc_array(void **arr, size_t from, size_t to, size_t elem_size, const char *where)
{
    void *p = realloc(*arr, to * elem_size);

    if (p != NULL) {
        memset(((uint8 *) p) + (from * elem_size), 0, (to - from) * elem_size);
        *arr = p;
    } else {
        free(*arr);
        *arr = NULL;
        return sim_messagef(SCPE_MEM,
                            "realloc_array (%s, from %" SIZE_T_FMT "u, to %" SIZE_T_FMT "u): "
                            "realloc() failed\n",
                            where, from, to);
    }

    return SCPE_OK;
}

t_stat vid_show (FILE* st, DEVICE *dptr,  UNIT* uptr, int32 val, const char* desc)
{
UNUSED_ARG(dptr);
return vid_show_video (st, uptr, val, desc);
}

#if defined(USE_SIM_VIDEO) && defined(HAVE_LIBSDL)

static const char *vid_dname (DEVICE *dev)
{
return dev ? sim_dname(dev) : "Video Device";
}

char vid_release_key[64] = "Ctrl-Right-Shift";

#include <SDL.h>
#include <SDL_thread.h>

static const char *key_names[] =
    {"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
     "0",   "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",
     "A",   "B",  "C",  "D",  "E",  "F",  "G",  "H",  "I",  "J",
     "K",   "L",  "M",  "N",  "O",  "P",  "Q",  "R",  "S",  "T",
     "U",   "V",  "W",  "X",  "Y",  "Z",
     "BACKQUOTE",   "MINUS",   "EQUALS", "LEFT_BRACKET", "RIGHT_BRACKET",
     "SEMICOLON", "SINGLE_QUOTE", "BACKSLASH", "LEFT_BACKSLASH", "COMMA",
     "PERIOD", "SLASH", "PRINT", "SCRL_LOCK", "PAUSE", "ESC", "BACKSPACE",
     "TAB", "ENTER", "SPACE", "INSERT", "DELETE", "HOME", "END", "PAGE_UP",
     "PAGE_DOWN", "UP", "DOWN", "LEFT", "RIGHT", "CAPS_LOCK", "NUM_LOCK",
     "ALT_L", "ALT_R", "CTRL_L", "CTRL_R", "SHIFT_L", "SHIFT_R",
     "WIN_L", "WIN_R", "MENU", "KP_ADD", "KP_SUBTRACT", "KP_END", "KP_DOWN",
     "KP_PAGE_DOWN", "KP_LEFT", "KP_RIGHT", "KP_HOME", "KP_UP", "KP_PAGE_UP",
     "KP_INSERT", "KP_DELETE", "KP_5", "KP_ENTER", "KP_MULTIPLY", "KP_DIVIDE"
     };

const char *vid_key_name (uint32 key)
{
static char tmp_key_name[40];

    if (key < sizeof(key_names)/sizeof(key_names[0]))
        sprintf (tmp_key_name, "SIM_KEY_%s", key_names[key]);
    else
        sprintf (tmp_key_name, "UNKNOWN KEY: %u", key);
    return tmp_key_name;
}

/* 32 bits per pixel. */
static const int SIMH_PIXEL_DEPTH = 32;
/* SIMH's pixel format (fewer magic numbers in the code.) */
const SDL_PixelFormatEnum SIMH_PIXEL_FORMAT = SDL_PIXELFORMAT_ARGB8888;

#if defined(HAVE_LIBPNG)
/* From: https://github.com/driedfruit/SDL_SavePNG */

/* Forward decl... */
static int SDL_SavePNG_RW(SDL_Surface *surface, SDL_RWops *dst, int freedst);

/*
 * Save an SDL_Surface as a PNG file.
 *
 * Returns 0 success or -1 on failure, the error message is then retrievable
 * via SDL_GetError().
 */
static inline int SDL_SavePNG(SDL_Surface *surface, const char *file)
{
    return SDL_SavePNG_RW(surface, SDL_RWFromFile(file, "wb"), 1);
}

/*
 * SDL_SavePNG -- libpng-based SDL_Surface writer.
 *
 * This code is free software, available under zlib/libpng license.
 * http://www.libpng.org/pub/png/src/libpng-LICENSE.txt
 */
#include <SDL.h>
#include <zlib.h>

#define SUCCESS 0
#define ERROR -1

#define USE_ROW_POINTERS

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define rmask 0xFF000000
#define gmask 0x00FF0000
#define bmask 0x0000FF00
#define amask 0x000000FF
#else
#define rmask 0x000000FF
#define gmask 0x0000FF00
#define bmask 0x00FF0000
#define amask 0xFF000000
#endif

/* libpng callbacks */
static void png_error_SDL(png_structp ctx, png_const_charp str)
{
    UNUSED_ARG(ctx);
    SDL_SetError("libpng: %s\n", str);
}
static void png_write_SDL(png_structp png_ptr, png_bytep data, png_size_t length)
{
    SDL_RWops *rw = (SDL_RWops*)png_get_io_ptr(png_ptr);
    SDL_RWwrite(rw, data, sizeof(png_byte), length);
}

static int SDL_SavePNG_RW(SDL_Surface *surface, SDL_RWops *dst, int freedst)
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_colorp pal_ptr;
    SDL_Palette *pal;
    int i, colortype;
#ifdef USE_ROW_POINTERS
    png_bytep *row_pointers;
#endif
    /* Initialize and do basic error checking */
    if (dst == NULL)
    {
        SDL_SetError("Argument 2 to SDL_SavePNG_RW can't be NULL, expecting SDL_RWops*\n");
        return (ERROR);
    }
    if (surface == NULL)
    {
        SDL_SetError("Argument 1 to SDL_SavePNG_RW can't be NULL, expecting SDL_Surface*\n");
        if (freedst) SDL_RWclose(dst);
        return (ERROR);
    }
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, png_error_SDL, NULL); /* err_ptr, err_fn, warn_fn */
    if (png_ptr == NULL)
    {
        SDL_SetError("Unable to png_create_write_struct on %s\n", PNG_LIBPNG_VER_STRING);
        if (freedst) SDL_RWclose(dst);
        return (ERROR);
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        SDL_SetError("Unable to png_create_info_struct\n");
        png_destroy_write_struct(&png_ptr, NULL);
        if (freedst) SDL_RWclose(dst);
        return (ERROR);
    }
    if (setjmp(png_jmpbuf(png_ptr)))    /* All other errors, see also "png_error_SDL" */
    {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        if (freedst) SDL_RWclose(dst);
        return (ERROR);
    }

    /* Setup our RWops writer */
    png_set_write_fn(png_ptr, dst, png_write_SDL, NULL); /* w_ptr, write_fn, flush_fn */

    /* Prepare chunks */
    colortype = PNG_COLOR_MASK_COLOR;
    if (surface->format->BytesPerPixel > 0
    &&  surface->format->BytesPerPixel <= 8
    && (pal = surface->format->palette) != NULL)
    {
        colortype |= PNG_COLOR_MASK_PALETTE;
        pal_ptr = (png_colorp) malloc(pal->ncolors * sizeof(png_color));
        for (i = 0; i < pal->ncolors; i++) {
            pal_ptr[i].red   = pal->colors[i].r;
            pal_ptr[i].green = pal->colors[i].g;
            pal_ptr[i].blue  = pal->colors[i].b;
        }
        png_set_PLTE(png_ptr, info_ptr, pal_ptr, pal->ncolors);
        free(pal_ptr);
    }
    else if (surface->format->BytesPerPixel > 3 || surface->format->Amask)
        colortype |= PNG_COLOR_MASK_ALPHA;

    png_set_IHDR(png_ptr, info_ptr, surface->w, surface->h, 8, colortype,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

//    png_set_packing(png_ptr);

    /* Allow BGR surfaces */
    if (surface->format->Rmask == bmask
    && surface->format->Gmask == gmask
    && surface->format->Bmask == rmask)
        png_set_bgr(png_ptr);

    /* Write everything */
    png_write_info(png_ptr, info_ptr);
#ifdef USE_ROW_POINTERS
    row_pointers = (png_bytep*) malloc(sizeof(png_bytep)*surface->h);
    for (i = 0; i < surface->h; i++)
        row_pointers[i] = (png_bytep)(Uint8*)surface->pixels + i * surface->pitch;
    png_write_image(png_ptr, row_pointers);
    free(row_pointers);
#else
    for (i = 0; i < surface->h; i++)
        png_write_row(png_ptr, (png_bytep)(Uint8*)surface->pixels + i * surface->pitch);
#endif
    png_write_end(png_ptr, info_ptr);

    /* Done */
    png_destroy_write_struct(&png_ptr, &info_ptr);
    if (freedst) SDL_RWclose(dst);
    return (SUCCESS);
}
#endif /* defined(HAVE_LIBPNG) */


#define MAX_EVENTS       20                   /* max events in queue */

typedef struct KeyEventQueue_s {
    SIM_KEY_EVENT events[MAX_EVENTS];
    SDL_sem *sem;
    int32 head;
    int32 tail;
    int32 count;
    } KEY_EVENT_QUEUE;

typedef struct MouseEventQueue_s {
    SIM_MOUSE_EVENT events[MAX_EVENTS];
    SDL_sem *sem;
    int32 head;
    int32 tail;
    int32 count;
    } MOUSE_EVENT_QUEUE;

/* SDL Audio/SIMH beep data */
#include <SDL_audio.h>
#include <math.h>

/* Beep output states:
 *
 * BEEP_IDLE is the starting state (waiting to beep)
 * START_BEEP is the trigger to start the output, transitions into OUTPUT_TONE
 * OUTPUT_TONE should be self explanitory. If there are pending tones, transitions
 *    into OUTPUT_GAP
 * OUTPUT_GAP emits a small amount of silence between pending tones.
 * BEEP_DONE: Final state, stops audio output, transitions to BEEP_IDLE
 **/
typedef enum
{
    BEEP_IDLE,
    START_BEEP,
    OUTPUT_TONE,
    OUTPUT_GAP,
    BEEP_DONE
} SimBeepState;

/* SDL audio context structure for SIMH: */
typedef struct SimBeepContext {
    SimBeepState beep_state;

    int16 *beep_data;                   /* The tone's samples */
    size_t beep_samples;                /* Number of samples in beep_data*/
    int beep_duration;                  /* Beep's duration, ms */
    int gap_samples;                    /* Number of samples in silence gap */
    int sdl_samples;                    /* Number of samples reported by SDL */

    /* Offset into beep_data from whence samples are copied. SDL's buffer is
     * likely to be smaller, need to keep track of where we're playing out. */
    int beep_offset;

    /* If consecutive beep tone requests arrived during playout, keep track of
     * them. This tells the vid_audio_callback() state machine to continue
     * playback, possibly inserting a silence gap. */
    SDL_atomic_t beeps_pending;
} SimBeepContext;

#if SDL_VERSION_ATLEAST(2, 0, 6)
SDL_AudioDeviceID vid_audio_dev = 0;
#endif

/* The beep context... */
SimBeepContext beep_ctx;

static void unpause_audio();
static void pause_audio();
static void close_audio();

static void vid_beep_setup (int duration_ms, int tone_frequency);
static void vid_beep_cleanup (void);

static void vid_audio_callback(void *ctx, Uint8 *stream, int length);

/* Manage a draw_op. */
static inline int set_draw_op(VID_DISPLAY *vptr, size_t idx, const uint32 *buf, int32 x, int32 y, int32 w, int32 h);

/* Drawing operation (bit blit) state. */
typedef struct DrawOp {
  SDL_Rect blit_rect;
  uint32 *blit_data;
  size_t n_blit_data;
  size_t alloc_blit_data;
  SDL_atomic_t in_use;
} DrawOp;

typedef struct DrawOnto {
    SDL_Rect onto_rect;
    VID_DRAW_ONTO_CALLBACK draw_fn;
    void *draw_data;
    SDL_atomic_t pending;
} DrawOnto;

/* Hardware (accelerated) vs. software rendering. The default is software
 * rendering, but can be switched to hardware. */
typedef enum {
    RENDER_MODE_HW,
    RENDER_MODE_SW
} SimhRenderMode;

/* VID_DISPLAY: The display-side of the simulator. */
struct VID_DISPLAY {
    t_bool vid_active_window;
    t_bool vid_mouse_captured;
    uint32 vid_flags;                                   /* Open Flags */
    int32 vid_width;
    int32 vid_height;
    SDL_atomic_t vid_ready;
    char vid_title[128];
    SDL_Renderer *renderer;                             /* Accelerated (H/W) renderer*/
    SDL_Texture *texture;                               /* video buffer in GPU */
    SDL_Window *vid_window;                             /* window handle */
    SDL_PixelFormat *vid_format;
    uint32 vid_windowID;
    SDL_Cursor *vid_cursor;                             /* current cursor */
    t_bool vid_cursor_visible;                          /* cursor visibility state */
    DEVICE *vid_dev;
    t_bool vid_key_state[SDL_NUM_SCANCODES];
    VID_DISPLAY *next;                                  /* Next VID_DISPLAY if multi-headed. */
    t_bool vid_blending;
    SDL_Rect vid_rect;                                  /* Window dimensions */

    /*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
     * EVENT_DRAW state:
     *=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/

    SDL_mutex *draw_mutex;                              /* Window update mutex */

    DrawOp *draw_ops;                                   /* Pending blit array. */
    size_t alloc_draw_ops;                              /* Allocated size of draw_ops */

    DrawOnto *onto_ops;                                 /* Direct-to-texture blits */
    size_t alloc_onto_ops;                              /* Allocated size of onto_ops */
    size_t max_onto_ops;                                /* High water mark */

    /*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
     * Event helper data, to avoid additional helper structures.
     * EVENT_CURSOR and EVENT_FULLSCREEEN are one-shot events.
     *=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/

    /* SIMH_EVENT_CURSOR helper data: */
    SDL_Cursor *cursor_data;
    t_bool cursor_visible;

    /* SIMH_EVENT_FULLSCREEN data: */
    t_bool fullscreen_flag;
};

/* Default rendering mode -- RENDER_MODE_SW is the default. "SET VIDEO NATIVE"
 * before the first window will set the default to RENDER_MODE_HW. */
static SimhRenderMode vid_render_mode = RENDER_MODE_SW;

/* Internal constants: */
/* (sigh) C -- initialize constants with values... */
#define INVALID_SDL_EVENT ((Uint32) -1)

/* INITIAL_DRAW_OPS: For the PDP-11 VT "lunar lander" demo mode, there is only
 * one outstanding draw operation. Leave some headroom for other displays and
 * simulators (but realistically, there's only likely to be one active draw op.) */
static const size_t      INITIAL_DRAW_OPS  = 8;
static const size_t      INITIAL_ONTO_OPS  = 8;

/* Audio (beep) constants: */
static const int AMPLITUDE           = 20000;
static const int SAMPLE_FREQUENCY    = 11025;
static const int BEEP_TONE_FREQ      = 330 /* hz */;
/* Silence gap for consecutive beeps. */
static const int BEEP_EVENT_GAP      = 75 /* ms */;

static SDL_atomic_t vid_active;                         /* == 0 -> inactive, crosses threads */

/* Linked list of video displays. Note that this used to be vid_first. It is now
 * a pointer, where vid_first is indirectly referenced as the first element of
 * the list. */
static VID_DISPLAY *sim_displays = NULL;
/* Mutex for operations on vid_first itself or subsidiary displays. */
static SDL_mutex *sim_displays_mutex = NULL;

static KEY_EVENT_QUEUE vid_key_events;                  /* keyboard events */
static MOUSE_EVENT_QUEUE vid_mouse_events;              /* mouse events */

/* Custom SDL user events: */
static void assign_user_event_ids();

static void null_simh_event (const SDL_Event *ev);
static void simh_close_event (const SDL_Event *ev);
static void simh_update_cursor (const SDL_Event *ev);
static void simh_warp_position (const SDL_Event *ev);
static void simh_draw_event (const SDL_Event *ev);
static void simh_open_event (const SDL_Event *ev);
static void simh_open_complete (const SDL_Event *ev);
static void simh_beep_event (const SDL_Event *ev);
static void simh_size_event(const SDL_Event *ev);
static void simh_show_video_event (const SDL_Event *ev);
static void simh_screenshot_event (const SDL_Event *ev);
static void simh_fullscreen_event(const SDL_Event *ev);
static void simh_set_logical_size(const SDL_Event *ev);
static void simh_switch_renderer(const SDL_Event *ev);
static void simh_draw_onto(const SDL_Event *ev);

typedef enum {
    /* Dispatchable events: */
    SIMH_EVENT_CLOSE,                                   /* Close event for SDL */
    SIMH_EVENT_CURSOR,                                  /* New cursor for SDL */
    SIMH_EVENT_WARP,                                    /* Warp mouse position for SDL */
    SIMH_EVENT_DRAW,                                    /* Draw/blit region for SDL */
    SIMH_EVENT_OPEN,                                    /* vid_open request */
    SIMH_EVENT_OPENCOMPLETE,                            /* vid_open complete */
    SIMH_EVENT_BEEP,                                    /* Audio beep ("Ring my bell!") */
    SIMH_EVENT_SIZE,                                    /* Set window size. */
    SIMH_EVENT_SHOW,                                    /* Show SDL capabilities */
    SIMH_EVENT_SCREENSHOT,                              /* Capture screenshot of video window */
    SIMH_EVENT_FULLSCREEN,                              /* Go fullscreen */
    SIMH_EVENT_LOGICAL_SIZE,                            /* Set window's logical size */
    SIMH_SWITCH_RENDERER,                               /* Update hardware texture from S/W renderer */
    SIMH_DRAW_ONTO,                                     /* Draw region from SDL main thread. */

    /* One very special event... */
    SIMH_EXIT_EVENT,                                    /* Exit event */

    /* Useful limit constant. */
    N_SIMH_EVENTS,

    /* Useful constants: */
    FIRST_SIMH_DISPATCH = SIMH_EVENT_CLOSE,
    LAST_SIMH_DISPATCH = SIMH_DRAW_ONTO
} SIMHEvents;


/* SIMH event dispatch array. */
struct {
    Uint32 event_id;                                    /* SDL event identifier */
    const char *description;                            /* Debugging description */
    void (*simh_event)(const SDL_Event *ev);            /* Handler function */
} simh_event_dispatch[] = {
    { INVALID_SDL_EVENT, "SIMH close",  simh_close_event },
    { INVALID_SDL_EVENT, "SIMH update cursor", simh_update_cursor },
    { INVALID_SDL_EVENT, "SIMH warp cursor", simh_warp_position },
    { INVALID_SDL_EVENT, "SIMH draw", simh_draw_event },
    { INVALID_SDL_EVENT, "SIMH window open", simh_open_event },
    { INVALID_SDL_EVENT, "SIMH open completed", simh_open_complete },
    { INVALID_SDL_EVENT, "SIMH beep", simh_beep_event },
    { INVALID_SDL_EVENT, "SIMH window size", simh_size_event },
    { INVALID_SDL_EVENT, "SIMH show video", simh_show_video_event },
    { INVALID_SDL_EVENT, "SIMH screenshot", simh_screenshot_event },
    { INVALID_SDL_EVENT, "SIMH fullscreen", simh_fullscreen_event },
    { INVALID_SDL_EVENT, "SIMH set logical size", simh_set_logical_size },
    { INVALID_SDL_EVENT, "SIMH switch renderer", simh_switch_renderer },
    { INVALID_SDL_EVENT, "SIMH draw onto", simh_draw_onto },

    /* Need the event identifer for SIMH_EXIT_EVENT. The event function
     * shouldn't be NULL, although it'll never get called. */
    { INVALID_SDL_EVENT, "SIMH exit", null_simh_event },
};

/* Screenshot state: */
typedef struct SimScreenShot {
    SDL_cond *screenshot_cond;
    SDL_mutex *screenshot_mutex;
    SDL_atomic_t screenshot_stat;
    const char *filename;
} SimScreenShot_t;

/* SDL event loop side screenshot workhorse. */
static t_stat do_screenshot (VID_DISPLAY *vptr, const char *filename);

/* SIMH_EVENT_SHOW state ("show video" command output generator). All
 * of the data SIMH wants to show is generated by the SDL main thread. */
typedef struct ShowVideoCmd
{
    SDL_cond *show_cond;
    SDL_mutex *show_mutex;
    SDL_atomic_t show_stat;
    FILE *show_stream;
    UNIT *show_uptr;
    int32 show_val;
    const void *show_desc;
} ShowVideoCmd_t;

static t_stat show_sdl_info(FILE *st, UNIT *uptr, int32 val, const void *desc);

/* Debugging pretty printers: */
static void initWindowEventNames();
static const char *getWindowEventName(Uint32 win_event);
static void initEventNames();
static const char *getEventName(Uint32 ev);
static const char *getUserCodeName(Uint32 user_ev_code);
static const char *getBeepStateName(SimBeepState state);

static const char *eventtypes[SDL_LASTEVENT];
static const char *windoweventtypes[256];

static const char *beep_state_names[] = {
    "BEEP_IDLE",
    "START_BEEP",
    "OUTPUT_TONE",
    "OUTPUT_GAP",
    "BEEP_DONE"
};

/* Condition and mutex needed by SIMH_EVENT_OPENCOMPLETE to signal that
 * vid_video_events() or SIMH_EVENT_OPEN has completed. */
typedef struct open_complete_t {
    SDL_cond *completed_cond;
    SDL_mutex *completed_mutex;
} open_complete_t;

static open_complete_t did_opencomplete;

/* Deferred SDL startup: SIMH needs to defer entering the SDL event loop for as
 * long as possible. Video displays are the exception, not the rule. SDL's
 * SDL_WaitEvent() polls its event queue at 1ms intervals, so the objective is
 * to not penalize a simulator that's not actively using a display, e.g., PDP-11
 * running RSX or 3b2 running SYSVr3.
 *
 * At the same time, the CLI executes inside its own thread and has to send a
 * message to SDL via the event loop that it has terminated so that main() exits
 * properly.
 *
 * - main_event_cond, main_event_mutex: This is main()'s condition. main() will
 *   wait for main_event_cond to signal. See vid_open_window(), vid_open() and
 *   vid_init() -- they call invoke do_deferred_initialization().
 *
 * - sdl_initialized_cond, sdl_initialized_mutex: do_deferred_initialization()
 *   acquires the condition and waits until main() signals the condition.
 *   sdl_initialized_cond signals that main() has entered the event loop.
 *
 * This is all handled by a three states:
 *
 * - FULL_INITIALIZATION state: Everything needed by a video display. This is
 *   the default initialization value (see initialize_deferred_state())
 *
 * - EVENT_ONLY_INITIALIZATION state: Just the SDL event system. If the SDL
 *   event loop hasn't been entered, maybe_event_only_initialization() will set
 *   the startup state to this value so that SDL only initializes its event
 *   system.
 *
 * - STARTUP_COMPLETE state: Deferred initialization completed. This state
 *   executes null (nop) functions.
 *
 *   deferred_startup[STARTUP_COMPLETE].startup_done() turns into a no-op at the
 *   top of the main() event loop.
 */

typedef enum {
    FULL_INITIALIZATION,
    EVENT_ONLY_INITIALIZATION,
    STARTUP_COMPLETE,
} DeferredStartupState;

/* Data to pass to main_thread. */
typedef struct simulator_thread_args_s {
    int           argc;
    char        **argv;
} simulator_thread_args_t;

static SDL_atomic_t  startup_state;
static SDL_cond     *main_event_cond;
static SDL_mutex    *main_event_mutex;
static SDL_cond     *sdl_initialized_cond;
static SDL_mutex    *sdl_initialized_mutex;

/* A device simulator can optionally set the vid_display_kb_event_process
 * routine pointer to the address of a routine.
 * Simulator code which uses the display library which processes window
 * keyboard data with code in display/sim_ws.c can use this routine to
 * explicitly get access to keyboard events that arrive in the display
 * window.  This routine should return 0 if it has handled the event that
 * was passed, and non zero if it didn't handle it.  If the routine address
 * is not set or a non zero return value occurs, then the keyboard event
 * will be processed by the display library which may then be handled as
 * console character input if the device console code is implemented to
 * accept this.
 */
int (*vid_display_kb_event_process)(SIM_KEY_EVENT *kev) = NULL;

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * Startup/initialization:
 *=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

/* Forward decl... */
static void sim_video_dealloc_resources();

/* Null/NOP functions */

static int null_startup()
{
    return 0;
}

static void null_startup_done()
{ /* NOP */ }

static void sim_video_init_done()
{
    SDL_LockMutex(sdl_initialized_mutex);

    /* Transition to STARTUP_COMPLETE. */
    SDL_AtomicSet(&startup_state, STARTUP_COMPLETE);

    if (sdl_initialized_cond != NULL)
        SDL_CondSignal(sdl_initialized_cond);

    SDL_UnlockMutex(sdl_initialized_mutex);
}

static void simh_sdl_common_initialization()
{
    initWindowEventNames();
    initEventNames();
    assign_user_event_ids();
}

/* SIMH SDL2 initialization. */
static int sim_video_system_init()
{
    int retval;

    simh_sdl_common_initialization();
    SDL_AtomicSet(&vid_active, 0);

    retval = SDL_Init(SDL_INIT_EVERYTHING);
    if (retval >= 0) {
        sim_displays_mutex = SDL_CreateMutex();
        did_opencomplete.completed_cond = SDL_CreateCond();
        did_opencomplete.completed_mutex = SDL_CreateMutex();

        /* Yes, this is a clever way to set retval = -1 if any of the resources
         * weren't acquired without an 'if' statement. Boolean expressions
         * evaluate to 0 (false) or 1 (true). */
        retval = -1 * (sim_displays_mutex == NULL
                       || did_opencomplete.completed_cond == NULL
                       || did_opencomplete.completed_mutex == NULL);

        if (retval >= 0) {
            /* Audio system setup: */
            SDL_AudioSpec desiredSpec, actualSpec;

            /* Set up the beep context */
            beep_ctx.beep_state = BEEP_IDLE;
            beep_ctx.beep_data = NULL;
            beep_ctx.beep_samples = 0;
            beep_ctx.beep_duration = BEEP_LONG;
            beep_ctx.beep_offset = 0;
            SDL_AtomicSet(&beep_ctx.beeps_pending, 0);

            memset (&desiredSpec, 0, sizeof(desiredSpec));
            desiredSpec.freq = SAMPLE_FREQUENCY;
            desiredSpec.format = AUDIO_S16SYS;
            desiredSpec.channels = 1;
            desiredSpec.samples = 2048;
            desiredSpec.callback = vid_audio_callback;
            desiredSpec.userdata = &beep_ctx;

#if SDL_VERSION_ATLEAST(2, 0, 6)
            vid_audio_dev = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, &actualSpec, SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
            if (vid_audio_dev == 0) {
                sim_printf ("Error acquiring audio device, no audio output: %s\n", SDL_GetError());
                --retval;
            }
#else
            SDL_OpenAudio(&desiredSpec, NULL);
#endif

            if (!retval) {
                /* SDL will tell us how large the sample buffer that we need to fill will be... */
                beep_ctx.sdl_samples = actualSpec.samples;
                /* Generate the tone data. */
                vid_beep_setup (BEEP_LONG, BEEP_TONE_FREQ);

                /* Joystick/gamepad initialization */
                SDL_version ver;

                /* Check that the SDL_GameControllerFromInstanceID function is
                available at run time. */
                SDL_GetVersion(&ver);

                if (SDL_JoystickEventState (SDL_ENABLE) < 0) {
                    sim_printf ("sim_video_system_init: SDL_JoystickEventState error: %s\n", SDL_GetError());
                    --retval;
                }

                if (!retval && SDL_GameControllerEventState (SDL_ENABLE) < 0) {
                    sim_printf ("sim_video_system_init: SDL_GameControllerEventState error: %s\n", SDL_GetError());
                    --retval;
                }

                if (!retval) {
                    SDL_Joystick *y;
                    int i, n;

                    n = SDL_NumJoysticks();

                    for (i = 0; i < n; i++) {
                        if (SDL_IsGameController (i)) {
                            const SDL_GameController *x = SDL_GameControllerOpen (i);
                            if (x != NULL && sim_deb != NULL) {
                                sim_printf ("Game controller: %s\n", SDL_GameControllerNameForIndex(i));
                            }
                        } else {
                            y = SDL_JoystickOpen (i);
                            if (y != NULL && sim_deb != NULL) {
                                sim_printf ("Joystick %s: Number of axes: %d, buttons: %d\n", SDL_JoystickNameForIndex(i),
                                            SDL_JoystickNumAxes(y), SDL_JoystickNumButtons(y));
                            }
                        }
                    }
                }
            }
        }
    }

    /* Deal with errors... */
    if (retval < 0) {
        sim_video_dealloc_resources();
    }

    return retval;
}

/* Minimalist initialization that only activates the event loop. */
static int sim_event_only_init()
{
    simh_sdl_common_initialization();
    SDL_AtomicSet(&vid_active, 0);

    return SDL_Init(SDL_INIT_EVENTS);
}

static void sim_video_dealloc_resources()
{
    if (sim_displays_mutex != NULL)
        SDL_DestroyMutex(sim_displays_mutex);
    if (did_opencomplete.completed_cond != NULL)
        SDL_DestroyCond(did_opencomplete.completed_cond);
    if (did_opencomplete.completed_mutex != NULL)
        SDL_DestroyMutex(did_opencomplete.completed_mutex);

    did_opencomplete.completed_cond = NULL;
    did_opencomplete.completed_mutex = NULL;

    /* Joysticks, gamepad. */
    memset (motion_callback, 0, sizeof motion_callback);
    memset (button_callback, 0, sizeof button_callback);

}

/* And run the process in reverse... */
static void sim_video_system_cleanup()
{
    sim_video_dealloc_resources();
    vid_beep_cleanup ();
}

static void maybe_event_only_initialization()
{
    if (SDL_AtomicGet(&startup_state) != STARTUP_COMPLETE) {
        SDL_AtomicSet(&startup_state, EVENT_ONLY_INITIALIZATION);
    }
}

static void do_deferred_initialization()
{
    SDL_LockMutex(sdl_initialized_mutex);

    if (SDL_AtomicGet(&startup_state) != STARTUP_COMPLETE) {
        /* Signal the waiting main thread */
        if (main_event_cond != NULL) {
            SDL_LockMutex(main_event_mutex);
            SDL_CondSignal(main_event_cond);
            SDL_UnlockMutex(main_event_mutex);
        }

        /* Wait for the signal that main() is in the event queue...
         *
         * Defensive code: OS thread scheduling can prevent us from receiving
         * the condition variable's signal, i.e., main() executes the
         * SDL_CondSignal(), but we're not actually waiting for it yet.
         */
        while (SDL_AtomicGet(&startup_state) != STARTUP_COMPLETE) {
            SDL_CondWait(sdl_initialized_cond, sdl_initialized_mutex);
        }
    }

    SDL_UnlockMutex(sdl_initialized_mutex);
}

static int initialize_deferred_state()
{
    int retval = 0;

    SDL_AtomicSet(&startup_state, FULL_INITIALIZATION);

    if ((main_event_cond = SDL_CreateCond()) == NULL) {
        fprintf (stderr, "SDL_CreateCond(main_event_cond) failed: %s\n", SDL_GetError ());
        ++retval;
    }

    if (!retval && (main_event_mutex = SDL_CreateMutex()) == NULL) {
        fprintf (stderr, "SDL_CreateMutex(main_event_mutex) failed: %s\n", SDL_GetError ());
        ++retval;
    }

    if (!retval && (sdl_initialized_cond = SDL_CreateCond()) == NULL) {
        fprintf (stderr, "SDL_CreateCond(sdl_initialized_cond) failed: %s\n", SDL_GetError ());
        ++retval;
    }

    if (!retval && (sdl_initialized_mutex = SDL_CreateMutex()) == NULL) {
        fprintf (stderr, "SDL_CreateMutex(sdl_initialized_mutex) failed: %s\n", SDL_GetError ());
        ++retval;
    }

    return retval;
}

static inline int deferred_startup_completed()
{
    return (SDL_AtomicGet(&startup_state) == STARTUP_COMPLETE);
}

/* The dispatch function array corresponding to the deferred startup state machine: */
struct {
    int (*do_startup)();
    void (*startup_done)();
} deferred_startup[] = {
    { sim_video_system_init, sim_video_init_done },  /* FULL_INITIALIZATION */
    { sim_event_only_init, sim_video_init_done },    /* EVENT_ONLY_INITIALIZATION */
    { null_startup, null_startup_done },             /* STARTUP_COMPLETE */
};

static void exec_deferred_startup()
{
    DeferredStartupState current_sstate = SDL_AtomicGet(&startup_state);

    if (current_sstate < FULL_INITIALIZATION || current_sstate > STARTUP_COMPLETE) {
        fprintf(stderr, "exec_deferred_startup: Illegal startup transition state: %d.\n", current_sstate);
        fprintf(stderr, "Exiting.\n");
        exit(1);
    }

    if (deferred_startup[current_sstate].do_startup() < 0) {
        fprintf(stderr, "Deferred initialization failed: %s\n", SDL_GetError());
        fprintf(stderr, "Exiting.\n");
        exit(1);
    }
}

static void exec_startup_complete()
{
    DeferredStartupState current_sstate = SDL_AtomicGet(&startup_state);

    /* Now that we're in the event loop, execute any post-initialization code. */
    if (current_sstate < FULL_INITIALIZATION || current_sstate > STARTUP_COMPLETE) {
        fprintf(stderr, "exec_startup_complete: Illegal startup transition state: %d\n", current_sstate);
        fprintf(stderr, "Exiting.\n");
        exit(1);
    }

    deferred_startup[current_sstate].startup_done();
}

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * Utility functions:
 *=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

/* Accessor function for vid_active.
 *
 * vid_active is used across threads, necessitating atomic operations (important
 * on AARCH64.)
 */
int vid_is_active()
{
  return SDL_AtomicGet(&vid_active);
}

static void assign_user_event_ids()
{
    /* SDL_RegisterEvents() returns the first of registered event identifiers */
    const size_t needed_ids = sizeof(simh_event_dispatch) / sizeof(simh_event_dispatch[0]);
    Uint32       event_id = SDL_RegisterEvents((int) needed_ids);
    size_t       i;

    for (i = 0; i < needed_ids; ++i, ++event_id) {
        simh_event_dispatch[i].event_id = event_id;
        eventtypes[event_id] = simh_event_dispatch[i].description;
    }
}

/* Convert SIMHEvent to its corresponding SDL event identifier. Returns INVALID_SDL_EVENT
 * if the event_enum is invalid. */
static inline Uint32 evidentifier_for(SIMHEvents event_enum)
{
    if (event_enum >= FIRST_SIMH_DISPATCH && event_enum < N_SIMH_EVENTS)
        return simh_event_dispatch[event_enum].event_id;

    sim_messagef(SCPE_NXM, "evidentifier_for(%d/0x%04x): Unknown event identifier.\n", event_enum, event_enum);
    return INVALID_SDL_EVENT;
}

/* Return the default "first" display. */
static inline VID_DISPLAY *vid_first()
{
    ASSURE(sim_displays != NULL);
    return sim_displays;
}

/* Translate a SDL window ID into a VID_DISPLAY. */
static VID_DISPLAY *vid_get_event_window(Uint32 windowID)
{
    VID_DISPLAY *vptr;

    /* Ensure no changes to the linked list whilst we're traversing it. */
    SDL_LockMutex(sim_displays_mutex);

    for (vptr = sim_displays; vptr != NULL && windowID != vptr->vid_windowID; vptr = vptr->next)
        /* NOP */;

    SDL_UnlockMutex(sim_displays_mutex);

    return vptr;
}

/* */
static void set_window_scale(VID_DISPLAY *vptr, int x, int y, int w, int h)
{
    /* Don't do this unless SDL can scale the simulator's surface to the
     * display's texture buffer. */
#if SDL_VERSION_ATLEAST(2, 0, 12)
    float adjusted_w = ((float) w) / ((float) vptr->vid_width);
    float adjusted_h = ((float) h) / ((float) vptr->vid_height);
    SDL_SetWindowPosition(vptr->vid_window, x, y);
    SDL_SetWindowSize(vptr->vid_window, w, h);
    SDL_RenderSetScale(vptr->renderer, adjusted_w, adjusted_h);

    /* Scaling mode: This needs some twiddling. */
    SDL_SetTextureScaleMode(vptr->texture, SDL_ScaleModeBest);
#endif
}

/* Rescale the window if it doesn't fit in the physical display. */
static t_stat rescale_window(VID_DISPLAY *vptr)
{
    int border_top, border_left, border_bottom, border_right;
    /* Simulator's original/expected aspect ratio. */
    SDL_Rect usable = { 0, 0, 0, 0 };
    const char *syndrome = NULL;
    int got_syndrome = 0;
    t_stat retval = SCPE_OK;

    /* Don't let the window exceed the size of the actual screen. */
    if (SDL_GetWindowBordersSize(vptr->vid_window, &border_top, &border_left, &border_bottom, &border_right) != 0) {
        syndrome = "SDL_GetWindowBorderSize";
        ++got_syndrome;
    }

    if (!got_syndrome && SDL_GetDisplayUsableBounds(SDL_GetWindowDisplayIndex(vptr->vid_window), &usable) != 0) {
        syndrome = "SDL_GetDisplayUsableBounds";
        ++got_syndrome;
    }

    if (!got_syndrome) {
        /* SDL can rescale the window to the display automagically. */
        int window_height, window_width;

        SDL_GetWindowSize(vptr->vid_window, &window_width, &window_height);

#if SDL_VERSION_ATLEAST(2, 0, 12)
        int changed = 0;
        int window_y, window_x;
        double aspect = (double) vptr->vid_height / (double) vptr->vid_width;

        /* Usually, it's the height that exceeds the physical display. */
        if (window_height > usable.h - border_top - border_bottom) {
            window_height = usable.h - border_top - border_bottom;
            window_width = (int) ((double) window_height / aspect);
            ++changed;
        }

        if (window_width > usable.w - border_left) {
            window_width = usable.w - border_left;
            window_height = (int) (aspect * (double) window_width);
            ++changed;
        }

        SDL_GetWindowPosition(vptr->vid_window, &window_x, &window_y);
        if (window_y < border_top) {
            window_y = border_top;
            ++changed;
        }

        if (window_x < border_left) {
            window_x = border_left;
            ++changed;
        }

        if (changed) {
            set_window_scale(vptr, window_x, window_y, window_width, window_height);
            sim_messagef(SCPE_OK, "Simulator window rescaled to fit physical display.\n");
        }
#else
        if (window_height > usable.h - border_top - border_bottom || window_width > usable.w - border_left) {
            retval = sim_messagef(SCPE_OK, "SDL prior to 2.0.22 cannot rescale the window to fit physical display.\n");
        }
#endif
    } else {
        retval = SCPE_NXM;
        sim_messagef(retval, "rescale_window (%s): SDL error: %s\n", syndrome, SDL_GetError());
    }

    return retval;
}

static t_stat preserve_aspect(VID_DISPLAY *vptr)
{
    /* Simulator's original/expected aspect ratio. */
    double aspect = (double) vptr->vid_height / (double) vptr->vid_width;
    int w, h;
    const char *syndrome = NULL;
    int got_syndrome = 0;
    t_stat retval = SCPE_OK;

    /* Preserve the aspect ratio. */
    if (SDL_GetRendererOutputSize(vptr->renderer, &w, &h)) {
        syndrome = "SDL_GetWindowRendererOutputSize";
        ++got_syndrome;
    }

    if (!got_syndrome) {
        /* Assume the height is OK, but width needs to be adjusted. */
        int aspect_w = (int) ((double) h / aspect);
        int aspect_w_diff = w - aspect_w;

        /* Abs value... */
        if (aspect_w_diff < 0)
            aspect_w_diff = -aspect_w_diff;

        /* If the aspect-corrected width is more than 3 pixels, adjust. */
        if (aspect_w_diff > 3) {
            SDL_SetWindowSize(vptr->vid_window, h, aspect_w);
        }
    } else {
        retval = SCPE_NXM;
        sim_messagef(retval, "preserve_aspect (%s): SDL error: %s\n", syndrome, SDL_GetError());
    }

    return retval;
}

static void report_unrecognized_event(const SDL_Event *ev)
{
    const SDL_KeyboardEvent *kev;
    const SDL_MouseButtonEvent *bev;
    const SDL_MouseMotionEvent *mev;
    const SDL_WindowEvent *wev;
    const SDL_UserEvent *uev;

    switch (ev->type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        kev = &ev->key;
        sim_messagef(SCPE_OK, "Unrecognized key event.\n");
        sim_messagef(SCPE_OK, "  type = %s\n", getEventName(ev->type));
        sim_messagef(SCPE_OK, "  timestamp = %u\n", kev->timestamp);
        sim_messagef(SCPE_OK, "  windowID = %u\n", kev->windowID);
        sim_messagef(SCPE_OK, "  state = %u\n", kev->state);
        sim_messagef(SCPE_OK, "  repeat = %u\n", kev->repeat);
        sim_messagef(SCPE_OK, "  scancode = %d\n", kev->keysym.scancode);
        sim_messagef(SCPE_OK, "  sym = %d\n", kev->keysym.sym);
        sim_messagef(SCPE_OK, "  mod = %u\n", kev->keysym.mod);
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        bev = &ev->button;
        sim_messagef(SCPE_OK, "Unrecognized mouse button event.\n");
        sim_messagef(SCPE_OK, "  type = %s\n", getEventName(ev->type));
        sim_messagef(SCPE_OK, "  timestamp = %u\n", bev->timestamp);
        sim_messagef(SCPE_OK, "  windowID = %u\n", bev->windowID);
        sim_messagef(SCPE_OK, "  which = %u\n", bev->which);
        sim_messagef(SCPE_OK, "  button = %u\n", bev->button);
        sim_messagef(SCPE_OK, "  state = %u\n", bev->state);
        sim_messagef(SCPE_OK, "  clicks = %u\n", bev->clicks);
        sim_messagef(SCPE_OK, "  x = %d\n", bev->x);
        sim_messagef(SCPE_OK, "  y = %d\n", bev->y);
        break;
    case SDL_MOUSEMOTION:
        mev = &ev->motion;
        sim_messagef(SCPE_OK, "Unrecognized mouse motion event.\n");
        sim_messagef(SCPE_OK, "  type = %s\n", getEventName(ev->type));
        sim_messagef(SCPE_OK, "  timestamp = %u\n", mev->timestamp);
        sim_messagef(SCPE_OK, "  windowID = %u\n", mev->windowID);
        sim_messagef(SCPE_OK, "  which = %u\n", mev->which);
        sim_messagef(SCPE_OK, "  state = %u\n", mev->state);
        sim_messagef(SCPE_OK, "  x = %d\n", mev->x);
        sim_messagef(SCPE_OK, "  y = %d\n", mev->y);
        sim_messagef(SCPE_OK, "  xrel = %d\n", mev->xrel);
        sim_messagef(SCPE_OK, "  yrel = %d\n", mev->yrel);
        break;
    case SDL_WINDOWEVENT:
        wev = &ev->window;
        sim_messagef(SCPE_OK, "Unrecognized window event.\n");
        sim_messagef(SCPE_OK, "  type = %s\n", getEventName(ev->type));
        sim_messagef(SCPE_OK, "  timestamp = %u\n", wev->timestamp);
        sim_messagef(SCPE_OK, "  windowID = %u\n", wev->windowID);
        sim_messagef(SCPE_OK, "  event = %s\n", getWindowEventName(wev->event));
        sim_messagef(SCPE_OK, "  data1 = 0x%08x\n", wev->data1);
        sim_messagef(SCPE_OK, "  data2 = 0x%08x\n", wev->data2);
        break;
    case SDL_USEREVENT:
        uev = &ev->user;
        sim_messagef(SCPE_OK, "Unrecognized user event.\n");
        sim_messagef(SCPE_OK, "  type = %s\n", getEventName(uev->type));
        sim_messagef(SCPE_OK, "  timestamp = %u\n", uev->timestamp);
        sim_messagef(SCPE_OK, "  windowID = %u\n", uev->windowID);
        sim_messagef(SCPE_OK, "  code = %s\n", getUserCodeName(uev->code));
        sim_messagef(SCPE_OK, "  data1 = 0x%p\n", uev->data1);
        sim_messagef(SCPE_OK, "  data2 = 0x%p\n", uev->data2);
        break;
    default:
        sim_messagef(SCPE_OK, "Unrecognized event type %u\n", ev->type);
        break;
    }
}

static inline int queue_sdl_event(SDL_Event *event, const char *caller, VID_DISPLAY *vptr)
{
    int retval;

    /* Looking at SDL's actual SDL_PushEvent() code, this should never fail. */
    retval = SDL_PushEvent(event);
    if (retval < 0) {
        if (vptr != NULL) {
            sim_messagef(SCPE_NXM, "%s: %s did not enqueue SDL event: %s\n", vid_dname(vptr->vid_dev), caller, SDL_GetError());
        } else {
            fprintf(stderr, "%s did not enqueue SDL event: %s\n", caller, SDL_GetError());
            fflush(stderr);
        }
    }

    return retval;
}

static inline int queue_simh_sdl_event(SIMHEvents ev_type, const char *caller, VID_DISPLAY *vptr)
{
    SDL_Event user_event = {
        .user.type = evidentifier_for(ev_type),
        .user.windowID = (vptr != NULL ? vptr->vid_windowID : 0),
        .user.code = 0,
        .user.data1 = (void *) vptr,
        .user.data2 = NULL
    };

    return queue_sdl_event(&user_event, caller, vptr);
}

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * SIMH API functions:
 *=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

/* Forward decls: */
static t_stat vid_init_window(VID_DISPLAY *vptr, DEVICE *dptr, const char *title, uint32 width, uint32 height, int flags);
static t_stat vid_create_window (VID_DISPLAY *vptr);

t_stat vid_open (DEVICE *dptr, const char *title, uint32 width, uint32 height, int flags)
{
    t_stat r = SCPE_OK;

    if (sim_displays == NULL) {
        do_deferred_initialization();
        r = vid_open_window(&sim_displays, dptr, title, width, height, flags);
    }

    return r;
}

t_stat vid_open_window (VID_DISPLAY **vptr, DEVICE *dptr, const char *title, uint32 width, uint32 height, int flags)
{
    t_stat r;

    do_deferred_initialization();

    *vptr = (VID_DISPLAY *)calloc (1, sizeof (VID_DISPLAY));
    if (*vptr == NULL)
        return SCPE_MEM;

    r = vid_init_window (*vptr, dptr, title, width, height, flags);

    if (r != SCPE_OK) {
        free (*vptr);
        *vptr = NULL;
    } else {
        VID_DISPLAY *p;

        /* Minimize mutex exposure. */
        SDL_LockMutex(sim_displays_mutex);

        /* Here's the cute hack: If vid_open() gets called, sim_displays is
         * automagically initialized to a non-NULL value.
         *
         * Someone could actually just call vid_open_window() and the
         * sim_displays list could be empty (tt2500 -- we're staring at you...)
         */
        if (sim_displays != NULL && sim_displays != *vptr) {
            for (p = sim_displays; p->next != NULL; p = p->next)
                /* NOP */;

            p->next = *vptr;
        } else
            sim_displays = *vptr;

        SDL_UnlockMutex(sim_displays_mutex);

        r = vid_create_window(*vptr);
    }

    return r;
}

t_stat vid_close (void)
{
    if (sim_displays != NULL)
        return vid_close_window (sim_displays);

    return SCPE_OK;
}

t_stat vid_close_window(VID_DISPLAY *vptr)
{
    int active_status = SDL_AtomicGet(&vid_active);

    if (SDL_AtomicGet(&vptr->vid_ready) != FALSE) {
        sim_debug(SIM_VID_DBG_ALL, vptr->vid_dev, "vid_close()\n");
        queue_simh_sdl_event(SIMH_EVENT_CLOSE, "vid_close_window", vptr);
        vptr->vid_dev = NULL;
    }

    while (SDL_AtomicGet(&vptr->vid_ready) != FALSE)
        sim_os_ms_sleep(10);

    vptr->vid_active_window = FALSE;
    if (active_status == 0 && vid_mouse_events.sem) {
        SDL_DestroySemaphore(vid_mouse_events.sem);
        vid_mouse_events.sem = NULL;
    }
    if (active_status == 0 && vid_key_events.sem) {
        SDL_DestroySemaphore(vid_key_events.sem);
        vid_key_events.sem = NULL;
    }
    return SCPE_OK;
}

t_stat vid_close_all(void)
{
    VID_DISPLAY *vptr;

    for (vptr = sim_displays; vptr != NULL; vptr = vptr->next)
        vid_close_window(vptr);

    return SCPE_OK;
}

/* Draw into the default VID_DISPLAY */
void vid_draw (int32 x, int32 y, int32 w, int32 h, const uint32 *buf)
{
    vid_draw_window (vid_first(), x, y, w, h, buf);
}

void vid_draw_window (VID_DISPLAY *vptr, int32 x, int32 y, int32 w, int32 h, const uint32 *buf)
{
    size_t i, slot = vptr->alloc_draw_ops;
    t_bool good = FALSE;

    do {
        t_bool overlapped = FALSE;

        for (i = 0; i < vptr->alloc_draw_ops; ++i) {
            overlapped = vptr->draw_ops[i].blit_rect.x == x
                         && vptr->draw_ops[i].blit_rect.y == y
                         && vptr->draw_ops[i].blit_rect.w == w
                         && vptr->draw_ops[i].blit_rect.h == h;

            if (overlapped || SDL_AtomicGet(&vptr->draw_ops[i].in_use) == FALSE) {
                slot = i;
                if (overlapped) {
                    sim_debug (SIM_VID_DBG_VIDEO, vptr->vid_dev, "[event] [%" SIZE_T_FMT "u] reusing unrendered frame.\n", i);
                }
                break;
            }
        }

        if (slot >= vptr->alloc_draw_ops) {
            DrawOp *p;
            size_t prev_alloc = vptr->alloc_draw_ops;

            vptr->alloc_draw_ops *= 2;
            p = (DrawOp *) realloc(vptr->draw_ops, vptr->alloc_draw_ops * sizeof(DrawOp));
            if (p == NULL) {
                sim_messagef(SCPE_NXM, "%s: vid_draw_window(): realloc() draw_ops array error.\n", vid_dname(vptr->vid_dev));
                return;
            }

            vptr->draw_ops = p;
            memset(&vptr->draw_ops[prev_alloc], 0, (vptr->alloc_draw_ops - prev_alloc) * sizeof(DrawOp));

            sim_debug (SIM_VID_DBG_VIDEO, vptr->vid_dev,
                        "vid_draw_window() reallocated draw_ops from %" SIZE_T_FMT "u to %" SIZE_T_FMT "u\n",
                        prev_alloc, vptr->alloc_draw_ops);
        }

        if (overlapped || SDL_AtomicCAS(&vptr->draw_ops[slot].in_use, FALSE, TRUE) == SDL_TRUE)
            good = TRUE;
    } while (!good);


    if (!set_draw_op(vptr, slot, buf, x, y, w, h)) {
        SDL_Event user_event = {
            .user.type = evidentifier_for(SIMH_EVENT_DRAW),
            .user.windowID = 0,
            .user.code = 0,
            .user.data1 = (void *) vptr,
            /* Not a fan of sending the index diguised as a pointer, although
             * (void *) and size_t are the same size. An atrocious practice
             * that should be avoided.
             *
             * However, it's unsafe to send a pointer into the vptr->draw_ops
             * array because the array might be resized and moved, thereby
             * invalidating previous SIMH_EVENT_DRAW pointers. */
            .user.data2 = (void *) slot
        };

        queue_sdl_event(&user_event, "vid_draw_window_event", vptr);
        sim_debug (SIM_VID_DBG_VIDEO, vptr->vid_dev, "[event] [%" SIZE_T_FMT "u] vid_draw(%d, %d, %d, %d)\n", slot, x, y, w, h);
    }

    SDL_UnlockMutex (vptr->draw_mutex);
}

void vid_draw_onto(int32 x, int32 y, int32 w, int32 h, VID_DRAW_ONTO_CALLBACK draw_fn, void *draw_data)
{
    vid_draw_onto_window(vid_first(), x, y, w, h, draw_fn, draw_data);
}

void vid_draw_onto_window(VID_DISPLAY *vptr, int32 x, int32 y, int32 w, int32 h, VID_DRAW_ONTO_CALLBACK draw_fn, void *draw_data)
{
    size_t i;
    t_bool good = FALSE;

    do {
        for (i = 0; i < vptr->alloc_onto_ops && SDL_AtomicGet(&vptr->onto_ops[i].pending) == TRUE; ++i)
            /* NOP */;

        if (i >= vptr->alloc_onto_ops) {
            size_t new_alloc = vptr->alloc_onto_ops * 2;
            size_t prev_alloc = vptr->alloc_onto_ops;
            DrawOnto *p = (DrawOnto *) realloc(vptr->onto_ops, new_alloc * sizeof(DrawOnto));

            if (p != NULL) {
                vptr->onto_ops = p;
                memset(&vptr->onto_ops[prev_alloc], 0, (new_alloc - prev_alloc) * sizeof(DrawOnto));
                vptr->alloc_onto_ops = new_alloc;

                sim_debug (SIM_VID_DBG_VIDEO, vptr->vid_dev,
                            "vid_draw_onto_window() reallocated onto_ops from %" SIZE_T_FMT "u to %" SIZE_T_FMT "u\n",
                            prev_alloc, vptr->alloc_onto_ops);
            } else {
                sim_messagef(SCPE_NXM, "%s: vid_draw_onto_window(): realloc() onto_ops array error.\n", vid_dname(vptr->vid_dev));
                return;
            }
        }

        if (SDL_AtomicCAS(&vptr->onto_ops[i].pending, FALSE, TRUE) == SDL_TRUE)
            good = TRUE;
    } while (!good);

    vptr->onto_ops[i].onto_rect.x = x;
    vptr->onto_ops[i].onto_rect.y = y;
    vptr->onto_ops[i].onto_rect.w = w;
    vptr->onto_ops[i].onto_rect.h = h;
    vptr->onto_ops[i].draw_fn = draw_fn;
    vptr->onto_ops[i].draw_data = draw_data;

    SDL_Event user_event = {
        .user.type = evidentifier_for(SIMH_DRAW_ONTO),
        .user.code = 0,
        .user.data1 = vptr,
        /* See note in vid_draw_window(). */
        .user.data2 = (void *) i
    };

    queue_sdl_event(&user_event, "vid_draw_onto_window", vptr);
}

void vid_refresh (void)
{
    vid_refresh_window (vid_first());
}

void vid_refresh_window (VID_DISPLAY *vptr)
{
}

void vid_render_set_logical_size (VID_DISPLAY *vptr, int32 w, int32 h)
{
    vptr->vid_rect.h = h;
    vptr->vid_rect.w = w;

    SDL_Event user_event = {
        .user.type = evidentifier_for(SIMH_EVENT_LOGICAL_SIZE),
        .user.code = 0,
        .user.data1 = vptr,
        .user.data2 = NULL
    };

  queue_sdl_event(&user_event, "vid_set_logical_size", vptr);
}

t_bool vid_is_fullscreen(void)
{
    return vid_is_fullscreen_window(vid_first());
}

t_bool vid_is_fullscreen_window(VID_DISPLAY *vptr)
{
    return (SDL_GetWindowFlags(vptr->vid_window) & SDL_WINDOW_FULLSCREEN_DESKTOP);
}

t_stat vid_set_fullscreen(t_bool flag)
{
    return vid_set_fullscreen_window(vid_first(), flag);
}

t_stat vid_set_fullscreen_window(VID_DISPLAY *vptr, t_bool flag)
{
    SDL_LockMutex(vptr->draw_mutex);
    vptr->fullscreen_flag = flag;
    SDL_UnlockMutex(vptr->draw_mutex);

    queue_simh_sdl_event(SIMH_EVENT_FULLSCREEN, "vid_set_fullscreen_window", vptr);

    return SCPE_OK;
}

t_stat vid_show_video(FILE *st, UNIT *uptr, int32 val, const void *desc)
{
    SDL_cond *show_cond = SDL_CreateCond();
    SDL_mutex *show_mutex = SDL_CreateMutex();
    /* Safe to pass this to the SIMH_EVENT_SHOW handler because it'll still be live
     * until this function exits. */
    ShowVideoCmd_t cmd = {
        .show_cond = show_cond,
        .show_mutex = show_mutex,
        .show_stream = st,
        .show_uptr = uptr,
        .show_val = val,
        .show_desc = desc
    };

    SDL_AtomicSet(&cmd.show_stat, -1);

    if (show_cond != NULL && show_mutex != NULL) {
        SDL_Event user_event = {
            .user.type = evidentifier_for(SIMH_EVENT_SHOW),
            .user.data1 = &cmd
        };

        SDL_LockMutex(show_mutex);
        if (deferred_startup_completed() && queue_sdl_event(&user_event, "vid_show_video", NULL) >= 0) {
            SDL_CondWait(show_cond, show_mutex);
        } else {
            sim_messagef(SCPE_OK, "Video subsystem not currently active or initialized.\n");
        }
        SDL_UnlockMutex(show_mutex);
    } else if (show_cond == NULL) {
        sim_messagef(SCPE_NXM, "vid_show_video(): SDL_CreateCond() error: %s\n", SDL_GetError());
    } else if (show_mutex == NULL) {
        sim_messagef(SCPE_NXM, "vid_show_video(): SDL_CreateMutex() error: %s\n", SDL_GetError());
    }

    if (show_mutex != NULL)
        SDL_DestroyMutex(show_mutex);
    if (show_cond != NULL)
        SDL_DestroyCond(show_cond);

    return SDL_AtomicGet(&cmd.show_stat);
}

void vid_beep(void)
{
    int pending = SDL_AtomicGet(&beep_ctx.beeps_pending);

    if (pending >= 0) {
        if (pending == 0) {
            queue_simh_sdl_event(SIMH_EVENT_BEEP, "vid_beep", NULL);
        }

        SDL_AtomicIncRef(&beep_ctx.beeps_pending);
    } else {
        sim_messagef(SCPE_OK, "Pending beeps less than 0, resetting.\n");
        SDL_AtomicSet(&beep_ctx.beeps_pending, 0);
    }
}

/* Adjust the beep's duration. */
void vid_set_beep_duration(VID_BEEP_DURATION duration)
{
    vid_beep_setup(duration, BEEP_TONE_FREQ);
}

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * SDL_main/main:
 *
 *  Some platforms, principally macOS, require that ALL input event processing
 *  be performed by the main thread of the process. Windows also requires
 *  special processing before calling the application's main().
 *
 *  SDL works around these issues by:
 *
 *           #define main SDL_main
 *
 * The simplest thing to do is allow SDL to operate the way SDL was intended to
 * operate. This means that all platforms execute the SDL event loop within the
 * main() function, whether main() has been renamed or not. One could try to be
 * clever and detect the renaming with SDL_MAIN_AVAILABLE and SDL_MAIN_NEEDED,
 * writing complicated code in the process.
 *
 * The SIMH command line interface executes within a separate thread. The CLI
 * thread is created with a 8M (2048 4K pages) stack size. The stack size
 * argument is generally "advisory" -- you get what you get.
 *~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/

static SDL_Thread *the_simulator_thread = NULL;

static SDL_Thread *create_simulator_thread(simulator_thread_args_t *);
static int exec_simulator_thread(void *arg);
void vid_key (SDL_KeyboardEvent *event);
void vid_mouse_move (SDL_MouseMotionEvent *event);
void vid_mouse_button (SDL_MouseButtonEvent *event);
void vid_set_window_size (VID_DISPLAY *vptr, int32 w, int32 h);
void vid_joy_motion (SDL_JoyAxisEvent *event);
void vid_joy_button (SDL_JoyButtonEvent *event);
void vid_controller_motion (const SDL_ControllerAxisEvent *event);
void vid_controller_button (SDL_ControllerButtonEvent *event);


int main (int argc, char *argv[])
{
    SDL_Event event;
    int status, event_flow;

    simulator_thread_args_t simthread_args = {
        .argc = argc,
        .argv = argv
    };

#if defined (SDL_HINT_VIDEO_ALLOW_SCREENSAVER)
    /* If this hint is defined, the default is to disable the screen saver.
        We want to leave the screen saver enabled. */
    SDL_SetHint (SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
#endif

    if (initialize_deferred_state())
        exit(1);

    if ((the_simulator_thread = create_simulator_thread(&simthread_args)) == NULL) {
        fprintf (stderr, "SDL_CreateThread failed: %s\n", SDL_GetError ());
        exit (1);
    }

    /* Now wait for the deferred initialization signal: */
    SDL_LockMutex(main_event_mutex);
    SDL_CondWait(main_event_cond, main_event_mutex);
    exec_deferred_startup();
    SDL_UnlockMutex(main_event_mutex);

    for (event_flow = 1; event_flow; /* empty */) {
        exec_startup_complete();

        memset(&event, 0, sizeof(event));
        status = SDL_WaitEvent (&event);
        if (status == 1) {
            VID_DISPLAY *vptr;

            switch (event.type) {
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    vid_key (&event.key);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    vid_mouse_button (&event.button);
                    break;

                case SDL_MOUSEMOTION:
                    vid_mouse_move (&event.motion);
                    break;

                case SDL_JOYAXISMOTION:
                    vid_joy_motion (&event.jaxis);
                    break;

                case SDL_JOYBUTTONUP:
                case SDL_JOYBUTTONDOWN:
                    vid_joy_button (&event.jbutton);
                    break;

                case SDL_CONTROLLERAXISMOTION:
                    vid_controller_motion (&event.caxis);
                    break;

                case SDL_CONTROLLERBUTTONUP:
                case SDL_CONTROLLERBUTTONDOWN:
                    vid_controller_button (&event.cbutton);
                    break;

                case SDL_WINDOWEVENT:
                    {
                        int vptr_expected = 0;

                        vptr = vid_get_event_window (event.window.windowID);

                        if (sim_deb != NULL) {
                            fprintf (sim_deb, "[%s] SDL_WINDOWEVENT %s (0x%04x)\n", (vptr != NULL ? vid_dname(vptr->vid_dev) : "Unknown"),
                                     getWindowEventName(event.window.event), event.window.event);
                        }

                        switch (event.window.event) {
                            case SDL_WINDOWEVENT_ENTER:
                                if (vptr != NULL) {
                                    if (vptr->vid_flags & SIM_VID_INPUTCAPTURED)
                                        SDL_WarpMouseInWindow (NULL, vptr->vid_width/2, vptr->vid_height/2);   /* center position */
                                } else
                                    ++vptr_expected;
                                break;
                            case SDL_WINDOWEVENT_EXPOSED:
                                if (vptr != NULL) {
                                    if (SDL_AtomicGet(&vptr->vid_ready) != 0) {
                                        SDL_RenderClear(vptr->renderer);
                                        SDL_RenderCopy(vptr->renderer, vptr->texture, NULL, NULL);
                                        SDL_RenderPresent(vptr->renderer);
                                    }
                                } else
                                    ++vptr_expected;
                                break;
                            case SDL_WINDOWEVENT_CLOSE:
                                break;
                            case SDL_WINDOWEVENT_RESIZED:
                                if (vptr != NULL) {
                                    preserve_aspect(vptr);
                                } else
                                    ++vptr_expected;
                                break;
                            default:
                                if (sim_deb != NULL) {
                                    fprintf (sim_deb, "Ignored window event: 0x%04x - %s\n",
                                             event.window.event, getWindowEventName(event.window.event));
                                }
                                break;
                        }

                        if (vptr_expected && vptr == NULL) {
                            sim_printf("SDL event loop: Window event expected a valid window.\n");
                            report_unrecognized_event(&event);
                        }
                    } break;

                default:
                    if (event.type >= SDL_USEREVENT) {
                        if (event.user.type >= evidentifier_for(FIRST_SIMH_DISPATCH)
                            && event.user.type <= evidentifier_for(LAST_SIMH_DISPATCH)) {
                            size_t idx = event.user.type - evidentifier_for(FIRST_SIMH_DISPATCH);

                            simh_event_dispatch[idx].simh_event(&event);
                        } else if (event.user.type == evidentifier_for(SIMH_EXIT_EVENT)) {
                            if (sim_deb != NULL) {
                                fprintf (sim_deb, "SDL event loop - exit event.\n");
                            }

                            SDL_AtomicSet(&vid_active, 0);
                            event_flow = 0;
                            break;
                        } else {
                            if (sim_deb != NULL) {
                                fprintf (sim_deb, "Ignored SDL user event: %s (0x%04x)\n", getEventName(event.type), event.type);
                            }
                        }
                    } else {
                        if (sim_deb != NULL) {
                            fprintf(sim_deb, "Ignored SDL event: %s (0x%04x)\n", getEventName(event.type), event.type);
                        }
                    }
                    break;

                case SDL_QUIT: {
                    size_t i;

                    if ((vptr = vid_get_event_window(event.user.windowID)) != NULL) {
                        sim_debug (SIM_VID_DBG_ALL, vptr->vid_dev,
                                   "SDL event loop - QUIT Event - %s\n",
                                   (n_vid_quit_callbacks > 0) ? "Signaled" : "Ignored");
                    }

                    for (i = 0; i < n_vid_quit_callbacks; ++i)
                        vid_quit_callbacks[i]();
                } break;
            }
        } else {
            if (status < 0)
                sim_printf ("main() - error: %s\n", SDL_GetError());
        }
    }

    SDL_WaitThread (the_simulator_thread, &status);
    sim_video_system_cleanup();
    SDL_Quit ();
    return status;
}

static SDL_Thread *create_simulator_thread(simulator_thread_args_t *simthread_args)
{
    SDL_Thread *retval;
    const char *thread_name = "simh-main";

#if SDL_VERSION_ATLEAST(2, 0, 9)
    /* 8M stack size = 8192 * 1024 = 2048 * 4096. 4K is a common page size. This can be
     * tuned for different platforms as an exercise to the reader. :-)
     *
     * - 8M is Linux's thread stack size.
     * - 512K is macOS' default, so hopefully asking for more is granted subject to
     *   ulimit()-s.
     * - 1M is Windows' default stack size. It does give the CLI thread 8M when asked.
     * */
    retval = SDL_CreateThreadWithStackSize(exec_simulator_thread, thread_name, 2048 * 4096, simthread_args);
    if (retval == NULL) {
        /* Try again with the default stack size. */
        retval = SDL_CreateThreadWithStackSize(exec_simulator_thread, thread_name, 0, simthread_args);
    }
#else
    /* Really ancient SDL2... */
    retval = SDL_CreateThread (exec_simulator_thread , thread_name, simthread_args);
#endif

    return retval;
}


/* Run the CLI loop in its own thread. */
static int exec_simulator_thread (void *arg)
{
    simulator_thread_args_t *main_args = (simulator_thread_args_t *) arg;
    int stat;

    stat = simulator_main (main_args->argc, main_args->argv);

    maybe_event_only_initialization();
    do_deferred_initialization();
    queue_simh_sdl_event(SIMH_EXIT_EVENT, "exec_cli_thread", NULL);

    return stat;
}

static t_stat vid_create_window (VID_DISPLAY *vptr)
{
SDL_Event user_event = {
    .user.type = evidentifier_for(SIMH_EVENT_OPEN),
    .user.code = 0,
    .user.data1 = vptr,
    .user.data2 = &did_opencomplete
};

SDL_LockMutex(did_opencomplete.completed_mutex);

SDL_AtomicSet(&vptr->vid_ready, FALSE);
queue_sdl_event(&user_event, "vid_create_window", vptr);

/* SIMH_EVENT_OPENCOMPLETE event signals that vid_open() has completed
 * deferred initialization. */
SDL_CondWait(did_opencomplete.completed_cond, did_opencomplete.completed_mutex);
SDL_UnlockMutex(did_opencomplete.completed_mutex);

if (SDL_AtomicGet(&vptr->vid_ready) == FALSE) {
    vid_close ();
    return SCPE_OPENERR;
    }
return SCPE_OK;
}


static t_stat vid_init_window(VID_DISPLAY *vptr, DEVICE *dptr, const char *title, uint32 width, uint32 height, int flags)
{
    vptr->vid_active_window = TRUE;
    vptr->vid_mouse_captured = FALSE;
    vptr->vid_flags = flags;
    vptr->vid_width = width;
    vptr->vid_height = height;
    SDL_AtomicSet(&vptr->vid_ready, FALSE);
    if ((strlen(sim_name) + 7 + (dptr ? strlen(dptr->name) : 0) + (title ? strlen(title) : 0)) < sizeof(vptr->vid_title))
        sprintf(vptr->vid_title, "%s%s%s%s%s", sim_name, dptr ? " - " : "", dptr ? dptr->name : "", title ? " - " : "",
                title ? title : "");
    else
        sprintf(vptr->vid_title, "%s", sim_name);
    vptr->vid_cursor_visible = (vptr->vid_flags & SIM_VID_INPUTCAPTURED);
    vptr->vid_blending = FALSE;

    if (SDL_AtomicGet(&vid_active) == 0) {
        vid_key_events.head = 0;
        vid_key_events.tail = 0;
        vid_key_events.count = 0;
        vid_key_events.sem = SDL_CreateSemaphore(1);
        vid_mouse_events.head = 0;
        vid_mouse_events.tail = 0;
        vid_mouse_events.count = 0;
        vid_mouse_events.sem = SDL_CreateSemaphore(1);
    }

    vptr->vid_dev = dptr;

    memset(motion_callback, 0, sizeof motion_callback);
    memset(button_callback, 0, sizeof button_callback);

    vptr->renderer = NULL;
    vptr->texture = NULL;

    /* Drawing operations... */
    vptr->draw_mutex = SDL_CreateMutex();
    if (vptr->draw_mutex == NULL) {
        SDL_Quit();
        return sim_messagef(SCPE_NXM, "%s: create draw_mutex failed: %s\n", vid_dname(vptr->vid_dev), SDL_GetError());
    }

    if ((vptr->draw_ops = (DrawOp *) calloc(INITIAL_DRAW_OPS, sizeof(DrawOp))) == NULL) {
        SDL_DestroyMutex(vptr->draw_mutex);
        return sim_messagef(SCPE_NXM, "%s: could not allocate draw_ops.\n", vid_dname(vptr->vid_dev));
    } else
        vptr->alloc_draw_ops = INITIAL_DRAW_OPS;

    if ((vptr->onto_ops = (DrawOnto *) calloc(INITIAL_ONTO_OPS, sizeof(DrawOnto))) == NULL) {
        SDL_DestroyMutex(vptr->draw_mutex);
        free(vptr->draw_ops);
        vptr->alloc_draw_ops = 0;
        return sim_messagef(SCPE_NXM, "%s: could not allocate onto_ops.\n", vid_dname(vptr->vid_dev));
    } else
        vptr->alloc_onto_ops = INITIAL_ONTO_OPS;

    sim_debug(SIM_VID_DBG_ALL, vptr->vid_dev, "vid_init_window() - Success\n");

    return SCPE_OK;
}

t_stat vid_poll_kb (SIM_KEY_EVENT *ev)
{
if (SDL_SemTryWait (vid_key_events.sem) == 0) {         /* get lock */
    if (vid_key_events.count > 0) {                     /* events in queue? */
        *ev = vid_key_events.events[vid_key_events.head++];
        vid_key_events.count--;
        if (vid_key_events.head == MAX_EVENTS)
            vid_key_events.head = 0;
        SDL_SemPost (vid_key_events.sem);
        return SCPE_OK;
        }
    SDL_SemPost (vid_key_events.sem);
    }
return SCPE_EOF;
}

t_stat vid_poll_mouse (SIM_MOUSE_EVENT *ev)
{
t_stat stat = SCPE_EOF;

if (SDL_SemTryWait (vid_mouse_events.sem) == 0) {
    if (vid_mouse_events.count > 0) {
        const SIM_MOUSE_EVENT *mev;

        stat = SCPE_OK;
        *ev = vid_mouse_events.events[vid_mouse_events.head++];
        vid_mouse_events.count--;
        if (vid_mouse_events.head == MAX_EVENTS)
            vid_mouse_events.head = 0;

        mev = &vid_mouse_events.events[vid_mouse_events.head];
        if ((vid_mouse_events.count > 0) &&
            (0 == (ev->x_rel + mev->x_rel)) &&
            (0 == (ev->y_rel + mev->y_rel)) &&
            (ev->b1_state == mev->b1_state) &&
            (ev->b2_state == mev->b2_state) &&
            (ev->b3_state == mev->b3_state)) {
            if ((++vid_mouse_events.head) == MAX_EVENTS)
                vid_mouse_events.head = 0;
            vid_mouse_events.count--;
            stat = SCPE_EOF;
            sim_debug (SIM_VID_DBG_MOUSE, ev->dev, "vid_poll_mouse: ignoring bouncing events\n");
            }
        }
    if (SDL_SemPost (vid_mouse_events.sem))
        sim_printf ("vid_poll_mouse(): SDL_SemPost error: %s\n", SDL_GetError());
    }
return stat;
}

uint32 vid_map_rgb_window (VID_DISPLAY *vptr, uint8 r, uint8 g, uint8 b)
{
return SDL_MapRGB (vptr->vid_format, r, g, b);
}

uint32 vid_map_rgb (uint8 r, uint8 g, uint8 b)
{
return vid_map_rgb_window (vid_first(), r, g, b);
}

uint32 vid_map_rgba_window (VID_DISPLAY *vptr, uint8 r, uint8 g, uint8 b, uint8 a)
{
return SDL_MapRGBA (vptr->vid_format, r, g, b, a);
}

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * Drawing operations:
 *=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

/* Resize draw_ops if needed, resize the blit_data bitmap and populate the blit data values. */
static inline int set_draw_op(VID_DISPLAY *vptr, size_t idx, const uint32 *buf, int32 x, int32 y, int32 w, int32 h)
{
    DrawOp *draw = &vptr->draw_ops[idx];
    const size_t n_buf = w * h * sizeof(*buf);

    if (draw->alloc_blit_data < n_buf) {
        /* NB: draw->vid_data may be NULL, which makes the realloc() == malloc(). */
        uint32 *newbuf = (uint32 *) realloc(draw->blit_data, n_buf);
        if (newbuf == NULL) {
            sim_printf ("%s: set_draw_op() realloc blit_data allocation error\n", vid_dname(vptr->vid_dev));
            return 1;
        }

        sim_debug (SIM_VID_DBG_VIDEO, vptr->vid_dev,
                    "set_draw_op() reallocated draw_ops[%" SIZE_T_FMT "u] from %" SIZE_T_FMT "u to %" SIZE_T_FMT "u\n",
                    draw - vptr->draw_ops, draw->n_blit_data, n_buf);

        draw->blit_data = newbuf;
        draw->alloc_blit_data = n_buf;
    }

    draw->blit_rect.x = x;
    draw->blit_rect.y = y;
    draw->blit_rect.h = h;
    draw->blit_rect.w = w;
    draw->n_blit_data = n_buf;
    memcpy (draw->blit_data, buf, n_buf);
    return 0;
}

t_stat vid_set_cursor_window (VID_DISPLAY *vptr, t_bool visible, uint32 width, uint32 height, uint8 *data, uint8 *mask, uint32 hot_x, uint32 hot_y)
{
SDL_Cursor *cursor = SDL_CreateCursor (data, mask, width, height, hot_x, hot_y);

sim_debug (SIM_VID_DBG_CURSOR, vptr->vid_dev, "vid_set_cursor(%s, %d, %d) Setting New Cursor\n", visible ? "visible" : "invisible", width, height);
if (sim_deb) {
    uint32 i, j;

    for (i=0; i<height; i++) {
        sim_debug (SIM_VID_DBG_CURSOR, vptr->vid_dev, "Cursor:  ");
        for (j=0; j<width; j++) {
            int byte = (j + i*width) >> 3;
            int bit = 7 - ((j + i*width) & 0x7);
            static char mode[] = "TWIB";

            sim_debug (SIM_VID_DBG_CURSOR, vptr->vid_dev, "%c", mode[(((data[byte]>>bit)&1)<<1)|((mask[byte]>>bit)&1)]);
            }
        sim_debug (SIM_VID_DBG_CURSOR, vptr->vid_dev, "\n");
        }
    }

SDL_LockMutex (vptr->draw_mutex);
vptr->cursor_data = cursor;
vptr->cursor_visible = visible;
SDL_UnlockMutex (vptr->draw_mutex);

return queue_simh_sdl_event(SIMH_EVENT_CURSOR, "vid_set_cursor_window", vptr);
}

t_stat vid_set_cursor (t_bool visible, uint32 width, uint32 height, uint8 *data, uint8 *mask, uint32 hot_x, uint32 hot_y)
{
return vid_set_cursor_window (vid_first(), visible, width, height, data, mask, hot_x, hot_y);
}

void vid_set_cursor_position_window (VID_DISPLAY *vptr, int32 x, int32 y)
{
int32 x_delta = vid_cursor_x - x;
int32 y_delta = vid_cursor_y - y;

if (vptr->vid_flags & SIM_VID_INPUTCAPTURED)
    return;

if ((x_delta) || (y_delta)) {
    sim_debug (SIM_VID_DBG_CURSOR, vptr->vid_dev, "vid_set_cursor_position(%d, %d) - Cursor position changed\n", x, y);
    /* Any queued mouse motion events need to have their relative
       positions adjusted since they were queued based on different info. */
    if (SDL_SemWait (vid_mouse_events.sem) == 0) {
        int32 i;

        for (i=0; i<vid_mouse_events.count; i++) {
            SIM_MOUSE_EVENT *ev = &vid_mouse_events.events[(vid_mouse_events.head + i)%MAX_EVENTS];

            sim_debug (SIM_VID_DBG_CURSOR, vptr->vid_dev, "Pending Mouse Motion Event Adjusted from: (%d, %d) to (%d, %d)\n", ev->x_rel, ev->y_rel, ev->x_rel + x_delta, ev->y_rel + y_delta);
            ev->x_rel += x_delta;
            ev->y_rel += y_delta;
            }
        if (SDL_SemPost (vid_mouse_events.sem))
            sim_printf ("%s: vid_set_cursor_position(): SDL_SemPost error: %s\n", vid_dname(vptr->vid_dev), SDL_GetError());
        }
    else {
        sim_printf ("%s: vid_set_cursor_position(): SDL_SemWait error: %s\n", vid_dname(vptr->vid_dev), SDL_GetError());
        }
    vid_cursor_x = x;
    vid_cursor_y = y;
    if (vptr->vid_cursor_visible) {
        queue_simh_sdl_event(SIMH_EVENT_WARP, "vid_set_cursor_position", vptr);
        sim_debug (SIM_VID_DBG_CURSOR, vptr->vid_dev, "vid_set_cursor_position() - Warp Queued\n");
        }
    else {
        sim_debug (SIM_VID_DBG_CURSOR, vptr->vid_dev, "vid_set_cursor_position() - Warp Skipped\n");
        }
    }
}

void vid_set_cursor_position (int32 x, int32 y)
{
vid_set_cursor_position_window (vid_first(), x, y);
}

int vid_map_key (int key)
{
switch (key) {

    case SDLK_BACKSPACE:
        return SIM_KEY_BACKSPACE;

    case SDLK_TAB:
        return SIM_KEY_TAB;

    case SDLK_RETURN:
        return SIM_KEY_ENTER;

    case SDLK_ESCAPE:
        return SIM_KEY_ESC;

    case SDLK_SPACE:
        return SIM_KEY_SPACE;

    case SDLK_QUOTE:
        return SIM_KEY_SINGLE_QUOTE;

    case SDLK_COMMA:
        return SIM_KEY_COMMA;

    case SDLK_MINUS:
        return SIM_KEY_MINUS;

    case SDLK_PERIOD:
        return SIM_KEY_PERIOD;

    case SDLK_SLASH:
        return SIM_KEY_SLASH;

    case SDLK_0:
        return SIM_KEY_0;

    case SDLK_1:
        return SIM_KEY_1;

    case SDLK_2:
        return SIM_KEY_2;

    case SDLK_3:
        return SIM_KEY_3;

    case SDLK_4:
        return SIM_KEY_4;

    case SDLK_5:
        return SIM_KEY_5;

    case SDLK_6:
        return SIM_KEY_6;

    case SDLK_7:
        return SIM_KEY_7;

    case SDLK_8:
        return SIM_KEY_8;

    case SDLK_9:
        return SIM_KEY_9;

    case SDLK_SEMICOLON:
        return SIM_KEY_SEMICOLON;

    case SDLK_EQUALS:
        return SIM_KEY_EQUALS;

    case SDLK_LEFTBRACKET:
        return SIM_KEY_LEFT_BRACKET;

    case SDLK_BACKSLASH:
        return SIM_KEY_BACKSLASH;

    case SDLK_RIGHTBRACKET:
        return SIM_KEY_RIGHT_BRACKET;

    case SDLK_BACKQUOTE:
        return SIM_KEY_BACKQUOTE;

    case SDLK_a:
        return SIM_KEY_A;

    case SDLK_b:
        return SIM_KEY_B;

    case SDLK_c:
        return SIM_KEY_C;

    case SDLK_d:
        return SIM_KEY_D;

    case SDLK_e:
        return SIM_KEY_E;

    case SDLK_f:
        return SIM_KEY_F;

    case SDLK_g:
        return SIM_KEY_G;

    case SDLK_h:
        return SIM_KEY_H;

    case SDLK_i:
        return SIM_KEY_I;

    case SDLK_j:
        return SIM_KEY_J;

    case SDLK_k:
        return SIM_KEY_K;

    case SDLK_l:
        return SIM_KEY_L;

    case SDLK_m:
        return SIM_KEY_M;

    case SDLK_n:
        return SIM_KEY_N;

    case SDLK_o:
        return SIM_KEY_O;

    case SDLK_p:
        return SIM_KEY_P;

    case SDLK_q:
        return SIM_KEY_Q;

    case SDLK_r:
        return SIM_KEY_R;

    case SDLK_s:
        return SIM_KEY_S;

    case SDLK_t:
        return SIM_KEY_T;

    case SDLK_u:
        return SIM_KEY_U;

    case SDLK_v:
        return SIM_KEY_V;

    case SDLK_w:
        return SIM_KEY_W;

    case SDLK_x:
        return SIM_KEY_X;

    case SDLK_y:
        return SIM_KEY_Y;

    case SDLK_z:
        return SIM_KEY_Z;

    case SDLK_DELETE:
        return SIM_KEY_DELETE;
    case SDLK_KP_0:
        return SIM_KEY_KP_INSERT;

    case SDLK_KP_1:
        return SIM_KEY_KP_END;

    case SDLK_KP_2:
        return SIM_KEY_KP_DOWN;

    case SDLK_KP_3:
        return SIM_KEY_KP_PAGE_DOWN;

    case SDLK_KP_4:
        return SIM_KEY_KP_LEFT;

    case SDLK_KP_5:
        return SIM_KEY_KP_5;

    case SDLK_KP_6:
        return SIM_KEY_KP_RIGHT;

    case SDLK_KP_7:
        return SIM_KEY_KP_HOME;

    case SDLK_KP_8:
        return SIM_KEY_KP_UP;

    case SDLK_KP_9:
        return SIM_KEY_KP_PAGE_UP;

    case SDLK_KP_PERIOD:
        return SIM_KEY_KP_DELETE;

    case SDLK_KP_DIVIDE:
        return SIM_KEY_KP_DIVIDE;

    case SDLK_KP_MULTIPLY:
        return SIM_KEY_KP_MULTIPLY;

    case SDLK_KP_MINUS:
        return SIM_KEY_KP_SUBTRACT;

    case SDLK_KP_PLUS:
        return SIM_KEY_KP_ADD;

    case SDLK_KP_ENTER:
        return SIM_KEY_KP_ENTER;

    case SDLK_UP:
        return SIM_KEY_UP;

    case SDLK_DOWN:
        return SIM_KEY_DOWN;

    case SDLK_RIGHT:
        return SIM_KEY_RIGHT;

    case SDLK_LEFT:
        return SIM_KEY_LEFT;

    case SDLK_INSERT:
        return SIM_KEY_INSERT;

    case SDLK_HOME:
        return SIM_KEY_HOME;

    case SDLK_END:
        return SIM_KEY_END;

    case SDLK_PAGEUP:
        return SIM_KEY_PAGE_UP;

    case SDLK_PAGEDOWN:
        return SIM_KEY_PAGE_DOWN;

    case SDLK_F1:
        return SIM_KEY_F1;

    case SDLK_F2:
        return SIM_KEY_F2;

    case SDLK_F3:
        return SIM_KEY_F3;

    case SDLK_F4:
        return SIM_KEY_F4;

    case SDLK_F5:
        return SIM_KEY_F5;

    case SDLK_F6:
        return SIM_KEY_F6;

    case SDLK_F7:
        return SIM_KEY_F7;

    case SDLK_F8:
        return SIM_KEY_F8;

    case SDLK_F9:
        return SIM_KEY_F9;

    case SDLK_F10:
        return SIM_KEY_F10;

    case SDLK_F11:
        return SIM_KEY_F11;

    case SDLK_F12:
        return SIM_KEY_F12;

    case SDLK_NUMLOCKCLEAR:
        return SIM_KEY_NUM_LOCK;

    case SDLK_CAPSLOCK:
        return SIM_KEY_CAPS_LOCK;

    case SDLK_SCROLLLOCK:
        return SIM_KEY_SCRL_LOCK;

    case SDLK_RSHIFT:
        return SIM_KEY_SHIFT_R;

    case SDLK_LSHIFT:
        return SIM_KEY_SHIFT_L;

    case SDLK_RCTRL:
        return SIM_KEY_CTRL_R;

    case SDLK_LCTRL:
        return SIM_KEY_CTRL_L;

    case SDLK_RALT:
        return SIM_KEY_ALT_R;

    case SDLK_LALT:
        return SIM_KEY_ALT_L;

    case SDLK_LGUI:
        return SIM_KEY_WIN_L;

    case SDLK_RGUI:
        return SIM_KEY_WIN_R;

    case SDLK_PRINTSCREEN:
        return SIM_KEY_PRINT;

    case SDLK_PAUSE:
        return SIM_KEY_PAUSE;

    case SDLK_MENU:
        return SIM_KEY_MENU;

    default:
        return SIM_KEY_UNKNOWN;
        }
}

void vid_joy_motion (SDL_JoyAxisEvent *event)
{
    int n = sizeof motion_callback / sizeof (VID_GAMEPAD_CALLBACK);
    int i;

    for (i = 0; i < n; i++) {
        if (motion_callback[i]) {
            motion_callback[i](event->which, event->axis, event->value);
            }
        }
}

void vid_joy_button (SDL_JoyButtonEvent *event)
{
    int n = sizeof button_callback / sizeof (VID_GAMEPAD_CALLBACK);
    int i;

    for (i = 0; i < n; i++) {
        if (button_callback[i]) {
            button_callback[i](event->which, event->button, event->state);
            }
        }
}

void vid_controller_motion (const SDL_ControllerAxisEvent *event)
{
    SDL_JoyAxisEvent e;
    e.which = event->which;
    e.axis = event->axis;
    e.value = event->value;
    vid_joy_motion (&e);
}

void vid_controller_button (SDL_ControllerButtonEvent *event)
{
    SDL_JoyButtonEvent e;
    SDL_GameControllerButtonBind b;
    SDL_GameController *c;
    SDL_GameControllerButton button = (SDL_GameControllerButton)event->button;

    c = SDL_GameControllerFromInstanceID (event->which);
    b = SDL_GameControllerGetBindForButton (c, button);
    e.which = event->which;
    e.button = (Uint8) b.value.button;
    e.state = event->state;
    vid_joy_button (&e);
}

void vid_key(SDL_KeyboardEvent *event)
{
    /* If this window didn't have focus, then windowID is 0. From the SDL_KeyboardEvent doc:
     * windowID -> "The window with keyboard focus, if any". So, it's possible for the
     * windowID == 0. */

    if (event->windowID == 0)
        return;

    VID_DISPLAY *vptr = vid_get_event_window(event->windowID);
    if (vptr == NULL) {
        /* Should never happen. Defensive code. */
        sim_printf("vid_key: Key event expected a valid window.\n");
        report_unrecognized_event((SDL_Event *)event);
        return;
    }

    if (vptr->vid_mouse_captured) {
        static const Uint8 *KeyStates = NULL;
        static int numkeys;

        if (KeyStates == NULL)
            KeyStates = SDL_GetKeyboardState(&numkeys);
        if ((vptr->vid_flags & SIM_VID_INPUTCAPTURED) && (event->state == SDL_PRESSED) && KeyStates[SDL_SCANCODE_RSHIFT] &&
            (KeyStates[SDL_SCANCODE_LCTRL] || KeyStates[SDL_SCANCODE_RCTRL])) {
            sim_debug(SIM_VID_DBG_KEY, vptr->vid_dev, "vid_key() - Cursor Release\n");
            if (SDL_SetRelativeMouseMode(SDL_FALSE) < 0) /* release cursor, show cursor */
                sim_printf("%s: vid_key(): SDL_SetRelativeMouseMode error: %s\n", vid_dname(vptr->vid_dev), SDL_GetError());
            vptr->vid_mouse_captured = FALSE;
            return;
        }
    }
    if (!sim_is_running)
        return;
    if (SDL_SemWait(vid_key_events.sem) == 0) {
        if (vid_key_events.count < MAX_EVENTS) {
            SIM_KEY_EVENT ev;

            ev.key = vid_map_key(event->keysym.sym);
            ev.dev = vptr->vid_dev;
            ev.vptr = vptr;
            sim_debug(SIM_VID_DBG_KEY, vptr->vid_dev, "Keyboard Event: State: %s, Keysym(scancode,sym): (%d,%d) - %s\n",
                      (event->state == SDL_PRESSED) ? "PRESSED" : "RELEASED", event->keysym.scancode, event->keysym.sym,
                      vid_key_name(ev.key));
            if (event->state == SDL_PRESSED) {
                if (!vptr->vid_key_state[event->keysym.scancode]) { /* Key was not down before */
                    vptr->vid_key_state[event->keysym.scancode] = TRUE;
                    ev.state = SIM_KEYPRESS_DOWN;
                } else
                    ev.state = SIM_KEYPRESS_REPEAT;
            } else {
                vptr->vid_key_state[event->keysym.scancode] = FALSE;
                ev.state = SIM_KEYPRESS_UP;
            }
            vid_key_events.events[vid_key_events.tail++] = ev;
            vid_key_events.count++;
            if (vid_key_events.tail == MAX_EVENTS)
                vid_key_events.tail = 0;
        } else {
            sim_debug(SIM_VID_DBG_KEY, vptr->vid_dev, "Keyboard Event DISCARDED: State: %s, Keysym: Scancode: %d, Keysym: %d\n",
                      (event->state == SDL_PRESSED) ? "PRESSED" : "RELEASED", event->keysym.scancode, event->keysym.sym);
        }
        if (SDL_SemPost(vid_key_events.sem))
            sim_printf("%s: vid_key(): SDL_SemPost error: %s\n", vid_dname(vptr->vid_dev), SDL_GetError());
    }
}

void vid_mouse_move(SDL_MouseMotionEvent *event)
{
    SDL_Event dummy_event;

    /* If this window didn't have focus, then windowID is 0. From the SDL_MouseEvent doc:
     * windowID -> "The window with mouse focus, if any". So, it's possible for the
     * windowID == 0. */

    if (event->windowID == 0)
        return;

    /* Yes, this is legal C99. Wheat from chaff, so to speak. :-) */
    VID_DISPLAY *vptr = vid_get_event_window(event->windowID);
    if (vptr == NULL) {
        sim_printf("vid_mouse_move: Mouse motion expected a valid window.\n");
        report_unrecognized_event((SDL_Event *)event);
        return;
    }

    if ((!vptr->vid_mouse_captured) && (vptr->vid_flags & SIM_VID_INPUTCAPTURED))
        return;

    if (!sim_is_running)
        return;
    if (!vptr->vid_cursor_visible)
        return;
    sim_debug(SIM_VID_DBG_MOUSE, vptr->vid_dev, "Mouse Move Event: pos:(%d,%d) rel:(%d,%d) buttons:(%d,%d,%d)\n", event->x,
              event->y, event->xrel, event->yrel, (event->state & SDL_BUTTON(SDL_BUTTON_LEFT)) ? 1 : 0,
              (event->state & SDL_BUTTON(SDL_BUTTON_MIDDLE)) ? 1 : 0, (event->state & SDL_BUTTON(SDL_BUTTON_RIGHT)) ? 1 : 0);
    while (SDL_PeepEvents(&dummy_event, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION)) {
        SDL_MouseMotionEvent *dev = (SDL_MouseMotionEvent *)&dummy_event;

        /* Coalesce motion activity to avoid thrashing */
        event->xrel += dev->xrel;
        event->yrel += dev->yrel;
        event->x = dev->x;
        event->y = dev->y;
        event->state = dev->state;
        sim_debug(SIM_VID_DBG_MOUSE, vptr->vid_dev,
                  "Mouse Move Event: Additional Event Coalesced:pos:(%d,%d) rel:(%d,%d) buttons:(%d,%d,%d)\n", dev->x, dev->y,
                  dev->xrel, dev->yrel, (dev->state & SDL_BUTTON(SDL_BUTTON_LEFT)) ? 1 : 0,
                  (dev->state & SDL_BUTTON(SDL_BUTTON_MIDDLE)) ? 1 : 0, (dev->state & SDL_BUTTON(SDL_BUTTON_RIGHT)) ? 1 : 0);
    };
    if (SDL_SemWait(vid_mouse_events.sem) == 0) {
        if (!vptr->vid_mouse_captured) {
            event->xrel = (event->x - vid_cursor_x);
            event->yrel = (event->y - vid_cursor_y);
        }
        vid_mouse_b1 = (event->state & SDL_BUTTON(SDL_BUTTON_LEFT)) ? TRUE : FALSE;
        vid_mouse_b2 = (event->state & SDL_BUTTON(SDL_BUTTON_MIDDLE)) ? TRUE : FALSE;
        vid_mouse_b3 = (event->state & SDL_BUTTON(SDL_BUTTON_RIGHT)) ? TRUE : FALSE;
        sim_debug(SIM_VID_DBG_MOUSE, vptr->vid_dev,
                  "Mouse Move Event: pos:(%d,%d) rel:(%d,%d) buttons:(%d,%d,%d) - Count: %d vid_cursor:(%d,%d)\n", event->x,
                  event->y, event->xrel, event->yrel, (event->state & SDL_BUTTON(SDL_BUTTON_LEFT)) ? 1 : 0,
                  (event->state & SDL_BUTTON(SDL_BUTTON_MIDDLE)) ? 1 : 0, (event->state & SDL_BUTTON(SDL_BUTTON_RIGHT)) ? 1 : 0,
                  vid_mouse_events.count, vid_cursor_x, vid_cursor_y);
        if (vid_mouse_events.count < MAX_EVENTS) {
            SIM_MOUSE_EVENT *tail = &vid_mouse_events.events[(vid_mouse_events.tail + MAX_EVENTS - 1) % MAX_EVENTS];
            SIM_MOUSE_EVENT ev;

            ev.x_rel = event->xrel;
            ev.y_rel = event->yrel;
            ev.x_pos = event->x;
            ev.y_pos = event->y;
            ev.b1_state = vid_mouse_b1;
            ev.b2_state = vid_mouse_b2;
            ev.b3_state = vid_mouse_b3;
            ev.dev = vptr->vid_dev;
            ev.vptr = vptr;

            if ((vid_mouse_events.count > 0) &&                                       /* Is there a tail event? */
                (ev.b1_state == tail->b1_state) &&                                    /* With the same button state? */
                (ev.b2_state == tail->b2_state) && (ev.b3_state == tail->b3_state)) { /* Merge the motion */
                tail->x_rel += ev.x_rel;
                tail->y_rel += ev.y_rel;
                tail->x_pos = ev.x_pos;
                tail->y_pos = ev.y_pos;
                sim_debug(SIM_VID_DBG_MOUSE, vptr->vid_dev, "Mouse Move Event: Coalesced into pending event: (%d,%d)\n",
                          tail->x_rel, tail->y_rel);
            } else { /* Add a new event */
                vid_mouse_events.events[vid_mouse_events.tail++] = ev;
                vid_mouse_events.count++;
                if (vid_mouse_events.tail == MAX_EVENTS)
                    vid_mouse_events.tail = 0;
            }
        } else {
            sim_debug(SIM_VID_DBG_MOUSE, vptr->vid_dev, "Mouse Move Event Discarded: Count: %d\n", vid_mouse_events.count);
        }
        if (SDL_SemPost(vid_mouse_events.sem))
            sim_printf("%s: vid_mouse_move(): SDL_SemPost error: %s\n", vid_dname(vptr->vid_dev), SDL_GetError());
    }
}

void vid_mouse_button(SDL_MouseButtonEvent *event)
{
    /* If this window didn't have focus, then windowID is 0. From the SDL_MouseButtonEvent doc:
     * windowID -> "The window with mouse focus, if any". So, it's possible for the
     * windowID == 0. */

    if (event->windowID == 0)
        return;

    SDL_Event dummy_event;
    t_bool state;
    VID_DISPLAY *vptr = vid_get_event_window(event->windowID);

    if (vptr == NULL) {
        sim_printf("vid_mouse_button: Mouse button expected a valid window.\n");
        report_unrecognized_event((SDL_Event *)event);
        return;
    }

    if ((!vptr->vid_mouse_captured) && (vptr->vid_flags & SIM_VID_INPUTCAPTURED)) {
        if ((event->state == SDL_PRESSED) && (event->button == SDL_BUTTON_LEFT)) { /* left click and cursor not captured? */
            sim_debug(SIM_VID_DBG_KEY, vptr->vid_dev, "vid_mouse_button() - Cursor Captured\n");
            if (SDL_SetRelativeMouseMode(SDL_TRUE) < 0) /* lock cursor to window, hide cursor */
                sim_printf("%s: vid_mouse_button(): SDL_SetRelativeMouseMode error: %s\n", vid_dname(vptr->vid_dev),
                           SDL_GetError());
            SDL_WarpMouseInWindow(NULL, vptr->vid_width / 2, vptr->vid_height / 2); /* back to center */
            SDL_PumpEvents();
            while (SDL_PeepEvents(&dummy_event, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION)) {
            };
            vptr->vid_mouse_captured = TRUE;
        }
        return;
    }
    if (!sim_is_running)
        return;
    state = (event->state == SDL_PRESSED) ? TRUE : FALSE;
    if (SDL_SemWait(vid_mouse_events.sem) == 0) {
        switch (event->button) {
        case SDL_BUTTON_LEFT:
            vid_mouse_b1 = state;
            break;
        case SDL_BUTTON_MIDDLE:
            vid_mouse_b2 = state;
            break;
        case SDL_BUTTON_RIGHT:
            vid_mouse_b3 = state;
            break;
        }
        sim_debug(SIM_VID_DBG_MOUSE, vptr->vid_dev, "Mouse Button Event: State: %d, Button: %d, (%d,%d)\n", event->state,
                  event->button, event->x, event->y);
        if (vid_mouse_events.count < MAX_EVENTS) {
            SIM_MOUSE_EVENT ev;

            ev.x_rel = 0;
            ev.y_rel = 0;
            ev.x_pos = event->x;
            ev.y_pos = event->y;
            ev.b1_state = vid_mouse_b1;
            ev.b2_state = vid_mouse_b2;
            ev.b3_state = vid_mouse_b3;
            ev.dev = vptr->vid_dev;
            ev.vptr = vptr;

            vid_mouse_events.events[vid_mouse_events.tail++] = ev;
            vid_mouse_events.count++;
            if (vid_mouse_events.tail == MAX_EVENTS)
                vid_mouse_events.tail = 0;
        } else {
            sim_debug(SIM_VID_DBG_MOUSE, vptr->vid_dev, "Mouse Button Event Discarded: Count: %d\n", vid_mouse_events.count);
        }
        if (SDL_SemPost(vid_mouse_events.sem))
            sim_printf("%s: Mouse Button Event: SDL_SemPost error: %s\n", vid_dname(vptr->vid_dev), SDL_GetError());
    }
}

void vid_set_window_size(VID_DISPLAY *vptr, int32 w, int32 h)
{
    SDL_LockMutex(vptr->draw_mutex);
    vptr->vid_rect.h = h;
    vptr->vid_rect.w = w;
    SDL_UnlockMutex(vptr->draw_mutex);

    queue_simh_sdl_event(SIMH_EVENT_SIZE, "vid_set_window_size", vptr);
}

t_stat vid_native_renderer(t_bool flag)
{
    VID_DISPLAY *vptr;
    SimhRenderMode new_render_mode;

    new_render_mode = flag ? RENDER_MODE_HW : RENDER_MODE_SW;
    if (new_render_mode != vid_render_mode) {
        for (vptr = sim_displays; vptr != NULL; vptr = vptr->next) {
            queue_simh_sdl_event(SIMH_SWITCH_RENDERER, "vid_native_renderer", vptr);
        }

        vid_render_mode = flag ? RENDER_MODE_HW : RENDER_MODE_SW;
    }

    return SCPE_OK;
}

static int vid_new_window(VID_DISPLAY *vptr)
{
    Uint32 render_flags;

    vptr->vid_window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                        vptr->vid_width, vptr->vid_height, SDL_WINDOW_SHOWN);

    if (vptr->vid_window == NULL) {
        sim_printf("%s: Error Creating Video Window: %s\n", vid_dname(vptr->vid_dev), SDL_GetError());
        goto bail;
    }

    render_flags = SDL_RENDERER_TARGETTEXTURE
                   | (vid_render_mode == RENDER_MODE_SW ? SDL_RENDERER_SOFTWARE : SDL_RENDERER_ACCELERATED);

    vptr->renderer = SDL_CreateRenderer(vptr->vid_window, -1, render_flags);
    if (vptr->renderer == NULL) {
        sim_printf("%s: Error Creating renderer: %s\n", vid_dname(vptr->vid_dev), SDL_GetError());
        goto bail_1;
    }

    vptr->texture = SDL_CreateTexture(vptr->renderer, SIMH_PIXEL_FORMAT, SDL_TEXTUREACCESS_STREAMING,
                                         vptr->vid_width, vptr->vid_height);
    if (vptr->texture == NULL) {
        sim_printf("%s: Error renderer's texture: %s\n", vid_dname(vptr->vid_dev), SDL_GetError());
        goto bail_2;
    }

    vptr->vid_format = SDL_AllocFormat(SIMH_PIXEL_FORMAT);

    if (vptr->vid_flags & SIM_VID_RESIZABLE) {
        SDL_SetWindowResizable(vptr->vid_window, SDL_TRUE);
    }

    SDL_SetRenderDrawColor(vptr->renderer, 0, 0, 0, 255);
    SDL_RenderClear(vptr->renderer);
    SDL_RenderPresent(vptr->renderer);

    SDL_StopTextInput();

#if SDL_VERSION_ATLEAST(2, 0, 12)
    SDL_SetTextureScaleMode(vptr->texture, SDL_ScaleModeBest);
#endif

    vptr->vid_windowID = SDL_GetWindowID(vptr->vid_window);

    /* Rescale the window to fit physical display, if needed. */
    rescale_window(vptr);

    if (vptr->vid_flags & SIM_VID_INPUTCAPTURED) {
        char title[150];

        memset(title, 0, sizeof(title));
        strlcpy(title, vptr->vid_title, sizeof(title));
        strlcat(title, "                                             ReleaseKey=", sizeof(title));
        strlcat(title, vid_release_key, sizeof(title));
        SDL_SetWindowTitle(vptr->vid_window, title);
    } else
        SDL_SetWindowTitle(vptr->vid_window, vptr->vid_title);

    memset(&vptr->vid_key_state, 0, sizeof(vptr->vid_key_state));

    SDL_AtomicIncRef(&vid_active);
    return 1;

    /* Cleanup and bail out. */
bail_2:
    SDL_DestroyRenderer(vptr->renderer);
    vptr->renderer = NULL;
bail_1:
    SDL_DestroyWindow(vptr->vid_window);
    vptr->vid_window = NULL;
bail:
    SDL_Quit();
    return 0;
}

t_stat vid_set_alpha_mode (VID_DISPLAY *vptr, int mode)
{
SDL_BlendMode x;
switch (mode) {
    case SIM_ALPHA_NONE:
        vptr->vid_blending = FALSE;
        x = SDL_BLENDMODE_NONE;
        break;
    case SIM_ALPHA_BLEND:
        vptr->vid_blending = TRUE;
        x = SDL_BLENDMODE_BLEND;
        break;
    case SIM_ALPHA_ADD:
        vptr->vid_blending = TRUE;
        x = SDL_BLENDMODE_ADD;
        break;
    case SIM_ALPHA_MOD:
        vptr->vid_blending = TRUE;
        x = SDL_BLENDMODE_MOD;
        break;
    default:
        return SCPE_ARG;
    }
if (SDL_SetTextureBlendMode (vptr->texture, x))
    return SCPE_IERR;
if (SDL_SetRenderDrawBlendMode (vptr->renderer, x)
    || SDL_SetSurfaceBlendMode (SDL_GetWindowSurface(vptr->vid_window), x))
    return SCPE_IERR;
return SCPE_OK;
}

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * SIMH event handlers:
 *=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

/* Null (no-op) user event handler. */
static void null_simh_event (const SDL_Event *ev)
{
    UNUSED_ARG(ev);
}

/* SIMH_EVENT_SIZE */
static void simh_size_event(const SDL_Event *ev)
{
    VID_DISPLAY *vptr = (VID_DISPLAY *) ev->user.data1;

    SDL_SetWindowSize(vptr->vid_window, vptr->vid_rect.w, vptr->vid_rect.h);
    SDL_RenderSetLogicalSize(vptr->renderer, vptr->vid_rect.w, vptr->vid_rect.h);
}

/* SIMH_EVENT_CLOSE */
static void simh_close_event(const SDL_Event *ev)
{
    VID_DISPLAY *vptr = (VID_DISPLAY *) ev->user.data1;
    VID_DISPLAY *parent;

    SDL_AtomicSet(&vptr->vid_ready, FALSE);
    if (vptr->vid_cursor != NULL) {
        SDL_FreeCursor(vptr->vid_cursor);
        vptr->vid_cursor = NULL;
    }

    SDL_DestroyTexture(vptr->texture);
    vptr->texture = NULL;
    SDL_DestroyRenderer(vptr->renderer);
    vptr->renderer = NULL;
    SDL_DestroyWindow(vptr->vid_window);
    vptr->vid_window = NULL;
    SDL_DestroyMutex(vptr->draw_mutex);
    vptr->draw_mutex = NULL;

    for (parent = sim_displays; parent != NULL; parent = parent->next) {
        if (parent->next == vptr)
            parent->next = vptr->next;
    }

    SDL_AtomicDecRef(&vid_active);
}

/* SIMH_EVENT_CURSOR */
static void simh_update_cursor(const SDL_Event *ev)
{
    VID_DISPLAY *vptr = (VID_DISPLAY *) ev->user.data1;
    SDL_Cursor *cursor = vptr->cursor_data;
    t_bool visible = vptr->cursor_visible;

    if (cursor == NULL)
        return;

    sim_debug(SIM_VID_DBG_VIDEO, vptr->vid_dev,
              "Cursor Update Event: Previously %s, Now %s, New Cursor object "
              "at: %p, Old Cursor object at: %p\n",
              SDL_ShowCursor(-1) ? "visible" : "invisible",
              visible ? "visible" : "invisible", cursor, vptr->vid_cursor);
    SDL_SetCursor(cursor);
    if ((vptr->vid_window == SDL_GetMouseFocus()) && visible)
        SDL_WarpMouseInWindow(NULL, vid_cursor_x,
                              vid_cursor_y); /* sync position */
    if ((vptr->vid_cursor != cursor) && (vptr->vid_cursor))
        SDL_FreeCursor(vptr->vid_cursor);
    vptr->vid_cursor = cursor;
    SDL_ShowCursor(visible);
    vptr->vid_cursor_visible = visible;
}

/* SIMH_EVENT_CURSOR */
static void simh_warp_position(const SDL_Event *ev)
{
    VID_DISPLAY *vptr = (VID_DISPLAY *) ev->user.data1;

    UNUSED_ARG(ev);

    sim_debug(SIM_VID_DBG_VIDEO, vptr->vid_dev,
              "Mouse Warp Event: Warp to: (%d,%d)\n", vid_cursor_x,
              vid_cursor_y);

    SDL_PumpEvents();
    SDL_WarpMouseInWindow(NULL, vid_cursor_x, vid_cursor_y);
    SDL_PumpEvents();
}

/* SIMH_EVENT_DRAW */
/* Collect the video displays in the events array (bounded by n_events), invoke SDL_RenderClear
 * to initialize the drawing/output update. Returns the number of unique VID_DISPLAYs.
 *
 * By convention, the .user.data1 SDL_Event member is the VID_DISPLAY pointer.
 * 
 * events: The caller's collected events arrau
 * n_events: Number of elements to scan
 * disps: The caller's VID_DISPLAY * array 
 * n_disps: The size of the disps array.
 */
static size_t do_render_init(SDL_Event *events, int n_events, VID_DISPLAY **disps, size_t n_disps)
{
    size_t i, total_disps = 0;

    for (i = 0; i < (size_t) n_events && total_disps < n_disps; ++i) {
        size_t j;
        VID_DISPLAY *vptr = (VID_DISPLAY *) events[i].user.data1;

        for (j = 0; j < total_disps && disps[j] != vptr; ++j)
            /* NOP */;
        if (j >= total_disps && SDL_AtomicGet(&vptr->vid_ready) != 0) {
            disps[total_disps] = vptr;
            SDL_RenderClear(disps[total_disps]->renderer);
            ++total_disps;
        }
    }

    return total_disps;
}

/* do_render_init()'s counterpart: Send the updated SDL backing store to the physical display
 * via SDL_RenderPresent. */
static void do_render_update(VID_DISPLAY **disps, size_t n_disps)
{
    size_t i;

    for (i = 0; i < n_disps; ++i) {
        if (SDL_AtomicGet(&disps[i]->vid_ready) != 0) {
            if (!disps[i]->vid_blending && SDL_RenderCopy (disps[i]->renderer, disps[i]->texture, NULL, NULL))
                sim_printf ("%s: do_render_update: SDL_RenderCopy error: %s\n", vid_dname(disps[i]->vid_dev), SDL_GetError());
            SDL_RenderPresent(disps[i]->renderer);

            /* Bit of a punt on the debugging output -- one of the devices might have debugging turned on. */
            sim_debug(SIM_VID_DBG_VIDEO, disps[i]->vid_dev, "do_render_update: Render update sent to %" SIZE_T_FMT "u displays\n", n_disps);
        }
    }
}

static void do_draw_blit(VID_DISPLAY *vptr, size_t idx)
{
    DrawOp       *draw = &vptr->draw_ops[idx];
    SDL_Rect     *blit_rect = &draw->blit_rect;
    const uint32 *buf       = draw->blit_data;
    size_t        n_buf     = draw->n_blit_data;
    void         *blit_area = NULL;
    int           blit_pitch = 0;

    if (SDL_AtomicGet(&vptr->vid_ready) == 0)
        return;

    sim_debug(SIM_VID_DBG_VIDEO, vptr->vid_dev,
              "do_draw_blit: index [%" SIZE_T_FMT "u] (%d,%d,%d,%d) in_use = %d\n",
              draw - vptr->draw_ops, draw->blit_rect.x, draw->blit_rect.y, draw->blit_rect.w, draw->blit_rect.h,
              SDL_AtomicGet(&draw->in_use));

    SDL_LockTexture(vptr->texture, blit_rect, &blit_area, &blit_pitch);
    ASSURE(blit_pitch == blit_rect->w * sizeof(buf[0]));
    memcpy(blit_area, buf, n_buf);
    SDL_UnlockTexture(vptr->texture);

    if (vptr->vid_blending && SDL_RenderCopy (vptr->renderer, vptr->texture, blit_rect, blit_rect)) {
        sim_printf ("%s: SDL_RenderCopy: %s\n", vid_dname(vptr->vid_dev), SDL_GetError());
    }

    /* Acquire mutex for only the things we mutate. */
    SDL_AtomicSet(&draw->in_use, FALSE);
}

static void simh_draw_event(const SDL_Event *ev)
{
    int          n_events = 0, n_peek;
    /* Coalesced events. */
    SDL_Event    draw_events[18] = { 0, };
    /* Eight displays seems like overkill. See note in simh_draw_onto() for future
     * per-VID_DISPLAY processing. */
    VID_DISPLAY *disps[8] = { NULL, };
    Uint32       draw_event_id = evidentifier_for(SIMH_EVENT_DRAW);
    const int    n_draw_events = sizeof(draw_events) / sizeof(draw_events[0]);
    const size_t n_displays = sizeof(disps) / sizeof(disps[0]);
    size_t       i, n_disps;

    draw_events[0] = *ev;
    ++n_events;

    /* Grab other draw events that might be in the SDL event queue, being
     * opportunistic to avoid round trips through the SDL event loop, one
     * at a time. 
     * 
     * TT2500 is **VERY** timing sensitive and SDL_RenderPresent() is a
     * heavyweight operation, which necessitates this bulk processing. */
    if ((n_peek = SDL_PeepEvents(draw_events + 1, n_draw_events - 1, SDL_GETEVENT, draw_event_id, draw_event_id)) > 0) {
        n_events += n_peek;
    }

    n_disps = do_render_init(draw_events, n_events, disps, n_displays);

    for (i = 0; i < (size_t) n_events; ++i) {
        do_draw_blit((VID_DISPLAY *) draw_events[i].user.data1, (size_t) draw_events[i].user.data2);
    }

    do_render_update(disps, n_disps);

    sim_debug(SIM_VID_DBG_VIDEO, ((VID_DISPLAY *) ev->user.data1)->vid_dev, "simh_draw_event: processed %d events\n", n_events);
}

/* SIMH_EVENT_SHOW ("show video") */
static void simh_show_video_event(const SDL_Event *ev)
{
    ShowVideoCmd_t *cmd = (ShowVideoCmd_t *) ev->user.data1;
    t_stat show_status;;

    SDL_LockMutex(cmd->show_mutex);
    show_status = show_sdl_info(cmd->show_stream, cmd->show_uptr, cmd->show_val, cmd->show_desc);
    SDL_AtomicSet(&cmd->show_stat, show_status);
    SDL_CondSignal(cmd->show_cond);
    SDL_UnlockMutex(cmd->show_mutex);
}

/* SIMH_EVENT_OPEN, SIMH_EVENT_OPENCOMPLETE: Previous implementation used a 20x100ms delay
 * in the hope that the event thread would start and process the SIMH_EVENT_OPEN
 * message to initialize the simulator's video. The delay time is
 * non-determinisitic due to accrued delays across OS thread scheduling latency,
 * window subsystem initialization and other factors.
 *
 * The current implementation uses a condition variable and associated mutex to
 * signal SIMH_EVENT_OPEN's completion.
 *
 *   - The thread that generates the SIMH_EVENT_OPEN event initializes a completion
 *     structure that contains the condition variable, queues the SIMH_EVENT_OPEN
 *     event and waits for the condition to signal.
 *
 *   - The SDL event loop processes the SIMH_EVENT_OPEN, does all of the deferred
 *     initialization, and queues SIMH_EVENT_OPENCOMPLETE.
 *
 *   - SIMH_EVENT_OPENCOMPLETE signals the condition variable and the
 *     SDL_CondWait()-ing thread awakes.
 *
 * The SIMH_EVENT_OPENCOMPLETE event is queued vice signaled in simh_open_event() so
 * that SDL has a chance to process the underlying windowing system's events.
 */
static void simh_open_event(const SDL_Event *ev)
{
    VID_DISPLAY *vptr = (VID_DISPLAY *) ev->user.data1;
    open_complete_t *completion = (open_complete_t *) ev->user.data2;
    SDL_Event complete_event = {
        .user.type = evidentifier_for(SIMH_EVENT_OPENCOMPLETE),
        .user.code = 0,
        .user.data1 = completion,
        .user.data2 = NULL
    };

    vid_new_window(vptr);
    SDL_AtomicSet(&vptr->vid_ready, TRUE);
    queue_sdl_event(&complete_event, "simh_open_event", vptr);
}

static void simh_open_complete(const SDL_Event *ev)
{
    /* SIMH_EVENT_OPEN has completed, release the condition */
    open_complete_t *complete = (open_complete_t *) ev->user.data1;

    SDL_LockMutex(complete->completed_mutex);
    SDL_CondSignal(complete->completed_cond);
    SDL_UnlockMutex(complete->completed_mutex);
}

/* SIMH_EVENT_SCREENSHOT */
static void simh_screenshot_event(const SDL_Event *ev)
{
    SimScreenShot_t *sshot = (SimScreenShot_t *) ev->user.data1;
    char *name = (char *) calloc(strlen(sshot->filename) + 5, sizeof(char));
    const char *extension = strrchr(sshot->filename, '.');
    t_stat shot_status = SCPE_OK;

    if (name != NULL) {
        VID_DISPLAY *vptr;
        int i = 0;
        size_t n;

        if (extension != NULL) {
            n = extension - sshot->filename;
        } else {
            n = strlen(sshot->filename);
            extension = (char *) "";
        }

        strncpy(name, sshot->filename, n);

        for (vptr = sim_displays; vptr != NULL; vptr = vptr->next) {
            if (SDL_AtomicGet(&vid_active) > 1)
                sprintf(name + n, "%d%s", i++, extension);
            else
                sprintf(name + n, "%s", extension);

            if ((shot_status = do_screenshot(vptr, name)) != SCPE_OK)
                break;
        }
        free(name);
    } else {
        shot_status = SCPE_NXM;
    }

    SDL_LockMutex(sshot->screenshot_mutex);
    SDL_AtomicSet(&sshot->screenshot_stat, shot_status);
    SDL_CondSignal(sshot->screenshot_cond);
    SDL_UnlockMutex(sshot->screenshot_mutex);
}

/* SIMH_EVENT_BEEP handler: */
static void simh_beep_event (const SDL_Event *ev)
{
    UNUSED_ARG(ev);

    beep_ctx.beep_state = START_BEEP;
    unpause_audio();
}

/* SIMH_EVENT_FULLSCREEN */
static void simh_fullscreen_event(const SDL_Event *ev)
{
    VID_DISPLAY *vptr = (VID_DISPLAY *) ev->user.data1;

    if (vptr->fullscreen_flag)
        SDL_SetWindowFullscreen(vptr->vid_window,
                                SDL_WINDOW_FULLSCREEN_DESKTOP);
    else
        SDL_SetWindowFullscreen(vptr->vid_window, 0);
}

/* SIMH_EVENT_LOGICAL_SIZE */
static void simh_set_logical_size(const SDL_Event *ev)
{
    VID_DISPLAY *vptr = (VID_DISPLAY *) ev->user.data1;

    /* vid_set_logical_size updated vptr->vid_rect; set the renderer's logical
     * size here in the SDL main thread. */
    SDL_RenderSetLogicalSize(vptr->renderer, vptr->vid_rect.w, vptr->vid_rect.h);
}

/* SIMH_SWITCH_RENDERER: Copy the current window surface into the hardware texture.
 * One might be tempted to do something clever, like SDL_CreateTextureFromSurface()
 * and SDL_RenderCopy(). That path is paved with sharp rocks. */
static void simh_switch_renderer(const SDL_Event *ev)
{
    VID_DISPLAY *vptr = (VID_DISPLAY *) ev->user.data1;

    if (SDL_AtomicGet(&vptr->vid_ready) != 0) {
        int w, h;
        Uint32 *pixels;
        Uint32 flags;
        void *texbuf;
        int texpitch;

        SDL_LockMutex(vptr->draw_mutex);

        SDL_GetWindowSize(vptr->vid_window, &w, &h);
        pixels = (Uint32 *) calloc(1, w * h * sizeof(*pixels));
        SDL_RenderReadPixels(vptr->renderer, NULL, SIMH_PIXEL_FORMAT, pixels, w * sizeof(*pixels));

        SDL_DestroyTexture(vptr->texture);
        SDL_DestroyRenderer(vptr->renderer);


        flags = SDL_RENDERER_TARGETTEXTURE
                | (vid_render_mode == RENDER_MODE_SW ? SDL_RENDERER_SOFTWARE : SDL_RENDERER_ACCELERATED);

        vptr->renderer = SDL_CreateRenderer(vptr->vid_window, -1, flags);
        vptr->texture = SDL_CreateTexture(vptr->renderer, SIMH_PIXEL_FORMAT, SDL_TEXTUREACCESS_STREAMING,
                                        vptr->vid_width, vptr->vid_height);

        SDL_RenderClear(vptr->renderer);

        SDL_LockTexture(vptr->texture, NULL, &texbuf, &texpitch);
        memcpy(texbuf, pixels, w * h * sizeof(*pixels));
        SDL_UnlockTexture(vptr->texture);

        SDL_RenderCopy(vptr->renderer, vptr->texture, NULL, NULL);
        SDL_RenderPresent(vptr->renderer);

        free(pixels);
        SDL_UnlockMutex(vptr->draw_mutex);
    }
}

/* SIMH_DRAW_ONTO */
static void simh_draw_onto(const SDL_Event *ev)
{
    /* Coalesced events. */
    SDL_Event    onto_events[18] = { 0, };
    VID_DISPLAY *disps[14];
    int          n_events, n_peep;
    const int    n_onto_events = sizeof(onto_events) / sizeof(onto_events[0]);
    const size_t n_displays = sizeof(disps) / sizeof(disps[0]);
    Uint32       onto_event_id = evidentifier_for(SIMH_DRAW_ONTO);
    size_t       i, n_disps = 0;

    /* Bulk process SIMH_DRAW_ONTO events that are in the current SDL event queue. Note that
     * we can't make (m)any simplifying assumptions that allow us to combine these events.
     * The 'onto' draw_data might mutate some internal state that would become unsynchronized
     * were we to combine/elide SIMH_DRAW_ONTO events.
     * 
     * Future: Could rip through the events and process each VID_DISPLAY, combining all of its
     * updates in one pass. SDL_RenderPresent() is a heavyweight operation, so it can't be
     * called for each onto_fn(), but could be called periodically, depending on the number of
     * updates pending.
     * 
     * IMLAC is notorious for queuing a lot of updates that result in weird flickering
     * behavior. */
    onto_events[0] = *ev;
    n_events = 1;
    if ((n_peep = SDL_PeepEvents(onto_events + 1, n_onto_events - 1, SDL_GETEVENT, onto_event_id, onto_event_id)) > 0) {
        n_events += n_peep;
    }

    /* SDL_RenderClear -- clear the SDL backing store in prep for update. */
    n_disps = do_render_init(onto_events, n_events, disps, n_displays);

    /* Rip through the events... */
    for (i = 0; i < (size_t) n_events; ++i) {
        VID_DISPLAY *vptr = (VID_DISPLAY *) onto_events[i].user.data1;
        size_t       idx = (size_t) onto_events[i].user.data2;
        DrawOnto    *onto = &vptr->onto_ops[idx];
        void        *blit_area;
        int          blit_pitch;

        SDL_LockTexture(vptr->texture, &onto->onto_rect, &blit_area, &blit_pitch);
        onto->draw_fn(vptr, vptr->vid_dev, onto->onto_rect.x, onto->onto_rect.y, onto->onto_rect.w, onto->onto_rect.h,
                      onto->draw_data, blit_area, blit_pitch);
        SDL_UnlockTexture(vptr->texture);
        SDL_AtomicSet(&onto->pending, FALSE);
    }

    /* Emit SDL's backing store to the user's monitor: */
    do_render_update(disps, n_disps);
}

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * Version string formatting
 *=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

const char *vid_version(void)
{
static char SDLVersion[160];
SDL_version compiled = { 0, }, running = { 0, };

SDL_GetVersion(&running);

SDL_VERSION(&compiled);

SDLVersion[sizeof (SDLVersion) - 1] = '\0';
if ((compiled.major == running.major) &&
    (compiled.minor == running.minor) &&
    (compiled.patch == running.patch))
    snprintf(SDLVersion, sizeof (SDLVersion) - 1, "SDL Version %d.%d.%d",
                        compiled.major, compiled.minor, compiled.patch);
else
    snprintf(SDLVersion, sizeof (SDLVersion) - 1, "SDL Version (Compiled: %d.%d.%d, Runtime: %d.%d.%d)",
                        compiled.major, compiled.minor, compiled.patch,
                        running.major, running.minor, running.patch);
#if defined (HAVE_LIBPNG)
if (1) {
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (strcmp (PNG_LIBPNG_VER_STRING, png_get_libpng_ver (png)))
        snprintf(&SDLVersion[strlen (SDLVersion)], sizeof (SDLVersion) - (strlen (SDLVersion) + 1),
                            ", PNG Version (Compiled: %s, Runtime: %s)",
                            PNG_LIBPNG_VER_STRING, png_get_libpng_ver (png));
    else
        snprintf(&SDLVersion[strlen (SDLVersion)], sizeof (SDLVersion) - (strlen (SDLVersion) + 1),
                            ", PNG Version %s", PNG_LIBPNG_VER_STRING);
    png_destroy_read_struct(&png, NULL, NULL);
#if defined (ZLIB_VERSION)
    if (strcmp (ZLIB_VERSION, zlibVersion ()))
        snprintf(&SDLVersion[strlen (SDLVersion)], sizeof (SDLVersion) - (strlen (SDLVersion) + 1),
                            ", zlib: (Compiled: %s, Runtime: %s)", ZLIB_VERSION, zlibVersion ());
    else
        snprintf(&SDLVersion[strlen (SDLVersion)], sizeof (SDLVersion) - (strlen (SDLVersion) + 1),
                            ", zlib: %s", ZLIB_VERSION);
#endif
    }
#endif
return (const char *)SDLVersion;
}

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * "show video" command:
 *=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

t_stat vid_set_release_key(FILE *st, UNIT *uptr, int32 val, const void *desc)
{
    UNUSED_ARG(st);
    UNUSED_ARG(uptr);
    UNUSED_ARG(val);
    UNUSED_ARG(desc);

    return SCPE_NOFNC;
}

t_stat vid_show_release_key(FILE *st, UNIT *uptr, int32 val, const void *desc)
{
    VID_DISPLAY *vptr;

    UNUSED_ARG(uptr);
    UNUSED_ARG(val);
    UNUSED_ARG(desc);

    for (vptr = sim_displays; vptr != NULL; vptr = vptr->next) {
        if (vptr->vid_flags & SIM_VID_INPUTCAPTURED) {
            fprintf(st, "ReleaseKey=%s", vid_release_key);
            return SCPE_OK;
        }
    }
    return SCPE_OK;
}

static t_stat show_sdl_info(FILE *st, UNIT *uptr, int32 val, const void *desc)
{
    int i;
    VID_DISPLAY *vptr;

    fprintf(st, "Video support using SDL: %s\n", vid_version());
    if (SDL_AtomicGet(&vid_active) != 0) {
        for (vptr = sim_displays; vptr != NULL; vptr = vptr->next) {
            if (!vptr->vid_active_window)
                continue;
            fprintf(st, "  Currently Active Video Window: (%d by %d pixels)\n", vptr->vid_width, vptr->vid_height);
            fprintf(st, "  ");
            vid_show_release_key(st, uptr, val, desc);
        }
        fprintf(st, "\n");
        fprintf(st, "  SDL Video Driver: %s\n", SDL_GetCurrentVideoDriver());
    }
    for (i = 0; i < SDL_GetNumVideoDisplays(); ++i) {
        SDL_DisplayMode display;

        if (SDL_GetCurrentDisplayMode(i, &display)) {
            fprintf(st, "Could not get display mode for video display #%d: %s", i, SDL_GetError());
        } else {
            fprintf(st, "  Display %s(#%d): current display mode is %dx%dpx @ %dhz. \n", SDL_GetDisplayName(i), i, display.w,
                    display.h, display.refresh_rate);
        }
    }
    fprintf(st, "  Available SDL Renderers:\n");
    for (i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
        SDL_RendererInfo info;

        if (SDL_GetRenderDriverInfo(i, &info)) {
            fprintf(st, "Could not get render driver info for driver #%d: %s", i, SDL_GetError());
        } else {
            uint32 j, k;
            static const struct {
                uint32 format;
                const char *name;
            } PixelFormats[] = {{SDL_PIXELFORMAT_INDEX1LSB, "Index1LSB"}, {SDL_PIXELFORMAT_INDEX1MSB, "Index1MSB"},
                                {SDL_PIXELFORMAT_INDEX4LSB, "Index4LSB"}, {SDL_PIXELFORMAT_INDEX4MSB, "Index4MSB"},
                                {SDL_PIXELFORMAT_INDEX8, "Index8"},       {SDL_PIXELFORMAT_RGB332, "RGB332"},
                                {SDL_PIXELFORMAT_RGB444, "RGB444"},       {SDL_PIXELFORMAT_RGB555, "RGB555"},
                                {SDL_PIXELFORMAT_BGR555, "BGR555"},       {SDL_PIXELFORMAT_ARGB4444, "ARGB4444"},
                                {SDL_PIXELFORMAT_RGBA4444, "RGBA4444"},   {SDL_PIXELFORMAT_ABGR4444, "ABGR4444"},
                                {SDL_PIXELFORMAT_BGRA4444, "BGRA4444"},   {SDL_PIXELFORMAT_ARGB1555, "ARGB1555"},
                                {SDL_PIXELFORMAT_RGBA5551, "RGBA5551"},   {SDL_PIXELFORMAT_ABGR1555, "ABGR1555"},
                                {SDL_PIXELFORMAT_BGRA5551, "BGRA5551"},   {SDL_PIXELFORMAT_RGB565, "RGB565"},
                                {SDL_PIXELFORMAT_BGR565, "BGR565"},       {SDL_PIXELFORMAT_RGB24, "RGB24"},
                                {SDL_PIXELFORMAT_BGR24, "BGR24"},         {SDL_PIXELFORMAT_RGB888, "RGB888"},
                                {SDL_PIXELFORMAT_RGBX8888, "RGBX8888"},   {SDL_PIXELFORMAT_BGR888, "BGR888"},
                                {SDL_PIXELFORMAT_BGRX8888, "BGRX8888"},   {SDL_PIXELFORMAT_ARGB8888, "ARGB8888"},
                                {SDL_PIXELFORMAT_RGBA8888, "RGBA8888"},   {SDL_PIXELFORMAT_ABGR8888, "ABGR8888"},
                                {SDL_PIXELFORMAT_BGRA8888, "BGRA8888"},   {SDL_PIXELFORMAT_ARGB2101010, "ARGB2101010"},
                                {SDL_PIXELFORMAT_YV12, "YV12"},           {SDL_PIXELFORMAT_IYUV, "IYUV"},
                                {SDL_PIXELFORMAT_YUY2, "YUY2"},           {SDL_PIXELFORMAT_UYVY, "UYVY"},
                                {SDL_PIXELFORMAT_YVYU, "YVYU"},           {SDL_PIXELFORMAT_UNKNOWN, "Unknown"}};

            fprintf(st, "     Render #%d - %s\n", i, info.name);
            fprintf(st, "        Flags: 0x%X - ", info.flags);
            if (info.flags & SDL_RENDERER_SOFTWARE)
                fprintf(st, "Software|");
            if (info.flags & SDL_RENDERER_ACCELERATED)
                fprintf(st, "Accelerated|");
            if (info.flags & SDL_RENDERER_PRESENTVSYNC)
                fprintf(st, "PresentVSync|");
            if (info.flags & SDL_RENDERER_TARGETTEXTURE)
                fprintf(st, "TargetTexture|");
            fprintf(st, "\n");
            if ((info.max_texture_height != 0) || (info.max_texture_width != 0))
                fprintf(st, "        Max Texture: %d by %d\n", info.max_texture_height, info.max_texture_width);
            fprintf(st, "        Pixel Formats:\n");
            for (j = 0; j < info.num_texture_formats; j++) {
                for (k = 0; k < sizeof(PixelFormats) / sizeof(PixelFormats[0]); k++) {
                    if (PixelFormats[k].format == info.texture_formats[j]) {
                        fprintf(st, "            %s\n", PixelFormats[k].name);
                        break;
                    }
                    if (PixelFormats[k].format == SDL_PIXELFORMAT_UNKNOWN) {
                        fprintf(st, "            %s - 0x%X\n", PixelFormats[k].name, info.texture_formats[j]);
                        break;
                    }
                }
            }
        }
    }
    if (SDL_AtomicGet(&vid_active) != 0) {
        SDL_RendererInfo info;

        info.name = "";

        for (vptr = sim_displays; vptr != NULL; vptr = vptr->next) {
            if (vptr->vid_active_window) {
                SDL_GetRendererInfo(vptr->renderer, &info);
                break;
            }
        }
        fprintf(st, "  Currently Active Renderer: %s\n", info.name);
    }
    if (1) {
        static const char *hints[] = {
#if defined(SDL_HINT_FRAMEBUFFER_ACCELERATION)
            SDL_HINT_FRAMEBUFFER_ACCELERATION,
#endif
#if defined(SDL_HINT_RENDER_DRIVER)
            SDL_HINT_RENDER_DRIVER,
#endif
#if defined(SDL_HINT_RENDER_OPENGL_SHADERS)
            SDL_HINT_RENDER_OPENGL_SHADERS,
#endif
#if defined(SDL_HINT_RENDER_DIRECT3D_THREADSAFE)
            SDL_HINT_RENDER_DIRECT3D_THREADSAFE,
#endif
#if defined(SDL_HINT_RENDER_DIRECT3D11_DEBUG)
            SDL_HINT_RENDER_DIRECT3D11_DEBUG,
#endif
#if defined(SDL_HINT_RENDER_SCALE_QUALITY)
            SDL_HINT_RENDER_SCALE_QUALITY,
#endif
#if defined(SDL_HINT_RENDER_VSYNC)
            SDL_HINT_RENDER_VSYNC,
#endif
#if defined(SDL_HINT_VIDEO_ALLOW_SCREENSAVER)
            SDL_HINT_VIDEO_ALLOW_SCREENSAVER,
#endif
#if defined(SDL_HINT_VIDEO_X11_XVIDMODE)
            SDL_HINT_VIDEO_X11_XVIDMODE,
#endif
#if defined(SDL_HINT_VIDEO_X11_XINERAMA)
            SDL_HINT_VIDEO_X11_XINERAMA,
#endif
#if defined(SDL_HINT_VIDEO_X11_XRANDR)
            SDL_HINT_VIDEO_X11_XRANDR,
#endif
#if defined(SDL_HINT_GRAB_KEYBOARD)
            SDL_HINT_GRAB_KEYBOARD,
#endif
#if defined(SDL_HINT_MOUSE_RELATIVE_MODE_WARP)
            SDL_HINT_MOUSE_RELATIVE_MODE_WARP,
#endif
#if defined(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS)
            SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS,
#endif
#if defined(SDL_HINT_IDLE_TIMER_DISABLED)
            SDL_HINT_IDLE_TIMER_DISABLED,
#endif
#if defined(SDL_HINT_ORIENTATIONS)
            SDL_HINT_ORIENTATIONS,
#endif
#if defined(SDL_HINT_ACCELEROMETER_AS_JOYSTICK)
            SDL_HINT_ACCELEROMETER_AS_JOYSTICK,
#endif
#if defined(SDL_HINT_XINPUT_ENABLED)
            SDL_HINT_XINPUT_ENABLED,
#endif
#if defined(SDL_HINT_GAMECONTROLLERCONFIG)
            SDL_HINT_GAMECONTROLLERCONFIG,
#endif
#if defined(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS)
            SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,
#endif
#if defined(SDL_HINT_ALLOW_TOPMOST)
            SDL_HINT_ALLOW_TOPMOST,
#endif
#if defined(SDL_HINT_TIMER_RESOLUTION)
            SDL_HINT_TIMER_RESOLUTION,
#endif
#if defined(SDL_HINT_VIDEO_HIGHDPI_DISABLED)
            SDL_HINT_VIDEO_HIGHDPI_DISABLED,
#endif
#if defined(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK)
            SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK,
#endif
#if defined(SDL_HINT_VIDEO_WIN_D3DCOMPILER)
            SDL_HINT_VIDEO_WIN_D3DCOMPILER,
#endif
#if defined(SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT)
            SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT,
#endif
#if defined(SDL_HINT_WINRT_PRIVACY_POLICY_URL)
            SDL_HINT_WINRT_PRIVACY_POLICY_URL,
#endif
#if defined(SDL_HINT_WINRT_PRIVACY_POLICY_LABEL)
            SDL_HINT_WINRT_PRIVACY_POLICY_LABEL,
#endif
#if defined(SDL_HINT_WINRT_HANDLE_BACK_BUTTON)
            SDL_HINT_WINRT_HANDLE_BACK_BUTTON,
#endif
#if defined(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES)
            SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES,
#endif
            NULL};
        fprintf(st, "  Currently Active SDL Hints:\n");
        for (i = 0; hints[i]; i++) {
            if (SDL_GetHint(hints[i]))
                fprintf(st, "      %s = %s\n", hints[i], SDL_GetHint(hints[i]));
        }
    }
    return SCPE_OK;
}

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * Screenshot support:
 *=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

static t_stat do_screenshot(VID_DISPLAY *vptr, const char *filename)
{
    int stat;
    char *fullname = NULL;

    if (SDL_AtomicGet(&vid_active) == 0) {
        sim_printf("No video display is active\n");
        return SCPE_UDIS | SCPE_NOMESSAGE;
    }

    fullname = (char *)calloc(strlen(filename) + 5, sizeof(char));
    if (fullname == NULL)
        return SCPE_MEM;

    SDL_Surface *sshot = SDL_CreateRGBSurfaceWithFormat(0, vptr->vid_width, vptr->vid_height, SIMH_PIXEL_DEPTH, SIMH_PIXEL_FORMAT);

    SDL_RenderReadPixels(vptr->renderer, NULL, SIMH_PIXEL_FORMAT, sshot->pixels, sshot->pitch);

#if defined(HAVE_LIBPNG)
    if (!match_ext(filename, "bmp")) {
        sprintf(fullname, "%s%s", filename, match_ext(filename, "png") ? "" : ".png");
        stat = SDL_SavePNG(sshot, fullname);
    } else {
        sprintf(fullname, "%s", filename);
        stat = SDL_SaveBMP(sshot, fullname);
    }
#else
    sprintf(fullname, "%s%s", filename, match_ext(filename, "bmp") ? "" : ".bmp");
    stat = SDL_SaveBMP(sshot, fullname);
#endif /* defined(HAVE_LIBPNG) */

    SDL_FreeSurface(sshot);

    if (stat) {
        sim_printf("Error saving screenshot to %s: %s\n", fullname, SDL_GetError());
        free(fullname);
        return SCPE_IOERR | SCPE_NOMESSAGE;
    } else {
        if (!sim_quiet)
            sim_printf("Screenshot saved to %s\n", fullname);
        free(fullname);
        return SCPE_OK;
    }
}

t_stat vid_screenshot(const char *filename)
{
    SDL_cond *sshot_cond = SDL_CreateCond();
    SDL_mutex *sshot_mutex = SDL_CreateMutex();
    SimScreenShot_t sshot = {
        .screenshot_cond = sshot_cond,
        .screenshot_mutex = sshot_mutex,
        /* screenshot_stat initialized separately. */
        .filename = filename
    };

    SDL_AtomicSet(&sshot.screenshot_stat, -1);

    if (sshot_cond != NULL && sshot_mutex != NULL) {
        SDL_Event user_event = {
            .user.type = evidentifier_for(SIMH_EVENT_SCREENSHOT),
            .user.data1 = &sshot
        };

        SDL_LockMutex(sshot_mutex);
        if (deferred_startup_completed() && queue_sdl_event(&user_event, "vid_screenshot", NULL) >= 0) {
            SDL_CondWait(sshot_cond, sshot_mutex);
        } else {
            sim_messagef(SCPE_OK, "Video subsystem not currently active or initialized.\n");
        }
        SDL_UnlockMutex(sshot_mutex);
    } else if (sshot_cond == NULL) {
        sim_messagef(SCPE_NXM, "vid_screenshot(): SDL_CreateCond() error: %s\n", SDL_GetError());
    } else if (sshot_mutex == NULL) {
        sim_messagef(SCPE_NXM, "vid_screenshot(): SDL_CreateMutex() error: %s\n", SDL_GetError());
    }

    return SDL_AtomicGet(&sshot.screenshot_stat);
}

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * Beep/audio:
 *=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

static void unpause_audio()
{
#if SDL_VERSION_ATLEAST(2, 0, 6)
    if (vid_audio_dev > 0)
        SDL_PauseAudioDevice(vid_audio_dev, 0);
#else
    SDL_PauseAudio(0);
#endif
}

static void pause_audio()
{
#if SDL_VERSION_ATLEAST(2, 0, 6)
    if (vid_audio_dev > 0)
        SDL_PauseAudioDevice(vid_audio_dev, 1);
#else
    SDL_PauseAudio(1);
#endif
}

static void close_audio()
{
#if SDL_VERSION_ATLEAST(2, 0, 6)
    SDL_CloseAudioDevice(vid_audio_dev);
#else
    SDL_CloseAudio();
#endif
}

/* The audio playback workhorse function. This operates a state machine,
 * depending on whether the simulator waits for the audio tone to complete
 * (blocking on a condition variable) or the simulator just expects the callback
 * to output silence between consecutive tones (fully asynchronous audio.)
 *
 * Linux PulseAudio: 4K buffer (typically), so consecutive beeps will get
 *   aggregated together in one buffer. Some latency between nonconsecutive
 *   beeps.
 * Windows: 111 byte buffer. Better chance at aggregating beeps.
 */
static void vid_audio_callback(void *ctx, Uint8 *stream, int length)
{
    SimBeepContext *bctx = (SimBeepContext *)ctx;
    int filled;
    int stream_offset = 0;
    const int sample_size = (int)sizeof(*bctx->beep_data);

    sim_debug(SIM_VID_DBG_VIDEO, vid_first()->vid_dev, "audio: entry state %s\n", getBeepStateName(bctx->beep_state));

    for (filled = 0; !filled; /* empty */) {
        switch (bctx->beep_state) {
        case BEEP_IDLE:
            /* Should never see this in the callback. */
            break;

        case START_BEEP:
            bctx->beep_offset = 0;
            bctx->beep_state = OUTPUT_TONE;
            break;

        case OUTPUT_TONE: {
            int n_playout = (int) (bctx->beep_samples - bctx->beep_offset);
            int segment_len = n_playout * sample_size;

            if (length <= stream_offset + segment_len) {
                /* More samples than stream. */
                memcpy(stream + stream_offset, &bctx->beep_data[bctx->beep_offset], length - stream_offset);
                bctx->beep_offset += (length - stream_offset) / sample_size;
                filled = !filled;

                sim_debug(SIM_VID_DBG_VIDEO, vid_first()->vid_dev,
                          "audio: OUTPUT_TONE (filled) length %d, n_playout %d, stream_offset %d, beep_offset %d\n", length,
                          n_playout, stream_offset, bctx->beep_offset);
            } else {
                /* Copy remaining tone samples. */
                memcpy(stream + stream_offset, &bctx->beep_data[bctx->beep_offset], segment_len);
                bctx->beep_offset += n_playout;

                /* Fill remaining stream with silence. */
                stream_offset += segment_len;
                memset(stream + stream_offset, 0, length - stream_offset);

                if (!SDL_AtomicDecRef(&bctx->beeps_pending)) {
                    /* We have a pending beep to play out. Keep track of the amount of the silence
                        * gap we've just output and transition. */

                    bctx->beep_offset = 0;
                    bctx->beep_state = OUTPUT_GAP;

                    sim_debug(SIM_VID_DBG_VIDEO, vid_first()->vid_dev,
                                "audio: OUTPUT_TONE (transition) length %d, n_playout %d, stream_offset %d, beep_offset %d\n",
                                length, n_playout, stream_offset, bctx->beep_offset);
                } else {
                    /* Let the remainder play out */
                    bctx->beep_state = BEEP_DONE;
                    filled = !filled;

                    sim_debug(SIM_VID_DBG_VIDEO, vid_first()->vid_dev,
                                "audio: OUTPUT_TONE (done) length %d, n_playout %d, stream_offset %d, beep_offset %d\n", length,
                                n_playout, stream_offset, bctx->beep_offset);
                }
            }
        } break;

        case OUTPUT_GAP: {
            int n_playout = bctx->gap_samples - bctx->beep_offset;
            int segment_len = n_playout * sample_size;

            if (length <= stream_offset + segment_len) {
                /* Fill remainder, then come back. */
                memset(stream + stream_offset, 0, length - stream_offset);
                bctx->beep_offset += (length - stream_offset) / sample_size;
                filled = !filled;

                sim_debug(SIM_VID_DBG_VIDEO, vid_first()->vid_dev,
                          "audio: OUTPUT_GAP (more gap) length %d, n_playout %d, stream_offset %d, beep_offset %d\n", length,
                          n_playout, stream_offset, bctx->beep_offset);
            } else {
                /* Done with the silence gap... */
                memset(stream + stream_offset, 0, segment_len);
                stream_offset += segment_len;
                bctx->beep_offset = 0;
                bctx->beep_state = OUTPUT_TONE;
                sim_debug(SIM_VID_DBG_VIDEO, vid_first()->vid_dev,
                          "audio: OUTPUT_GAP (done gap) length %d, n_playout %d, stream_offset %d, beep_offset %d\n", length,
                          n_playout, stream_offset, bctx->beep_offset);
            }
        } break;

        case BEEP_DONE:
            /* All done. */
            pause_audio();
            bctx->beep_state = BEEP_IDLE;
            filled = !filled;
            break;
        }
    }

    sim_debug(SIM_VID_DBG_VIDEO, vid_first()->vid_dev, "audio: exit state %s\n", getBeepStateName(bctx->beep_state));
}

static void vid_beep_setup(int duration_ms, int tone_frequency)
{
    size_t i;
    int actual_dur;
    double omega = (2.0 * M_PI * ((double) tone_frequency)) / ((double) SAMPLE_FREQUENCY);
    double sample_pt;

    if (beep_ctx.beep_data != NULL) {
        free(beep_ctx.beep_data);
    }

    beep_ctx.beep_duration = duration_ms;

    actual_dur = duration_ms;

    beep_ctx.beep_samples = (SAMPLE_FREQUENCY * actual_dur) / 1000;
    beep_ctx.beep_data = (int16 *) malloc(sizeof(*beep_ctx.beep_data) * beep_ctx.beep_samples);
    for (i = 0, sample_pt = 0.0; i < beep_ctx.beep_samples; i++, sample_pt += 1.0)
        beep_ctx.beep_data[i] = (int16) (AMPLITUDE * sin(sample_pt * omega));

    beep_ctx.beep_offset = 0;
    beep_ctx.gap_samples = (SAMPLE_FREQUENCY * BEEP_EVENT_GAP) / 1000;
}

static void vid_beep_cleanup(void)
{
    close_audio();
    free(beep_ctx.beep_data);
    beep_ctx.beep_data = NULL;

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * Debug pretty printing:
 *=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

static void initWindowEventNames()
{
    size_t i;

    for (i = 0; i < sizeof(windoweventtypes) / sizeof(windoweventtypes[0]); ++i)
      windoweventtypes[i] = NULL;

    windoweventtypes[SDL_WINDOWEVENT_NONE] = "NONE";                /**< Never used */
    windoweventtypes[SDL_WINDOWEVENT_SHOWN] = "SHOWN";              /**< Window has been shown */
    windoweventtypes[SDL_WINDOWEVENT_HIDDEN] = "HIDDEN";            /**< Window has been hidden */
    windoweventtypes[SDL_WINDOWEVENT_EXPOSED] = "EXPOSED";          /**< Window has been exposed and should be
                                                                         redrawn */
    windoweventtypes[SDL_WINDOWEVENT_MOVED] = "MOVED";              /**< Window has been moved to data1, data2
                                     */
    windoweventtypes[SDL_WINDOWEVENT_RESIZED] = "RESIZED";          /**< Window has been resized to data1xdata2 */
    windoweventtypes[SDL_WINDOWEVENT_SIZE_CHANGED] = "SIZE_CHANGED";/**< The window size has changed, either as a result
                                                                         of an API call or through the system or user changing
                                                                         the window size. */
    windoweventtypes[SDL_WINDOWEVENT_MINIMIZED] = "MINIMIZED";      /**< Window has been minimized */
    windoweventtypes[SDL_WINDOWEVENT_MAXIMIZED] = "MAXIMIZED";      /**< Window has been maximized */
    windoweventtypes[SDL_WINDOWEVENT_RESTORED] = "RESTORED";        /**< Window has been restored to normal size
                                                                         and position */
    windoweventtypes[SDL_WINDOWEVENT_ENTER] = "ENTER";              /**< Window has gained mouse focus */
    windoweventtypes[SDL_WINDOWEVENT_LEAVE] = "LEAVE";              /**< Window has lost mouse focus */
    windoweventtypes[SDL_WINDOWEVENT_FOCUS_GAINED] = "FOCUS_GAINED";/**< Window has gained keyboard focus */
    windoweventtypes[SDL_WINDOWEVENT_FOCUS_LOST] = "FOCUS_LOST";    /**< Window has lost keyboard focus */
    windoweventtypes[SDL_WINDOWEVENT_CLOSE] = "CLOSE";              /**< The window manager requests that the
                                                                         window be closed */
    windoweventtypes[SDL_WINDOWEVENT_TAKE_FOCUS] = "TAKE_FOCUS";    /**< Window is being offered a focus (should
                                                                         SetWindowInputFocus() on itself or a subwindow,
                                                                         or ignore) */
}

static const char *getWindowEventName(Uint32 win_event)
{
    static char winevname_buf[48] = { '\0', };

    if (win_event < sizeof(windoweventtypes) / sizeof(windoweventtypes[0])) {
        const char *p = windoweventtypes[win_event];
        return (p != NULL ? p : "not documented");
    }

    sprintf(winevname_buf, "SDL window event %u", (Uint32) win_event);
    return winevname_buf;
}

static void initEventNames()
{
    size_t i;

    for (i = 0; i < sizeof(eventtypes) / sizeof(eventtypes[0]); ++i)
      eventtypes[i] = NULL;

    eventtypes[SDL_QUIT] = "QUIT";          /**< User-requested quit */

    /* These application events have special meaning on iOS, see README-ios.txt for details */
    eventtypes[SDL_APP_TERMINATING] = "APP_TERMINATING";
    eventtypes[SDL_APP_LOWMEMORY] = "APP_LOWMEMORY";
    eventtypes[SDL_APP_WILLENTERBACKGROUND] = "APP_WILLENTERBACKGROUND";
    eventtypes[SDL_APP_DIDENTERBACKGROUND] = "APP_DIDENTERBACKGROUND";
    eventtypes[SDL_APP_WILLENTERFOREGROUND] = "APP_WILLENTERFOREGROUND";
    eventtypes[SDL_APP_DIDENTERFOREGROUND] = "APP_DIDENTERFOREGROUND";

    /* Window events */
    eventtypes[SDL_WINDOWEVENT] = "WINDOWEVENT";
    eventtypes[SDL_SYSWMEVENT] = "SYSWMEVENT";

    /* Keyboard events */
    eventtypes[SDL_KEYDOWN] = "KEYDOWN";
    eventtypes[SDL_KEYUP] = "KEYUP";
    eventtypes[SDL_TEXTEDITING] = "TEXTEDITING";
    eventtypes[SDL_TEXTINPUT] = "TEXTINPUT";
    eventtypes[SDL_KEYMAPCHANGED] = "SDL_KEYMAPCHANGED";

    eventtypes[SDL_MOUSEMOTION] = "MOUSEMOTION";
    eventtypes[SDL_MOUSEBUTTONDOWN] = "MOUSEBUTTONDOWN";
    eventtypes[SDL_MOUSEBUTTONUP] = "MOUSEBUTTONUP";
    eventtypes[SDL_MOUSEWHEEL] = "MOUSEWHEEL";

    eventtypes[SDL_JOYAXISMOTION] = "JOYAXISMOTION";
    eventtypes[SDL_JOYBALLMOTION] = "JOYBALLMOTION";
    eventtypes[SDL_JOYHATMOTION] = "JOYHATMOTION";
    eventtypes[SDL_JOYBUTTONDOWN] = "JOYBUTTONDOWN";
    eventtypes[SDL_JOYBUTTONUP] = "JOYBUTTONUP";
    eventtypes[SDL_JOYDEVICEADDED] = "JOYDEVICEADDED";
    eventtypes[SDL_JOYDEVICEREMOVED] = "JOYDEVICEREMOVED";

    eventtypes[SDL_CONTROLLERAXISMOTION] = "CONTROLLERAXISMOTION";
    eventtypes[SDL_CONTROLLERBUTTONDOWN] = "CONTROLLERBUTTONDOWN";
    eventtypes[SDL_CONTROLLERBUTTONUP] = "CONTROLLERBUTTONUP";
    eventtypes[SDL_CONTROLLERDEVICEADDED] = "CONTROLLERDEVICEADDED";
    eventtypes[SDL_CONTROLLERDEVICEREMOVED] = "CONTROLLERDEVICEREMOVED";
    eventtypes[SDL_CONTROLLERDEVICEREMAPPED] = "CONTROLLERDEVICEREMAPPED";

    /* Touch events */
    eventtypes[SDL_FINGERDOWN] = "FINGERDOWN";
    eventtypes[SDL_FINGERUP] = "FINGERUP";
    eventtypes[SDL_FINGERMOTION] = "FINGERMOTION";

    /* Gesture events */
    eventtypes[SDL_DOLLARGESTURE] = "DOLLARGESTURE";
    eventtypes[SDL_DOLLARRECORD] = "DOLLARRECORD";
    eventtypes[SDL_MULTIGESTURE] = "MULTIGESTURE";

    /* Clipboard events */
    eventtypes[SDL_CLIPBOARDUPDATE] = "CLIPBOARDUPDATE"; /**< The clipboard changed */

    /* Audio events */
    eventtypes[SDL_AUDIODEVICEADDED] = "AUDIODEVICEADDED";
    eventtypes[SDL_AUDIODEVICEREMOVED] = "AUDIODEVICEREMOVED";

    /* Drag and drop events */
    eventtypes[SDL_DROPFILE] = "DROPFILE"; /**< The system requests a file open */

#if (SDL_MINOR_VERSION > 0) || (SDL_PATCHLEVEL >= 3)
    /* Render events */
    eventtypes[SDL_RENDER_TARGETS_RESET] = "RENDER_TARGETS_RESET"; /**< The render targets have been reset */
#endif

#if (SDL_MINOR_VERSION > 0) || (SDL_PATCHLEVEL >= 4)
    /* Render events */
    eventtypes[SDL_RENDER_DEVICE_RESET] = "RENDER_DEVICE_RESET"; /**< The render device has been reset */
#endif

    /* Initializing eventtypes array with the SIMH user events will overwrite this entry. */
    eventtypes[SDL_USEREVENT] = "USEREVENT";
}

static const char *getEventName(Uint32 ev)
{
    static char evname_buf[48] = { '\0', };

    if (ev < sizeof(eventtypes) / sizeof(eventtypes[0])) {
        const char *p = eventtypes[ev];
        return (p != NULL ? p : "not documented");
    }

    sprintf(evname_buf, "SDL event %u", (Uint32) ev);
    return evname_buf;
}

static const char *getUserCodeName(Uint32 user_ev_code)
{
    static char uevname_buf[48] = { '\0', };
    size_t event_idx = user_ev_code - simh_event_dispatch[0].event_id;

    if (event_idx < N_SIMH_EVENTS)
        return simh_event_dispatch[event_idx].description;

    sprintf(uevname_buf, "SIMH user event %d", user_ev_code);
    return uevname_buf;
}

static const char *getBeepStateName(SimBeepState state)
{
    static char beep_state_buf[48];

    if (state >= BEEP_IDLE && state <= BEEP_DONE)
        return beep_state_names[state];

    sprintf(beep_state_buf, "Beep state %d", state);
    return beep_state_buf;
}

#else /* !(defined(USE_SIM_VIDEO) && defined(HAVE_LIBSDL)) */
/* Non-implemented versions */

int vid_is_active()
{
  return 0;
}

t_stat vid_open (DEVICE *dptr, const char *title, uint32 width, uint32 height, int flags)
{
return SCPE_NOFNC;
}

t_stat vid_close (void)
{
return SCPE_OK;
}

t_stat vid_close_all (void)
{
return SCPE_OK;
}

t_stat vid_poll_kb (SIM_KEY_EVENT *ev)
{
return SCPE_EOF;
}

t_stat vid_poll_mouse (SIM_MOUSE_EVENT *ev)
{
return SCPE_EOF;
}

uint32 vid_map_rgb (uint8 r, uint8 g, uint8 b)
{
return 0;
}

void vid_draw (int32 x, int32 y, int32 w, int32 h, const uint32 *buf)
{
return;
}

t_stat vid_set_cursor (t_bool visible, uint32 width, uint32 height, uint8 *data, uint8 *mask, uint32 hot_x, uint32 hot_y)
{
return SCPE_NOFNC;
}

void vid_set_cursor_position (int32 x, int32 y)
{
return;
}

void vid_refresh (void)
{
return;
}

void vid_beep (void)
{
return;
}

const char *vid_version (void)
{
return "No Video Support";
}

t_stat vid_set_release_key (FILE* st, UNIT* uptr, int32 val, const void* desc)
{
return SCPE_NOFNC;
}

t_stat vid_show_release_key (FILE* st, UNIT* uptr, int32 val, const void* desc)
{
fprintf (st, "no release key");
return SCPE_OK;
}

t_stat vid_show_video (FILE* st, UNIT* uptr, int32 val, const void* desc)
{
fprintf (st, "video support unavailable\n");
return SCPE_OK;
}

t_stat vid_screenshot (const char *filename)
{
sim_printf ("video support unavailable\n");
return SCPE_NOFNC|SCPE_NOMESSAGE;
}

t_bool vid_is_fullscreen (void)
{
sim_printf ("video support unavailable\n");
return FALSE;
}

t_stat vid_set_fullscreen (t_bool flag)
{
sim_printf ("video support unavailable\n");
return SCPE_OK;
}

t_stat vid_native_renderer(t_bool flag)
{
UNUSED_ARG(flag);
sim_printf ("video support unavailable\n");
return SCPE_OK;
}

t_stat vid_open_window (VID_DISPLAY **vptr, DEVICE *dptr, const char *title, uint32 width, uint32 height, int flags)
{
*vptr = NULL;
return SCPE_NOFNC;
}

t_stat vid_close_window (VID_DISPLAY *vptr)
{
return SCPE_OK;
}

uint32 vid_map_rgb_window (VID_DISPLAY *vptr, uint8 r, uint8 g, uint8 b)
{
return 0;
}

void vid_draw_window (VID_DISPLAY *vptr, int32 x, int32 y, int32 w, int32 h, const uint32 *buf)
{
return;
}

void vid_refresh_window (VID_DISPLAY *vptr)
{
return;
}

t_stat vid_set_cursor_window (VID_DISPLAY *vptr, t_bool visible, uint32 width, uint32 height, uint8 *data, uint8 *mask, uint32 hot_x, uint32 hot_y)
{
return SCPE_NOFNC;
}

t_bool vid_is_fullscreen_window (VID_DISPLAY *vptr)
{
sim_printf ("video support unavailable\n");
return FALSE;
}

t_stat vid_set_fullscreen_window (VID_DISPLAY *vptr, t_bool flag)
{
sim_printf ("video support unavailable\n");
return SCPE_OK;
}

void vid_set_cursor_position_window (VID_DISPLAY *vptr, int32 x, int32 y)
{
return;
}

void vid_set_window_size (VID_DISPLAY *vptr, int32 w, int32 h)
{
return;
}

void vid_render_set_logical_size (VID_DISPLAY *vptr, int32 w, int32 h)
{
return;
}

const char *vid_key_name (uint32 key)
{
return "";
}

#endif /* defined(USE_SIM_VIDEO) */
