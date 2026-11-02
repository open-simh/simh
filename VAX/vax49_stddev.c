/* vax49_stddev.c: VAXstation 4000 Model 90 (KA49) standard devices

   Copyright (c) 2026, Douglas P. Fields, Jr.
   This module incorporates code from SimH, Copyright (c) 1998-2008, Robert M Supnik,
   and from the KA4xx standard devices, Copyright (c) 2019, Matt Burke

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
   THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name(s) of the author(s) shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from the author(s).

   rom          console ROM (no registers)
   nvr          DS1287A battery-backed RAM, registers 14-63 (no registers)
   clk          100Hz calibration clock (no KA49 register)
   tmr          NVAX interval timer (ICCS, NICR, ICR)

   The KA49 differs from the other KA4xx boards in three ways that make
   vax4xx_stddev.c unsuitable: its ROM is 512 KB, its clock and RAM chip is
   reached one register per longword (register n in bits <9:2> of the
   longword at 2540.0000 + 4n, the KA46 overlays a 1 KB RAM), and the NVAX
   has the full VAX interval timer (ICCS, NICR, ICR counting microseconds)
   instead of the 100 Hz subset.
*/

#include "vax_defs.h"

#define UNIT_V_NODELAY  (UNIT_V_UF + 0)                 /* ROM access equal to RAM access */
#define UNIT_NODELAY    (1u << UNIT_V_NODELAY)

#define CLK_DELAY       5000                            /* 100 Hz */
#define TMXR_MULT       1                               /* 100 Hz */

/* Interval timer (VAX architecture; NVAX full implementation) */

#define TMR_CSR_ERR     0x80000000                      /* error W1C */
#define TMR_CSR_DON     0x00000080                      /* done (interrupt) W1C */
#define TMR_CSR_IE      0x00000040                      /* int enb RW */
#define TMR_CSR_SGL     0x00000020                      /* single WO */
#define TMR_CSR_XFR     0x00000010                      /* xfer WO */
#define TMR_CSR_RUN     0x00000001                      /* run RW */
#define TMR_CSR_RD      (TMR_CSR_W1C | TMR_CSR_WR)
#define TMR_CSR_W1C     (TMR_CSR_ERR | TMR_CSR_DON)
#define TMR_CSR_WR      (TMR_CSR_IE | TMR_CSR_RUN)

/* Debug flags */

#define TMR_DB_REG      0x01                            /* register access */
#define TMR_DB_TICK     0x02                            /* ticks */
#define TMR_DB_SCHED    0x04                            /* scheduling */
#define TMR_DB_INT      0x08                            /* interrupts */

#define NVR_DB_REG      0x01                            /* register access */

/* DS1287A: registers 0-13 are the clock (vax_watch.c); register n is in
   bits <9:2> of its longword; bits <1:0> and <15:10> of the bus are not
   driven and read as ones; the chip sits on a 16-bit path, so the high
   halfword of a longword read repeats the low one.  The real 4000/90 read
   FCB3FCB3 for a register holding 2C. */

#define NVR_CLKREGS     14                              /* registers 0-13: the clock */
#define NVR_NREGS       (NVRSIZE >> 2)                  /* 64 */
#define NVR_UNDRIVEN    0xFC03                          /* bits that read as ones */

/* The battery-backed RAM as the console's own initialisation leaves it
   (its NVR test writes these when the RAM is not valid): all zero except
   the halt/mailbox byte (register 14) = 62 (SET HALT 2, MOP 1), the
   validity signature 55 AA 33 0F in registers 34-37, and the SCSI byte
   (register 47) = 06 (host ID 6). */

static const struct { int32 reg; uint8 val; } nvr_defaults[] = {
    { 14, 0x62 }, { 34, 0x55 }, { 35, 0xAA }, { 36, 0x33 }, { 37, 0x0F }, { 47, 0x06 },
    { -1, 0 }
    };

uint32 *rom = NULL;                                     /* boot ROM */
uint32 *nvr = NULL;                                     /* non-volatile mem */
int32 tmr_iccs = 0;                                     /* interval timer csr */
uint32 tmr_icr = 0;                                     /* curr interval */
uint32 tmr_nicr = 0;                                    /* next interval */
int32 tmr_int = 0;                                      /* interrupt */
int32 clk_tps = 100;                                    /* ticks/second */
int32 tmxr_poll = CLK_DELAY * TMXR_MULT;                /* term mux poll */
int32 tmr_poll = CLK_DELAY;                             /* pgm timer poll */

