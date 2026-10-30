/* vax_lcspx.c: DEC LCSPX graphics option

   Copyright (c) 2026, Douglas P. Fields, Jr.

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

   lcspx        LCSPX 8-plane graphics option

   The LCSPX is the low-cost member of DEC's SPX family of accelerated
   colour frame buffers, the standard graphics of the VAXstation 4000
   Model 90/90A/96 (VMS device GEA0, driver GEBDRIVER; NetBSD spx).  It is
   built from a frame buffer of 8-bit pixels, a scan processor (a
   microcoded pixel engine that fills, copies and draws), the TBC (the
   timing and bus controller: video timing, interrupts, the command FIFO)
   and a Brooktree Bt459 RAMDAC with its colour map and hardware cursor.
   The option carries a ROM in DEC's option ROM format, which the console
   reads at power-up: a header, the device name, the frame-buffer
   description strings, a self-test of 32 subtests that the console copies
   to memory and runs, and a console driver image that the console keeps
   resident at the top of memory.

   Nothing in DEC's published documentation describes the option at the
   register level.  What is modelled comes from the option ROM's own code
   (the self-test says what every register must do), NetBSD's spx.c
   (addresses, frame-buffer layout, the Bt459 ports) and the real
   machines' console output (hardware/register-map.md and
   learnings/m8-lcspx.md in the project that wrote this model).  Open
   SIMH's vax4xx_ve.c models the SPX of the VAXstation 3100-76 and 4000
   VLC at the same addresses; where both model something they agree.
   Interrupts from the option to the CPU are not modelled yet.

   Address map, offsets from the option's base (VAXstation 4000/90:
   3800.0000):

        000.0000 - 03F.FFFF     frame buffer, one byte per pixel
        100.0000 - 12F.FFFF     FIFO 0's command window
        130.0000 - 13F.FFFF     scan processor (138.0000 up: its
                                diagnostic view)
        140.0000 - 16F.FFFF     FIFO 1's command window
        170.0000 - 17F.FFFF     the scan processor again (the
                                self-test's streaming subtests 28 and
                                29 set it up here)
        180.0000 - 1AF.FFFF     FIFO 2's command window
        1C0.0000 - 1EF.FFFF     FIFO 3's command window
        1B0.0000 - 1B0.03FF     TBC
        1B1.0000                Bt459 address register, low byte
        1B1.4000                Bt459 address register, high byte
        1B1.8000                Bt459 register port
        1B1.C000                Bt459 colour map port
        1B2.0000 - 1B3.FFFF     TBC FIFO memory
        1BC.0000 - 1BF.FFFF     option ROM: 16 bits wide at a longword
                                stride, bits <31:16> read C000

   Any other reference in the window is absent: a read machine-checks, as
   for any address nothing answers, and a write is dropped.

   The option ROM, DEC's firmware, is built in: vax_lcspx_bin.h, which
   sim_BuildROMs makes from lcspx.bin, the ROM of a VAXstation 4000/90 as
   its CPU reads it.  ATTACH LCSPX <file> replaces it with another image,
   either as dumped (256 KB of longwords, each C000xxxx) or as the 128 KB
   logical byte stream (lane 0, then lane 1, of each longword); DETACH
   LCSPX goes back to the built-in image.  Attaching an empty file leaves
   the ROM window reading zero, which the console rejects as an invalid
   option ROM header: the graphics test then reports "?? 016 2 UNK 0014"
   and the console goes on.

   SET LCSPX MONITOR terminates the video outputs, as a monitor or a
   terminated video cable does (NOMONITOR, the default, as on the
   author's headless 4000/90, leaves them unterminated, which the
   option's self-test notices).  With MONITOR the model shows the
   display in a window, when the simulator is built with video (SDL) and
   the host has a display; the keyboard and mouse of that window go to
   the LK401 and VSXXX devices (vax_lk.c, vax_vs.c).  SET LCSPX
   SCREENSHOT=<file> writes the picture the monitor would show as a PNG
   file, with or without a window.

   SET LCSPX DISABLED removes the option: its window no longer answers and
   the configuration register reports no LCSPX. */

#include "vax_defs.h"
#include "vax_lcspx.h"
#include "sim_video.h"
#include "vax_lk.h"
#include "vax_vs.h"
#include "vax_lcspx_bin.h"                              /* the built-in option ROM, vax_lcspx_bin */
#if defined (HAVE_LIBPNG)
#include <png.h>
#endif

#if !defined (LCSPX_BASE)
#error "the model must define LCSPX_BASE, the option's window in I/O space"
#endif

#define LCSPX_ROMWORDS  (LCSPX_ROMSIZE >> 2)            /* 64K 16-bit words */
/* The configuration and diagnostic register is read in bits <31:16> of
   every option ROM longword (the service manual names the bits; the
   self-test's subtest 7 shows <31>'s behaviour):
        <31>    scan processor test: reads 0 for a while after the scan
                processor is reset (TBC +00C <4>), then 1
        <30>    timeout
        <17>    0: 72 Hz monitor timing, 1: 66 Hz
   The real 4000/90 reads C000 in every longword of the ROM. */

#define LCSPX_ROMHI     0xC0000000                      /* bits <31:16> of every ROM read */
#define CFG_SPTEST      0x80000000
#define SP_RESET_INSTR  1000.0                          /* the scan processor's reset, in instructions */

/* Debugging bitmaps */

#define DBG_ROM         0x0001                          /* option ROM */
#define DBG_REG         0x0002                          /* register access */
#define DBG_FB          0x0004                          /* frame-buffer access */
#define DBG_SP          0x0008                          /* scan processor operations */

static uint16 *lcspx_rom = NULL;                        /* option ROM, one word per longword;
                                                           NULL: an empty file is attached */
static uint8 *lcspx_fb = NULL;                          /* frame buffer */
static t_bool lcspx_dirty = TRUE;                       /* the picture has changed */
static uint32 lcspx_sp[LCSPX_SPSIZE >> 3];              /* scan processor space */
static uint32 sp_lanes[64];                             /* diagnostic view: lanes not written */

/* Scan processor

   The scan processor's space is 512 KB; the second 512 KB is a
   diagnostic view of it.  In the normal view:
        2100-21FC, 2200-22FC    the register file: register i (0-63) is
                                at 2200 + 4i for i < 32, else 2100 + 4i
        2100                    read only.  <31:24>: nonzero when the
                                microcode is in the chip, zero when the
                                console must load it (its init routine
                                and several subtests take the one path or
                                the other, reading only this byte); <9>:
                                a status the console's resident driver
                                waits for in one routine that a serial-
                                console machine never runs; <8>: set,
                                meaning unknown.  A real 4000/90 with no
                                monitor read 01000100 at its console
                                (E/P/L 39302100, 2026-10-21), which the
                                model returns
        212C                    microcode address <25:16> and the
                                microword's bits <47:32>; reads give the
                                address and bits <47:32> of the word
                                there (subtest 9)
        23F8                    the microword's bits <31:0>: a write
                                stores the 48-bit word at the address
        316C                    plane mask control: <17:16> apply the
                                write mask to the CPU's frame-buffer
                                writes, <25:24> the read mask to its
                                reads (subtest 16; what each of the two
                                bits of a pair selects is not known)
        3170, 3174              write and read plane masks, one bit per
                                bit of a frame-buffer longword
        31A8                    the frame buffer's refresh counter: it
                                changes by itself (subtest 15 waits for
                                a change); the model counts every 16
                                instructions, modulo 1024 (the real
                                counter's width is not known)
   The microcode RAM holds 1024 48-bit words; the console loads it at
   power-up (the option's init routine) and the scan processor runs it.
   In the diagnostic view (+80000) the option ROM's self-test (subtests 7
   and 8) needs:
        2100-22FC               the register file, as in the normal view
        2300 + 4i               register i, one byte lane at a time
        2000 + 4i               reads the complement of the byte lanes
                                that the last write through 2300 + 4i
                                carried; writable
        6000-613F               the same memory as in the normal view
        A000-A13F               reads the complement of what was written
   What these views are for in the hardware (the register file is built
   from byte-sliced chips) is not established; the model gives exactly
   what the self-test checks. */

#define SP_DIAG         0x00080000                      /* the diagnostic view */
#define SP_REGS         0x2000                          /* the registers the command path reaches */
#define SP_UCADDR       0x212C                          /* microcode address, bits <47:32> */
#define SP_UCDATA       0x23F8                          /* microcode bits <31:0> */
#define SP_UCSIZE       1024                            /* microcode words */
#define SP_REFRESH      0x31A8                          /* refresh counter */
#define SP_ID           0x2100                          /* <31:24> nonzero: microcode in the chip */
#define SP_ID_INIT      0x01000100
#define SP_XSTART       0x21D0                          /* edges, <31:16> */
#define SP_YSTART       0x21D4
#define SP_XEND         0x21D8
#define SP_YEND         0x21DC
#define SP_DSTPIX       0x21E0                          /* <31:30> depth, <23:0> first pixel */
#define SP_SRCPIX       0x21E4
#define SP_STRIDE       0x21E8                          /* <31:16> source, <15:0> destination */
#define SP_COMMAND      0x21EC                          /* <0> right to left, <1> bottom to top */
#define SP_SRCMASK      0x21F0
#define SP_DSTMASK      0x21F4
#define SP_MPC          0x21FC                          /* a write starts the microcode */
#define SP_FG           0x2260
#define SP_BG           0x2264
#define SP_DESTLOOP     0x2270                          /* <3:0> logical function, <31:16> variant */
#define SP_PLANE        0x219C                          /* copy plane: the source bit(s) */

#define SP_CLIPMIN      0x21C0                          /* clip rectangle: <31:16> y, <15:0> x */
#define SP_CLIPMAX      0x21C4                          /* (exclusive) */

#define CMD_XNEG        0x00000001
#define CMD_YNEG        0x00000002
#define CMD_CLIP        0x00000020                      /* clip to CLIPMIN..CLIPMAX */
#define CMD_TRAP        0x00008000                      /* fill a trapezoid */
#define SP_DXL          0x23C8                          /* trapezoid: XSTART's edge step */
#define SP_DXR          0x23CC                          /* XEND's edge step */
#define CMD_WRMASK      0x00020000                      /* SRCMASK masks the writes */
#define CMD_RDMASK      0x00040000                      /* SRCMASK masks the destination read */
#define CMD_SRCMASK     0x01000000                      /* DSTMASK masks the source read */
#define PIX_INDEX       0x00FFFFFF
#define MPC_FILL        0x2019                          /* fill a rectangle with FG */
#define MPC_COPY        0x201A                          /* copy a rectangle */
#define MPC_VECTOR      0x2099                          /* draw a vector */
#define MPC_STREAMIN    0x201F                          /* write the rectangle from a FIFO */
#define MPC_STREAMOUT   0x201E                          /* read the rectangle into a FIFO */
#define CMD_NOFIRST     0x00000800                      /* vector: leave out the first point */
#define CMD_NOLAST      0x00000400                      /* vector: leave out the last point */
#define SP_PMCTL        0x316C                          /* plane mask control */
#define SP_WRMASK       0x3170                          /* write plane mask */
#define SP_RDMASK       0x3174                          /* read plane mask */
#define PMCTL_WREN      0x00030000                      /* the write mask applies */
#define PMCTL_RDEN      0x03000000                      /* the read mask applies */
#define SP_LANEDIAG     0x2000                          /* lanes view */
#define SP_BYTEREG      0x2300                          /* register file by byte lane */
#define SP_INVMEM       0xA000                          /* complemented memory */

