/*
 * $Id: display.c,v 1.56 2004/02/03 21:44:34 phil Exp - revised by DAG $
 * Simulator and host O/S independent XY display simulator
 * Phil Budne <phil@ultimate.com>
 * September 2003
 *
 * with changes by Douglas A. Gwyn, 05 Feb. 2004
 *
 * started from PDP-8/E simulator vc8e.c;
 *  This PDP8 Emulator was written by Douglas W. Jones at the
 *  University of Iowa.  It is distributed as freeware, of
 *  uncertain function and uncertain utility.
 */

/*
 * Copyright (c) 2003-2018 Philip L. Budne
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization
 * from the authors.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>         /* for USHRT_MAX */
#include <pthread.h>
#include "sim_video.h"
#include "display.h"

#if !defined(MIN)
#  if defined(__GNUC__) || defined(__clang__)
#    define MIN(A, B) ({ __typeof__ (A) _a = (A); __typeof__ (B) _b = (B); _a < _b ? _a : _b; })
#  elif defined(_MSC_VER)
     /* from stdlib.h */
#    define MIN(A, B) __min((A), (B))
#  else
#    define MIN(A, B) ((A) < (B) ? (A) : (B))
#  endif
#endif

#if !defined(MAX)
#  if defined(__GNUC__) || defined(__clang__)
#    define MAX(A, B) ({ __typeof__ (A) _a = (A); __typeof__ (B) _b = (B); _a > _b ? _a : _b; })
#  elif defined(_MSC_VER)
     /* from stdlib.h */
#    define MAX(A, B) __max((A), (B))
#  else
#    define MAX(A, B) ((A) > (B) ? (A) : (B))
#  endif
#endif

/*
 * The user may select (at compile time) how big a window is used to
 * emulate the display.  Using smaller windows saves memory and screen space.
 *
 * Type 30 has 1024x1024 addressing, but only 512x512 visible points.
 * VR14 has only 1024x768 visible points; VR17 has 1024x1024 visible points.
 * VT11 supports 4096x4096 addressing, clipping to the lowest 1024x1024 region.
 * VR48 has 1024x1024 visible points in the main display area and 128x1024
 * visible points in a menu area on the right-hand side (1152x1024 total).
 * VT48 supports 8192x8192 (signed) main-area addressing, clipping to a
 * 1024x1024 window which can be located anywhere within that region.
 * (XXX -- That is what the VT11/VT48 manuals say; however, evidence suggests
 * that the VT11 may actually support 8192x8192 (signed) addressing too.)
 */

/* Define the default display type (if display_init() not called) */
#ifndef DISPLAY_TYPE
#define DISPLAY_TYPE DIS_TYPE30
#endif /* DISPLAY_TYPE not defined */

/* select a default resolution if display_init() not called */
/* XXX keep in struct display? */
#ifndef PIX_SCALE
#define PIX_SCALE RES_HALF
#endif /* PIX_SCALE not defined */

/* select a default light-pen hit radius if display_init() not called */
#ifndef PEN_RADIUS
#define PEN_RADIUS 4
#endif /* PEN_RADIUS not defined */


/*
 * note: displays can have up to two different colors (eg VR20)
 * each color can be made up of any number of phosphors
 * with different colors and decay characteristics (eg Type 30)
 */

#define ELEMENTS(X) (sizeof(X)/sizeof(X[0]))

struct phosphor {
    double red, green, blue;
    double level;           /* decay level (0.5 for half life) */
    double t_level;         /* seconds to decay to level */
};

struct color {
    struct phosphor *phosphors;
    int nphosphors;
    int half_life;          /* for refresh calc */
};

struct display {
    enum display_type type;
    const char *name;
    struct color *color0, *color1;
    short xpoints, ypoints;
};

/*
 * original phosphor constants from Raphael Nabet's XMame 0.72.1 PDP-1 sim.
 *
 * http://bitsavers.trailing-edge.com/components/rca/hb-3/1963_HB-3_CRT_Storage_Tube_and_Monoscope_Section.pdf
 * pdf p374 says 16ADP7 used P7 phosphor.
 * pdf pp28-32 describe P7 phosphor (spectra, buildup, persistence)
 *
 * https://www.youtube.com/watch?v=hZumwXS4fJo
 * "3RP7A CRT - P7 Phosphor Persistence" shows colors/persistence
 */
static struct phosphor p7[] = {
    {0.11, 0.11, 1.0,  0.5, 0.05},  /* fast blue */
    {1.0,  1.0,  0.11, 0.5, 0.20}   /* slow yellow/green */
};
static struct color color_p7 = { p7, ELEMENTS(p7), 125000 };

/* green phosphor for VR14, VR17, VR20 */
static struct phosphor p29[] = {{0.0260, 1.0, 0.00121, 0.5, 0.025}};
struct color color_p29 = { p29, ELEMENTS(p29), 25000 };

/* green phosphor for Tek 611 */
static struct phosphor p31[] = {{0.0, 1.0, 0.77, 0.5, .1}};
struct color color_p31 = { p31, ELEMENTS(p31), 100000 };

/* green phosphor for III */
static struct phosphor p39[] = {{0.2, 1.0, 0.0, 0.5, 0.01}};
struct color color_p39 = { p39, ELEMENTS(p39), 20000 };

static struct phosphor p40[] = {
    /* P40 blue-white spot with yellow-green decay (.045s to 10%?) */
    {0.4, 0.2, 0.924, 0.5, 0.0135},
    {0.5, 0.7, 0.076, 0.5, 0.065}
};
static struct color color_p40 = { p40, ELEMENTS(p40), 20000 };

/* "red" -- until real VR20 phosphor type/number/constants known */
static struct phosphor pred[] = { {1.0, 0.37, 0.37, 0.5, 0.10} };
static struct color color_red = { pred, ELEMENTS(pred), 100000 };

static struct display displays[] = {
   /*
     * TX-0
     *
     * Unknown manufacturer
     * 
     * 12" tube, 
     * maximum dot size ???
     * 50us point plot time (20,000 points/sec)
     * P7 Phosphor??? Two phosphor layers:
     * fast blue (.05s half life), and slow green (.2s half life)
     */
    { DIS_TX0, "MIT TX-0", &color_p7, NULL, 512, 512 },

    
    /*
     * Type 30
     * PDP-1/4/5/8/9/10 "Precision CRT" display system
     *
     * Raytheon 16ADP7A CRT?
     * web searches for 16ADP7 finds useful information!!
     * 16" tube, 14 3/8" square raster
     * maximum dot size .015"
     * 50us point plot time (20,000 points/sec)
     * P7 Phosphor??? Two phosphor layers:
     * fast blue (.05s half life), and slow green (.2s half life)
     * 360 lb
     * 7A at 115+-10V 60Hz
     */
    { DIS_TYPE30, "Type 30", &color_p7, NULL, 1024, 1024 },

    /*
     * VR14
     * used w/ GT40/44, AX08, VC8E
     *
     * Viewable area 6.75" x 9"
     * 12" diagonal
     * brightness >= 30 fL
     * dot size .02" (20 mils)
     * settle time:
     *  full screen 18us to +/-1 spot diameter
     *  .1" change 1us to +/-.5 spot diameter
     * weight 75lb
     */
    { DIS_VR14, "VR14", &color_p29, NULL, 1024, 768 },

    /*
     * VR17
     * used w/ GT40/44, AX08, VC8E
     *
     * Viewable area 9.25" x 9.25"
     * 17" diagonal
     * dot size .02" (20 mils)
     * brightness >= 25 fL
     * phosphor: P39 doped for IR light pen use
     * light pen: Type 375
     * weight 85lb
     */
    { DIS_VR17, "VR17", &color_p29, NULL, 1024, 1024 },