t_stat rom_ex (t_value *vptr, t_addr exta, UNIT *uptr, int32 sw);
t_stat rom_dep (t_value val, t_addr exta, UNIT *uptr, int32 sw);
t_stat rom_reset (DEVICE *dptr);
t_stat rom_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr);
const char *rom_description (DEVICE *dptr);
t_stat nvr_ex (t_value *vptr, t_addr exta, UNIT *uptr, int32 sw);
t_stat nvr_dep (t_value val, t_addr exta, UNIT *uptr, int32 sw);
t_stat nvr_reset (DEVICE *dptr);
t_stat nvr_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr);
t_stat nvr_attach (UNIT *uptr, CONST char *cptr);
t_stat nvr_detach (UNIT *uptr);
const char *nvr_description (DEVICE *dptr);
t_stat clk_svc (UNIT *uptr);
t_stat clk_reset (DEVICE *dptr);
const char *clk_description (DEVICE *dptr);
t_stat tmr_svc (UNIT *uptr);
t_stat tmr_reset (DEVICE *dptr);
t_stat tmr_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr);
const char *tmr_description (DEVICE *dptr);
int32 icr_rd (void);
void tmr_sched (uint32 incr);

/* ROM data structures

   rom_dev      ROM device descriptor
   rom_unit     ROM units
   rom_reg      ROM register list

   ROM reads are not slowed by default: unlike the KA4x consoles, the KA49
   console times all its delays with the interval timer, not with
   instruction loops.
*/

UNIT rom_unit = { UDATA (NULL, UNIT_FIX+UNIT_BINK+UNIT_NODELAY, ROMSIZE) };

REG rom_reg[] = {
    { NULL }
    };

MTAB rom_mod[] = {
    { UNIT_NODELAY, UNIT_NODELAY, "fast access", "NODELAY", NULL, NULL, NULL, "Disable calibrated ROM access speed" },
    { UNIT_NODELAY, 0, "1usec calibrated access", "DELAY",  NULL, NULL, NULL, "Enable calibrated ROM access speed" },
    { 0 }
    };

DEVICE rom_dev = {
    "ROM", &rom_unit, rom_reg, rom_mod,
    1, 16, ROMAWIDTH, 4, 16, 32,
    &rom_ex, &rom_dep, &rom_reset,
    NULL, NULL, NULL,
    NULL, 0, 0, NULL, NULL, NULL, &rom_help, NULL, NULL,
    &rom_description
    };

/* NVR data structures

   nvr_dev      NVR device descriptor
   nvr_unit     NVR units
   nvr_reg      NVR register list

   The attached file holds one longword per register, registers 0-63; the
   clock registers 0-13 are not kept there (the clock follows the host).
*/

UNIT nvr_unit =
    { UDATA (NULL, UNIT_FIX+UNIT_BINK, NVRSIZE) };

REG nvr_reg[] = {
    { NULL }
    };

DEBTAB nvr_debug[] = {
    { "REG", NVR_DB_REG, "register reads and writes" },
    { NULL, 0 }
    };

DEVICE nvr_dev = {
    "NVR", &nvr_unit, nvr_reg, NULL,
    1, 16, NVRAWIDTH, 4, 16, 32,
    &nvr_ex, &nvr_dep, &nvr_reset,
    NULL, &nvr_attach, &nvr_detach,
    NULL, DEV_DEBUG, 0, nvr_debug, NULL, NULL, &nvr_help, NULL, NULL,
    &nvr_description
    };

/* CLK data structures

   clk_dev      CLK device descriptor
   clk_unit     CLK unit descriptor
   clk_reg      CLK register list

   The KA49 has no 100 Hz clock of its own; this unit calibrates the
   simulator's instruction rate against real time (for the terminal
   multiplexer polls and the interval timer's 10 ms ticks).
*/

UNIT clk_unit = { UDATA (&clk_svc, UNIT_IDLE, 0), CLK_DELAY };