static uint32 sp_ucaddr = 0;                            /* microcode address */
static uint32 sp_uchi = 0;                              /* microword <47:32>, as written */
static uint32 sp_uclo[SP_UCSIZE];                       /* microcode RAM <31:0> */
static uint16 sp_ucmhi[SP_UCSIZE];                      /* microcode RAM <47:32> */

/* The normal-view offset of register i of the register file */

static uint32 sp_regoff (uint32 i)
{
return (i < 32)? (0x2200 + 4 * i): (0x2100 + 4 * i);
}
static uint32 lcspx_tbc[LCSPX_TBCSIZE >> 2];            /* TBC registers */

/* TBC: timing and bus controller

   The TBC generates the video timing.  A line counter runs while the
   timing CSR's RUN bit is set; it counts the lines of a frame from 0 and
   wraps at the vertical blank finish line.  The registers below are the
   ones the option ROM's self-test gives a meaning (subtests 3 and 4);
   the rest are plain read/write registers.

   Timing CSR (+100):
        <2:0>   requests that the TBC carries out at the next vertical
                blank finish, and then clears, while <3> is set; with
                <3> clear they stay set (subtest 4)
        <3>     REQEN: carry out the requests at vertical blank finish
        <5>     RUN: the line counter runs
        <17>    V0: bit 0 of the line counter (read only)
        <18>    VS: in vertical sync (read only)
   Vertical timing, 11-bit fields, in lines from the start of a frame:
        +110    VSS, vertical sync start
        +112    VSF, vertical sync finish
        +114    VBS, vertical blank start
        +116    VBF, vertical blank finish: the frame's last line
   CVAX interrupt status (+018): <3> VBS reached, <4> VBF reached, both
   latched; writing ones clears them.

   The TBC has four FIFOs of 4096 longwords (subtests 10 and 11).  FIFO k
   (0-3) keeps its entries in the FIFO memory at 1B2.0000 + 8000k, which
   the CPU can also read and write directly, and has four registers at
   +040 + 20k:
        +0      data: a write appends an entry, a read takes the oldest
        +4      write pointer <13:2>: where the next entry goes
        +8      read pointer <13:2>: the oldest entry
        +C      free space <14:2>: 1000 when empty
   Writing a one to bit k of +00C resets FIFO k: both pointers 0, empty;
   a one in bit 4 resets the scan processor (see the configuration and
   diagnostic register).

   A longword write into FIFO k's command window (offset w from the
   option's 100.0000) appends two entries: an address entry, 1000000 | w,
   and the data.  The FIFO keeps a tag with each entry (the data may have
   bit 24 set too).  While TBC +004<23:21> names the FIFO (4 + k;
   subtests 12, 13 and 28) the TBC takes its entries and writes each
   datum to the register its address entry names, advancing the address
   by four for further data; data with no address entry before them go
   to the scan processor as pixels (streaming), and wait in the FIFO
   until it takes them.  The other bits of +004
   are set per FIFO by the self-test; their meaning is not known.  Within the FIFO's
   window:
        00.0000 - 1F.FFFF       scan processor register (a<9:2>) at
                                2000 + 4r (as Open SIMH's SPX model,
                                vax4xx_ve.c, has it; the self-test does
                                not use it)
        20.0000 - 23.FFFF       the TBC's space: its registers, the
                                Bt459, the FIFO memory (1B0.0000 up;
                                subtests 12 and 13)
   With routing disabled the entries wait, and the CPU can see them in the
   FIFO memory. */

#define TBC_CVINTMSK    (0x014 >> 2)                    /* CVAX interrupt mask */
#define TBC_CVINTSTS    (0x018 >> 2)                    /* CVAX interrupt status */
#define TBC_GRINTMSK    (0x024 >> 2)                    /* graphic interrupt mask */
#define TBC_GRINTSTS    (0x028 >> 2)                    /* graphic interrupt status */
#define TBC_TCSR        (0x100 >> 2)                    /* timing CSR */
#define TBC_VSYNC       (0x110 >> 2)                    /* VSS <10:0>, VSF <26:16> */
#define TBC_VBLANK      (0x114 >> 2)                    /* VBS <10:0>, VBF <26:16> */

#define TCSR_REQ        0x00000007                      /* requests done at VBF */
#define TCSR_REQEN      0x00000008                      /* ... while this is set */
#define TCSR_LEGO        0x00000003                     /* request: play the LEGO table */
#define TBC_LEGOTAB     (0x118 >> 2)                    /* LEGO table: <10:0> entries, <23:16> offset */
#define TCSR_RUN        0x00000020                      /* timing runs */
#define TCSR_V0         0x00020000                      /* line counter <0> */
#define TCSR_VS         0x00040000                      /* in vertical sync */
#define TCSR_RO         (TCSR_V0 | TCSR_VS)

#define CVI_VBS         0x00000008                      /* vertical blank start */
#define CVI_VBF         0x00000010                      /* vertical blank finish */
#define CVI_W1C         0xFFFFFFFF                      /* every status bit: write one to clear */

#define TBC_VMASK       0x7FF                           /* vertical fields: 11 bits */

#define TBC_FIFORST     (0x00C >> 2)                    /* <3:0> reset FIFO 3-0, <4> the scan processor */
#define RST_SP          0x00000010
#define TBC_FIFO0       (0x040 >> 2)                    /* FIFO 0's data port */
#define TBC_FIFOSTEP    (0x020 >> 2)                    /* to the next FIFO's */
#define FIFO_N          4                               /* FIFOs */
#define FIFO_DEPTH      4096                            /* entries */
#define FIFO_BLOCK      (0x8000 >> 2)                   /* FIFO memory per FIFO, longwords */
#define TBC_ROUTE       (0x004 >> 2)                    /* FIFO routing */
#define FIFO_ADDR       0x01000000                      /* an address entry */
#define FIFO_NODEST     0xFFFFFFFF                      /* no address entry yet */
#define CMD_TBC         0x00200000                      /* command address: TBC space */
#define CMD_TBCSIZE     0x00040000
#define TBC_LINE_USEC   13.0                            /* one line, 1280 x 1024 at 72 Hz */

static uint32 lcspx_fifomem[LCSPX_FIFOSIZE >> 2];       /* FIFO memory */
static uint8 lcspx_fifotag[LCSPX_FIFOSIZE >> 2];        /* 1: an address entry */
static uint32 fifo_wp[FIFO_N], fifo_rp[FIFO_N], fifo_cnt[FIFO_N];
static uint32 fifo_dest[FIFO_N];                        /* where the next datum goes */
static uint32 tbc_line = 0;                             /* line counter */
static uint32 tbc_frames = 0;                           /* frames displayed (VBS reached) */
static double sp_treset = -1.0e30;                      /* when the scan processor was reset */
static double tbc_tlast = 0;                            /* sim time of the line's start */

/* Bt459 RAMDAC

   Four ports, one per 16 KB of the option's window, data on bits <7:0>:
        0       address register <7:0>
        1       address register <11:8>
        2       the register the address selects
        3       the colour map entry the address selects
   The 12-bit address register increments after each access to a single
   register, and after the third access (red, green, blue: the modulo-3
   counter) to a colour map entry, an overlay colour (100-10F) or a
   cursor colour (181-183); loading either half of it resets the counter.
   Brooktree's Bt459 data sheet gives the register set:
        000-0FF colour map (port 3)     200     ID, reads 4A
        100-10F overlay colours         201-203 command registers 0-2
        181-183 cursor colours          204     pixel read mask
        300-30C cursor and window       206     pixel blink mask
                command and position    208-209 overlay read, blink masks
        400-7FF cursor RAM              20A     interleave
                                        20B     test
                                        20C-20E signature (red, green, blue) */

#define BT_ID           0x4A                            /* Bt459 identification */
#define BT_SAR          0x20C                           /* signature registers 20C-20E */
#define BT_TEST         0x20B                           /* test register: <2:0> pixel phase */
#define BT_PRM          0x204                           /* pixel read mask */
#define BT_AMASK        0xFFF                           /* address register */

static uint32 bt_addr = 0;                              /* address register */
static uint32 bt_m3 = 0;                                /* modulo-3 counter */
static uint8 bt_cmap[256][3];                           /* colour map */
static uint8 bt_ovl[16][3];                             /* overlay colours */
static uint8 bt_curcol[3][3];                           /* cursor colours */
static uint8 bt_reg[16];                                /* 200-20F */
static uint8 bt_cur[16];                                /* 300-30F */
static uint8 bt_curram[1024];                           /* 400-7FF */
static uint32 bt_sar = 0;                               /* signature: red <7:0>, green, blue */
static uint32 bt_sar_frames = 0;                        /* frames already in it */