    /*
     * VR20
     * on VC8E
     * Two colors!!
     */
    { DIS_VR20, "VR20", &color_p29, &color_red, 1024, 1024 },

    /*
     * VR48
     * (on VT48 in VS60)
     * from Douglas A. Gwyn 23 Nov. 2003
     *
     * Viewable area 12" x 12", plus 1.5" x 12" menu area on right-hand side
     * 21" diagonal
     * dot size <= .01" (10 mils)
     * brightness >= 31 fL
     * phosphor: P40 (blue-white fluorescence with yellow-green phosphorescence)
     * light pen: Type 377A (with tip switch)
     * driving circuitry separate
     * (normally under table on which CRT is mounted)
     */
    { DIS_VR48, "VR48", &color_p40, NULL, 1024+VR48_GUTTER+128, 1024 },

    /*
     * Type 340 Display system
     * on PDP-1/4/6/7/9/10
     *
     * Raytheon 16ADP7A CRT, same as Type 30
     * 1024x1024
     * 9 3/8" raster (.01" dot pitch)
     * 0,0 at lower left
     * 8 intensity levels
     */
    { DIS_TYPE340, "Type 340", &color_p7, NULL, 1024, 1024 },

    /*
     * NG display
     * on PDP-11/45
     *
     * Tektronix 611
     * 512x512, out of 800x600
     * 0,0 at middle
     */
    { DIS_NG, "NG Display", &color_p31, NULL, 512, 512 },

    /*
     * III display
     * on PDP-10
     */
    { DIS_III, "III Display", &color_p39, NULL, 1024, 1024 },

    /*
     * Imlac display
     * 1024x1024 addressable points.
     * P31 phosphor according to "Heads-Up Display for Flight
     * Simulator for Advanced Aircraft"
     */
    { DIS_IMLAC, "Imlac Display", &color_p31, NULL, 1024, 1024 },

    /*
     * TT2500 display
     * 512x512 addressable points.
     * P31 phosphor according to "Heads-Up Display for Flight
     * Simulator for Advanced Aircraft"
     */
    { DIS_TT2500, "TT2500 Display", &color_p31, NULL, 512, 512 }
};

/*
 * Unit time (in microseconds) used to store display point time to
 * live at current aging level.  If this is too small, delay values
 * cannot fit in an unsigned short.  If it is too large all pixels
 * will age at once.  Perhaps a suitable value should be calculated at
 * run time?  When display_init() calculates refresh_interval it
 * sanity checks for both cases.
 */
#define DELAY_UNIT 250

/* levels to display in first half-life; determines refresh rate */
#ifndef LEVELS_PER_HALFLIFE
#define LEVELS_PER_HALFLIFE 4
#endif

/* after 5 half lives (.5**5) remaining intensity is 3% of original */
#ifndef HALF_LIVES_TO_DISPLAY
#define HALF_LIVES_TO_DISPLAY 5
#endif

/*
 * refresh_rate is number of times per (simulated) second a pixel is
 * aged to next lowest intensity level.
 *
 * refresh_rate = ((1e6*LEVELS_PER_HALFLIFE)/PHOSPHOR_HALF_LIFE)
 * refresh_interval = 1e6/DELAY_UNIT/refresh_rate
 *          = PHOSPHOR_HALF_LIFE/LEVELS_PER_HALF_LIFE
 * intensities = (HALF_LIVES_TO_DISPLAY*PHOSPHOR_HALF_LIFE)/refresh_interval
 *         = HALF_LIVES_TO_DISPLAY*LEVELS_PER_HALFLIFE
 *
 * See also comments on display_age()
 *
 * Try to keep LEVELS_PER_HALFLIFE*HALF_LIVES_TO_DISPLAY*NLEVELS <= 192
 * to run on 8-bit (256 color) displays!
 */

/*
 * number of aging periods to display a point for
 */
#define NTTL (HALF_LIVES_TO_DISPLAY*LEVELS_PER_HALFLIFE)

/*
 * maximum (initial) TTL for a point.
 * TTL's are stored 1-based
 * (a stored TTL of zero means the point is off)
 */
#define MAXTTL NTTL

/*
 * number of drawing intensity levels
 */
#define NLEVELS (DISPLAY_INT_MAX-DISPLAY_INT_MIN+1)

#define MAXLEVEL (NLEVELS-1)

/*
 * Display Device Implementation
 */

/*
 * Each point on the display is represented by a "struct point".  When
 * a point isn't dark (intensity > 0), it is linked into a circular,
 * doubly linked delta queue (a priority queue where "delay"
 * represents the time difference from the previous entry (if any) in
 * the queue.
 *
 * All points are aged refresh_rate times/second, each time moved to the
 * next (logarithmically) lower intensity level.  When display_age() is
 * called, only the entries which have expired are processed.  Calling
 * display_age() often allows spreading out the workload.
 *
 * An alternative would be to have intensity levels represent linear
 * decreases in intensity, and have the decay time at each level change.
 * Inverting the decay function for a multi-component phosphor may be
 * tricky, and the two different colors would need different time tables.
 * Furthermore, it would require finding the correct location in the
 * queue when adding a point (currently only need to add points at end)
 */

/*
 * 12 bytes/entry on 32-bit system when REFRESH_RATE > 15
 * (requires 3MB for 512x512 display).
 */

typedef unsigned short delay_t;
#define DELAY_T_MAX USHRT_MAX

struct point {
    struct point *next;         /* next entry in queue */
    struct point *prev;         /* prev entry in queue */
    delay_t delay;              /* delta T in DELAY_UNITs */
    unsigned char ttl;          /* zero means off, not linked in */
    unsigned char level : 7;    /* intensity level */
    unsigned char color : 1;    /* for VR20 (two colors) */
};

static const unsigned char MAX_LEVEL = (unsigned char) ((1 << 7) - 1);
static const unsigned char MAX_COLOR = (unsigned char) ((1 << 1) - 1);

static struct point *points;    /* allocated array of points */
static struct point _head;
#define head (&_head)

/* Pixel displays: This is a double buffering design that accomodates threading
 * to avoid mutex arbitration to the individual pixels that allows direct
 * transfer to the video subsystem outside of the simulator thread.
 *
 * There are two pixel planes, pixelplanes[0] and pixelplanes[1]. current_pixelplane is the
 * destination into which the simulator's display updates pixel values.
 */
static pthread_mutex_t pixelplane_mutex = PTHREAD_MUTEX_INITIALIZER;
static size_t current_pixelplane = 0;

