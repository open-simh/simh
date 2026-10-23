/* vax_tca.c: TURBOchannel adapter (TCA) of the VAXstation 4000 Model 90 and 96


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

   The TCA connects one TURBOchannel option module to the system.  The
   VAXstation 4000 Model 96 has one; the service manual shows the adapter
   installed in a Model 90 as well.  The console finds it from the
   configuration register's bit 0 and then does exactly two things with it:

   1. It reads an option ROM at 3600.0000 with the generic routine that it
      also uses for the LCSPX, SPXg/gt and DSW21 options.  The ROM must
      carry DEC's option-ROM header: at a longword stride, byte lane i of
      the longwords

        0       the width code w: 1, 2 or 4 byte lanes carry data
        3       the lane number i
        4, 5, 6 55, AA, 33
        7       N: the ROM is N x 4 KB of address (N x 1024 longwords)
        last    the lane's checksum

      The checksum of a lane starts at zero; for each of the first
      N x 1024 - 1 longwords it rotates the 8-bit sum left one place and
      adds the lane's byte.  A header that fails these tests makes the
      console report the device failed ("?? 048 13 UNK 0014" at power-up
      for a bad width code, the answer in decimal for the others: 0256 width,
      0512 checksum, 0768 patterns, 1024 lane number) and stop there.  A header that passes has a device block (name,
      script entries) that the console copies, and the option's own
      self-test, T/UT utility and console driver run from the ROM.  All of
      that is DEC's code in the ROM: the TCA register self-test (CSR reset,
      timeout, FIFO, interrupt, DMA triggers, the probe of TURBOchannel
      slot 0) and the "OPT PRS / NO OPT PRES" text are not in the console
      ROM.  So the model cannot make the console print "13 TCA OK" without
      a dump of the adapter's option ROM, which is not built in.
   2. Whatever the ROM's test returned, it writes 100 to 3680.0000, waits
      one millisecond and writes 0: a reset pulse to, most probably, the
      adapter's CSR (bit 8; EK-KA490-SV Table 5-7 lists RESET among the
      CSR's functions).  The console never reads the register.

   The adapter's other parts are the ones that option ROM's self-test uses
   (EK-KA490-SV Table 5-7 and A-20):

   - the CSR at 3680.0000: bit 8 RESET, 11 TURBOchannel timeout, 12
     interrupt enable, 13 set by 11 with 12 (VMS: spurious), 9 map enable, 10 and 14
     write-one-to-clear errors (10 is INV, an invalid map reference), 15 and
     16 and the low byte plain storage, 17 FIFO not empty;
   - a 512-longword FIFO behind 3500.0000, and the two DMA triggers at
     3400.0000, a read and a write: the first FIFO longword is the DMA
     address, a read trigger fills the FIFO with 128 longwords (one 512-byte
     page) of memory, a write trigger stores the longwords then in the FIFO;
   - 512 KB of map RAM at 3580.0000: bit 31 valid, the low bits a 512-byte
     page frame number, one entry per 512 bytes of TURBOchannel address;
   - slot 0 of the TURBOchannel at 3000.0000: a read that nothing answers
     (no module, or RESET asserted) sets CSR bit 11 and reads zero;
   - the interrupt: INT_REQ bit 0 at IPL 15, vector 0144, a level that is up
     while CSR bit 12 is set and bit 11, an error bit or the module asks.

   Not known, and chosen to make the real ROM's tests pass (marked "guess"
   where used): how the diagnostic DMA address word maps to memory (the
   test's 0x10000 reaches 0x2000; the model shifts right by 3), what events
   set CSR bits 10 and 14 (none here), what a read of 3400.0000 returns
   (zero), what a read of an empty FIFO returns (zero), and bit 9's meaning
   (map enable, from the manual's "INV by doing an invalid map DMA").

   The module in slot 0 (the 4000/96's PMAZ-AA, PKB0) is not part of this
   file; SET TCA MODULE says that one is fitted, which only changes whether
   a read of slot 0 times out: without one the console prints
   "NO OPT PRES v1.0" for the adapter, with one "OPT PRS    V1.0".

   Without a file attached the adapter is absent: the configuration
   register's bit 0 is clear, the console does not look for it, and the
   windows machine-check on a read.  An empty file is an adapter whose ROM
   window reads zero, which the console rejects (width code 0): the way to
   see the failure report.

   Attach the ROM before BOOT CPU: the configuration register is set at
   reset. */