REG clk_reg[] = {
    { DRDATAD (TIME,   clk_unit.wait,        24, "initial poll interval"), REG_NZ + PV_LEFT },
    { DRDATAD (POLL,        tmr_poll,        24, "calibrated poll interval"), REG_NZ + PV_LEFT + REG_HRO },
    { DRDATAD (TPS,          clk_tps,         8, "ticks per second (100)"), REG_NZ + PV_LEFT },
#if defined (SIM_ASYNCH_IO)
    { DRDATAD (ASYNCH,            sim_asynch_enabled,         1, "asynch I/O enabled flag"), PV_LEFT },
    { DRDATAD (LATENCY,           sim_asynch_latency,        32, "desired asynch interrupt latency"), PV_LEFT },
    { DRDATAD (INST_LATENCY, sim_asynch_inst_latency,        32, "calibrated instruction latency"), PV_LEFT },
#endif
    { NULL }
    };

DEVICE clk_dev = {
    "CLK", &clk_unit, clk_reg, NULL,
    1, 0, 0, 0, 0, 0,
    NULL, NULL, &clk_reset,
    NULL, NULL, NULL,
    NULL, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL,
    &clk_description
    };

/* TMR data structures

   tmr_dev      TMR device descriptor
   tmr_unit     TMR unit descriptor
   tmr_reg      TMR register list
*/

static BITFIELD tmr_iccs_bits [] = {
    BIT(RUN),                                   /* Run */
    BITNCF(3),                                  /* unused */
    BIT(XFR),                                   /* Transfer */
    BIT(SGL),                                   /* Single */
    BIT(IE),                                    /* Interrupt Enable */
    BIT(DON),                                   /* Done */
    BITNCF(23),                                 /* unused */
    BIT(ERR),                                   /* Error */
    ENDBITS
    };

UNIT tmr_unit = { UDATA (&tmr_svc, 0, 0) };

REG tmr_reg[] = {
    { HRDATADF (ICCS,         tmr_iccs, 32, "interval timer control and status", tmr_iccs_bits) },
    { HRDATAD  (ICR,           tmr_icr, 32, "interval count register") },
    { HRDATAD  (NICR,         tmr_nicr, 32, "next interval count register") },
    { FLDATAD  (INT,           tmr_int,  0, "interrupt request") },
    { NULL }
    };

DEBTAB tmr_deb[] = {
    { "REG",   TMR_DB_REG,   "Register Access"},
    { "TICK",  TMR_DB_TICK,  "Ticks"},
    { "SCHED", TMR_DB_SCHED, "Scheduling"},
    { "INT",   TMR_DB_INT,   "Interrupts"},
    { NULL, 0 }
    };

DEVICE tmr_dev = {
    "TMR", &tmr_unit, tmr_reg, NULL,
    1, 0, 0, 0, 0, 0,
    NULL, NULL, &tmr_reset,
    NULL, NULL, NULL,
    NULL, DEV_DEBUG, 0,
    tmr_deb, NULL, NULL, &tmr_help, NULL, NULL,
    &tmr_description
    };

/* ROM: read only memory - stored in a buffered file
   Register space access routines see ROM twice */

int32 rom_rd (int32 pa)
{
int32 rg = ((pa - ROMBASE) & ROMAMASK) >> 2;
int32 val = rom[rg];

if (rom_unit.flags & UNIT_NODELAY)
    return val;

return sim_rom_read_with_delay (val);
}

void rom_wr_B (int32 pa, int32 val)
{
int32 rg = ((pa - ROMBASE) & ROMAMASK) >> 2;
int32 sc = (pa & 3) << 3;

rom[rg] = ((val & 0xFF) << sc) | (rom[rg] & ~(0xFF << sc));
return;
}

/* ROM examine */

t_stat rom_ex (t_value *vptr, t_addr exta, UNIT *uptr, int32 sw)
{
uint32 addr = (uint32) exta;

if ((vptr == NULL) || (addr & 03))
    return SCPE_ARG;
if (addr >= ROMSIZE)
    return SCPE_NXM;
*vptr = rom[addr >> 2];
return SCPE_OK;
}

/* ROM deposit */

t_stat rom_dep (t_value val, t_addr exta, UNIT *uptr, int32 sw)
{
uint32 addr = (uint32) exta;

if (addr & 03)
    return SCPE_ARG;
if (addr >= ROMSIZE)
    return SCPE_NXM;
rom[addr >> 2] = (uint32) val;
return SCPE_OK;
}