static uint32 *pixelplanes[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

/*
 * time span of all entries in queue
 * should never exceed refresh_interval
 * (should be possible to make this a delay_t)
 */
static long queue_interval;

static int initialized = 0;
static void *device = NULL;  /* Current display device. */

/*
 * global set by O/S display level to indicate "light pen tip switch activated"
 * (This is used only by the VS60 emulation, also by vttest to change patterns)
 */
unsigned char display_lp_sw = 0;

/*
 * global set by DR11-C simulation when DR device enabled; deactivates
 * light pen and instead reports mouse coordinates as Talos digitizer
 * data via DR11-C
 */
unsigned char display_tablet = 0;

/*
 * can be changed with display_lp_radius()
 */
static long scaled_pen_radius_squared;

/* run-time -- set by display_init() */
static int xpoints, ypoints;
static int xpixels, ypixels;
static int refresh_rate;
static long refresh_interval;
static int n_beam_colors;
static enum display_type display_type;
static int scale;

/*
 * Globals set by O/S display level to SCALED location in display
 * coordinate system in order to save an upcall on every mouse
 * movement.
 *
 * *NOT* for consumption by clients of display.c; although display
 * clients can now get the scaling factor, real displays only give you
 * a light pen "hit" when the beam passes under the light pen.
 */
int light_pen_x = -1;
int light_pen_y = -1;

/*
 * relative brightness for each display level
 * (all but last must be less than 1.0)
 */
static double level_scale[NLEVELS];

/*
 * Table of indices into the disp_colors color table for painting each age level,
 * intensity level and beam color.
 */
size_t beam_colors[2][NLEVELS][NTTL];

/* Beam color table:
 *
 * pixel_color: These are video system pixel values returned by vid_map_rgb(). For
 *   SDL, these are 32-bit ARGB (alpha + RGB). Other graphic systems might use
 *   smaller pixel values; 32-bits should be large enough (but should be suitably
 *   truncated if necessary by the graphic system.) */
typedef struct colortable {
    uint32 pixel_color;
} COLORTABLE;

static COLORTABLE mono_palette[2] = {                       /* Monochrome palette */
    { 0 },          /* black */
    { 0 }           /* white */
};

static COLORTABLE *disp_colors = NULL;                      /* The color palette */
static size_t n_dispcolors = 0;
size_t size_dispcolors = 0;

/* Drawing boundary: Keep track of the actual drawing boundaries that need
 * updating.
 */
typedef struct {
    /* Minimum left edge */
    int min_x, min_y;
    /* Maximum right edge */
    int max_x, max_y;
} DrawingBounds;

static DrawingBounds draw_bound;

/* Pixel parameters: */
/* Pixel size (this is probably invariant across all displays and simulators.) */
#if !defined(PIX_SIZE)
#define PIX_SIZE 1
#endif

static const int pix_size = PIX_SIZE;

/* Cursors: */
typedef struct cursor {
    Uint8 *data;
    Uint8 *mask;
    int width;
    int height;
    int hot_x;
    int hot_y;
} CURSOR;

static CURSOR *arrow_cursor = NULL;
static CURSOR *cross_cursor = NULL;

/* Arrow and cross cursor shapes: */
/* XPM */
static const char *arrow[] = {
    /* width height num_colors chars_per_pixel */
    "    16    16        3            1",
    /* colors */
    "X c #000000", /* black */
    ". c #ffffff", /* white */
    "  c None",
    /* pixels */
    "X               ",
    "XX              ",
    "X.X             ",
    "X..X            ",
    "X...X           ",
    "X....X          ",
    "X.....X         ",
    "X......X        ",
    "X.......X       ",
    "X........X      ",
    "X.....XXXXX     ",
    "X..X..X         ",
    "X.X X..X        ",
    "XX   X..X       ",
    "X     X..X      ",
    "       XX       ",
};

/* XPM */
static const char *cross[] = {
  /* width height num_colors chars_per_pixel hot_x hot_y*/
  "    16    16        3            1          7     7",
  /* colors */
  "X c #000000",    /* black */
  ". c #ffffff",    /* white */
  "  c None",
  /* pixels */
  "      XXXX      ",
  "      X..X      ",
  "      X..X      ",
  "      X..X      ",
  "      X..X      ",
  "      X..X      ",
  "XXXXXXX..XXXXXXX",
  "X..............X",
  "X..............X",
  "XXXXXXX..XXXXXXX",
  "      X..X      ",
  "      X..X      ",
  "      X..X      ",
  "      X..X      ",
  "      X..X      ",
  "      XXXX      ",
  "7,7"
};


/* Forward declarations: */
static size_t get_vid_color(uint8 r, uint8 g, uint8 b);
static CURSOR *create_cursor(const char *image[]);
static void free_cursor (CURSOR *cursor);
static int poll_for_events(int *valp, int maxus);
static inline void set_pixel_value(const struct point *p, uint32 pixel_value);
static inline void initialize_drawing_bound(DrawingBounds *b);
static unsigned long os_elapsed(void);

/* Utility functions: */

/* convert X,Y to a "struct point *" */
static inline struct point *P(int X, int Y)
{
     return &points[Y * xpixels + X];
}

/* convert "struct point *" to X and Y. Coordinates are ints. */
static inline int X(const struct point *pt)
{
    return ((int) (pt - points) % xpixels);
}

static inline int Y(const struct point *pt)
{
    return ((int) (pt - points) / xpixels);
}

void
display_lp_radius(int r)
{
    r /= scale;
    scaled_pen_radius_squared = r * r;
}

/*
 * from display_age and display_point
 * since all points age at the same rate,
 * only adds points at end of list.
 */
static void
queue_point(struct point *p)
{
    long d;

    d = refresh_interval - queue_interval;
    queue_interval += d;
    /* queue_interval should now be == refresh_interval */

#ifdef PARANOIA
    if (p->ttl == 0 || p->ttl > MAXTTL)
    printf("queuing %d,%d level %d!\n", X(p), Y(p), p->level);
    if (d > DELAY_T_MAX)
    printf("queuing %d,%d delay %d!\n", X(p), Y(p), d);
    if (queue_interval > DELAY_T_MAX)
    printf("queue_interval (%d) > DELAY_T_MAX (%d)\n",
           (int)queue_interval, DELAY_T_MAX);
#endif /* PARANOIA defined */

    p->next = head;
    p->prev = head->prev;

    head->prev->next = p;
    head->prev = p;

    p->delay = (delay_t) (MIN(d, DELAY_T_MAX));
}

/*
 * Return true if the display is blank, i.e. no active points in list.
 */
int
display_is_blank(void)
{
    return head->next == head;
}

/*
 * here to to dynamically adjust interval for examination
 * of elapsed vs. simulated time, and fritter away
 * any extra wall-clock time without eating CPU
 */

/*
 * more parameters!
 */

/*
 * upper bound for elapsed time between elapsed time checks.
 * if more than MAXELAPSED microseconds elapsed while simulating
 * delay_check simulated microseconds, decrease delay_check.
 */
#define MAXELAPSED 100000       /* 10Hz */

/*
 * lower bound for elapsed time between elapsed time checks.
 * if fewer than MINELAPSED microseconds elapsed while simulating
 * delay_check simulated microseconds, increase delay_check.
 */
#define MINELAPSED 50000        /* 20Hz */

/*
 * upper bound for delay (sleep/poll).
 * If difference between elapsed time and simulated time is
 * larger than MAXDELAY microseconds, decrease delay_check.
 *
 * since delay is elapsed time - simulated time, MAXDELAY
 * should be <= MAXELAPSED
 */
#ifndef MAXDELAY
#define MAXDELAY 100000         /* 100ms */
#endif /* MAXDELAY not defined */

/*
 * lower bound for delay (sleep/poll).
 * If difference between elapsed time and simulated time is
 * smaller than MINDELAY microseconds, increase delay_check.
 *
 * since delay is elapsed time - simulated time, MINDELAY
 * should be <= MINELAPSED
 */
#ifndef MINDELAY
#define MINDELAY 50000          /* 50ms */
#endif /* MINDELAY not defined */

/*
 * Initial amount of simulated time to elapse before polling.
 * Value is very low to ensure polling occurs on slow systems.
 * Fast systems should ramp up quickly.
 */
#ifndef INITIAL_DELAY_CHECK
#define INITIAL_DELAY_CHECK 1000    /* 1ms */
#endif /* INITIAL_DELAY_CHECK */

/*
 * gain factor (2**-GAINSHIFT) for adjustment of adjustment
 * of delay_check
 */
#ifndef GAINSHIFT
#define GAINSHIFT 3         /* gain=0.125 (12.5%) */
#endif /* GAINSHIFT not defined */

static void
display_delay(int t, int slowdown)
{
    /* how often (in simulated us) to poll/check for delay */
    static unsigned long delay_check = INITIAL_DELAY_CHECK;

    /* accumulated simulated time */
    static unsigned long sim_time = 0;
    unsigned long elapsed;
    long delay;

    sim_time += t;
    if (sim_time < delay_check)
        return;

    elapsed = os_elapsed();     /* read and reset elapsed timer */
    if (elapsed == (unsigned long)(~0L)) {/* first time thru? */
        slowdown = 0;           /* no adjustments */
        elapsed = sim_time;
        }

    /*
     * get delta between elapsed (real) time, and simulated time.
     * if simulated time running faster, we need to slow things down (delay)
     */
    if (slowdown)
        delay = sim_time - elapsed;
    else
        delay = 0;              /* just poll */

#ifdef DEBUG_DELAY2
    printf("sim %lu elapsed %lu delay %ld\r\n", sim_time, elapsed, delay);
#endif

    /*
     * Try to keep the elapsed (real world) time between checks for
     * delay (and poll for window system events) bounded between
     * MAXELAPSED and MINELAPSED.  Also tries to keep
     * delay/poll time bounded between MAXDELAY and MINDELAY -- large
     * delays make the simulation spastic, while very small ones are
     * inefficient (too many system calls) and tend to be inaccurate
     * (operating systems have a certain granularity for time
     * measurement, and when you try to sleep/poll for very short
     * amounts of time, the noise will dominate).
     *
     * delay_check period may be adjusted often, and oscillate.  There
     * is no single "right value", the important things are to keep
     * the delay time and max poll intervals bounded, and responsive
     * to system load.
     */
    if (elapsed > MAXELAPSED || delay > MAXDELAY) {
        /* too much elapsed time passed, or delay too large; shrink interval */
        if (delay_check > 1) {
            delay_check -= delay_check>>GAINSHIFT;
#ifdef DEBUG_DELAY
            printf("reduced period to %lu\r\n", delay_check);
#endif /* DEBUG_DELAY defined */
            }
        }
    else 
        if ((elapsed < MINELAPSED) || (slowdown && (delay < MINDELAY))) {
            /* too little elapsed time passed, or delta very small */
            int gain = delay_check>>GAINSHIFT;

            if (gain == 0)
                gain = 1;           /* make sure some change made! */
            delay_check += gain;
#ifdef DEBUG_DELAY
            printf("increased period to %lu\r\n", delay_check);
#endif /* DEBUG_DELAY defined */
            }
    if (delay < 0)
        delay = 0;
    /* else if delay < MINDELAY, clamp at MINDELAY??? */

    /* poll for window system events and/or delay */
    poll_for_events(NULL, delay);

    sim_time = 0;                   /* reset simulated time clock */

    /*
     * delay (poll/sleep) time included in next "elapsed" period
     * (clock not reset after a delay)
     */
} /* display_delay */

/* Callback to transfer point pixel values directly to the video system. For SDL, this draws
 * into the buffer provided by the renderer's texture via SDL_LockTexture/SDL_UnlockTexture.
 *
 * Normal case is to copy one pixel plane to another, row by row. The special case is when
 * the entire pixel plane is copied, which devolves into a simple memcpy().
 *
 * dst_stride: This is the row length (stride) in bytes by which the destination offset is
 * incremented. The destination stride is not necessarily the same as the width, w.
 * 
 * SDL2 passes vid_stride in bytes per row, which we convert to a pixel offset.
 */
static void display_age_onto(VID_DISPLAY *vptr, DEVICE *dev, int x, int y, int w, int h, 
                             void *draw_data, void *vid_dest, int dst_stride)
{
    const uint32 *src_plane = (const uint32 *) draw_data;
    uint32       *dst_plane = (uint32 *) vid_dest;

    /* dst_stride is in bytes; adjust to uint32 array elements. */
    dst_stride /= sizeof(uint32);

    pthread_mutex_lock(&pixelplane_mutex);

    if (x != 0 || y != 0 || w != xpixels || h != ypixels) {
        int py;
        size_t dst_offset = 0;
        size_t src_offset = y * xpixels + x;
        const size_t row_bytes = w * sizeof(uint32);

        for (py = h; py >= 8; py -= 8) {
            memcpy(&dst_plane[dst_offset + 0 * dst_stride], &src_plane[src_offset + 0 * xpixels], row_bytes);
            memcpy(&dst_plane[dst_offset + 1 * dst_stride], &src_plane[src_offset + 1 * xpixels], row_bytes);
            memcpy(&dst_plane[dst_offset + 2 * dst_stride], &src_plane[src_offset + 2 * xpixels], row_bytes);
            memcpy(&dst_plane[dst_offset + 3 * dst_stride], &src_plane[src_offset + 3 * xpixels], row_bytes);
            memcpy(&dst_plane[dst_offset + 4 * dst_stride], &src_plane[src_offset + 4 * xpixels], row_bytes);
            memcpy(&dst_plane[dst_offset + 5 * dst_stride], &src_plane[src_offset + 5 * xpixels], row_bytes);
            memcpy(&dst_plane[dst_offset + 6 * dst_stride], &src_plane[src_offset + 6 * xpixels], row_bytes);
            memcpy(&dst_plane[dst_offset + 7 * dst_stride], &src_plane[src_offset + 7 * xpixels], row_bytes);

            dst_offset += 8 * dst_stride;
            src_offset += 8 * xpixels;
        }

        if (py & 4) {
            memcpy(&dst_plane[dst_offset + 0 * dst_stride], &src_plane[src_offset + 0 * xpixels], row_bytes);
            memcpy(&dst_plane[dst_offset + 1 * dst_stride], &src_plane[src_offset + 1 * xpixels], row_bytes);
            memcpy(&dst_plane[dst_offset + 2 * dst_stride], &src_plane[src_offset + 2 * xpixels], row_bytes);
            memcpy(&dst_plane[dst_offset + 3 * dst_stride], &src_plane[src_offset + 3 * xpixels], row_bytes);

            dst_offset += 4 * dst_stride;
            src_offset += 4 * xpixels;
            py -= 4;
        }

        if (py & 2) {
            memcpy(&dst_plane[dst_offset + 0 * dst_stride], &src_plane[src_offset + 0 * xpixels], row_bytes);
            memcpy(&dst_plane[dst_offset + 1 * dst_stride], &src_plane[src_offset + 1 * xpixels], row_bytes);

            dst_offset += 2 * dst_stride;
            src_offset += 2 * xpixels;
            py -= 2;
        }

        if (py & 1) {
            memcpy(&dst_plane[dst_offset + 0 * dst_stride], &src_plane[src_offset + 0 * xpixels], row_bytes);
            py--;
        }

        ASSURE(py == 0);
    } else {
        memcpy(dst_plane, src_plane, w * h * sizeof(uint32));
    }

    pthread_mutex_unlock(&pixelplane_mutex);
}

/*
 * here periodically from simulator to age pixels.
 *
 * calling often with small values will age a few pixels at a time,
 * and assist with graceful aging of display, and pixel aging.
 *
 * values should be smaller than refresh_interval!
 *
 * returns true if anything on screen changed.
 */
/* Age the display's phosphor.
 *
 * t: Simulated microseconds (us) since last call.
 * slowdown: Slowdown to simulated speed.
 */
int display_age(int t, int slowdown)
{
    struct point *p;
    static int elapsed = 0;
    static int refresh_elapsed = 0; /* in units of DELAY_UNIT bounded by refresh_interval */
    int changed;

    if (!initialized && !display_init(DISPLAY_TYPE, PIX_SCALE, NULL))
        return 0;

    if (slowdown)
        display_delay(t, slowdown);

    changed = 0;

    elapsed += t;
    if (elapsed < DELAY_UNIT)
        return 0;

    t = elapsed / DELAY_UNIT;
    elapsed %= DELAY_UNIT;

    ++refresh_elapsed;
    if (refresh_elapsed >= refresh_interval) {
        display_sync ();
        refresh_elapsed = 0;
        }

    while ((p = head->next) != head) {
        /* look at oldest entry */
        if (p->delay > t) {                              /* further than our reach? */
            p->delay -= (delay_t) (MIN(t, DELAY_T_MAX)); /* update head */
            queue_interval -= t;                         /* update span */
            break;                                       /* quit */
            }

#ifdef PARANOIA
        if (p->ttl == 0)
            printf("BUG: age %d,%d ttl zero\n", X(p), Y(p));
#endif /* PARANOIA defined */

        /* dequeue point */
        p->prev->next = p->next;
        p->next->prev = p->prev;

        t -= p->delay;              /* lessen our reach */
        queue_interval -= p->delay; /* update queue span */

        /* Update the pixel value... */
        set_pixel_value(p, disp_colors[beam_colors[p->color][p->level][--p->ttl]].pixel_color);
        ++changed;

        /* queue it back up, unless we just turned it off! */
        if (p->ttl > 0)
            queue_point(p);
        }

    return changed;
} /* display_age */

/* Intesify a point.
 *
 * x: 0..xpixels
 * y: 0..ypixels
 * level: 0..MAXLEVEL
 * color: Index of the color to be used.
 */
static int intensify(int x, int y, int level, int color)
{
    struct point *p;
    int bleed;

    p = P(x,y);
    if (p->ttl) {           /* currently lit? */
#ifdef LOUD
        printf("%d,%d old level %d ttl %d new %d\r\n",
               x, y, p->level, p->ttl, level);
#endif /* LOUD defined */

        /* unlink from delta queue */
        p->prev->next = p->next;

        if (p->next == head)
            queue_interval -= p->delay;
        else
            p->next->delay += p->delay;
        p->next->prev = p->prev;
        }

    bleed = 0;              /* no bleeding for now */

    /* EXP: doesn't work... yet */
    /* if "recently" drawn, same or brighter, same color, make even brighter */
    if (p->ttl >= MAXTTL*2/3 && 
        level >= p->level && 
        p->color == color &&
        level < MAXLEVEL)
        level++;

    /*
     * this allows a dim beam to suck light out of
     * a recently drawn bright spot!!
     */
    if (p->ttl != MAXTTL || p->level != level || p->color != color) {
        p->ttl = MAXTTL;
        p->level = (unsigned char) (MIN(level, MAX_LEVEL));
        p->color = (unsigned char) (MIN(color, MAX_COLOR)); /* save color even if monochrome */
        set_pixel_value(p, disp_colors[beam_colors[p->color][p->level][p->ttl-1]].pixel_color);
        }

    queue_point(p);         /* put at end of list */
    return bleed;
}

int
display_point(int x,        /* 0..xpixels (unscaled) */
          int y,            /* 0..ypixels (unscaled) */
          int level,        /* DISPLAY_INT_xxx */
          int color)        /* for VR20! 0 or 1 */
{
    long lx, ly;

    if (!initialized && !display_init(DISPLAY_TYPE, PIX_SCALE, NULL))
        return 0;

    /* scale x and y to the displayed number of pixels */
    /* handle common cases quickly */
    if (scale > 1) {
        if (scale == 2) {
            x >>= 1;
            y >>= 1;
            }
        else {
            x /= scale;
            y /= scale;
            }
        }

#if DISPLAY_INT_MIN > 0
    level -= DISPLAY_INT_MIN;       /* make zero based */
#endif
    intensify(x, y, level, color);
    /* no bleeding for now (used to recurse for neighbor points) */

    if (light_pen_x == -1 || light_pen_y == -1)
        return 0;

    lx = x - light_pen_x;
    ly = y - light_pen_y;
    return lx*lx + ly*ly <= scaled_pen_radius_squared;
} /* display_point */

#define ABS(_X) ((_X) >= 0 ? (_X) : -(_X))
#define SIGN(_X) ((_X) >= 0 ? 1 : -1)

static void
xline (int x, int y, int x2, int dx, int dy, int level)
{
    int ix = SIGN(dx);
    int iy = SIGN(dy);
    int ay;

    dx = ABS(dx);
    dy = ABS(dy);

    ay = dy/2;
    for (;;) {
        display_point (x, y, level, 0);
        if (x == x2)
            break;
        if (ay > 0) {
            y += iy;
            ay -= dx;
        }
        ay += dy;
        x += ix;
    }
}
  
static void
yline (int x, int y, int y2, int dx, int dy, int level)
{
    int ix = SIGN(dx);
    int iy = SIGN(dy);
    int ax;

    dx = ABS(dx);
    dy = ABS(dy);

    ax = dx/2;
    for (;;) {
        display_point (x, y, level, 0);
        if (y == y2)
            break;
        if (ax > 0) {
            x += ix;
            ax -= dy;
        }
        ax += dx;
        y += iy;
    }
}

void
display_line(int x1,        /* 0..xpixels (unscaled) */
          int y1,           /* 0..ypixels (unscaled) */
          int x2,           /* 0..xpixels (unscaled) */
          int y2,           /* 0..ypixels (unscaled) */
          int level)        /* DISPLAY_INT_xxx */
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (ABS (dx) > ABS(dy))
        xline (x1, y1, x2, dx, dy, level);
    else
        yline (x1, y1, y2, dx, dy, level);
} /* display_line */

