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

   The window therefore serves the ROM exactly as the CPU reads it: the file
   is a raw dump of the longwords at 3600.0000 up to the size the header
   gives (N x 4 KB, N being byte 7 of the header), and any width code works.
   Nothing else of the adapter is modelled: the CSR's bits, the FIFO, the
   interrupt, the DMA path, and the second SCSI controller PKB0 that VMS
   finds on the 4000/96 are open until the option ROM has been read.  A
   read of the CSR is therefore a machine check, like any address that
   nothing answers, and so is a read of the window past the end of the file.

   Without a file attached the adapter is absent: the configuration
   register's bit 0 is clear, the console does not look for it, and the two
   windows machine-check on a read.  An empty file is an adapter whose ROM
   window reads zero, which the console rejects (width code 0): the way to
   see the failure report.

   Attach the ROM before BOOT CPU: the configuration register is set at
   reset. */

#include "vax_defs.h"
#include "vax_tca.h"

#if !defined (TCA_BASE) || !defined (TCA_CSR)
#error "the model must define TCA_BASE and TCA_CSR, the adapter's windows in I/O space"
#endif

#define DBG_ROM         0x0001                          /* option ROM reads */
#define DBG_REG         0x0002                          /* the CSR */

static uint32 *tca_rom = NULL;                          /* the dump, one entry per longword */
static uint32 tca_romlen = 0;                           /* longwords in it */
static t_bool tca_zero = FALSE;                         /* an empty file: the window reads zero */
static uint32 tca_csr_last = 0;                         /* the last value written to the CSR */
static uint32 tca_csr_writes = 0;                       /* number of writes to it */

t_stat tca_reset (DEVICE *dptr);
t_stat tca_attach (UNIT *uptr, CONST char *cptr);
t_stat tca_detach (UNIT *uptr);
t_stat tca_show_rom (FILE *st, UNIT *uptr, int32 val, CONST void *desc);
t_stat tca_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr);
const char *tca_description (DEVICE *dptr);

/* TCA data structures

   tca_dev      TCA device descriptor
   tca_unit     TCA unit: the option ROM dump
   tca_reg      TCA register list
*/

UNIT tca_unit = { UDATA (NULL, UNIT_ATTABLE + UNIT_RO, 0) };

MTAB tca_mod[] = {
    { MTAB_XTD|MTAB_VDV, 0, "ROM", NULL, NULL, &tca_show_rom, NULL, "Show which option ROM dump is in use" },
    { 0 }
    };

REG tca_reg[] = {
    { HRDATAD (CSRLAST, tca_csr_last, 32, "last value written to the CSR (3680.0000)") },
    { DRDATAD (CSRWRITES, tca_csr_writes, 32, "writes to the CSR since reset") },
    { NULL }
    };

DEBTAB tca_debug[] = {
    { "ROM", DBG_ROM, "option ROM reads" },
    { "REG", DBG_REG, "CSR writes" },
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

/* The CSR (3680.0000).  Only writes are known: the console writes 100,
   then, a millisecond later, 0.  What a read returns is not known. */

int32 tca_csr_rd (int32 pa)
{
sim_debug (DBG_REG, &tca_dev, "CSR read: not known, PC %08X\n", fault_PC);
MACH_CHECK (MCHK_READ);
return 0;
}

void tca_csr_wr (int32 pa, int32 val, int32 lnt)
{
if (!tca_present ())
    return;
tca_csr_last = (uint32) val;
tca_csr_writes++;
sim_debug (DBG_REG, &tca_dev, "CSR write %08X (%d), PC %08X\n", val, lnt, fault_PC);
return;
}

t_stat tca_reset (DEVICE *dptr)
{
tca_csr_last = 0;
tca_csr_writes = 0;
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
fprintf (st, "window reads zero, which the console reports as failed.  Only the ROM\n");
fprintf (st, "window and the write of the console's reset pulse are modelled; the\n");
fprintf (st, "adapter's own registers and the second SCSI controller (PKB0 under VMS)\n");
fprintf (st, "are not.  SHOW TCA ROM says which dump is in use.\n");
fprint_set_help (st, dptr);
fprint_show_help (st, dptr);
fprint_reg_help (st, dptr);
return SCPE_OK;
}

const char *tca_description (DEVICE *dptr)
{
return "TURBOchannel adapter";
}