/* ROM reset */

t_stat rom_reset (DEVICE *dptr)
{
if (rom == NULL)
    rom = (uint32 *) calloc (ROMSIZE >> 2, sizeof (uint32));
if (rom == NULL)
    return SCPE_MEM;
return SCPE_OK;
}

t_stat rom_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
fprintf (st, "Read-only memory (ROM)\n\n");
fprintf (st, "The console ROM consists of a single unit, simulating the 512KB of flash\n");
fprintf (st, "memory that holds the KA49 console.  It has no registers.  The ROM is\n");
fprintf (st, "loaded with a binary byte stream using the LOAD -r command:\n\n");
fprintf (st, "   LOAD -r KA49A.BIN      load ROM image KA49A.BIN\n\n");
fprintf (st, "When the simulator starts running (via the BOOT command), if the ROM has\n");
fprintf (st, "not yet been loaded, it is loaded from the image built into the simulator,\n");
fprintf (st, "or from the file ka49a.bin in the current working directory when the\n");
fprintf (st, "simulator was built without one (a tree without VAX/ka49a.bin, or a build\n");
fprintf (st, "with DONT_USE_INTERNAL_ROM).  The image is DEC's KA49-A firmware, 524288\n");
fprintf (st, "bytes, published in the Oxide VAX PROM archive (www.oxide.org/vaxproms,\n");
fprintf (st, "KA49-A_V1.4-0D0-V4.4.bin.gz) and as the last 524288 bytes of cougar.sys in\n");
fprintf (st, "DEC's V1.4 firmware update kit; BOOT without it says more.\n\n");
fprintf (st, "ROM reads are not slowed down by default: the KA49 console times its\n");
fprintf (st, "delays with the interval timer.  SET ROM DELAY slows execution from the\n");
fprintf (st, "ROM to about 500K instructions per second.\n");
fprint_set_help (st, dptr);
return SCPE_OK;
}

const char *rom_description (DEVICE *dptr)
{
return "read-only memory";
}

/* NVR: the DS1287A registers in I/O space.  Register n reads back as
   bits <9:2> of a 16-bit word whose other bits read as ones, repeated in
   both halves of the longword. */

static int32 nvr_reg_rd (int32 rg)
{
if (rg < NVR_CLKREGS)                                   /* watch chip */
    return wtc_rd (rg) & 0xFF;
return nvr[rg] & 0xFF;
}

static void nvr_reg_wr (int32 rg, int32 val)
{
val = val & 0xFF;
if (rg < NVR_CLKREGS)                                   /* watch chip */
    wtc_wr (rg, val);
else
    nvr[rg] = val;
}

int32 nvr_rd (int32 pa)
{
int32 rg = ((pa - NVRBASE) >> 2) & (NVR_NREGS - 1);
uint32 w = NVR_UNDRIVEN | ((uint32) nvr_reg_rd (rg) << 2);

sim_debug (NVR_DB_REG, &nvr_dev, "nvr_rd(reg=%d) = %02X\n", rg, (w >> 2) & 0xFF);
return (int32) ((w << 16) | w);
}

/* A write reaches the chip through bits <9:2> of the 16-bit path.  A byte
   write drives only one of the two bytes, so it changes only the register
   bits in that byte: a write of the low byte sets register bits <5:0>, a
   write of the high byte register bits <7:6> (NetBSD's byte write of CF to
   the mailbox stores register value 33 that way; what the undriven byte
   lane really does is not known). */

void nvr_wr (int32 pa, int32 val, int32 lnt)
{
int32 rg = ((pa - NVRBASE) >> 2) & (NVR_NREGS - 1);
uint32 w = NVR_UNDRIVEN | ((uint32) nvr_reg_rd (rg) << 2);

if (lnt == L_BYTE) {
    if (pa & 1)
        w = (w & 0x00FF) | ((((uint32) val) & 0xFF) << 8);
    else
        w = (w & 0xFF00) | (((uint32) val) & 0xFF);
    }
else
    w = ((uint32) val) & 0xFFFF;
sim_debug (NVR_DB_REG, &nvr_dev, "nvr_wr(reg=%d) = %02X\n", rg, (w >> 2) & 0xFF);
nvr_reg_wr (rg, (int32) ((w >> 2) & 0xFF));
}