/*
 * calculate decay color table for a phosphor mixture
 * must be called AFTER refresh_rate initialized!
 */
static void
phosphor_init(struct phosphor *phosphors, int nphosphors, int color)
{
    int ttl;

    /* for each display ttl level; newest to oldest */
    for (ttl = NTTL-1; ttl > 0; ttl--) {
        struct phosphor *pp;
        double rr, rg, rb;  /* real values */

        /* fractional seconds */
        double t = ((double)(NTTL-1-ttl))/refresh_rate;

        int ilevel;         /* intensity levels */
        int p;

        /* sum over all phosphors in mixture */
        rr = rg = rb = 0.0;
        for (pp = phosphors, p = 0; p < nphosphors; pp++, p++) {
            double decay = pow(pp->level, t/pp->t_level);

            rr += decay * pp->red;
            rg += decay * pp->green;
            rb += decay * pp->blue;
            }

        /* scale for brightness for each intensity level */
        for (ilevel = MAXLEVEL; ilevel >= 0; ilevel--) {
             uint8 r, g, b;

             /*
              * convert to 16-bit integer; clamp at 16 bits.
              * this allows the sum of brightness factors across phosphors
              * for each of R G and B to be greater than 1.0
              */

             r = (uint8) (rr * level_scale[ilevel] * 255.0);
             g = (uint8) (rg * level_scale[ilevel] * 255.0);
             b = (uint8) (rb * level_scale[ilevel] * 255.0);

             beam_colors[color][ilevel][ttl] = get_vid_color(r, g, b);
        } /* for each intensity level */
    } /* for each TTL */
} /* phosphor_init */