#include "vax_defs.h"
#include "vax_tca.h"

#if !defined (TCA_BASE) || !defined (TCA_CSR) || !defined (TCA_MAP) || !defined (TCA_SET_INT)
#error "the model must define TCA_BASE, TCA_CSR, TCA_MAP and TCA_SET_INT, the adapter's windows and its request"
#endif

#define DBG_ROM         0x0001                          /* option ROM reads */
#define DBG_REG         0x0002                          /* the CSR */
#define DBG_FIFO        0x0004                          /* FIFO traffic and DMA triggers */
#define DBG_SLOT        0x0008                          /* slot 0 accesses */
#define DBG_INT         0x0010                          /* the interrupt line */

/* CSR bits (3680.0000) */

#define CSR_RW          0x00019BFF                      /* stored as written: <9:0>, <11>, <12>, <15>, <16> */
#define CSR_RESET       0x00000100                      /* <8> */
#define CSR_MAPEN       0x00000200                      /* <9> guess: map enable */
#define CSR_INV         0x00000400                      /* <10> INV: invalid map reference, write one to clear */
#define CSR_TMO         0x00000800                      /* <11> TURBOchannel timeout; writable */
#define CSR_INTEN       0x00001000                      /* <12> interrupt enable */
#define CSR_INT         0x00002000                      /* <13> read only: interrupt forced by <11> with <12>
                                                           set; VMS treats it as spurious */
#define CSR_ERR14       0x00004000                      /* <14> error (cause not known), write one to clear */
#define CSR_FIFONE      0x00020000                      /* <17> FIFO not empty (read only) */

#define TCA_FIFOLEN     512                             /* FIFO depth in longwords */
#define TCA_DMALEN      128                             /* a read trigger moves one 512-byte page */
#define TCA_MAPLEN      0x20000                         /* map RAM longwords (512 KB at 3580.0000) */
#define TCA_MAPENTRIES  8192                            /* entries the map uses (32 KB, 4 MB of address) */
#define MAP_VALID       0x80000000u                     /* map entry: valid, low bits = 512-byte PFN */

static uint32 *tca_rom = NULL;                          /* the dump, one entry per longword */
static uint32 tca_romlen = 0;                           /* longwords in it */
static t_bool tca_zero = FALSE;                         /* an empty file: the window reads zero */
static uint32 tca_csr = 0;                              /* CSR bits that are stored (CSR_RW) */
static uint32 tca_err = 0;                              /* CSR_INV, CSR_ERR14 as latched */
static uint32 tca_csr_last = 0;                         /* the last value written to the CSR */
static uint32 tca_csr_writes = 0;                       /* number of writes to it */
static uint32 tca_fifo[TCA_FIFOLEN];                    /* the FIFO behind 3500.0000 */
static uint32 tca_fifo_rp = 0, tca_fifo_wp = 0, tca_fifo_n = 0;
static uint32 *tca_map = NULL;                          /* map RAM, 3580.0000 */
static t_bool tca_module = FALSE;                       /* a module is fitted in slot 0 */
static t_bool tca_option_irq = FALSE;                   /* the module asks for an interrupt */
static t_bool tca_int = FALSE;                          /* the request as last given to the CPU */