/* NVR examine */

t_stat nvr_ex (t_value *vptr, t_addr exta, UNIT *uptr, int32 sw)
{
uint32 addr = (uint32) exta;

if ((vptr == NULL) || (addr & 03))
    return SCPE_ARG;
if (addr >= NVRSIZE)
    return SCPE_NXM;
*vptr = nvr[addr >> 2];
return SCPE_OK;
}

/* NVR deposit */

t_stat nvr_dep (t_value val, t_addr exta, UNIT *uptr, int32 sw)
{
uint32 addr = (uint32) exta;

if (addr & 03)
    return SCPE_ARG;
if (addr >= NVRSIZE)
    return SCPE_NXM;
nvr[addr >> 2] = (uint32) val;
return SCPE_OK;
}

/* NVR reset */

t_stat nvr_reset (DEVICE *dptr)
{
if (nvr == NULL) {
    nvr = (uint32 *) calloc (NVRSIZE >> 2, sizeof (uint32));
    nvr_unit.filebuf = nvr;
    }
if (nvr == NULL)
    return SCPE_MEM;
return SCPE_OK;
}

t_stat nvr_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
fprintf (st, "Non-volatile Memory (NVR)\n\n");
fprintf (st, "The NVR simulates the 50 bytes of battery-backed RAM in the DS1287A clock\n");
fprintf (st, "chip (registers 14 to 63; registers 0 to 13 are the clock, see WTC).  The\n");
fprintf (st, "console keeps its settings there (SET HALT, SET BOOT, SET MOP and the\n");
fprintf (st, "others).  When the simulator starts, the RAM is cleared and the battery is\n");
fprintf (st, "flat, so the console reports an NVR error and writes its defaults.  The\n");
fprintf (st, "NVR can instead be attached to a file, which preserves the settings across\n");
fprintf (st, "simulator runs:\n\n");
fprintf (st, "   sim> ATTACH NVR ka49.nvr\n\n");
fprintf (st, "A new or empty file starts with the values the console would write itself\n");
fprintf (st, "(HALT 2, MOP 1, SCSI host ID 6, the validity signature), so a fresh\n");
fprintf (st, "machine powers up without an NVR error.\n");
return SCPE_OK;
}

/* NVR attach: a file without the console's signature (a new file) gets
   the console's defaults, and the battery is good while a file is
   attached. */

t_stat nvr_attach (UNIT *uptr, CONST char *cptr)
{
t_stat r;
int32 i;
t_bool fresh = TRUE;

uptr->flags = uptr->flags | (UNIT_ATTABLE | UNIT_BUFABLE);
r = attach_unit (uptr, cptr);
if (r != SCPE_OK)
    uptr->flags = uptr->flags & ~(UNIT_ATTABLE | UNIT_BUFABLE);
else {
    uptr->hwmark = (uint32) uptr->capac;
    for (i = NVR_CLKREGS; i < NVR_NREGS; i++) {
        if (nvr[i] != 0)
            fresh = FALSE;
        }
    if (fresh) {
        for (i = 0; nvr_defaults[i].reg >= 0; i++)
            nvr[nvr_defaults[i].reg] = nvr_defaults[i].val;
        }
    wtc_set_valid ();
    }
return r;
}

/* NVR detach */

t_stat nvr_detach (UNIT *uptr)
{
t_stat r;

r = detach_unit (uptr);
if ((uptr->flags & UNIT_ATT) == 0) {
    uptr->flags = uptr->flags & ~(UNIT_ATTABLE | UNIT_BUFABLE);
    wtc_set_invalid ();
    }
return r;
}

const char *nvr_description (DEVICE *dptr)
{
return "non-volatile memory";
}

/* Programmable timer

   The architected VAX timer, which increments at 1Mhz, cannot be
   accurately simulated due to the overhead that would be required
   for 1M clock events per second.  Instead 1Mhz intervals are
   derived from the calibrated instruction execution rate.

   If the interval register is read, then its value between events
   is interpolated relative to the elapsed instruction count.

   On the KA49 the same timer answers MTPR/MFPR of ICCS, NICR and ICR and
   the longwords at 2100.0060, 2100.0064 and 2100.0068 (vax49_sysdev.c).
*/