static struct display *
find_type(enum display_type type)
{
    size_t i;
    struct display *dp;

    for (i = 0, dp = displays; i < ELEMENTS(displays); i++, dp++)
        if (dp->type == type)
            return dp;
    return NULL;
}

int
display_init(enum display_type type, int sf, void *dptr)
{
    static int init_failed = 0;
    struct display *dp;
    int half_life;
    int i;

    if (initialized) {
        /* cannot change type once started */
        /* XXX say something???? */
        return type == display_type;
        }

    if (init_failed)
        return 0;               /* avoid thrashing */

    init_failed = 1;            /* assume the worst */
    dp = find_type(type);
    if (!dp) {
        fprintf(stderr, "Unknown display type %d\r\n", (int)type);
        goto failed;
        }

    /* Initialize display list */
    head->next = head->prev = head;

    display_type = type;
    scale = sf;

    xpoints = dp->xpoints;
    ypoints = dp->ypoints;

    /* increase scale factor if won't fit on desktop? */
    xpixels = xpoints / scale;
    ypixels = ypoints / scale;

    /* set default pen radius now that scale is set */
    display_lp_radius(PEN_RADIUS);

    n_beam_colors = 1;
    /*
     * use function to calculate from looking at avg (max?)
     * of phosphor half lives???
     */
#define COLOR_HALF_LIFE(C) ((C)->half_life)

    half_life = COLOR_HALF_LIFE(dp->color0);
    if (dp->color1) {
        if (dp->color1->half_life > half_life)
            half_life = COLOR_HALF_LIFE(dp->color1);
        n_beam_colors++;
        }

    /* before phosphor_init; */
    refresh_rate = (1000000*LEVELS_PER_HALFLIFE)/half_life;
    refresh_interval = 1000000/DELAY_UNIT/refresh_rate;

    /*
     * sanity check refresh_interval
     * calculating/selecting DELAY_UNIT at runtime might avoid this!
     */

    /* must be non-zero; interval of 1 means all pixels will age at once! */
    if (refresh_interval < 1) {
        /* decrease DELAY_UNIT? */
        fprintf(stderr, "NOTE! refresh_interval too small: %ld\r\n",
                        refresh_interval);

        /* dunno if this is a good idea, but might be better than dying */
        refresh_interval = 1;
        }

    /* point lifetime in DELAY_UNITs will not fit in p->delay field! */
    if (refresh_interval > DELAY_T_MAX) {
        /* increase DELAY_UNIT? */
        fprintf(stderr, "bad refresh_interval %ld > DELAY_T_MAX %d\r\n",
            refresh_interval, DELAY_T_MAX);
        goto failed;
        }

    /*
     * before phosphor_init;
     * set up relative brightness of display intensity levels
     * (could differ for different hardware)
     *
     * linear for now.  boost factor insures low intensities are visible
     */
#define BOOST 5.0
    for (i = 0; i < NLEVELS; i++)
        level_scale[i] = ((double) i + 1.0 + BOOST) /(NLEVELS + BOOST);

    points = (struct point *)calloc((size_t) (xpixels * ypixels), sizeof(struct point));
    if (points == NULL)
        goto failed;
    for (i = 0; i < sizeof(pixelplanes) / sizeof(pixelplanes[0]); ++i) {
        pixelplanes[i] = (uint32 *) calloc((size_t) (xpixels * ypixels), sizeof(uint32));
        if (pixelplanes[i] == NULL)
            goto failed;
    }
    current_pixelplane = 0;
    if (pthread_mutex_init(&pixelplane_mutex, NULL) != 0)
        goto failed;

    initialize_drawing_bound(&draw_bound);

    /* Initialize the display */
    arrow_cursor = create_cursor (arrow);
    cross_cursor = create_cursor (cross);
    if (vid_open ((DEVICE *) dptr, dp->name, xpixels * pix_size, ypixels * pix_size, 0) == 0) {
        vid_set_cursor (1, arrow_cursor->width, arrow_cursor->height,
                        arrow_cursor->data, arrow_cursor->mask, arrow_cursor->hot_x, arrow_cursor->hot_y);
    }

    /* Initialize the color palette (requires the display): */
    size_dispcolors = 16;
    disp_colors = calloc(size_dispcolors, sizeof(COLORTABLE));

    mono_palette[0].pixel_color = vid_map_rgb(0, 0, 0);
    mono_palette[1].pixel_color = vid_map_rgb(0xff, 0xff, 0xff);

    if (disp_colors != NULL) {
        disp_colors[0] = mono_palette[0];
        disp_colors[1] = mono_palette[1];
        n_dispcolors = 2;
    }

    phosphor_init(dp->color0->phosphors, dp->color0->nphosphors, 0);

    if (dp->color1)
        phosphor_init(dp->color1->phosphors, dp->color1->nphosphors, 1);

    initialized = 1;
    init_failed = 0;            /* hey, we made it! */
    device = dptr;
    return 1;

 failed:
    fprintf(stderr, "Display initialization failed\r\n");
    return 0;
}