t_stat tca_reset (DEVICE *dptr);
t_stat tca_attach (UNIT *uptr, CONST char *cptr);
t_stat tca_detach (UNIT *uptr);
t_stat tca_show_rom (FILE *st, UNIT *uptr, int32 val, CONST void *desc);
t_stat tca_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr);
const char *tca_description (DEVICE *dptr);
t_stat tca_set_module (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat tca_show_module (FILE *st, UNIT *uptr, int32 val, CONST void *desc);

/* TCA data structures

   tca_dev      TCA device descriptor
   tca_unit     TCA unit: the option ROM dump
   tca_reg      TCA register list
*/

UNIT tca_unit = { UDATA (NULL, UNIT_ATTABLE + UNIT_RO, 0) };

MTAB tca_mod[] = {
    { MTAB_XTD|MTAB_VDV, 0, "ROM", NULL, NULL, &tca_show_rom, NULL, "Show which option ROM dump is in use" },
    { MTAB_XTD|MTAB_VDV, 1, "MODULE", "MODULE", &tca_set_module, &tca_show_module, NULL,
      "A TURBOchannel module is fitted in slot 0 (a read of slot 0 does not time out)" },
    { MTAB_XTD|MTAB_VDV, 0, NULL, "NOMODULE", &tca_set_module, NULL, NULL,
      "No module in slot 0 (a read of slot 0 times out: NO OPT PRES)" },
    { 0 }
    };

REG tca_reg[] = {
    { HRDATAD (CSRLAST, tca_csr_last, 32, "last value written to the CSR (3680.0000)") },
    { DRDATAD (CSRWRITES, tca_csr_writes, 32, "writes to the CSR since reset") },
    { HRDATAD (CSR, tca_csr, 32, "CSR bits that are stored") },
    { HRDATAD (ERR, tca_err, 32, "CSR error bits <10> (INV) and <14> as latched") },
    { DRDATAD (FIFON, tca_fifo_n, 10, "longwords in the FIFO") },
    { FLDATAD (MODULE, tca_module, 0, "a module is fitted in slot 0") },
    { FLDATAD (OPTIRQ, tca_option_irq, 0, "the module in slot 0 asks for an interrupt") },
    { FLDATAD (INT, tca_int, 0, "the interrupt request to the CPU (INT_REQ<0>)") },
    { NULL }
    };

DEBTAB tca_debug[] = {
    { "ROM", DBG_ROM, "option ROM reads" },
    { "REG", DBG_REG, "CSR accesses" },
    { "FIFO", DBG_FIFO, "FIFO traffic and DMA triggers" },
    { "SLOT", DBG_SLOT, "slot 0 accesses" },
    { "INT", DBG_INT, "the interrupt line" },
    { NULL, 0 }
    };

DEVICE tca_dev = {
    "TCA", &tca_unit, tca_reg, tca_mod,
    1, 16, 32, 4, 16, 32,
    NULL, NULL, &tca_reset,
    NULL, &tca_attach, &tca_detach,
    NULL, DEV_DEBUG, 0, tca_debug, NULL, NULL, &tca_help,
    NULL, NULL, &tca_description
    };

/* The adapter is present when an option ROM dump (or an empty file) is attached */

t_bool tca_present (void)
{
return (tca_unit.flags & UNIT_ATT)? TRUE: FALSE;
}

/* The option ROM window: the longword of the dump, a machine check past its end */

int32 tca_rd (int32 pa)
{
uint32 off = ((uint32) pa) - TCA_BASE;

if (!tca_present () || (!tca_zero && ((off >> 2) >= tca_romlen))) {
    sim_debug (DBG_ROM, &tca_dev, "ROM read %05X: nothing answers, PC %08X\n", off, fault_PC);
    MACH_CHECK (MCHK_READ);
    }
if (tca_zero)
    return 0;
sim_debug (DBG_ROM, &tca_dev, "ROM read %05X = %08X\n", off, tca_rom[off >> 2]);
return (int32) tca_rom[off >> 2];
}

/* The ROM ignores writes */

void tca_wr (int32 pa, int32 val, int32 lnt)
{
return;
}

/* The interrupt, a level, is INT_REQ<0> at IPL 15.  It is asserted when the
   interrupt enable (bit 12) is set and there is a cause: the timeout (bit 11,
   which the self-test can force by writing it), an error bit, or the
   module's own request.  The CSR's bit 13 reads 1 when bits 12 and 11 are
   both set (the self-test needs that); VMS's interrupt routine treats an
   interrupt with bit 13 set as spurious and hands one with it clear to the
   option's handler, so an interrupt from the module must not set it.  Guess
   for the causes beyond bit 11, which is all that the console's self-test
   shows. */

static t_bool tca_csr_int (void)
{
if ((tca_csr & CSR_INTEN) &&
    ((tca_csr & CSR_TMO) || tca_err || tca_option_irq))
    return TRUE;
return FALSE;
}

static void tca_update_int (void)
{
t_bool i = tca_csr_int ();

if (i != tca_int) {
    tca_int = i;
    sim_debug (DBG_INT, &tca_dev, "interrupt request %d\n", i);
    TCA_SET_INT (i);
    }
}

/* The module's request, called from its controller (not yet modelled) */

void tca_module_irq (t_bool level)
{
tca_option_irq = level;
tca_update_int ();
}

/* The CSR (3680.0000), a longword.  Bits 14 and 10 are cleared by writing
   ones; the others that can be written are stored; 13 and 17 are outputs. */

static uint32 tca_csr_value (void)
{
uint32 v = tca_csr | tca_err;

if ((tca_csr & CSR_INTEN) && (tca_csr & CSR_TMO))
    v |= CSR_INT;
if (tca_fifo_n)
    v |= CSR_FIFONE;
return v;
}

int32 tca_csr_rd (int32 pa)
{
uint32 v;

if (!tca_present ()) {
    sim_debug (DBG_REG, &tca_dev, "CSR read: no adapter, PC %08X\n", fault_PC);
    MACH_CHECK (MCHK_READ);
    }
v = tca_csr_value ();
sim_debug (DBG_REG, &tca_dev, "CSR read %08X, PC %08X\n", v, fault_PC);
return (int32) v;
}

void tca_csr_wr (int32 pa, int32 val, int32 lnt)
{
uint32 v = (uint32) val;

if (!tca_present ())
    return;
tca_csr_last = v;
tca_csr_writes++;
sim_debug (DBG_REG, &tca_dev, "CSR write %08X (%d), PC %08X\n", v, lnt, fault_PC);
tca_err &= ~(v & (CSR_INV | CSR_ERR14));
tca_csr = v & CSR_RW;
tca_update_int ();
return;
}

/* The FIFO behind 3500.0000.  An empty FIFO reads zero (guess); a write to a
   full one is lost. */

static void tca_fifo_put (uint32 v)
{
if (tca_fifo_n >= TCA_FIFOLEN)
    return;
tca_fifo[tca_fifo_wp] = v;
tca_fifo_wp = (tca_fifo_wp + 1) % TCA_FIFOLEN;
tca_fifo_n++;
}

static uint32 tca_fifo_get (void)
{
uint32 v;

if (tca_fifo_n == 0)
    return 0;
v = tca_fifo[tca_fifo_rp];
tca_fifo_rp = (tca_fifo_rp + 1) % TCA_FIFOLEN;
tca_fifo_n--;
return v;
}

int32 tca_fifo_rd (int32 pa)
{
uint32 v;

if (!tca_present ())
    MACH_CHECK (MCHK_READ);
v = tca_fifo_get ();
sim_debug (DBG_FIFO, &tca_dev, "FIFO read %08X (%u left), PC %08X\n", v, tca_fifo_n, fault_PC);
return (int32) v;
}

void tca_fifo_wr (int32 pa, int32 val, int32 lnt)
{
if (!tca_present ())
    return;
tca_fifo_put ((uint32) val);
sim_debug (DBG_FIFO, &tca_dev, "FIFO write %08X (%u in), PC %08X\n", (uint32) val, tca_fifo_n, fault_PC);
}

/* A DMA address word from the FIFO to a physical address.  With the map
   enable clear (the self-test's case) the word is shifted right by 3: guess,
   the test's 0x10000 reaches physical 0x2000.  With it set, the word is a
   TURBOchannel address and the map RAM, one entry per 512 bytes, gives the
   page; an invalid entry sets INV (bit 10) and the transfer does not happen.
   Returns FALSE on INV. */

static t_bool tca_dma_addr (uint32 w, uint32 *pa)
{
if (tca_csr & CSR_MAPEN) {
    uint32 ix = w >> 9;

    if ((ix >= TCA_MAPENTRIES) || ((tca_map[ix] & MAP_VALID) == 0)) {
        tca_err |= CSR_INV;
        sim_debug (DBG_FIFO, &tca_dev, "DMA %08X: invalid map entry %u\n", w, ix);
        return FALSE;
        }
    *pa = ((tca_map[ix] & ~MAP_VALID) << 9) | (w & 0x1FF);
    return TRUE;
    }
*pa = w >> 3;
return TRUE;
}

/* The triggers at 3400.0000.  A read takes the FIFO's first longword as the
   DMA address and replaces the FIFO's contents with 128 longwords from there,
   one page; a write takes the address and stores the rest of the FIFO there.
   What a read of the trigger itself returns is not known: zero. */

int32 tca_trig_rd (int32 pa)
{
uint32 w, a, i;

if (!tca_present ())
    MACH_CHECK (MCHK_READ);
w = tca_fifo_get ();
if (tca_dma_addr (w, &a)) {
    sim_debug (DBG_FIFO, &tca_dev, "read trigger: %u longwords from %08X\n", TCA_DMALEN, a);
    tca_fifo_rp = tca_fifo_wp = tca_fifo_n = 0;
    for (i = 0; i < TCA_DMALEN; i++, a += 4)
        tca_fifo_put ((a < MEMSIZE)? M[a >> 2]: 0);
    }
return 0;
}

void tca_trig_wr (int32 pa, int32 val, int32 lnt)
{
uint32 w, a;

if (!tca_present ())
    return;
w = tca_fifo_get ();
if (tca_dma_addr (w, &a)) {
    sim_debug (DBG_FIFO, &tca_dev, "write trigger: %u longwords to %08X\n", tca_fifo_n, a);
    while (tca_fifo_n) {
        uint32 d = tca_fifo_get ();

        if (a < MEMSIZE)
            M[a >> 2] = d;
        a += 4;
        }
    }
}

/* The map RAM at 3580.0000 */

int32 tca_map_rd (int32 pa)
{
uint32 ix = (((uint32) pa) - TCA_MAP) >> 2;

if (!tca_present ())
    MACH_CHECK (MCHK_READ);
return (int32) tca_map[ix];
}

void tca_map_wr (int32 pa, int32 val, int32 lnt)
{
uint32 ix = (((uint32) pa) - TCA_MAP) >> 2;

if (!tca_present ())
    return;
tca_map[ix] = (uint32) val;
}

/* Slot 0 of the TURBOchannel.  With no module fitted, or the adapter's
   RESET asserted, nothing answers: the adapter ends the cycle with the
   timeout flag (bit 11) and the read gives zero, without a machine check.
   A module (the 4000/96's PMAZ-AA) is not modelled yet: it answers zero. */

int32 tca_slot_rd (int32 pa)
{
if (!tca_present ())
    MACH_CHECK (MCHK_READ);
if (!tca_module || (tca_csr & CSR_RESET)) {
    sim_debug (DBG_SLOT, &tca_dev, "slot read %08X: timeout, PC %08X\n", (uint32) pa, fault_PC);
    tca_csr |= CSR_TMO;
    tca_update_int ();
    return 0;
    }
sim_debug (DBG_SLOT, &tca_dev, "slot read %08X, PC %08X\n", (uint32) pa, fault_PC);
return 0;
}

void tca_slot_wr (int32 pa, int32 val, int32 lnt)
{
if (!tca_present ())
    return;
if (!tca_module || (tca_csr & CSR_RESET)) {
    sim_debug (DBG_SLOT, &tca_dev, "slot write %08X: timeout, PC %08X\n", (uint32) pa, fault_PC);
    tca_csr |= CSR_TMO;
    tca_update_int ();
    }
}

t_stat tca_reset (DEVICE *dptr)
{
tca_csr_last = 0;
tca_csr_writes = 0;
tca_csr = 0;
tca_err = 0;
tca_fifo_rp = tca_fifo_wp = tca_fifo_n = 0;
tca_option_irq = FALSE;
if (tca_map == NULL)
    tca_map = (uint32 *) calloc (TCA_MAPLEN, sizeof (uint32));
if (tca_map == NULL)
    return SCPE_MEM;
memset (tca_map, 0, TCA_MAPLEN * sizeof (uint32));
tca_int = FALSE;
TCA_SET_INT (FALSE);
return SCPE_OK;
}

t_stat tca_set_module (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
tca_module = val? TRUE: FALSE;
return SCPE_OK;
}

t_stat tca_show_module (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
fprintf (st, tca_module? "a module is fitted in slot 0": "no module in slot 0");
return SCPE_OK;
}

/* The console's tests of an option ROM header (system ROM 2004E083), applied
   to a dump: 1 for a header the console accepts; 0100 a bad width code, 0200
   a bad checksum, 0300 bad test patterns, 0400 a bad lane number.  A dump
   shorter than the header says is accepted without its checksum being
   tested (*short set). */

static uint32 tca_check (const uint32 *rom, uint32 n, t_bool *tshort)
{
uint32 w, i, k, need, sum;

*tshort = FALSE;
if (n < 8)
    return 0x0100;
w = rom[0] & 0xFF;                                      /* lane 0 gives the width */
if ((w != 1) && (w != 2) && (w != 4))
    return 0x0100;
need = (rom[7] & 0xFF) * 1024;
for (i = 0; i < w; i++) {                               /* the lanes in turn, as the console does */
    uint32 sh = 8 * i;

    switch ((rom[0] >> sh) & 0xFF) {                    /* each lane's width byte */
        case 1: case 2: case 4:
            break;
        default:
            return 0x0100;
        }
    if (((rom[3] >> sh) & 0xFF) != i)
        return 0x0400;
    if ((((rom[4] >> sh) & 0xFF) != 0x55) ||
        (((rom[5] >> sh) & 0xFF) != 0xAA) ||
        (((rom[6] >> sh) & 0xFF) != 0x33))
        return 0x0300;
    if (n < need) {
        *tshort = TRUE;
        continue;
        }
    sum = 0;
    for (k = 0; k + 1 < need; k++) {
        sum = ((sum << 1) | (sum >> 7)) & 0xFF;
        sum = (sum + (rom[k] >> sh)) & 0xFF;
        }
    if (need && (sum != ((rom[need - 1] >> sh) & 0xFF)))
        return 0x0200;
    }
return 1;
}

/* Attach an option ROM dump: the longwords as the CPU read them at 3600.0000.
   An empty file is an adapter whose window reads zero. */

t_stat tca_attach (UNIT *uptr, CONST char *cptr)
{
FILE *f;
uint8 *buf;
long len;
uint32 i, n, answer;
t_bool tshort;
t_stat r;

f = sim_fopen (cptr, "rb");
if (f == NULL)
    return sim_messagef (SCPE_OPENERR, "%s: cannot open %s\n", sim_dname (&tca_dev), cptr);
len = (long) sim_fsize (f);
if ((len & 3) || (len > TCA_WINSIZE)) {
    fclose (f);
    return sim_messagef (SCPE_ARG, "%s: %s is %ld bytes; the option ROM dump is a whole number of "
        "longwords, at most %d bytes\n", sim_dname (&tca_dev), cptr, len, TCA_WINSIZE);
    }
buf = (uint8 *) malloc ((size_t) len + 4);
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
free (tca_rom);
tca_rom = NULL;
tca_romlen = n = (uint32) (len >> 2);
tca_zero = (n == 0)? TRUE: FALSE;
if (!tca_zero) {
    tca_rom = (uint32 *) malloc (n * sizeof (uint32));
    if (tca_rom == NULL) {
        free (buf);
        detach_unit (uptr);
        return SCPE_MEM;
        }
    for (i = 0; i < n; i++)                             /* little-endian longwords */
        tca_rom[i] = (uint32) buf[4 * i] | ((uint32) buf[4 * i + 1] << 8) |
                     ((uint32) buf[4 * i + 2] << 16) | ((uint32) buf[4 * i + 3] << 24);
    answer = tca_check (tca_rom, n, &tshort);
    if (answer != 1)
        sim_messagef (SCPE_OK, "%s: %s does not pass the console's header test (answer %04X): "
            "the console will report the adapter failed\n", sim_dname (&tca_dev), cptr, answer);
    else if (tshort)
        sim_messagef (SCPE_OK, "%s: %s is shorter than its header says; the console's checksum "
            "will read past it (a machine check)\n", sim_dname (&tca_dev), cptr);
    }
else sim_messagef (SCPE_OK, "%s: %s is empty; the option ROM window reads zero and the console "
        "will report the adapter failed\n", sim_dname (&tca_dev), cptr);
free (buf);
return SCPE_OK;
}

/* Detach: the adapter is gone */

t_stat tca_detach (UNIT *uptr)
{
t_stat r = detach_unit (uptr);

if (r != SCPE_OK)
    return r;
free (tca_rom);
tca_rom = NULL;
tca_romlen = 0;
tca_zero = FALSE;
return SCPE_OK;
}

t_stat tca_show_rom (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
if (!tca_present ())
    fprintf (st, "no adapter (no option ROM dump attached)");
else if (tca_zero)
    fprintf (st, "empty file %s: the window reads zero", tca_unit.filename);
else fprintf (st, "option ROM dump %s, %u longwords", tca_unit.filename, tca_romlen);
return SCPE_OK;
}

t_stat tca_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
fprintf (st, "TURBOchannel adapter (%s)\n\n", dptr->name);
fprintf (st, "The VAXstation 4000 Model 96 has a TURBOchannel adapter; the Model 90 can\n");
fprintf (st, "take one.  The console finds it from the configuration register and runs\n");
fprintf (st, "its tests from an option ROM at 3600.0000, which is DEC's firmware and is\n");
fprintf (st, "not built in.  Give the model a dump of that ROM, the longwords as the CPU\n");
fprintf (st, "reads them from 3600.0000 up to the size its header gives, before BOOT CPU:\n\n");
fprintf (st, "   sim> ATTACH TCA <file>\n\n");
fprintf (st, "Without a file there is no adapter.  An empty file is an adapter whose ROM\n");
fprintf (st, "window reads zero, which the console reports as failed.  The adapter's CSR,\n");
fprintf (st, "FIFO, DMA triggers, map RAM and interrupt are modelled to the extent that\n");
fprintf (st, "its own self-test uses them.  SET TCA MODULE says a module is fitted in\n");
fprintf (st, "slot 0 (without one the console prints NO OPT PRES).  SHOW TCA ROM says\n");
fprintf (st, "which dump is in use.\n");
fprint_set_help (st, dptr);
fprint_show_help (st, dptr);
fprint_reg_help (st, dptr);
return SCPE_OK;
}

const char *tca_description (DEVICE *dptr)
{
return "TURBOchannel adapter";
}