t_stat lcspx_reset (DEVICE *dptr);
t_stat lcspx_svc (UNIT *uptr);
t_stat lcspx_disp_svc (UNIT *uptr);
t_stat lcspx_set_monitor (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat lcspx_screenshot (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat lcspx_show_rom (FILE *st, UNIT *uptr, int32 val, CONST void *desc);
static t_stat lcspx_rom_builtin (void);
static t_stat lcspx_display (void);
static void lcspx_wroff (uint32 off, int32 pa, int32 val, int32 lnt);
static void tbc_advance (void);
static uint32 tbc_rd (int32 i);
static void tbc_wr (int32 i, uint32 nv, uint32 wv);
static t_bool sp_stream_in (uint32 v);

#define SP_DONE_MIN     50                              /* instructions to "done", at least */

static int32 sp_in_left = 0;                            /* streaming in: pixels still to come */
static uint32 sp_in_x, sp_in_y, sp_in_w;                /* the next pixel, the width */
t_stat lcspx_attach (UNIT *uptr, CONST char *cptr);
t_stat lcspx_detach (UNIT *uptr);
t_stat lcspx_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr);
const char *lcspx_description (DEVICE *dptr);

/* LCSPX data structures

   lcspx_dev    LCSPX device descriptor
   lcspx_unit   LCSPX unit: the option ROM file
   lcspx_reg    LCSPX register list
*/

#define UNIT_V_MONITOR  (UNIT_V_UF + 0)                 /* the video outputs are terminated */
#define UNIT_MONITOR    (1u << UNIT_V_MONITOR)

UNIT lcspx_units[] = {
    { UDATA (&lcspx_svc, UNIT_ATTABLE + UNIT_RO, 0) },  /* option ROM; scan processor done */
    { UDATA (&lcspx_disp_svc, UNIT_DIS, 0) }            /* display refresh */
    };

#define lcspx_unit      lcspx_units[0]
#define lcspx_disp      lcspx_units[1]

MTAB lcspx_mod[] = {
    { UNIT_MONITOR, UNIT_MONITOR, "monitor", "MONITOR", &lcspx_set_monitor, NULL, NULL, "A monitor or a terminated video cable is connected (a window, with video)" },
    { UNIT_MONITOR, 0, "no monitor", "NOMONITOR", &lcspx_set_monitor, NULL, NULL, "Nothing terminates the video outputs (the default, as on the author's headless 4000/90)" },
    { MTAB_XTD|MTAB_VDV|MTAB_NC, 0, NULL, "SCREENSHOT", &lcspx_screenshot, NULL, NULL, "Write the picture to a PNG file: SCREENSHOT=file" },
    { MTAB_XTD|MTAB_VDV, 0, "ROM", NULL, NULL, &lcspx_show_rom, NULL, "Show which option ROM image is in use" },
    { 0 }
    };

REG lcspx_reg[] = {
    { BRDATAD (TBC,   lcspx_tbc, 16, 32, LCSPX_TBCSIZE >> 2, "TBC registers") },
    { HRDATAD (BTADDR, bt_addr, 12, "Bt459 address register") },
    { BRDATAD (BTREG, bt_reg, 16, 8, 16, "Bt459 registers 200-20F") },
    { BRDATAD (BTCUR, bt_cur, 16, 8, 16, "Bt459 cursor registers 300-30F") },
    { BRDATAD (BTCMAP, bt_cmap, 16, 8, 768, "Bt459 colour map, red green blue") },
    { NULL }
    };

DEBTAB lcspx_debug[] = {
    { "ROM", DBG_ROM, "option ROM" },
    { "REG", DBG_REG, "register access" },
    { "FB",  DBG_FB,  "frame-buffer access" },
    { "SP",  DBG_SP,  "scan processor operations" },
    { NULL, 0 }
    };

DEVICE lcspx_dev = {
    "LCSPX", lcspx_units, lcspx_reg, lcspx_mod,
    2, 16, 32, 4, 16, 32,
    NULL, NULL, &lcspx_reset,
    NULL, &lcspx_attach, &lcspx_detach,
    NULL, DEV_DISABLE | DEV_DEBUG, 0, lcspx_debug, NULL, NULL, &lcspx_help,
    NULL, NULL, &lcspx_description
    };

/* The option is present unless the device is disabled */

t_bool lcspx_present (void)
{
return (lcspx_dev.flags & DEV_DIS)? FALSE: TRUE;
}

/* Merge a byte or word write into a longword */

static uint32 lcspx_merge (uint32 old, int32 pa, int32 val, int32 lnt)
{
int32 sc = (pa & 3) << 3;
uint32 mask;

if (lnt >= L_LONG)
    return (uint32) val;
mask = ((lnt == L_BYTE)? 0xFFu: 0xFFFFu) << sc;
return (old & ~mask) | (((uint32) val << sc) & mask);
}

/* TBC video timing */

static uint32 tbc_field (int32 reg, int32 hi)
{
return (lcspx_tbc[reg] >> (hi? 16: 0)) & TBC_VMASK;
}

static t_bool tbc_in_vsync (void)
{
return (lcspx_tbc[TBC_TCSR] & TCSR_RUN) &&
    (tbc_line >= tbc_field (TBC_VSYNC, 0)) && (tbc_line < tbc_field (TBC_VSYNC, 1));
}

/* One line period in instructions, from the calibrated instruction rate
   (at least a few instructions, so that a polling loop sees every line) */

static double tbc_line_instr (void)
{
double ipl = sim_timer_inst_per_sec () * TBC_LINE_USEC / 1000000.0;

return (ipl < 16.0)? 16.0: ipl;
}

/* The LEGO table (subtest 31): at vertical blank finish, on request
   (timing CSR <1:0>), the TBC plays a table of Bt459 bus cycles kept in
   the frame buffer, so that software can change the colour map, cursor
   and registers without disturbing the picture.  Each entry is 16 bits,
   one bit per pixel (bit 0 of 16 consecutive pixels, least significant
   first); <9:8> is the Bt459 port (0 address low, 1 address high, 2
   register, 3 colour map) and <7:0> the data.  The self-test writes the
   table at the first row below the active display, offset by +118<23:16>
   pixels; the model plays +118<10:0> entries from there (the console sets
   53B).  That the count comes from +118 is a guess. */

static void bt_wr (uint32 port, uint32 v);
static void sp_wr (uint32 o, int32 pa, int32 val, int32 lnt);

static void tbc_lego (void)
{
uint32 w = ((lcspx_tbc[0x10C >> 2] & 0x1FF) + 1) * 4;
uint32 h = (lcspx_tbc[TBC_VBLANK] & TBC_VMASK) + 1;
uint32 p = h * w + ((lcspx_tbc[TBC_LEGOTAB] >> 16) & 0xFF);
uint32 n = lcspx_tbc[TBC_LEGOTAB] & 0x7FF;
uint32 i, b, e;

sim_debug (DBG_REG, &lcspx_dev, "LEGO table at pixel %06X, %d entries\n", p, n);
for (i = 0; i < n; i++) {
    for (b = 0, e = 0; b < 16; b++, p++)
        e = e | ((lcspx_fb[p & (LCSPX_FBSIZE - 1)] & 1) << b);
    bt_wr ((e >> 8) & 3, e & 0xFF);
    }
}

/* At vertical blank finish the TBC carries out the requests in the
   timing CSR */

static void tbc_vbf_reqs (void)
{
if (lcspx_tbc[TBC_TCSR] & TCSR_REQEN) {
    if (lcspx_tbc[TBC_TCSR] & TCSR_LEGO)
        tbc_lego ();
    lcspx_tbc[TBC_TCSR] &= ~TCSR_REQ;
    }
}

/* Bring the line counter up to the present, latching the vertical blank
   events it passes */

static void tbc_advance (void)
{
double now = sim_gtime ();
double ipl;
uint32 n, vbs, vbf;

if ((lcspx_tbc[TBC_TCSR] & TCSR_RUN) == 0) {
    tbc_tlast = now;
    return;
    }
ipl = tbc_line_instr ();
if (now - tbc_tlast < ipl)
    return;
vbs = tbc_field (TBC_VBLANK, 0);
vbf = tbc_field (TBC_VBLANK, 1);
if (vbf == 0)                                           /* a frame is at least one line */
    vbf = 1;
if ((now - tbc_tlast) >= (ipl * (vbf + 1))) {           /* a whole frame or more: every event */
    n = (uint32) ((now - tbc_tlast) / ipl);
    lcspx_tbc[TBC_CVINTSTS] |= CVI_VBS | CVI_VBF;
    tbc_frames = tbc_frames + ((n / (vbf + 1)) ? (n / (vbf + 1)): 1);
    tbc_vbf_reqs ();
    tbc_line = (tbc_line + n) % (vbf + 1);
    tbc_tlast = tbc_tlast + n * ipl;
    return;
    }
while (now - tbc_tlast >= ipl) {
    tbc_tlast = tbc_tlast + ipl;
    if (tbc_line >= vbf) {                              /* past the last line: a new frame */
        tbc_line = 0;
        lcspx_tbc[TBC_CVINTSTS] |= CVI_VBF;
        tbc_vbf_reqs ();
        }
    else tbc_line++;
    if (tbc_line == vbs) {
        lcspx_tbc[TBC_CVINTSTS] |= CVI_VBS;
        tbc_frames++;
        }
    }
}

/* TBC FIFOs */

static void fifo_put (int32 k, uint32 v, t_bool addr)
{
if (fifo_cnt[k] >= FIFO_DEPTH)                          /* full: lost */
    return;
lcspx_fifomem[k * FIFO_BLOCK + fifo_wp[k]] = v;
lcspx_fifotag[k * FIFO_BLOCK + fifo_wp[k]] = addr? 1: 0;
fifo_wp[k] = (fifo_wp[k] + 1) & (FIFO_DEPTH - 1);
fifo_cnt[k]++;
}

static uint32 fifo_get (int32 k, t_bool *addr)
{
uint32 v;

if (fifo_cnt[k] == 0) {                                 /* empty */
    *addr = FALSE;
    return 0;
    }
v = lcspx_fifomem[k * FIFO_BLOCK + fifo_rp[k]];
*addr = lcspx_fifotag[k * FIFO_BLOCK + fifo_rp[k]]? TRUE: FALSE;
fifo_rp[k] = (fifo_rp[k] + 1) & (FIFO_DEPTH - 1);
fifo_cnt[k]--;
return v;
}

/* Is FIFO k's routing enabled? */

static t_bool fifo_routed (int32 k)
{
return (((lcspx_tbc[TBC_ROUTE] >> 21) & 7) == (uint32) (4 | k))? TRUE: FALSE;
}

/* Carry out one datum taken from a FIFO */

static void cmd_write (uint32 a, uint32 v)
{
a = a % LCSPX_CMDSTEP;                                  /* the address within the FIFO's window */
if ((a - CMD_TBC) < CMD_TBCSIZE)                        /* TBC registers, Bt459, FIFO memory */
    lcspx_wroff (LCSPX_TBC + (a - CMD_TBC), (int32) a, (int32) v, L_LONG);
else if (((a >> 20) & 3) < 2)                           /* scan processor register */
    sp_wr (SP_REGS + (a & 0x3FC), (int32) a, (int32) v, L_LONG);
else sim_debug (DBG_REG, &lcspx_dev, "command to %06X = %08X not modelled\n", a, v);
}

/* Let the TBC take the entries of every routed FIFO */

static void fifo_drain (void)
{
static t_bool busy = FALSE;                             /* a routed write can route more */
t_bool addr;
int32 k;
uint32 v;

if (busy)
    return;
busy = TRUE;
for (k = 0; k < FIFO_N; k++) {
    while (fifo_routed (k) && (fifo_cnt[k] > 0)) {
        if (!lcspx_fifotag[k * FIFO_BLOCK + fifo_rp[k]] &&   /* pixel data, but no stream: wait */
            (fifo_dest[k] == FIFO_NODEST) && (sp_in_left <= 0))
            break;
        v = fifo_get (k, &addr);
        if (addr)
            fifo_dest[k] = v & 0x00FFFFFC;
        else if (fifo_dest[k] == FIFO_NODEST) {         /* pixel data for the scan processor */
            if (sp_stream_in (v) && (sp_in_left <= 0))
                sim_activate_abs (&lcspx_unit, SP_DONE_MIN);
            }
        else {
            cmd_write (fifo_dest[k], v);
            fifo_dest[k] = (fifo_dest[k] + 4) & 0x00FFFFFC;
            }
        }
    }
busy = FALSE;
}

/* Read a TBC register */

static uint32 tbc_rd (int32 i)
{
int32 k = (i - TBC_FIFO0) / TBC_FIFOSTEP;
uint32 v;
t_bool addr;

tbc_advance ();
if ((i >= TBC_FIFO0) && (k < FIFO_N) && (((i - TBC_FIFO0) % TBC_FIFOSTEP) < 4)) {
    switch ((i - TBC_FIFO0) % TBC_FIFOSTEP) {

        case 0:                                         /* data: take the oldest */
            return fifo_get (k, &addr);

        case 1:                                         /* write pointer */
            return fifo_wp[k] << 2;

        case 2:                                         /* read pointer */
            return fifo_rp[k] << 2;

        default:                                        /* free space */
            return (FIFO_DEPTH - fifo_cnt[k]) << 2;
            }
    }
v = lcspx_tbc[i];
if (i == TBC_TCSR)
    v = (v & ~TCSR_RO) | ((tbc_line & 1)? TCSR_V0: 0) | (tbc_in_vsync ()? TCSR_VS: 0);
return v;
}

/* Write a TBC register: nv is the register's new value, wv the bits the
   write carried (for the write-one-to-clear status registers) */

static void tbc_wr (int32 i, uint32 nv, uint32 wv)
{
int32 k = (i - TBC_FIFO0) / TBC_FIFOSTEP;

tbc_advance ();
if ((i >= TBC_FIFO0) && (k < FIFO_N) && (((i - TBC_FIFO0) % TBC_FIFOSTEP) < 4)) {
    switch ((i - TBC_FIFO0) % TBC_FIFOSTEP) {

        case 0:                                         /* data: append */
            fifo_put (k, nv, FALSE);
            fifo_drain ();
            break;

        default:                                        /* pointers: read only */
            break;
            }
    return;
    }
switch (i) {

    case TBC_CVINTSTS:                                  /* status: write one to clear */
        lcspx_tbc[i] &= ~(wv & CVI_W1C);
        break;

    case TBC_FIFORST:                                   /* FIFO and scan processor resets */
        for (k = 0; k < FIFO_N; k++) {
            if (wv & (1u << k)) {
                fifo_wp[k] = fifo_rp[k] = fifo_cnt[k] = 0;
                fifo_dest[k] = FIFO_NODEST;
                }
            }
        if (wv & RST_SP)
            sp_treset = sim_gtime ();
        lcspx_tbc[i] = nv;
        break;

    case TBC_TCSR:
        if ((nv & TCSR_RUN) && !(lcspx_tbc[i] & TCSR_RUN)) {    /* starts: line 0 */
            tbc_line = 0;
            tbc_tlast = sim_gtime ();
            }
        lcspx_tbc[i] = nv & ~TCSR_RO;
        break;

    case TBC_ROUTE:                                     /* routing: may start a FIFO */
        lcspx_tbc[i] = nv;
        fifo_drain ();
        break;

    default:
        lcspx_tbc[i] = nv;
        break;
        }
}

/* Bt459 signature analysis

   The three 8-bit signature registers (20C red, 20D green, 20E blue)
   form one 24-bit linear feedback shift register, red first.  During
   active display it takes one pixel per load group (the Bt459 here
   multiplexes four pixels; test register <2:0> picks which of the four):
   every bit n takes the colour bit n XOR the bit below it, bit 0 the
   feedback, the XOR of bits 2, 4, 6 of each register and bits 0 and 7 of
   blue ... as Brooktree's data sheet draws it (Bt459, figure 16, "Signature
   Analysis Register Circuit": red 2 4 6, green 0 2 4 6, blue 0 2 4 6 7),
   with the colour from the colour map complemented.  That reproduces
   all eight signatures that the option ROM's subtest 30 expects (four
   pixel phases in each of two display formats), so it is what the chip
   does.  The active display is (TBC +10C<8:0> + 1) four-pixel groups by
   (VBS + 1) lines, from the frame buffer's first pixel, one byte per
   pixel; the cursor and overlays are not included.

   The model brings the registers up to date when they are read or
   written: it runs the frames shown since then (at most two; software
   seeds them, waits a frame and reads them). */

static uint32 bt_sar_step (uint32 sv, uint32 d)
{
uint32 f = ((sv >> 2) ^ (sv >> 4) ^ (sv >> 6) ^ (sv >> 8) ^ (sv >> 10) ^ (sv >> 12) ^
    (sv >> 14) ^ (sv >> 16) ^ (sv >> 18) ^ (sv >> 20) ^ (sv >> 22) ^ (sv >> 23)) & 1;

return ((((sv << 1) & 0xFFFFFF) | f) ^ d) & 0xFFFFFF;
}

static void bt_sar_frame (void)
{
uint32 w = ((lcspx_tbc[0x10C >> 2] & 0x1FF) + 1) * 4;
uint32 h = (lcspx_tbc[TBC_VBLANK] & TBC_VMASK) + 1;
uint32 phase = bt_reg[BT_TEST - 0x200] & 3;
uint32 prm = bt_reg[BT_PRM - 0x200];
uint32 x, y, i, sv = bt_sar;

for (y = 0; y < h; y++) {
    for (x = phase; x < w; x = x + 4) {
        i = lcspx_fb[(y * w + x) & (LCSPX_FBSIZE - 1)] & prm;
        sv = bt_sar_step (sv, (bt_cmap[i][0] | (bt_cmap[i][1] << 8) | (bt_cmap[i][2] << 16)) ^ 0xFFFFFF);
        }
    }
bt_sar = sv;
}

static void bt_sar_update (void)
{
uint32 n;

tbc_advance ();
n = tbc_frames - bt_sar_frames;
if (n > 2)
    n = 2;
if (lcspx_tbc[TBC_TCSR] & TCSR_RUN) {
    while (n-- > 0)
        bt_sar_frame ();
    }
bt_sar_frames = tbc_frames;
}

/* Bt459 analog comparator (subtest 32)

   Test register <7:4> select two of the analog outputs (red <7> or
   green <6> against blue <5> or a 145 mV reference <4>); <3> reads one
   while the first is the higher.  The chip compares the outputs at the
   left edge of the cursor; the model takes the colour of the pixel
   there.  An output's level, in IRE units, is 7.5 (the pedestal, command
   register 2 <6>) plus 92.5 x code / 255, and green carries 40 more for
   the sync it carries (<7>).  The voltage per IRE unit depends on the
   load: the option ROM's subtest 32 first compares red at code 10
   with the reference and takes the smaller of two sets of test levels
   when red is above it.  The model uses 7.14 mV per IRE unit with the
   outputs terminated (the RS-343 level into a far-end 75 ohms, a
   monitor's or a terminated cable's, and the board's) and twice that
   without (no far-end termination).  Then the results follow: terminated,
   every check passes; unterminated (the author's headless 4000/90, whose
   readings the model reproduces) red 10 is above the reference, the
   smaller set leaves green with its sync above blue where the test wants
   it below, and the test fails at its error 6 with both readings 68. */

#define BT_CMD2         0x203
#define BT_REF_MV       145.0
#define BT_MV_IRE       7.14

static double bt_level (uint32 code, t_bool green)
{
double ire = 92.5 * (double) code / 255.0;

if (bt_reg[BT_CMD2 - 0x200] & 0x40)                     /* pedestal */
    ire = ire + 7.5;
if (green && (bt_reg[BT_CMD2 - 0x200] & 0x80))          /* sync on green */
    ire = ire + 40.0;
return ire * BT_MV_IRE * ((lcspx_unit.flags & UNIT_MONITOR)? 1.0: 2.0);
}

static uint32 bt_compare (void)
{
uint32 t = bt_reg[BT_TEST - 0x200];
int32 cx = (bt_cur[1] | (bt_cur[2] << 8)) - 360 - 1;    /* NetBSD's cursor bias: 360, 37 */
int32 cy = (bt_cur[3] | (bt_cur[4] << 8)) - 37;
uint32 w = ((lcspx_tbc[0x10C >> 2] & 0x1FF) + 1) * 4;
uint8 *rgb;
double a, b;

if (cx < 0)
    cx = 0;
if (cy < 0)
    cy = 0;
rgb = bt_cmap[lcspx_fb[(cy * w + cx) & (LCSPX_FBSIZE - 1)] & bt_reg[BT_PRM - 0x200]];
a = (t & 0x80)? bt_level (rgb[0], FALSE): bt_level (rgb[1], TRUE);
b = (t & 0x20)? bt_level (rgb[2], FALSE): BT_REF_MV;
sim_debug (DBG_REG, &lcspx_dev, "Bt459 compare test %02X colour %02X %02X %02X: %.0f mV against %.0f mV\n",
    t, rgb[0], rgb[1], rgb[2], a, b);
return ((t & 0xF0) && (a > b))? 0x08: 0;
}

/* Bt459: the colour triple at an address, or NULL for a single register */

static uint8 *bt_triple (uint32 a, t_bool cmap)
{
if (cmap)
    return bt_cmap[a & 0xFF];
if ((a >= 0x100) && (a <= 0x10F))
    return bt_ovl[a - 0x100];
if ((a >= 0x181) && (a <= 0x183))
    return bt_curcol[a - 0x181];
return NULL;
}

/* The single register at an address */

static uint8 *bt_single (uint32 a)
{
static uint8 none;

if ((a >= 0x200) && (a <= 0x20F))
    return &bt_reg[a - 0x200];
if ((a >= 0x300) && (a <= 0x30F))
    return &bt_cur[a - 0x300];
if ((a >= 0x400) && (a <= 0x7FF))
    return &bt_curram[a - 0x400];
none = 0;                                               /* reserved: reads zero */
return &none;
}

/* Step the address after an access to a triple or a single register */

static void bt_step (t_bool triple)
{
if (triple) {
    if (++bt_m3 < 3)
        return;
    bt_m3 = 0;
    }
bt_addr = (bt_addr + 1) & BT_AMASK;
}

static uint32 bt_rd (uint32 port)
{
uint8 *t;
uint32 v;

switch (port) {

    case 0:                                             /* address <7:0> */
        return bt_addr & 0xFF;

    case 1:                                             /* address <11:8> */
        return (bt_addr >> 8) & 0xF;

    case 2:                                             /* register */
    case 3:                                             /* colour map */
        t = bt_triple (bt_addr, port == 3);
        if (t != NULL) {
            v = t[bt_m3];
            bt_step (TRUE);
            return v;
            }
        if (bt_addr == 0x200)                           /* ID */
            v = BT_ID;
        else if ((bt_addr >= BT_SAR) && (bt_addr <= BT_SAR + 2)) {  /* signature */
            bt_sar_update ();
            v = (bt_sar >> ((bt_addr - BT_SAR) * 8)) & 0xFF;
            }
        else if (bt_addr == BT_TEST)                    /* with the comparator's result */
            v = (bt_reg[BT_TEST - 0x200] & ~0x08) | bt_compare ();
        else v = *bt_single (bt_addr);
        bt_step (FALSE);
        return v;
        }
return 0;
}

static void bt_wr (uint32 port, uint32 v)
{
uint8 *t;

lcspx_dirty = TRUE;                                     /* colours, cursor may change */
switch (port) {

    case 0:                                             /* address <7:0> */
        bt_addr = (bt_addr & 0xF00) | v;
        bt_m3 = 0;
        break;

    case 1:                                             /* address <11:8> */
        bt_addr = (bt_addr & 0x0FF) | ((v & 0xF) << 8);
        bt_m3 = 0;
        break;

    case 2:                                             /* register */
    case 3:                                             /* colour map */
        t = bt_triple (bt_addr, port == 3);
        if (t != NULL) {
            t[bt_m3] = (uint8) v;
            bt_step (TRUE);
            break;
            }
        if ((bt_addr >= BT_SAR) && (bt_addr <= BT_SAR + 2)) {  /* seed the signature */
            bt_sar_update ();
            bt_sar = (bt_sar & ~(0xFFu << ((bt_addr - BT_SAR) * 8))) | (v << ((bt_addr - BT_SAR) * 8));
            }
        else if (bt_addr != 0x200)                      /* the ID is read only */
            *bt_single (bt_addr) = (uint8) v;
        bt_step (FALSE);
        break;
        }
}

/* The scan processor's drawing operations

   The scan processor runs microcode; the model carries out what the
   microcode does at the entry points the option ROM and NetBSD's spx
   driver start (the microprogram counter, 21FC), from the registers they
   set up first:
        2019    fill the rectangle with FG (with COMMAND<15>: a trapezoid)
        201A    copy a rectangle from SRCPIX
        2099    draw a vector
        201F    write the rectangle from FIFO data (streaming in)
        201E    read the rectangle into a FIFO (streaming out)
   The rectangle's edges are XSTART..XEND and YSTART..YEND (pixels, in
   <31:16>); COMMAND<0> and <1> draw right to left and bottom to top, the
   columns from XEND to XSTART - 1 and the rows from YEND to YSTART - 1.
   DSTPIX (and SRCPIX) give the pixel at (XSTART, YSTART) as an index
   into the frame buffer, and the pixel depth in <31:30> (0: 8 bits, 1:
   16, 2: 32); STRIDE gives the line lengths in pixels.  DESTLOOP<3:0> is
   the logical function of source (FG, or the copied pixel) and
   destination, numbered as X11's: 3 copies.  SRCMASK (21F0) is a mask
   over each longword of the frame buffer (four 8-bit pixels): with
   COMMAND<17> it masks what is written, with COMMAND<18> it masks the
   destination before the logical function (subtest 20); with
   COMMAND<24> DSTMASK (21F4) masks the copied source pixels (subtest 23).
   (The names are NetBSD's.)  DESTLOOP<31:16> selects a variant of the
   copy: 0011 copies pixels; 2012 (the option ROM, subtest 24) and 2014
   (NetBSD's character drawing) copy a plane: a source pixel with any of
   the bits of 219C (its part of the longword mask) set gives FG, else BG.
   With COMMAND<5>
   only the
   pixels inside the clip rectangle are drawn: CLIPMIN (21C0) inclusive
   to CLIPMAX (21C4) exclusive, y in <31:16> and x in <15:0> (subtest
   18).  When the operation is done
   the TBC sets its CVAX interrupt status <1>; the model draws at once
   and reports done after a time that grows with the area (the software
   clears the status after it starts the operation, then waits). */

#define CVI_SPDONE      0x00000002                      /* scan processor done */


static uint32 sp_reg (uint32 o)
{
return lcspx_sp[o >> 2];
}

static uint32 sp_logic (uint32 fn, uint32 src, uint32 dst)
{
switch (fn & 0xF) {
    case 0x0:   return 0;                               /* clear */
    case 0x1:   return src & dst;                       /* and */
    case 0x2:   return src & ~dst;                      /* and reverse */
    case 0x3:   return src;                             /* copy */
    case 0x4:   return ~src & dst;                      /* and inverted */
    case 0x5:   return dst;                             /* no-op */
    case 0x6:   return src ^ dst;                       /* xor */
    case 0x7:   return src | dst;                       /* or */
    case 0x8:   return ~(src | dst);                    /* nor */
    case 0x9:   return ~(src ^ dst);                    /* equivalence */
    case 0xA:   return ~dst;                            /* invert */
    case 0xB:   return src | ~dst;                      /* or reverse */
    case 0xC:   return ~src;                            /* copy inverted */
    case 0xD:   return ~src | dst;                      /* or inverted */
    case 0xE:   return ~(src & dst);                    /* nand */
    default:    return 0xFFFFFFFF;                      /* set */
    }
}

/* A pixel of the frame buffer, by index, at a depth (0, 1, 2) */

static uint32 fb_getpix (uint32 idx, uint32 depth)
{
uint32 b = (idx << depth) & (LCSPX_FBSIZE - 1);

switch (depth) {
    case 0:     return lcspx_fb[b];
    case 1:     return lcspx_fb[b & ~1u] | (lcspx_fb[(b & ~1u) + 1] << 8);
    default:    b = b & ~3u;
                return lcspx_fb[b] | (lcspx_fb[b + 1] << 8) |
                    (lcspx_fb[b + 2] << 16) | ((uint32) lcspx_fb[b + 3] << 24);
    }
}

static void fb_putpix (uint32 idx, uint32 depth, uint32 v)
{
uint32 b = (idx << depth) & (LCSPX_FBSIZE - 1);

switch (depth) {
    case 0:     lcspx_fb[b] = v & 0xFF;
                break;
    case 1:     b = b & ~1u;
                lcspx_fb[b] = v & 0xFF;
                lcspx_fb[b + 1] = (v >> 8) & 0xFF;
                break;
    default:    b = b & ~3u;
                lcspx_fb[b] = v & 0xFF;
                lcspx_fb[b + 1] = (v >> 8) & 0xFF;
                lcspx_fb[b + 2] = (v >> 16) & 0xFF;
                lcspx_fb[b + 3] = (v >> 24) & 0xFF;
                break;
    }
}

static int32 sp_rect (t_bool copy)
{
uint32 cmd = sp_reg (SP_COMMAND);
int32 xs = (int32) (sp_reg (SP_XSTART) >> 16), xe = (int32) (sp_reg (SP_XEND) >> 16);
int32 ys = (int32) (sp_reg (SP_YSTART) >> 16), ye = (int32) (sp_reg (SP_YEND) >> 16);
uint32 depth = sp_reg (SP_DSTPIX) >> 30;
uint32 pmask = (depth == 0)? 0xFF: ((depth == 1)? 0xFFFF: 0xFFFFFFFF);
int32 dstride = (int32) (sp_reg (SP_STRIDE) & 0xFFFF);
int32 sstride = (int32) (sp_reg (SP_STRIDE) >> 16);
int32 dx = (cmd & CMD_XNEG)? -1: 1, dy = (cmd & CMD_YNEG)? -1: 1;
int32 w = (xe - xs) * dx, h = (ye - ys) * dy;           /* sizes, when the direction agrees */
int32 x0 = (dx < 0)? -1: 0, y0 = (dy < 0)? -1: 0;       /* first column and row from the start */
uint32 fn = sp_reg (SP_DESTLOOP) & 0xF;
uint32 lmask = sp_reg (SP_SRCMASK);                     /* over a frame-buffer longword */
uint32 smask = sp_reg (SP_DSTMASK);                     /* the same, for the source */
uint32 dbase = sp_reg (SP_DSTPIX) & PIX_INDEX, sbase = sp_reg (SP_SRCPIX) & PIX_INDEX;
uint32 fg = sp_reg (SP_FG) & pmask;
uint32 bg = sp_reg (SP_BG) & pmask;
uint32 variant = sp_reg (SP_DESTLOOP) >> 16;
t_bool plane = copy && (((variant & 0xF) == 2) || ((variant & 0xF) == 4));
t_bool clip = (cmd & CMD_CLIP)? TRUE: FALSE;
int32 cx0 = sp_reg (SP_CLIPMIN) & 0xFFFF, cy0 = sp_reg (SP_CLIPMIN) >> 16;
int32 cx1 = sp_reg (SP_CLIPMAX) & 0xFFFF, cy1 = sp_reg (SP_CLIPMAX) >> 16;
int32 r, c, x, y;
uint32 d, src, res, m, idx;

sim_debug (DBG_SP, &lcspx_dev, "%s x %d..%d y %d..%d cmd %08X fn %X dst %06X src %06X stride %d/%d fg %X mask %X\n",
    copy? "copy": "fill", xs, xe, ys, ye, cmd, fn, dbase, sbase, dstride, sstride, fg, lmask);
if ((w <= 0) || (h <= 0))
    return 0;
for (r = 0; r < h; r++) {
    y = y0 + r * dy;
    for (c = 0; c < w; c++) {
        x = x0 + c * dx;
        if (clip && (((xs + x) < cx0) || ((xs + x) >= cx1) ||
            ((ys + y) < cy0) || ((ys + y) >= cy1)))
            continue;
        if (copy) {
            idx = sbase + x + y * sstride;
            src = fb_getpix (idx, depth);
            if (cmd & CMD_SRCMASK)
                src = src & (smask >> (((idx << depth) & 3) << 3));
            if (plane)                                  /* expand the plane */
                src = (src & (sp_reg (SP_PLANE) >> (((idx << depth) & 3) << 3)) & pmask)? fg: bg;
            }
        else src = fg;
        idx = dbase + x + y * dstride;
        m = (lmask >> (((idx << depth) & 3) << 3)) & pmask; /* this pixel's part of the mask */
        d = fb_getpix (idx, depth);
        res = sp_logic (fn, src, (cmd & CMD_RDMASK)? (d & m): d) & pmask;
        if (cmd & CMD_WRMASK)
            res = (d & ~m) | (res & m);
        fb_putpix (idx, depth, res);
        }
    }
return w * h;
}

/* Fill a trapezoid (COMMAND<15> with the fill entry; subtest 26)

   The trapezoid lies between two edges, XSTART and XEND, from row
   YSTART to row YEND, all 16.16 fixed point; 23C8 and 23CC are the
   edges' steps per row.  Each row is filled from the left edge's integer
   part up to, not including, the right edge's.  COMMAND<0> says XEND's
   edge is the left one, COMMAND<1> that the rows go upwards (and the
   first row is then the one above YSTART).

   An edge steps as the option ROM's own check of the drawing computes
   it, which is not a plain fixed-point addition: the step's fraction is
   added to the edge's, and the carry goes into the integer part; but
   after a step that carried, the next step subtracts instead a fraction
   taken from YSTART<15:0> (the left edge) or YEND<15:0> (the right
   edge), and adds one more to the integer part unless that borrows.
   The fractions in YSTART and YEND are thus the edges' error terms.
   The real board passes the check, so this is taken to be what its
   microcode does; it is the least certain part of the model. */

typedef struct {
    uint32      f;                                      /* fraction */
    int32       i;                                      /* integer part */
    int32       si;                                     /* step: integer part */
    uint32      sf;                                     /* step: fraction */
    uint32      adj;                                    /* the fraction after a carry */
    t_bool      carried;
    } SP_EDGE;

static void sp_edge_init (SP_EDGE *e, uint32 x, uint32 step, uint32 adj)
{
e->f = x & 0xFFFF;
e->i = (int16) (x >> 16);
e->sf = step & 0xFFFF;
e->si = (int16) (step >> 16);
e->adj = adj & 0xFFFF;
e->carried = FALSE;
}

static void sp_edge_step (SP_EDGE *e)
{
uint32 t;

if (!e->carried) {
    t = e->f + e->sf;
    e->carried = (t & 0xFFFF0000)? TRUE: FALSE;
    }
else {
    t = e->f - e->adj;
    e->carried = (t & 0xFFFF0000)? FALSE: TRUE;
    }
e->f = t & 0xFFFF;
e->i = e->i + e->si + (e->carried? 1: 0);
}

static int32 sp_trap (void)
{
uint32 cmd = sp_reg (SP_COMMAND);
uint32 depth = sp_reg (SP_DSTPIX) >> 30;
uint32 pmask = (depth == 0)? 0xFF: ((depth == 1)? 0xFFFF: 0xFFFFFFFF);
int32 stride = (int32) (sp_reg (SP_STRIDE) & 0xFFFF);
uint32 fn = sp_reg (SP_DESTLOOP) & 0xF;
uint32 fg = sp_reg (SP_FG) & pmask;
uint32 base = sp_reg (SP_DSTPIX) & PIX_INDEX;
int32 x0 = (int16) (sp_reg (SP_XSTART) >> 16), y0 = (int16) (sp_reg (SP_YSTART) >> 16);
int32 y1 = (int16) (sp_reg (SP_YEND) >> 16);
int32 h = y1 - y0, dy = 1, row = y0, r, x, n = 0;
uint32 idx;
SP_EDGE le, re;

if (cmd & CMD_XNEG) {
    sp_edge_init (&le, sp_reg (SP_XEND), sp_reg (SP_DXR), sp_reg (SP_YSTART));
    sp_edge_init (&re, sp_reg (SP_XSTART), sp_reg (SP_DXL), sp_reg (SP_YEND));
    }
else {
    sp_edge_init (&le, sp_reg (SP_XSTART), sp_reg (SP_DXL), sp_reg (SP_YSTART));
    sp_edge_init (&re, sp_reg (SP_XEND), sp_reg (SP_DXR), sp_reg (SP_YEND));
    }
sim_debug (DBG_SP, &lcspx_dev, "trapezoid edges %d/%d rows %d..%d cmd %08X fn %X\n", le.i, re.i, y0, y1, cmd, fn);
if (h < 0) {                                            /* upwards */
    h = -h;
    dy = -1;
    sp_edge_step (&le);
    sp_edge_step (&re);
    row = y0 - 1;
    }
for (r = 0; r < h; r++, row = row + dy) {
    for (x = le.i; x < re.i; x++, n++) {
        idx = base + (x - x0) + (row - y0) * stride;    /* DSTPIX is the pixel at (XSTART, YSTART) */
        fb_putpix (idx, depth, sp_logic (fn, fg, fb_getpix (idx, depth)) & pmask);
        }
    sp_edge_step (&le);
    sp_edge_step (&re);
    }
return n;
}

/* Draw a vector (entry 2099; subtest 27)

   From (XSTART, YSTART) to (XEND, YEND), integer parts, the points that
   Bresenham's algorithm gives, as the option ROM's check computes them:
   the line is taken left to right (the end points, and with them
   COMMAND<11> and <10>, exchanged if need be), mirrored to go downwards,
   and with x and y exchanged when it is steep; the error term starts at
   2 dy - dx - 1, a point moves the minor axis while the term is not
   negative.  COMMAND<11> leaves out the first point, COMMAND<10> the
   last.  Each point takes the logical function of FG and the pixel. */

static int32 sp_vector (void)
{
uint32 cmd = sp_reg (SP_COMMAND);
uint32 depth = sp_reg (SP_DSTPIX) >> 30;
uint32 pmask = (depth == 0)? 0xFF: ((depth == 1)? 0xFFFF: 0xFFFFFFFF);
int32 stride = (int32) (sp_reg (SP_STRIDE) & 0xFFFF);
uint32 fn = sp_reg (SP_DESTLOOP) & 0xF;
uint32 fg = sp_reg (SP_FG) & pmask;
uint32 base = sp_reg (SP_DSTPIX) & PIX_INDEX;
int32 ox = (int16) (sp_reg (SP_XSTART) >> 16), oy = (int16) (sp_reg (SP_YSTART) >> 16);
int32 x0 = ox, y0 = oy;
int32 x1 = (int16) (sp_reg (SP_XEND) >> 16), y1 = (int16) (sp_reg (SP_YEND) >> 16);
t_bool nofirst = (cmd & CMD_NOFIRST)? TRUE: FALSE, nolast = (cmd & CMD_NOLAST)? TRUE: FALSE;
t_bool yneg = FALSE, steep = FALSE, tb;
int32 dx, dy, e, maj, min, last, t, n = 0, px, py;
uint32 idx;

dx = x1 - x0;
if (dx < 0) {                                           /* left to right */
    dx = -dx;
    t = x0; x0 = x1; x1 = t;
    t = y0; y0 = y1; y1 = t;
    tb = nofirst; nofirst = nolast; nolast = tb;
    }
dy = y1 - y0;
if (dy < 0) {                                           /* downwards */
    dy = -dy;
    y0 = -y0;
    y1 = -y1;
    yneg = TRUE;
    }
if (dy > dx) {                                          /* steep: exchange the axes */
    t = dx; dx = dy; dy = t;
    t = x0; x0 = y0; y0 = t;
    t = x1; x1 = y1; y1 = t;
    steep = TRUE;
    }
e = 2 * dy - dx - 1;
sim_debug (DBG_SP, &lcspx_dev, "vector (%d,%d) to (%d,%d) cmd %08X fn %X\n",
    ox, oy, (int16) (sp_reg (SP_XEND) >> 16), (int16) (sp_reg (SP_YEND) >> 16), cmd, fn);
last = nolast? x1 - 1: x1;
for (maj = x0, min = y0; maj <= last; ) {
    if ((maj != x0) || !nofirst) {
        if (steep) {
            px = min;
            py = yneg? -maj: maj;
            }
        else {
            px = maj;
            py = yneg? -min: min;
            }
        idx = base + (px - ox) + (py - oy) * stride;
        fb_putpix (idx, depth, sp_logic (fn, fg, fb_getpix (idx, depth)) & pmask);
        n++;
        }
    maj++;
    if (e < 0)
        e = e + 2 * dy;
    else {
        min++;
        e = e + 2 * dy - 2 * dx;
        }
    }
return n;
}

/* Streaming (entries 201F and 201E; subtests 28 and 29)

   The rectangle runs from XSTART to XEND and YSTART to YEND, both
   inclusive, from DSTPIX (in) or SRCPIX (out), and is taken row by row,
   one frame-buffer longword (four 8-bit pixels) per FIFO entry.
   Streaming in, the scan processor takes the data entries of the FIFO
   that the TBC routes to it (the entries with no address entry before
   them); streaming out, it appends the longwords to the FIFO that
   TBC +004<26:24> names (4 + k).  Each pixel is written as it comes: the
   logical function does not apply. */

static int32 sp_stream_size (uint32 *w)
{
int32 xs = (int16) (sp_reg (SP_XSTART) >> 16), xe = (int16) (sp_reg (SP_XEND) >> 16);
int32 ys = (int16) (sp_reg (SP_YSTART) >> 16), ye = (int16) (sp_reg (SP_YEND) >> 16);

if ((xe < xs) || (ye < ys))
    return 0;
*w = (uint32) (xe - xs + 1);
return (xe - xs + 1) * (ye - ys + 1);
}

static t_bool sp_stream_in (uint32 v)
{
uint32 depth = sp_reg (SP_DSTPIX) >> 30;
uint32 stride = sp_reg (SP_STRIDE) & 0xFFFF;
uint32 base = sp_reg (SP_DSTPIX) & PIX_INDEX;
uint32 per = 4 >> depth, k, bits = 8 << depth;

if (sp_in_left <= 0)
    return FALSE;
lcspx_dirty = TRUE;
for (k = 0; (k < per) && (sp_in_left > 0); k++, sp_in_left--) {
    fb_putpix (base + sp_in_x + sp_in_y * stride, depth,
        (bits == 32)? v: ((v >> (k * bits)) & ((1u << bits) - 1)));
    if (++sp_in_x >= sp_in_w) {
        sp_in_x = 0;
        sp_in_y++;
        }
    }
return TRUE;
}

static int32 sp_stream_out (void)
{
uint32 depth = sp_reg (SP_SRCPIX) >> 30;
uint32 stride = sp_reg (SP_STRIDE) >> 16;
uint32 base = sp_reg (SP_SRCPIX) & PIX_INDEX;
uint32 per = 4 >> depth, bits = 8 << depth;
uint32 sel = (lcspx_tbc[TBC_ROUTE] >> 24) & 7;
uint32 w, x, y, k, v;
int32 n, i;

n = sp_stream_size (&w);
if ((sel & 4) == 0)                                     /* no FIFO named */
    return n;
for (i = 0, x = y = 0; i < n; ) {
    for (k = 0, v = 0; (k < per) && (i < n); k++, i++) {
        v = v | (fb_getpix (base + x + y * stride, depth) << (k * bits));
        if (++x >= w) {
            x = 0;
            y++;
            }
        }
    fifo_put (sel & 3, v, FALSE);
    }
return n;
}

/* The microprogram counter was written: run the operation */

#define SP_PIX_PER_INSTR 8                              /* and per pixel drawn */

static void sp_start (uint32 mpc)
{
int32 n = 0;

switch (mpc & 0xFFFF) {

    case MPC_FILL:
        n = (sp_reg (SP_COMMAND) & CMD_TRAP)? sp_trap (): sp_rect (FALSE);
        break;

    case MPC_COPY:
        n = sp_rect (TRUE);
        break;

    case MPC_VECTOR:
        n = sp_vector ();
        break;

    case MPC_STREAMIN:
        sp_in_left = n = sp_stream_size (&sp_in_w);
        sp_in_x = sp_in_y = 0;
        fifo_drain ();                                  /* the data may be waiting */
        if (sp_in_left > 0)                             /* done when the last pixel comes */
            return;
        break;

    case MPC_STREAMOUT:
        n = sp_stream_out ();
        break;

    default:
        sim_debug (DBG_SP, &lcspx_dev, "microcode entry %04X not modelled\n", mpc);
        break;
        }
sim_cancel (&lcspx_unit);
sim_activate (&lcspx_unit, SP_DONE_MIN + n / SP_PIX_PER_INSTR);
lcspx_dirty = TRUE;
}

/* The scan processor has finished */

t_stat lcspx_svc (UNIT *uptr)
{
lcspx_tbc[TBC_CVINTSTS] |= CVI_SPDONE;
return SCPE_OK;
}

/* Scan processor space: o is the longword's offset in the space */

static uint32 sp_rd (uint32 o)
{
uint32 n = o & (SP_DIAG - 1);

if (o & SP_DIAG) {                                      /* diagnostic view */
    if ((n >= SP_LANEDIAG) && (n < SP_LANEDIAG + 0x100))
        return sp_lanes[(n - SP_LANEDIAG) >> 2];
    if ((n >= SP_BYTEREG) && (n < SP_BYTEREG + 0x100))
        n = sp_regoff ((n - SP_BYTEREG) >> 2);
    else if ((n >= SP_INVMEM) && (n < SP_INVMEM + 0x140))
        return ~lcspx_sp[n >> 2];
    }
else if (n == SP_UCADDR)
    return (sp_ucaddr << 16) | sp_ucmhi[sp_ucaddr];
else if (n == SP_UCDATA)
    return sp_uclo[sp_ucaddr];
else if (n == SP_REFRESH)
    return ((uint32) (sim_gtime () / 16.0)) & 0x3FF;
return lcspx_sp[n >> 2];
}

static void sp_wr (uint32 o, int32 pa, int32 val, int32 lnt)
{
uint32 n = o & (SP_DIAG - 1);
uint32 i, lanes;

if (o & SP_DIAG) {                                      /* diagnostic view */
    if ((n >= SP_LANEDIAG) && (n < SP_LANEDIAG + 0x100)) {
        i = (n - SP_LANEDIAG) >> 2;
        sp_lanes[i] = lcspx_merge (sp_lanes[i], pa, val, lnt);
        return;
        }
    if ((n >= SP_BYTEREG) && (n < SP_BYTEREG + 0x100)) {
        i = (n - SP_BYTEREG) >> 2;
        lanes = lcspx_merge (0, pa, -1, lnt);           /* the lanes this write carries */
        sp_lanes[i] = ~lanes;
        n = sp_regoff (i);
        }
    }
else if (n == SP_UCADDR) {
    i = lcspx_merge ((sp_ucaddr << 16) | sp_uchi, pa, val, lnt);
    sp_ucaddr = (i >> 16) & (SP_UCSIZE - 1);
    sp_uchi = i & 0xFFFF;
    return;
    }
else if (n == SP_UCDATA) {
    sp_uclo[sp_ucaddr] = lcspx_merge (sp_uclo[sp_ucaddr], pa, val, lnt);
    sp_ucmhi[sp_ucaddr] = (uint16) sp_uchi;
    return;
    }
if (n == SP_ID)                                         /* read only */
    return;
lcspx_sp[n >> 2] = lcspx_merge (lcspx_sp[n >> 2], pa, val, lnt);
if (n == SP_MPC)
    sp_start (lcspx_sp[n >> 2]);
}

/* Read the option's window

   Inputs:
        pa      =       physical address
   Output:
        longword of data (the caller selects the byte or word it wants)
*/

int32 lcspx_rd (int32 pa)
{
uint32 off = ((uint32) pa) - LCSPX_BASE;
uint32 v;

if (lcspx_dev.flags & DEV_DIS)
    MACH_CHECK (MCHK_READ);
if (off < LCSPX_FBSIZE) {                               /* frame buffer */
    off = off & ~3u;
    v = lcspx_fb[off] | (lcspx_fb[off + 1] << 8) |
        (lcspx_fb[off + 2] << 16) | ((uint32) lcspx_fb[off + 3] << 24);
    if (lcspx_sp[SP_PMCTL >> 2] & PMCTL_RDEN)           /* read plane mask */
        v = v & lcspx_sp[SP_RDMASK >> 2];
    sim_debug (DBG_FB, &lcspx_dev, "FB read %06X = %08X\n", off, v);
    return (int32) v;
    }
if ((off - LCSPX_ROM) < LCSPX_ROMSIZE) {                /* option ROM */
    if (lcspx_rom == NULL)                              /* an empty file: reads zero */
        return 0;
    v = LCSPX_ROMHI | lcspx_rom[(off - LCSPX_ROM) >> 2];
    if ((sim_gtime () - sp_treset) < SP_RESET_INSTR)  /* the scan processor is still resetting */
        v = v & ~CFG_SPTEST;
    sim_debug (DBG_ROM, &lcspx_dev, "ROM read %05X = %08X\n", off - LCSPX_ROM, v);
    return (int32) v;
    }
if ((off - LCSPX_SP2) < LCSPX_SPSIZE)                   /* the scan processor again */
    off = off - LCSPX_SP2 + LCSPX_SP;
if ((off - LCSPX_SP) < LCSPX_SPSIZE) {                  /* scan processor */
    v = sp_rd ((off - LCSPX_SP) & ~3u);
    sim_debug (DBG_REG, &lcspx_dev, "SP read %05X = %08X, PC %08X\n", off - LCSPX_SP, v, fault_PC);
    return (int32) v;
    }
if ((off - LCSPX_TBC) < LCSPX_TBCSIZE) {                /* TBC */
    v = tbc_rd ((off - LCSPX_TBC) >> 2);
    sim_debug (DBG_REG, &lcspx_dev, "TBC read %03X = %08X, PC %08X\n", off - LCSPX_TBC, v, fault_PC);
    return (int32) v;
    }
if ((off - LCSPX_FIFO) < LCSPX_FIFOSIZE) {              /* FIFO memory */
    v = lcspx_fifomem[(off - LCSPX_FIFO) >> 2];
    sim_debug (DBG_REG, &lcspx_dev, "FIFO memory read %05X = %08X, PC %08X\n", off - LCSPX_FIFO, v, fault_PC);
    return (int32) v;
    }
if ((off - LCSPX_BT) < LCSPX_BTSIZE) {                  /* Bt459: one port per 16 KB */
    v = bt_rd ((off - LCSPX_BT) >> 14);
    sim_debug (DBG_REG, &lcspx_dev, "Bt459 read port %d = %02X, PC %08X\n", (off - LCSPX_BT) >> 14, v, fault_PC);
    return (int32) v;
    }
MACH_CHECK (MCHK_READ);                                 /* nothing answers */
return 0;
}

/* Write the option's window

   Inputs:
        pa      =       physical address
        val     =       data, right justified
        lnt     =       length (L_BYTE, L_WORD, L_LONG)
   Outputs:
        none
*/

void lcspx_wr (int32 pa, int32 val, int32 lnt)
{
if (lcspx_dev.flags & DEV_DIS)
    return;
lcspx_wroff (((uint32) pa) - LCSPX_BASE, pa, val, lnt);
}

/* Write at an offset in the option's window, for the CPU or for the
   TBC's command path (pa gives the byte lanes of a short write) */

static void lcspx_wroff (uint32 off, int32 pa, int32 val, int32 lnt)
{
uint32 i;

if (off < LCSPX_FBSIZE) {                               /* frame buffer */
    uint32 old, nv, m;

    sim_debug (DBG_FB, &lcspx_dev, "FB write %06X = %08X (%d)\n", off, val, lnt);
    off = off & ~3u;
    old = lcspx_fb[off] | (lcspx_fb[off + 1] << 8) |
        (lcspx_fb[off + 2] << 16) | ((uint32) lcspx_fb[off + 3] << 24);
    nv = lcspx_merge (old, pa, val, lnt);
    if (lcspx_sp[SP_PMCTL >> 2] & PMCTL_WREN) {         /* write plane mask */
        m = lcspx_sp[SP_WRMASK >> 2];
        nv = (old & ~m) | (nv & m);
        }
    lcspx_fb[off] = nv & 0xFF;
    lcspx_fb[off + 1] = (nv >> 8) & 0xFF;
    lcspx_fb[off + 2] = (nv >> 16) & 0xFF;
    lcspx_fb[off + 3] = (nv >> 24) & 0xFF;
    lcspx_dirty = TRUE;
    return;
    }
if (((off - LCSPX_CMD) < (FIFO_N * LCSPX_CMDSTEP)) &&   /* a FIFO's command window */
    (((off - LCSPX_CMD) % LCSPX_CMDSTEP) < LCSPX_CMDSIZE)) {
    i = (off - LCSPX_CMD) / LCSPX_CMDSTEP;
    sim_debug (DBG_REG, &lcspx_dev, "FIFO %d command %06X = %08X (%d), PC %08X\n", i, off - LCSPX_CMD, val, lnt, fault_PC);
    if (lnt >= L_LONG) {
        fifo_put (i, FIFO_ADDR | ((off - LCSPX_CMD) & 0x00FFFFFC), TRUE);
        fifo_put (i, val, FALSE);
        fifo_drain ();
        }
    return;
    }
if ((off - LCSPX_SP2) < LCSPX_SPSIZE)                   /* the scan processor again */
    off = off - LCSPX_SP2 + LCSPX_SP;
if ((off - LCSPX_SP) < LCSPX_SPSIZE) {                  /* scan processor */
    sim_debug (DBG_REG, &lcspx_dev, "SP write %05X = %08X (%d), PC %08X\n", off - LCSPX_SP, val, lnt, fault_PC);
    sp_wr ((off - LCSPX_SP) & ~3u, pa, val, lnt);
    return;
    }
if ((off - LCSPX_TBC) < LCSPX_TBCSIZE) {                /* TBC */
    i = (off - LCSPX_TBC) >> 2;
    sim_debug (DBG_REG, &lcspx_dev, "TBC write %03X = %08X (%d), PC %08X\n", off - LCSPX_TBC, val, lnt, fault_PC);
    tbc_wr (i, lcspx_merge (lcspx_tbc[i], pa, val, lnt), lcspx_merge (0, pa, val, lnt));
    return;
    }
if ((off - LCSPX_FIFO) < LCSPX_FIFOSIZE) {              /* FIFO memory */
    i = (off - LCSPX_FIFO) >> 2;
    sim_debug (DBG_REG, &lcspx_dev, "FIFO memory write %05X = %08X (%d), PC %08X\n", off - LCSPX_FIFO, val, lnt, fault_PC);
    lcspx_fifomem[i] = lcspx_merge (lcspx_fifomem[i], pa, val, lnt);
    lcspx_fifotag[i] = 0;
    return;
    }
if ((off - LCSPX_BT) < LCSPX_BTSIZE) {                  /* Bt459 */
    i = (off - LCSPX_BT) >> 14;
    if (lnt < L_LONG)                                   /* the data lane is <7:0> */
        val = val << ((pa & 3) << 3);
    sim_debug (DBG_REG, &lcspx_dev, "Bt459 write port %d = %02X, PC %08X\n", i, val & 0xFF, fault_PC);
    bt_wr (i, val & 0xFF);
    return;
    }
return;                                                 /* the ROM and the holes ignore writes */
}

/* Reset */

t_stat lcspx_reset (DEVICE *dptr)
{
int32 i;

if (lcspx_fb == NULL) {
    lcspx_fb = (uint8 *) calloc (LCSPX_FBSIZE, sizeof (uint8));
    if (lcspx_fb == NULL)
        return SCPE_MEM;
    }
if ((lcspx_rom == NULL) && !(lcspx_unit.flags & UNIT_ATT) &&   /* no ROM yet: */
    (lcspx_rom_builtin () != SCPE_OK))                  /* the built-in one */
    return SCPE_MEM;
memset (lcspx_sp, 0, sizeof (lcspx_sp));
memset (sp_lanes, 0, sizeof (sp_lanes));
lcspx_sp[SP_ID >> 2] = SP_ID_INIT;
sim_cancel (&lcspx_unit);
sp_ucaddr = sp_uchi = 0;
memset (lcspx_tbc, 0, sizeof (lcspx_tbc));
tbc_line = 0;
tbc_tlast = sim_gtime ();
memset (lcspx_fifomem, 0, sizeof (lcspx_fifomem));
memset (fifo_wp, 0, sizeof (fifo_wp));
memset (fifo_rp, 0, sizeof (fifo_rp));
memset (fifo_cnt, 0, sizeof (fifo_cnt));
for (i = 0; i < FIFO_N; i++)
    fifo_dest[i] = FIFO_NODEST;
sp_in_left = 0;
memset (lcspx_fifotag, 0, sizeof (lcspx_fifotag));
bt_addr = bt_m3 = 0;
lcspx_dirty = TRUE;
return lcspx_display ();
}

/* The display

   The picture is the frame buffer's active area (TBC +10C and VBS give
   its size; 1280 x 1024 at 72 Hz as the console sets it up), each pixel
   through the Bt459's pixel read mask and colour map, with the Bt459's
   64 x 64 cursor over it when cursor command <7:6> enable it.  The
   cursor RAM holds 2 bits per pixel, the leftmost in <7:6> of each byte;
   1-3 select the cursor colours.  The cursor position registers count
   from 360 left and 37 above the first visible pixel (NetBSD's spx
   driver's biases).  With the timing stopped the screen is black. */

#define LCSPX_XSIZE     1280                            /* the window */
#define LCSPX_YSIZE     1024
#define LCSPX_REFRESH   20000                           /* usec between refreshes */

static t_bool lcspx_vid = FALSE;                        /* the window is open */
static uint32 *lcspx_pic = NULL;                        /* the picture, for the window */

typedef void (*LCSPX_PUT) (uint32 x, uint32 y, uint8 r, uint8 g, uint8 b, void *ctx);

static void lcspx_render (LCSPX_PUT put, void *ctx)
{
uint32 w = ((lcspx_tbc[0x10C >> 2] & 0x1FF) + 1) * 4;
uint32 h = (lcspx_tbc[TBC_VBLANK] & TBC_VMASK) + 1;
t_bool on = (lcspx_tbc[TBC_TCSR] & TCSR_RUN)? TRUE: FALSE;
uint32 prm = bt_reg[BT_PRM - 0x200];
t_bool cur = ((bt_cur[0] & 0xC0) != 0);
int32 cx = (int32) (bt_cur[1] | ((bt_cur[2] & 0xF) << 8)) - 360;
int32 cy = (int32) (bt_cur[3] | ((bt_cur[4] & 0xF) << 8)) - 37;
uint32 x, y, c;
int32 dx, dy;
uint8 *rgb;

for (y = 0; y < LCSPX_YSIZE; y++) {
    for (x = 0; x < LCSPX_XSIZE; x++) {
        if (!on || (x >= w) || (y >= h)) {
            put (x, y, 0, 0, 0, ctx);
            continue;
            }
        rgb = bt_cmap[lcspx_fb[(y * w + x) & (LCSPX_FBSIZE - 1)] & prm];
        dx = (int32) x - cx;
        dy = (int32) y - cy;
        if (cur && (dx >= 0) && (dx < 64) && (dy >= 0) && (dy < 64)) {
            c = (bt_curram[dy * 16 + (dx >> 2)] >> (6 - 2 * (dx & 3))) & 3;
            if (c)
                rgb = bt_curcol[c - 1];
            }
        put (x, y, rgb[0], rgb[1], rgb[2], ctx);
        }
    }
}

static void lcspx_put_window (uint32 x, uint32 y, uint8 r, uint8 g, uint8 b, void *ctx)
{
lcspx_pic[y * LCSPX_XSIZE + x] = vid_map_rgb (r, g, b);
}

/* Open or close the window to follow the MONITOR setting */

static t_stat lcspx_display (void)
{
t_stat r;

if ((lcspx_unit.flags & UNIT_MONITOR) && !(lcspx_dev.flags & DEV_DIS)) {
    if (!lcspx_vid && !vid_active) {
        r = vid_open (&lcspx_dev, "LCSPX", LCSPX_XSIZE, LCSPX_YSIZE, 0);
        if (r == SCPE_OK) {
            lcspx_pic = (uint32 *) calloc (LCSPX_XSIZE * LCSPX_YSIZE, sizeof (uint32));
            if (lcspx_pic == NULL) {
                vid_close ();
                return SCPE_MEM;
                }
            lcspx_vid = TRUE;
            lcspx_dirty = TRUE;
            }
        else sim_messagef (SCPE_OK, "%s: no window (%s); SET %s SCREENSHOT=file still works\n",
            sim_dname (&lcspx_dev), sim_error_text (r), sim_dname (&lcspx_dev));
        }
    if (lcspx_vid)
        sim_activate_after (&lcspx_disp, LCSPX_REFRESH);
    }
else if (lcspx_vid) {
    sim_cancel (&lcspx_disp);
    free (lcspx_pic);
    lcspx_pic = NULL;
    lcspx_vid = FALSE;
    return vid_close ();
    }
return SCPE_OK;
}

t_stat lcspx_set_monitor (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
lcspx_unit.flags = (lcspx_unit.flags & ~UNIT_MONITOR) | val;
return lcspx_display ();
}

/* Refresh the window, and pass the keyboard and mouse on */

t_stat lcspx_disp_svc (UNIT *uptr)
{
SIM_KEY_EVENT kev;
SIM_MOUSE_EVENT mev;

if (!lcspx_vid)
    return SCPE_OK;
if (vid_poll_kb (&kev) == SCPE_OK)
    lk_event (&kev);
if (vid_poll_mouse (&mev) == SCPE_OK)
    vs_event (&mev);
if (lcspx_dirty) {
    lcspx_dirty = FALSE;
    lcspx_render (&lcspx_put_window, NULL);
    vid_draw (0, 0, LCSPX_XSIZE, LCSPX_YSIZE, lcspx_pic);
    vid_refresh ();
    }
sim_activate_after (uptr, LCSPX_REFRESH);
return SCPE_OK;
}

/* SET LCSPX SCREENSHOT=file: the picture as a PNG file */

#if defined (HAVE_LIBPNG)
static void lcspx_put_png (uint32 x, uint32 y, uint8 r, uint8 g, uint8 b, void *ctx)
{
uint8 *row = ((uint8 **) ctx)[y];

row[3 * x] = r;
row[3 * x + 1] = g;
row[3 * x + 2] = b;
}
#endif

t_stat lcspx_screenshot (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
#if defined (HAVE_LIBPNG)
FILE *f;
png_structp png;
png_infop info;
uint8 *pix, **rows;
uint32 y;
t_stat r = SCPE_OK;

if ((cptr == NULL) || (*cptr == 0))
    return sim_messagef (SCPE_ARG, "SCREENSHOT needs a file name\n");
pix = (uint8 *) malloc (LCSPX_XSIZE * LCSPX_YSIZE * 3);
rows = (uint8 **) malloc (LCSPX_YSIZE * sizeof (uint8 *));
if ((pix == NULL) || (rows == NULL)) {
    free (pix);
    free (rows);
    return SCPE_MEM;
    }
for (y = 0; y < LCSPX_YSIZE; y++)
    rows[y] = pix + y * LCSPX_XSIZE * 3;
lcspx_render (&lcspx_put_png, rows);
f = sim_fopen (cptr, "wb");
if (f == NULL)
    r = sim_messagef (SCPE_OPENERR, "%s: cannot create %s\n", sim_dname (&lcspx_dev), cptr);
else {
    png = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info = (png == NULL)? NULL: png_create_info_struct (png);
    if ((info == NULL) || setjmp (png_jmpbuf (png)))
        r = SCPE_IOERR;
    else {
        png_init_io (png, f);
        png_set_IHDR (png, info, LCSPX_XSIZE, LCSPX_YSIZE, 8, PNG_COLOR_TYPE_RGB,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_write_info (png, info);
        png_write_image (png, rows);
        png_write_end (png, NULL);
        }
    png_destroy_write_struct (&png, &info);
    fclose (f);
    }
free (rows);
free (pix);
return r;
#else
return sim_messagef (SCPE_NOFNC, "%s: this simulator was built without PNG support\n", sim_dname (&lcspx_dev));
#endif
}

/* The option ROM image

   Two forms are accepted:
        262144 bytes    the ROM as the CPU reads it (a dump of
                        39BC.0000-39BF.FFFF): longwords, bits <15:0> data;
                        bits <31:16> should read C000 and are not kept
        131072 bytes    the logical ROM: a byte stream, two bytes per
                        longword, lane 0 (bits <7:0>) first
   The built-in image (lcspx.bin) is of the first form.  lcspx_rom_fill
   returns the number of longwords whose bits <31:16> do not read C000. */

static uint32 lcspx_rom_fill (const uint8 *buf, size_t len)
{
uint32 i, odd = 0;

for (i = 0; i < LCSPX_ROMWORDS; i++) {
    if (len == LCSPX_ROMSIZE) {                         /* as read: little-endian longwords */
        lcspx_rom[i] = buf[4 * i] | (buf[4 * i + 1] << 8);
        if ((buf[4 * i + 2] != 0x00) || (buf[4 * i + 3] != 0xC0))
            odd++;
        }
    else lcspx_rom[i] = buf[2 * i] | (buf[2 * i + 1] << 8);
    }
return odd;
}

static t_stat lcspx_rom_builtin (void)
{
if (lcspx_rom == NULL)
    lcspx_rom = (uint16 *) calloc (LCSPX_ROMWORDS, sizeof (uint16));
if (lcspx_rom == NULL)
    return SCPE_MEM;
lcspx_rom_fill (vax_lcspx_bin, sizeof (vax_lcspx_bin));
return SCPE_OK;
}

/* Attach an option ROM file in place of the built-in image; an empty
   file takes the ROM away (the window reads zero) */

t_stat lcspx_attach (UNIT *uptr, CONST char *cptr)
{
FILE *f;
uint8 *buf;
long len;
t_stat r;
uint32 odd;

f = sim_fopen (cptr, "rb");
if (f == NULL)
    return sim_messagef (SCPE_OPENERR, "%s: cannot open %s\n", sim_dname (&lcspx_dev), cptr);
len = (long) sim_fsize (f);
if (len == 0) {                                         /* empty: no ROM */
    fclose (f);
    r = attach_unit (uptr, cptr);
    if (r != SCPE_OK)
        return r;
    free (lcspx_rom);
    lcspx_rom = NULL;
    return sim_messagef (SCPE_OK, "%s: %s is empty; the option ROM reads zero\n",
        sim_dname (&lcspx_dev), cptr);
    }
if ((len != LCSPX_ROMSIZE) && (len != (LCSPX_ROMSIZE >> 1))) {
    fclose (f);
    return sim_messagef (SCPE_ARG, "%s: %s is %ld bytes; the option ROM is %d (as read) or %d (logical)\n",
        sim_dname (&lcspx_dev), cptr, len, LCSPX_ROMSIZE, LCSPX_ROMSIZE >> 1);
    }
buf = (uint8 *) malloc ((size_t) len);
if ((buf == NULL) || (fread (buf, 1, (size_t) len, f) != (size_t) len)) {
    free (buf);
    fclose (f);
    return SCPE_IOERR;
    }
fclose (f);
r = attach_unit (uptr, cptr);                           /* record the file name */
if (r != SCPE_OK) {
    free (buf);
    return r;
    }
if (lcspx_rom == NULL)
    lcspx_rom = (uint16 *) calloc (LCSPX_ROMWORDS, sizeof (uint16));
if (lcspx_rom == NULL) {
    free (buf);
    detach_unit (uptr);
    return SCPE_MEM;
    }
odd = lcspx_rom_fill (buf, (size_t) len);
free (buf);
if (odd)
    sim_messagef (SCPE_OK, "%s: %u longwords of %s do not read C000 in bits <31:16>\n",
        sim_dname (&lcspx_dev), odd, cptr);
return SCPE_OK;
}

/* Detach: back to the built-in image */

t_stat lcspx_detach (UNIT *uptr)
{
t_stat r = detach_unit (uptr);

if (r != SCPE_OK)
    return r;
return lcspx_rom_builtin ();
}

t_stat lcspx_show_rom (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
if (!(lcspx_unit.flags & UNIT_ATT))
    fprintf (st, "built-in option ROM");
else if (lcspx_rom == NULL)
    fprintf (st, "no option ROM (empty file %s)", lcspx_unit.filename);
else fprintf (st, "option ROM from %s", lcspx_unit.filename);
return SCPE_OK;
}

t_stat lcspx_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
fprintf (st, "LCSPX 8-plane graphics option (%s)\n\n", dptr->name);
fprintf (st, "The LCSPX is the colour graphics option of the VAXstation 4000 Model 90.\n");
fprintf (st, "Its option ROM, DEC firmware that the console reads and runs at power-up,\n");
fprintf (st, "is built in (a dump of the ROM of a VAXstation 4000/90).  Another image\n");
fprintf (st, "replaces it with\n\n");
fprintf (st, "   sim> ATTACH LCSPX <file>\n\n");
fprintf (st, "The file is either the ROM as the CPU reads it (262144 bytes, one longword\n");
fprintf (st, "C000xxxx per 16-bit word) or the logical ROM (131072 bytes).  DETACH LCSPX\n");
fprintf (st, "goes back to the built-in image.  An empty file takes the ROM away: it then\n");
fprintf (st, "reads as zero and the console reports the graphics as UNK.  SHOW LCSPX ROM\n");
fprintf (st, "says which image is in use.\n\n");
fprintf (st, "SET LCSPX MONITOR says that a monitor or a terminated video cable is\n");
fprintf (st, "connected; NOMONITOR, the default, that nothing terminates the video\n");
fprintf (st, "outputs, as on the author's headless VAXstation 4000/90: the console then\n");
fprintf (st, "reports the graphics as \"?? 001 0512\" (its Bt459 analog compare test needs\n");
fprintf (st, "terminated outputs).  SET LCSPX DISABLED removes the option altogether.\n");
fprint_set_help (st, dptr);
fprint_show_help (st, dptr);
fprint_reg_help (st, dptr);
return SCPE_OK;
}

const char *lcspx_description (DEVICE *dptr)
{
return "LCSPX graphics option";
}