void
display_close(const void *dptr)
{
    if (!initialized || device != dptr)
        return;

    free (points);

    free_cursor(arrow_cursor);
    free_cursor(cross_cursor);
    vid_close();

    initialized = 0;
    device = NULL;
}

void
display_reset(void)
{
    /* XXX tear down window? just clear it? */
}

void
display_sync(void)
{
    uint32 *plane_to_draw, *plane_to_use;
    int w = draw_bound.max_x - draw_bound.min_x + 1;
    int h = draw_bound.max_y - draw_bound.min_y + 1;

    plane_to_draw = pixelplanes[current_pixelplane];

    /* Minimize time we hold the mutex while we switch between pixel planes. */
    pthread_mutex_lock(&pixelplane_mutex);
    ++current_pixelplane;
    /* The right side boolean == 0 if current_pixelplane == # of pixel planes. */
    current_pixelplane *= (current_pixelplane < sizeof(pixelplanes) / sizeof(pixelplanes[0]));
    pthread_mutex_unlock(&pixelplane_mutex);

    plane_to_use = pixelplanes[current_pixelplane];

    if (draw_bound.max_x > draw_bound.min_x && draw_bound.max_y > draw_bound.min_y) {
        if (draw_bound.min_x != 0 || draw_bound.min_y != 0 || draw_bound.max_x != xpixels || draw_bound.max_y != ypixels) {
            int py;
            size_t py_offset = draw_bound.min_y * xpixels + draw_bound.min_x;
            const size_t row_bytes = w * sizeof(uint32);

            for (py = 0; py < h; ++py) {
                memcpy(&plane_to_use[py_offset], &plane_to_draw[py_offset], row_bytes);
                py_offset += xpixels;
            }
        } else {
            memcpy(plane_to_use, plane_to_draw, w * h * sizeof(uint32));
        }

        /* NB: These drawing bounds are window (upper left origin) coordinates. */
        vid_draw_onto(draw_bound.min_x, draw_bound.min_y, w, h, display_age_onto, plane_to_draw);
        vid_refresh();
        initialize_drawing_bound(&draw_bound);
    }

    poll_for_events (NULL, 0);
}

void
display_beep(void)
{
    vid_beep();
}

int
display_xpoints(void)
{
    return xpoints;
}

int
display_ypoints(void)
{
    return ypoints;
}

int
display_scale(void)
{
    return scale;
}

/*
 * handle keyboard events
 *
 * data switches: bit toggled on key up, all cleared on space
 * enough for PDP-1/4/7/9/15 (for munching squares!):
 * 123 456 789 qwe rty uio
 *
 * second set of 18 for PDP-6/10, IBM7xxx (shifted versions of above):
 * !@# $%^ &*( QWE RTY UIO
 *
 */
unsigned long spacewar_switches = 0;

unsigned char display_last_char;

/* here from window system */
void
display_keydown(int k)
{
    switch (k) {
/* handle spacewar switches: see display.h for copious commentary */
#define SWSW(LC,UC,BIT,POS36,FUNC36) \
    case LC: case UC: spacewar_switches |= BIT; return;
    SPACEWAR_SWITCHES
#undef SWSW
    default: return;
    }
}

