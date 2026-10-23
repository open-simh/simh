/* vax_pmaz.c: PMAZ-AA SCSI option module in TURBOchannel slot 0 of a VAXstation 4000 Model 96


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

   The 4000/96's second SCSI channel (VMS device PKB0, driver PKRDRIVER) is a
   PMAZ-AA, DEC's TURBOchannel SCSI option, in slot 0 of the TURBOchannel
   adapter (vax_tca.c).  VMS maps the module at the slot's base,
   3000.0000, with the layout that NetBSD's driver for the same module uses:

     +00.0000  the NCR 53C94, registers at a longword stride (RZB, the
               second controller of vax4xx_rz94.c)
     +04.0000  the DMA address register: bit 31 is the direction, <16:0> an
               offset into the module's SRAM; it advances with the data
     +08.0000  128 KB of SRAM, which the host reads and writes and the chip
               moves its data to and from (VMS stages up to 120 KB of a
               transfer there)
     +3C.03E0  the module's TURBOchannel ROM header

   The module has no DMA into main memory: the SCSI data goes between the chip
   and the SRAM, and the host copies it.  So the 53C94 model's board hooks
   (RZ_DMA_READ, RZ_DMA_WRITE, RZ_DMA_DONE in vax49_defs.h) need only the
   SRAM and the address register.  The chip's interrupt is the adapter's
   option request (tca_module_irq).

   The module's own firmware ROM is not available.  What VMS reads of it is
   only the header that names the module ("PMAZ-AA "); it is served here as the
   standard TURBOchannel header (width 1, stride 4, the test bytes 55 00 AA
   FF) with that name: a PLACEHOLDER whose size, revision and firmware-type
   fields are guesses.  Everything else in the module's address space reads
   as zero. */

#include "vax_defs.h"
#include "vax_tca.h"
#include "vax_pmaz.h"

#define PMAZ_DMAR       0x00040000                      /* DMA address register */
#define PMAZ_SRAM       0x00080000                      /* 128 KB of SRAM */
#define PMAZ_SRAMSIZE   0x00020000
#define PMAZ_ROMHDR     0x003C03E0                      /* TURBOchannel ROM header */
#define PMAZ_HDRLEN     0x88                            /* its length */

#define DMAR_DIR        0x80000000u                     /* direction bit */
#define DMAR_OFF        (PMAZ_SRAMSIZE - 1)             /* offset in the SRAM */

static uint32 pmaz_sram[PMAZ_SRAMSIZE / 4];
static uint32 pmaz_dmar = 0;                            /* direction bit and SRAM offset */
static uint8 pmaz_hdr[PMAZ_HDRLEN];                     /* the ROM header bytes */

/* The header, as the TURBOchannel specification lays it out (NetBSD's
   tc_rommap): fields of one byte at longword offsets, except the 16 test
   bytes, which are consecutive.  Values other than the name are guesses. */

static void pmaz_hdr_put (uint32 off, const char *s)
{
while (*s) {
    pmaz_hdr[off] = (uint8) *s++;
    off += 4;
    }
}

void pmaz_reset (void)
{
memset (pmaz_sram, 0, sizeof (pmaz_sram));
pmaz_dmar = 0;
memset (pmaz_hdr, 0, sizeof (pmaz_hdr));
pmaz_hdr[0x00] = 1;                                     /* width: one byte lane */
pmaz_hdr[0x04] = 4;                                     /* stride */
pmaz_hdr[0x08] = 0x20;                                  /* ROM size (guess) */
pmaz_hdr[0x0C] = 0x01;                                  /* slot size (guess) */
pmaz_hdr[0x10] = 0x55;                                  /* test pattern 55 00 AA FF */
pmaz_hdr[0x14] = 0x00;
pmaz_hdr[0x18] = 0xAA;
pmaz_hdr[0x1C] = 0xFF;
pmaz_hdr_put (0x20, "V1.0    ");                         /* revision (guess) */
pmaz_hdr_put (0x40, "DEC     ");                         /* vendor */
pmaz_hdr_put (0x60, "PMAZ-AA ");                        /* module */
pmaz_hdr_put (0x80, "TCOS");                             /* firmware type (guess) */
}

/* Module address space: offset from the slot base */

int32 pmaz_rd (uint32 off)
{
if (off < PMAZ_DMAR)                                    /* the chip */
    return rzb_rd ((int32) off);
if (off < PMAZ_SRAM)                                    /* DMA address register (a page) */
    return (int32) pmaz_dmar;
if (off < PMAZ_SRAM + PMAZ_SRAMSIZE)
    return (int32) pmaz_sram[(off - PMAZ_SRAM) >> 2];
if ((off >= PMAZ_ROMHDR) && (off < PMAZ_ROMHDR + PMAZ_HDRLEN)) {
    uint32 o = (off - PMAZ_ROMHDR) & ~3u;

    return (int32) ((uint32) pmaz_hdr[o] | ((uint32) pmaz_hdr[o + 1] << 8) |
                    ((uint32) pmaz_hdr[o + 2] << 16) | ((uint32) pmaz_hdr[o + 3] << 24));
    }
return 0;
}

void pmaz_wr (uint32 off, int32 val, int32 lnt)
{
if (off < PMAZ_DMAR)
    rzb_wr ((int32) off, val, lnt);
else if (off < PMAZ_SRAM)
    pmaz_dmar = ((uint32) val & DMAR_DIR) | ((uint32) val & DMAR_OFF);
else if (off < PMAZ_SRAM + PMAZ_SRAMSIZE) {
    uint32 *w = &pmaz_sram[(off - PMAZ_SRAM) >> 2];
    uint32 sh = (off & 3) * 8;

    switch (lnt) {
        case L_BYTE:
            *w = (*w & ~(0xFFu << sh)) | (((uint32) val & 0xFF) << sh);
            break;
        case L_WORD:
            *w = (*w & ~(0xFFFFu << sh)) | (((uint32) val & 0xFFFF) << sh);
            break;
        default:
            *w = (uint32) val;
            break;
        }
    }
}

/* The chip's data path: bytes between it and the SRAM, from the address in
   the address register.  Each routine returns the number of bytes not moved
   (the SRAM ends) without moving the address; pmaz_dma_done moves it. */

static uint8 *pmaz_byte (uint32 off)
{
return ((uint8 *) pmaz_sram) + off;
}

int32 pmaz_dma_read (uint32 bc, uint8 *buf)             /* SRAM to the bus */
{
uint32 off = pmaz_dmar & DMAR_OFF;
uint32 n = (off + bc > PMAZ_SRAMSIZE)? PMAZ_SRAMSIZE - off: bc;

memcpy (buf, pmaz_byte (off), n);
return (int32) (bc - n);
}

int32 pmaz_dma_write (uint32 bc, uint8 *buf)            /* the bus to SRAM */
{
uint32 off = pmaz_dmar & DMAR_OFF;
uint32 n = (off + bc > PMAZ_SRAMSIZE)? PMAZ_SRAMSIZE - off: bc;

memcpy (pmaz_byte (off), buf, n);
return (int32) (bc - n);
}

void pmaz_dma_done (uint32 n)
{
pmaz_dmar = (pmaz_dmar & DMAR_DIR) | ((pmaz_dmar + n) & DMAR_OFF);
}