int32 iccs_rd (void)
{
sim_debug_bits_hdr (TMR_DB_REG, &tmr_dev, "iccs_rd()", tmr_iccs_bits, tmr_iccs, tmr_iccs, TRUE);
return tmr_iccs & TMR_CSR_RD;
}

void iccs_wr (int32 val)
{
sim_debug_bits_hdr (TMR_DB_REG, &tmr_dev, "iccs_wr()", tmr_iccs_bits, tmr_iccs, val, TRUE);
if ((val & TMR_CSR_RUN) == 0) {                         /* clearing run? */
    if (tmr_iccs & TMR_CSR_RUN) {                       /* run 1 -> 0? */
        tmr_icr = icr_rd ();                            /* update icr */
        sim_debug (TMR_DB_REG, &tmr_dev, "iccs_wr() - stopped clock with remaining ICR=0x%08X\n", tmr_icr);
        }
    sim_cancel (&tmr_unit);                             /* cancel timer */
    }
if ((tmr_iccs & CSR_DONE) && (val & CSR_DONE) &&        /* Interrupt Acked? */
    (10000 == (tmr_nicr ? (~tmr_nicr + 1) : 0xFFFFFFFF)))/* of 10ms tick */
    sim_rtcn_tick_ack (20, TMR_CLK);                    /* Let timers know */
tmr_iccs = tmr_iccs & ~(val & TMR_CSR_W1C);             /* W1C csr */
tmr_iccs = (tmr_iccs & ~TMR_CSR_WR) |                   /* new r/w */
    (val & TMR_CSR_WR);
if (val & TMR_CSR_XFR)                                  /* xfr set? */
    tmr_icr = tmr_nicr;
if (val & TMR_CSR_RUN)  {                               /* run? */
    if (val & TMR_CSR_XFR)                              /* new tir? */
        sim_cancel (&tmr_unit);                         /* stop prev */
    if (!sim_is_active (&tmr_unit))                     /* not running? */
        tmr_sched (tmr_icr);                            /* activate */
    }
else {
    if (val & TMR_CSR_XFR)                              /* xfr set? */
        tmr_icr = tmr_nicr;
    if (val & TMR_CSR_SGL) {                            /* single step? */
        tmr_icr = tmr_icr + 1;                          /* incr tmr */
        if (tmr_icr == 0) {                             /* if ovflo, */
            if (tmr_iccs & TMR_CSR_DON)                 /* done? set err */
                tmr_iccs = tmr_iccs | TMR_CSR_ERR;
            else
                tmr_iccs = tmr_iccs | TMR_CSR_DON;      /* set done */
            if (tmr_iccs & TMR_CSR_IE) {                /* ie? */
                tmr_int = 1;                            /* set int req */
                sim_debug (TMR_DB_INT, &tmr_dev, "tmr_incr() - INT=1\n");
                }
            tmr_icr = tmr_nicr;                         /* reload tir */
            }
        }
    }
if ((tmr_iccs & (TMR_CSR_DON | TMR_CSR_IE)) !=          /* update int */
    (TMR_CSR_DON | TMR_CSR_IE)) {
    if (tmr_int) {
        tmr_int = 0;
        sim_debug (TMR_DB_INT, &tmr_dev, "iccs_wr() - INT=0\n");
        }
    }
SET_IRQL;
}

int32 icr_rd (void)
{
int32 result;

if (tmr_iccs & TMR_CSR_RUN) {                           /* running? */
    uint32 usecs_remaining = (uint32)sim_activate_time_usecs (&tmr_unit);

    result = (int32)(~usecs_remaining + 1);
    }
else
    result = (int32)tmr_icr;
sim_debug (TMR_DB_REG, &tmr_dev, "icr_rd() = 0x%08X%s\n", result, (tmr_iccs & TMR_CSR_RUN) ? " - interpolated" : "");
return result;
}

int32 nicr_rd (void)
{
sim_debug (TMR_DB_REG, &tmr_dev, "nicr_rd() = 0x%08X\n", tmr_nicr);
return tmr_nicr;
}