/* here from window system */
void
display_keyup(int k)
{
    unsigned long test_switches, test_switches2;

    cpu_get_switches(&test_switches, &test_switches2);
    switch (k) {
/* handle spacewar switches: see display.h for copious commentary */
#define SWSW(LC,UC,BIT,POS36,NAME36) \
    case LC: case UC: spacewar_switches &= ~BIT; return;

    SPACEWAR_SWITCHES
#undef SWSW

    case '1': test_switches ^= 1<<17; break;
    case '2': test_switches ^= 1<<16; break;
    case '3': test_switches ^= 1<<15; break;

    case '4': test_switches ^= 1<<14; break;
    case '5': test_switches ^= 1<<13; break;
    case '6': test_switches ^= 1<<12; break;

    case '7': test_switches ^= 1<<11; break;
    case '8': test_switches ^= 1<<10; break;
    case '9': test_switches ^= 1<<9; break;

    case 'q': test_switches ^= 1<<8; break;
    case 'w': test_switches ^= 1<<7; break;
    case 'e': test_switches ^= 1<<6; break;

    case 'r': test_switches ^= 1<<5; break;
    case 't': test_switches ^= 1<<4; break;
    case 'y': test_switches ^= 1<<3; break;

    case 'u': test_switches ^= 1<<2; break;
    case 'i': test_switches ^= 1<<1; break;
    case 'o': test_switches ^= 1; break;

    /* second set of 18 switches */
    case '!': test_switches2 ^= 1<<17; break;
    case '@': test_switches2 ^= 1<<16; break;
    case '#': test_switches2 ^= 1<<15; break;

    case '$': test_switches2 ^= 1<<14; break;
    case '%': test_switches2 ^= 1<<13; break;
    case '^': test_switches2 ^= 1<<12; break;

    case '&': test_switches2 ^= 1<<11; break;
    case '*': test_switches2 ^= 1<<10; break;
    case '(': test_switches2 ^= 1<<9; break;

    case 'Q': test_switches2 ^= 1<<8; break;
    case 'W': test_switches2 ^= 1<<7; break;
    case 'E': test_switches2 ^= 1<<6; break;

    case 'R': test_switches2 ^= 1<<5; break;
    case 'T': test_switches2 ^= 1<<4; break;
    case 'Y': test_switches2 ^= 1<<3; break;

    case 'U': test_switches2 ^= 1<<2; break;
    case 'I': test_switches2 ^= 1<<1; break;
    case 'O': test_switches2 ^= 1; break;

    case ' ': test_switches = test_switches2 = 0; break;
    default: return;
    }
    cpu_set_switches(test_switches, test_switches2);
}

static size_t get_vid_color(uint8 r, uint8 g, uint8 b)
{
    size_t i;
    Uint32 pixel_rgb = vid_map_rgb(r, g, b);

    for (i = 0; i < n_dispcolors; i++) {
        if (disp_colors[i].pixel_color == pixel_rgb)
            return i;
    }

    if (n_dispcolors == size_dispcolors) {
        COLORTABLE *p;

        /* Expect that the color table will grow slowly, so no reason to
         * bump the size by large increments. */
        size_dispcolors += 16;
        p = (COLORTABLE *) realloc(disp_colors, size_dispcolors * sizeof(*disp_colors));
        if (p != NULL) {
            disp_colors = p;
        } else {
            free(disp_colors);
            disp_colors = NULL;
            sim_messagef(SCPE_MEM, "get_vid_color: realloc from %" SIZE_T_FMT "u to %" SIZE_T_FMT "u failed. Cannot continue.\n", n_dispcolors,
                         size_dispcolors);
            abort();
        }
    }

    disp_colors[n_dispcolors].pixel_color = pixel_rgb;
    return n_dispcolors++;
}

/* Cursors: */
static CURSOR *create_cursor(const char *image[])
{
    int byte, bit, row, col;
    Uint8 *data = NULL;
    Uint8 *mask = NULL;
    char black, white, transparent;
    CURSOR *result = NULL;
    int width, height, curs_colors, cpp;
    int hot_x = 0, hot_y = 0;

    if (4 > sscanf(image[0], "%d %d %d %d %d %d", &width, &height, &curs_colors, &cpp, &hot_x, &hot_y))
        return result;
    if ((cpp != 1) || (0 != width % 8) || (curs_colors != 3))
        return result;
    black = image[1][0];
    white = image[2][0];
    transparent = image[3][0];
    data = (Uint8 *)calloc(1, (width / 8) * height);
    mask = (Uint8 *)calloc(1, (width / 8) * height);
    if (!data || !mask) {
        free(data);
        free(mask);
        return result;
    }
    bit = 7;
    byte = 0;
    for (row = 0; row < height; ++row) {
        for (col = 0; col < width; ++col) {
            if (image[curs_colors + 1 + row][col] == black) {
                data[byte] |= (1 << bit);
                mask[byte] |= (1 << bit);
            } else if (image[curs_colors + 1 + row][col] == white) {
                mask[byte] |= (1 << bit);
            } else if (image[curs_colors + 1 + row][col] != transparent) {
                free(data);
                free(mask);
                return result;
            }
            --bit;
            if (bit < 0) {
                ++byte;
                bit = 7;
            }
        }
    }
    result = (CURSOR *)calloc(1, sizeof(*result));
    if (result) {
        result->data = data;
        result->mask = mask;
        result->width = width;
        result->height = height;
        result->hot_x = hot_x;
        result->hot_y = hot_y;
    } else {
        free(data);
        free(mask);
    }
    return result;
}

static void free_cursor (CURSOR *cursor)
{
    if (cursor == NULL)
        return;
    free (cursor->data);
    free (cursor->mask);
    free (cursor);
}

/* Event polling: */
static void key_to_ascii (const SIM_KEY_EVENT *kev);
static int map_key(int k);

static int poll_for_events(int *valp, int maxus)
{
    SIM_MOUSE_EVENT mev;
    SIM_KEY_EVENT kev;

    if (maxus > 1000)
        sim_os_ms_sleep(maxus / 1000);

    if (SCPE_OK == vid_poll_mouse(&mev)) {
        unsigned char old_lp_sw = display_lp_sw;

        if ((display_lp_sw = (unsigned char)mev.b1_state) != 0) {
            light_pen_x = mev.x_pos;
            light_pen_y = (ypixels - 1) - mev.y_pos; /* range 0 - (ypixels-1) */
            /* convert to display coordinates */
            light_pen_x /= pix_size;
            light_pen_y /= pix_size;
            if (!old_lp_sw && !display_tablet)
                vid_set_cursor(1, cross_cursor->width, cross_cursor->height, cross_cursor->data, cross_cursor->mask,
                               cross_cursor->hot_x, cross_cursor->hot_y);
        } else {
            light_pen_x = light_pen_y = -1;
            if (old_lp_sw && !display_tablet)
                vid_set_cursor(1, arrow_cursor->width, arrow_cursor->height, arrow_cursor->data, arrow_cursor->mask,
                               arrow_cursor->hot_x, arrow_cursor->hot_y);
        }
        vid_set_cursor_position(mev.x_pos, mev.y_pos);
    }
    if (SCPE_OK == vid_poll_kb(&kev)) {
        if ((vid_display_kb_event_process == NULL) || (vid_display_kb_event_process(&kev) != 0)) {
            switch (kev.state) {
            case SIM_KEYPRESS_DOWN:
            case SIM_KEYPRESS_REPEAT:
                display_keydown(map_key(kev.key));
                break;
            case SIM_KEYPRESS_UP:
                display_keyup(map_key(kev.key));
                break;
            }
            key_to_ascii(&kev);
        }
    }
    return 1;
}


static int map_key(int k)
{
    switch (k) {
        case SIM_KEY_0:                   return '0';
        case SIM_KEY_1:                   return '1';
        case SIM_KEY_2:                   return '2';
        case SIM_KEY_3:                   return '3';
        case SIM_KEY_4:                   return '4';
        case SIM_KEY_5:                   return '5';
        case SIM_KEY_6:                   return '6';
        case SIM_KEY_7:                   return '7';
        case SIM_KEY_8:                   return '8';
        case SIM_KEY_9:                   return '9';
        case SIM_KEY_A:                   return 'a';
        case SIM_KEY_B:                   return 'b';
        case SIM_KEY_C:                   return 'c';
        case SIM_KEY_D:                   return 'd';
        case SIM_KEY_E:                   return 'e';
        case SIM_KEY_F:                   return 'f';
        case SIM_KEY_G:                   return 'g';
        case SIM_KEY_H:                   return 'h';
        case SIM_KEY_I:                   return 'i';
        case SIM_KEY_J:                   return 'j';
        case SIM_KEY_K:                   return 'k';
        case SIM_KEY_L:                   return 'l';
        case SIM_KEY_M:                   return 'm';
        case SIM_KEY_N:                   return 'n';
        case SIM_KEY_O:                   return 'o';
        case SIM_KEY_P:                   return 'p';
        case SIM_KEY_Q:                   return 'q';
        case SIM_KEY_R:                   return 'r';
        case SIM_KEY_S:                   return 's';
        case SIM_KEY_T:                   return 't';
        case SIM_KEY_U:                   return 'u';
        case SIM_KEY_V:                   return 'v';
        case SIM_KEY_W:                   return 'w';
        case SIM_KEY_X:                   return 'x';
        case SIM_KEY_Y:                   return 'y';
        case SIM_KEY_Z:                   return 'z';
        case SIM_KEY_BACKQUOTE:           return '`';
        case SIM_KEY_MINUS:               return '-';
        case SIM_KEY_EQUALS:              return '=';
        case SIM_KEY_LEFT_BRACKET:        return '[';
        case SIM_KEY_RIGHT_BRACKET:       return ']';
        case SIM_KEY_SEMICOLON:           return ';';
        case SIM_KEY_SINGLE_QUOTE:        return '\'';
        case SIM_KEY_BACKSLASH:           return '\\';
        case SIM_KEY_LEFT_BACKSLASH:      return '\\';
        case SIM_KEY_COMMA:               return ',';
        case SIM_KEY_PERIOD:              return '.';
        case SIM_KEY_SLASH:               return '/';
        case SIM_KEY_BACKSPACE:           return '\b';
        case SIM_KEY_TAB:                 return '\t';
        case SIM_KEY_ENTER:               return '\r';
        case SIM_KEY_SPACE:               return ' ';
        }
    return k;
}


static void key_to_ascii (const SIM_KEY_EVENT *kev)
{
    static t_bool k_ctrl, k_shift, k_alt, k_win;

#define MODKEY(L, R, mod)   \
    case L: case R: mod = (kev->state != SIM_KEYPRESS_UP); break;
#define MODIFIER_KEYS       \
    MODKEY(SIM_KEY_ALT_L,    SIM_KEY_ALT_R,      k_alt)     \
    MODKEY(SIM_KEY_CTRL_L,   SIM_KEY_CTRL_R,     k_ctrl)    \
    MODKEY(SIM_KEY_SHIFT_L,  SIM_KEY_SHIFT_R,    k_shift)   \
    MODKEY(SIM_KEY_WIN_L,    SIM_KEY_WIN_R,      k_win)
#define SPCLKEY(K, LC, UC)  \
    case K:                                                 \
        if (kev->state != SIM_KEYPRESS_UP)                  \
            display_last_char = (unsigned char)(k_shift ? UC : LC);\
        break;
#define SPECIAL_CHAR_KEYS   \
    SPCLKEY(SIM_KEY_BACKQUOTE,      '`',  '~')              \
    SPCLKEY(SIM_KEY_MINUS,          '-',  '_')              \
    SPCLKEY(SIM_KEY_EQUALS,         '=',  '+')              \
    SPCLKEY(SIM_KEY_LEFT_BRACKET,   '[',  '{')              \
    SPCLKEY(SIM_KEY_RIGHT_BRACKET,  ']',  '}')              \
    SPCLKEY(SIM_KEY_SEMICOLON,      ';',  ':')              \
    SPCLKEY(SIM_KEY_SINGLE_QUOTE,   '\'', '"')              \
    SPCLKEY(SIM_KEY_LEFT_BACKSLASH, '\\', '|')              \
    SPCLKEY(SIM_KEY_COMMA,          ',',  '<')              \
    SPCLKEY(SIM_KEY_PERIOD,         '.',  '>')              \
    SPCLKEY(SIM_KEY_SLASH,          '/',  '?')              \
    SPCLKEY(SIM_KEY_ESC,            '\033', '\033')         \
    SPCLKEY(SIM_KEY_BACKSPACE,      '\177', '\177')       \
    SPCLKEY(SIM_KEY_TAB,            '\t', '\t')             \
    SPCLKEY(SIM_KEY_ENTER,          '\r', '\r')             \
    SPCLKEY(SIM_KEY_SPACE,          ' ', ' ')

    switch (kev->key) {
        MODIFIER_KEYS
        SPECIAL_CHAR_KEYS
        case SIM_KEY_0: case SIM_KEY_1: case SIM_KEY_2: case SIM_KEY_3: case SIM_KEY_4:
        case SIM_KEY_5: case SIM_KEY_6: case SIM_KEY_7: case SIM_KEY_8: case SIM_KEY_9:
            if (kev->state != SIM_KEYPRESS_UP)
                display_last_char = (unsigned char)('0' + (kev->key - SIM_KEY_0)); 
            break;
        case SIM_KEY_A: case SIM_KEY_B: case SIM_KEY_C: case SIM_KEY_D: case SIM_KEY_E:
        case SIM_KEY_F: case SIM_KEY_G: case SIM_KEY_H: case SIM_KEY_I: case SIM_KEY_J:
        case SIM_KEY_K: case SIM_KEY_L: case SIM_KEY_M: case SIM_KEY_N: case SIM_KEY_O:
        case SIM_KEY_P: case SIM_KEY_Q: case SIM_KEY_R: case SIM_KEY_S: case SIM_KEY_T:
        case SIM_KEY_U: case SIM_KEY_V: case SIM_KEY_W: case SIM_KEY_X: case SIM_KEY_Y:
        case SIM_KEY_Z: 
            if (kev->state != SIM_KEYPRESS_UP)
                display_last_char = (unsigned char)((kev->key - SIM_KEY_A) + 
                                        (k_ctrl ? 1 : (k_shift ? 'A' : 'a')));
            break;
        }
}

/* Utility functions: */
static inline void initialize_drawing_bound(DrawingBounds *b)
{
    /* Initialize to the maximal values and apply MIN. */
    b->min_x = xpixels;
    b->min_y = ypixels;
    /* Initialize to the minimal values and apply MAX. */
    b->max_x = 0;
    b->max_y = 0;
}

/* Coordinates are in display coordinates (lower left origin.) */
static inline void set_pixel_value(const struct point *p, uint32 pixel_value)
{
    int x = X(p);
    /* Display origin is lower left, whereas window systems' origins are upper left.
     * Transform coordinate from display to window system's. */
    int y = ypixels - 1 - Y(p);

    pixelplanes[current_pixelplane][y * xpixels + x] = pixel_value;

    /* Branchless version of MIN on x and y, MAX on w and h. Gives the common
     * subexpression eliminator something to do. */
    draw_bound.min_x = (draw_bound.min_x >= x) * x + (!(draw_bound.min_x >= x)) * draw_bound.min_x;
    draw_bound.min_y = (draw_bound.min_y >= y) * y + (!(draw_bound.min_y >= y)) * draw_bound.min_y;
    draw_bound.max_x = (draw_bound.max_x <= x) * x + (!(draw_bound.max_x <= x)) * draw_bound.max_x;
    draw_bound.max_y = (draw_bound.max_y <= y) * y + (!(draw_bound.max_y <= y)) * draw_bound.max_y;
}

static unsigned long os_elapsed(void)
{
    static int tnew;
    unsigned long ret;
    static uint32 t[2];

    t[tnew] = sim_os_msec();
    if (t[!tnew] == 0)
        ret = (uint32)~0; /* +INF */
    else
        ret = (t[tnew] - t[!tnew]) * 1000; /* usecs */
    tnew = !tnew;                          /* Ecclesiastes III */
    return ret;
}