void nicr_wr (int32 val)
{
sim_debug (TMR_DB_REG, &tmr_dev, "nicr_wr(0x%08X)\n", val);
tmr_nicr = val;
}

/* Interval timer unit service */

t_stat tmr_svc (UNIT *uptr)
{
sim_debug (TMR_DB_TICK, &tmr_dev, "tmr_svc()\n");
tmxr_poll = tmr_poll * TMXR_MULT;                       /* set mux poll */
if (tmr_iccs & TMR_CSR_DON)                             /* done? set err */
    tmr_iccs = tmr_iccs | TMR_CSR_ERR;
else
    tmr_iccs = tmr_iccs | TMR_CSR_DON;                  /* set done */
if (tmr_iccs & TMR_CSR_RUN)                             /* run? */
    tmr_sched (tmr_nicr);                               /* reactivate */
if (tmr_iccs & TMR_CSR_IE) {                            /* ie? set int req */
    tmr_int = 1;
    sim_debug (TMR_DB_INT, &tmr_dev, "tmr_svc() - INT=1\n");
    }
else
    tmr_int = 0;
AIO_SET_INTERRUPT_LATENCY(tmr_poll*clk_tps);            /* set interrrupt latency */
return SCPE_OK;
}

/* Timer scheduling */

void tmr_sched (uint32 nicr)
{
double usecs = (nicr) ? (double)(~nicr + 1) : (double)0x100000000LL;

sim_debug (TMR_DB_SCHED, &tmr_dev, "tmr_sched(nicr=0x%08X-usecs=%.0f) - tps=%d\n", nicr, usecs, clk_tps);
if (usecs == 10000.0)
    sim_clock_coschedule_tmr (&tmr_unit, TMR_CLK, 1);
else
    sim_activate_after_d (&tmr_unit, usecs);
}

/* Interval timer reset */

t_stat tmr_reset (DEVICE *dptr)
{
tmr_iccs = 0;
tmr_icr = 0;
tmr_nicr = 0;
tmr_int = 0;
sim_cancel (&tmr_unit);                                 /* cancel timer */
return SCPE_OK;
}

t_stat tmr_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
fprintf (st, "Interval Timer (%s)\n\n", dptr->name);
fprintf (st, "The NVAX interval timer is the full VAX timer: ICCS, NICR and ICR count\n");
fprintf (st, "microseconds and interrupt at IPL 16 through SCB vector C0.  On the KA49\n");
fprintf (st, "the console reaches it both as processor registers and as the longwords at\n");
fprintf (st, "2100.0060 (ICCS), 2100.0064 (NICR) and 2100.0068 (ICR).  Microseconds are\n");
fprintf (st, "derived from the calibrated instruction rate.\n");
return SCPE_OK;
}

const char *tmr_description (DEVICE *dptr)
{
return "interval timer";
}

/* 100Hz calibration clock */

t_stat clk_svc (UNIT *uptr)
{
int32 t;

t = sim_rtcn_calb (clk_tps, TMR_CLK);                   /* calibrate clock */
sim_activate_after (uptr, 1000000/clk_tps);             /* reactivate unit */
tmr_poll = t;                                           /* set tmr poll */
tmxr_poll = t * TMXR_MULT;                              /* set mux poll */
AIO_SET_INTERRUPT_LATENCY(tmr_poll*clk_tps);            /* set interrrupt latency */
return SCPE_OK;
}

/* Reset routine */

t_stat clk_reset (DEVICE *dptr)
{
int32 t;

t = sim_rtcn_init_unit (&clk_unit, clk_unit.wait, TMR_CLK);/* init 100Hz timer */
sim_activate_after (&clk_unit, 1000000/clk_tps);        /* activate 100Hz unit */
tmr_poll = t;                                           /* set tmr poll */
tmxr_poll = t * TMXR_MULT;                              /* set mux poll */
return SCPE_OK;
}

const char *clk_description (DEVICE *dptr)
{
return "100hz calibration clock";
}

/* Dummy I/O space functions */

int32 ReadIO (uint32 pa, int32 lnt)
{
return 0;
}

void WriteIO (uint32 pa, int32 val, int32 lnt)
{
return;
}

int32 ReadIOU (uint32 pa, int32 lnt)
{
return 0;
}

void WriteIOU (uint32 pa, int32 val, int32 lnt)
{
return;
}
