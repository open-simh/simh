/* vax4xx_rz94.c: NCR 53C94 SCSI controller

   Copyright (c) 2019, Matt Burke

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

   rz           SCSI controller

   The 53C94 sits on a board that supplies its DMA engine and its interrupt
   request; the boards differ in both.  The model's _defs.h describes its
   board with these definitions:

   RZ_SCSI_ID           initiator SCSI ID; that target unit is reserved
   RZ_READB, RZ_WRITEB  memory access for the default DMA engine
   RZ_DMA_ADDR,         (optional) addresses of the board's DMA address and
   RZ_DMA_DIR           direction registers, if they sit in the chip's
                        address window (KA46/47/48): the default DMA engine
                        starts every transfer at the address register
   RZ_DMA_READ(rz,bc,buf)  (optional, all three together) the board's own
   RZ_DMA_WRITE(rz,bc,buf) DMA engine: fetch bc bytes from memory for the
   RZ_DMA_DONE(rz,n)       SCSI bus, store bc bytes from the bus in memory
                        (each returns the number of bytes not transferred,
                        without moving the board's address), and n bytes
                        have crossed the bus by DMA (the board's address
                        counter moves on)
   RZ_INT_LEVEL         (optional) the board's request follows the chip's
                        INT output as a level, so reading the interrupt
                        register removes it; without it the board latches
                        the request when the chip raises it, and the board's
                        acknowledge or interrupt clear removes it
   RZ_ABSENT_UNATTACHED (optional) a target unit with nothing attached that
                        is not a removable-media drive, and the chip's own
                        ID, do not answer selection (the bus has no such
                        device); without it every enabled unit answers
   RZ_CTLRS             (optional) number of controllers, 1 or 2; the second
                        is device RZB with the register routines rzb_rd and
                        rzb_wr.  A board with two chips has to tell them
                        apart in the macros below, which receive the
                        controller (rz->cnum is 0 or 1)
   RZ_SET_INT(rz),      (optional) the board's request for the chip rz is
   RZ_CLR_INT(rz)       raised and removed; the default is INT_REQ<1>, SC
   RZ_INITIATOR_ID(rz)  (optional) initiator SCSI ID of the chip; default
                        RZ_SCSI_ID

   The chip's state is kept per controller (CTLR), so that a board with two
   53C94s can have a second device; this module defines one (RZ), or two.
*/

#include "vax_defs.h"
#include "sim_scsi.h"
#include "vax_rzdev.h"

#if !defined (RZ_CTLRS)
#define RZ_CTLRS        1
#endif
#if !defined (RZ_SET_INT)
#define RZ_SET_INT(rz)  SET_INT (SC)
#define RZ_CLR_INT(rz)  CLR_INT (SC)
#endif
#if !defined (RZ_INITIATOR_ID)
#define RZ_INITIATOR_ID(rz) RZ_SCSI_ID
#endif

#define RZ_NUMDR        8                               /* targets on the bus */
#define RZ_CTLR         (RZ_NUMDR)                      /* unit for the chip's own events */

#define cnum            u4                              /* unit: controller index */

/* command groups */

#define CMD_DISC        0x40                            /* disconnected state group */
#define CMD_TARG        0x20                            /* target state group */
#define CMD_INIT        0x10                            /* initiator state group */

/* status register */

#define STS_INT         0x80                            /* interrupt */
#define STS_GE          0x40                            /* gross error */
#define STS_PE          0x20                            /* parity error */
#define STS_TC          0x10                            /* terminal count */
#define STS_VGC         0x08                            /* valid group code */
#define STS_PH          0x07                            /* SCSI phase */
#define STS_CLR         0x10

/* interrupt register */

#define INT_SCSIRST     0x80                            /* SCSI reset */
#define INT_ILLCMD      0x40                            /* illegal command */
#define INT_DIS         0x20                            /* disconnect */
#define INT_BUSSV       0x10                            /* bus service */
#define INT_FC          0x08                            /* function complete */
#define INT_RSEL        0x04                            /* reselected */
#define INT_SELA        0x02                            /* selected with ATN */
#define INT_SEL         0x01                            /* selected */

/* configuration register 1 */

#define CFG1_SLOW       0x80                            /* slow cable mode */
#define CFG1_SRD        0x40                            /* disable SCSI reset int */
#define CFG1_PTST       0x20                            /* parity test */
#define CFG1_PEN        0x10                            /* parity enable */
#define CFG1_TEST       0x08                            /* chip test */
#define CFG1_MYID       0x07                            /* my bus id */

#define UNIT_V_DTYPE    (SCSI_V_UF + 0)                 /* drive type */
#define UNIT_M_DTYPE    0x1F
#define UNIT_DTYPE      (UNIT_M_DTYPE << UNIT_V_DTYPE)
#define GET_DTYPE(x)    (((x) >> UNIT_V_DTYPE) & UNIT_M_DTYPE)

#define RZ_MAXFR        (1u << 16)                      /* max transfer */

/* The default DMA engine: every transfer starts at the board's DMA address
   register, which the CPU loads before each transfer */

#if !defined (RZ_DMA_READ)
#define RZ_DMA_READ(rz,bc,buf)  RZ_READB ((rz)->dma, (bc), (buf))
#define RZ_DMA_WRITE(rz,bc,buf) RZ_WRITEB ((rz)->dma, (bc), (buf))
#define RZ_DMA_DONE(rz,n)
#endif

typedef struct {
    uint32 cnum;                                        /* controller number */
    uint32 last_cmd;                                    /* last command */
    uint32 txi;                                         /* transfer count */
    uint32 txc;                                         /* transfer counter */
    uint8 cfg1;                                         /* config 1 */
    uint8 cfg2;                                         /* config 2 */
    uint8 cfg3;                                         /* config 3 */
    uint8 intr;                                         /* interrupt */
    uint8 stat;                                         /* status */
    uint32 seq;                                         /* sequence step */
    uint32 dest;                                        /* destination bus ID */
    uint8 fifo[16];
    uint32 fifo_t;
    uint32 fifo_b;
    uint32 fifo_c;
    uint32 dma;                                         /* board DMA address (RZ_DMA_ADDR) */
    uint32 dir;                                         /* board DMA direction (RZ_DMA_DIR) */
    uint8 *buf;                                         /* transfer buffer */
    SCSI_BUS bus;                                       /* SCSI bus state */
    DEVICE *dev;                                        /* the controller's device */
    UNIT *units;                                        /* its units: targets, then the chip's events */
    } CTLR;

/* debugging bitmaps */

#define DBG_REG         0x0001                          /* trace read/write registers */
#define DBG_CMD         0x0002                          /* display commands */
#define DBG_INT         0x0004                          /* display transfer requests */

DEBTAB rz_debug[] = {
    { "REG",  DBG_REG,      "Register activity" },
    { "CMD",  DBG_CMD,      "Chip commands" },
    { "INT",  DBG_INT,      "Interrupts" },
    { "SCMD", SCSI_DBG_CMD, "SCSI commands" },
    { "SMSG", SCSI_DBG_MSG, "SCSI messages" },
    { "SBUS", SCSI_DBG_BUS, "SCSI bus activity" },
    { "SDSK", SCSI_DBG_DSK, "SCSI disk activity" },
    { "STAP", SCSI_DBG_TAP, "SCSI tape activity" },
    { 0 }
    };

extern DEVICE rz_dev;
extern UNIT rz_unit[];
#if (RZ_CTLRS > 1)
extern DEVICE rzb_dev;
extern UNIT rzb_unit[];
#endif

t_stat rz_svc (UNIT *uptr);
t_stat rz_reset (DEVICE *dptr);
t_stat rz_attach (UNIT *uptr, CONST char *cptr);
void rz_sw_reset (CTLR *rz);
t_stat rz_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr);
void rz_cmd (CTLR *rz, uint32 cmd);
void rz_dma_stopped (CTLR *rz, uint32 cmd, int32 nshort);
t_stat rz_set_type (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat rz_show_type (FILE *st, UNIT *uptr, int32 val, CONST void *desc);
const char *rz_description (DEVICE *dptr);


/* RZ data structures

   rz_dev       RZ device descriptor
   rz_unit      RZ unit list
   rz_reg       RZ register list
*/

CTLR rz_ctx = { .cnum = 0, .dev = &rz_dev, .units = rz_unit };

#define RZ_UNIT_TARGET  { UDATA (&rz_svc, UNIT_FIX+UNIT_ATTABLE+UNIT_DISABLE+UNIT_ROABLE+ \
            (RZ23_DTYPE << UNIT_V_DTYPE), RZ_SIZE (RZ23)) }
#define RZ_UNIT_LIST \
    RZ_UNIT_TARGET, RZ_UNIT_TARGET, RZ_UNIT_TARGET, RZ_UNIT_TARGET, \
    RZ_UNIT_TARGET, RZ_UNIT_TARGET, RZ_UNIT_TARGET, RZ_UNIT_TARGET, \
    { UDATA (&rz_svc, UNIT_DIS, 0) }

UNIT rz_unit[] = {
    RZ_UNIT_LIST
    };

REG rz_reg[] = {
    { FLDATAD (INT, int_req[IPL_SC], INT_V_SC, "interrupt pending flag") },
    { NULL }
    };

#if (RZ_CTLRS > 1)
CTLR rzb_ctx = { .cnum = 1, .dev = &rzb_dev, .units = rzb_unit };

UNIT rzb_unit[] = {
    RZ_UNIT_LIST
    };

REG rzb_reg[] = {
    { NULL }
    };
#endif

MTAB rz_mod[] = {
    { MTAB_XTD|MTAB_VUN, 0, "write enabled", "WRITEENABLED",
        &scsi_set_wlk, &scsi_show_wlk,   NULL, "Write enable drive" },
    { MTAB_XTD|MTAB_VUN, 1, NULL, "LOCKED",
        &scsi_set_wlk, NULL,   NULL, "Write lock drive" },
    { MTAB_XTD|MTAB_VUN, RZ23_DTYPE, NULL, "RZ23",
      &rz_set_type, NULL, NULL, "Set RZ23 Disk Type" },
    { MTAB_XTD|MTAB_VUN, RZ23L_DTYPE, NULL, "RZ23L",
      &rz_set_type, NULL, NULL, "Set RZ23L Disk Type" },
    { MTAB_XTD|MTAB_VUN, RZ24_DTYPE, NULL, "RZ24",
      &rz_set_type, NULL, NULL, "Set RZ24 Disk Type" },
    { MTAB_XTD|MTAB_VUN, RZ24L_DTYPE, NULL, "RZ24L",
      &rz_set_type, NULL, NULL, "Set RZ24L Disk Type" },
    { MTAB_XTD|MTAB_VUN, RZ25_DTYPE, NULL, "RZ25",
      &rz_set_type, NULL, NULL, "Set RZ25 Disk Type" },
    { MTAB_XTD|MTAB_VUN, RZ25L_DTYPE, NULL, "RZ25L",
      &rz_set_type, NULL, NULL, "Set RZ25L Disk Type" },
    { MTAB_XTD|MTAB_VUN, RZ26_DTYPE, NULL, "RZ26",
      &rz_set_type, NULL, NULL, "Set RZ26 Disk Type" },
    { MTAB_XTD|MTAB_VUN, RZ26L_DTYPE, NULL, "RZ26L",
      &rz_set_type, NULL, NULL, "Set RZ26L Disk Type" },
    { MTAB_XTD|MTAB_VUN, RZ55_DTYPE, NULL, "RZ55",
      &rz_set_type, NULL, NULL, "Set RZ55 Disk Type" },
    { MTAB_XTD|MTAB_VUN, RRD40_DTYPE, NULL, "CDROM",
      &rz_set_type, NULL, NULL, "Set RRD40 Disk Type" },
    { MTAB_XTD|MTAB_VUN, RRD40_DTYPE, NULL, "RRD40",
      &rz_set_type, NULL, NULL, "Set RRD40 Disk Type" },
    { MTAB_XTD|MTAB_VUN, RRD42_DTYPE, NULL, "RRD42",
      &rz_set_type, NULL, NULL, "Set RRD42 Disk Type" },
    { MTAB_XTD|MTAB_VUN, RRW11_DTYPE, NULL, "RRW11",
      &rz_set_type, NULL, NULL, "Set RRW11 Disk Type" },
    { MTAB_XTD|MTAB_VUN, CDW900_DTYPE, NULL, "CDW900",
      &rz_set_type, NULL, NULL, "Set SONY CDW-900E Disk Type" },
    { MTAB_XTD|MTAB_VUN, XR1001_DTYPE, NULL, "XR1001",
      &rz_set_type, NULL, NULL, "Set JVC XR-W1001 Disk Type" },
    { MTAB_XTD|MTAB_VUN, TZK50_DTYPE, NULL, "TZK50",
      &rz_set_type, NULL, NULL, "Set DEC TZK50 Tape Type" },
    { MTAB_XTD|MTAB_VUN, TZ30_DTYPE, NULL, "TZ30",
      &rz_set_type, NULL, NULL, "Set DEC TZ30 Tape Type" },
    { MTAB_XTD|MTAB_VUN|MTAB_VALR, RZU_DTYPE, NULL, "RZUSER",
      &rz_set_type, NULL, NULL, "Set RZUSER=size Disk Type" },
    { MTAB_XTD|MTAB_VUN, 0, "TYPE", NULL,
      NULL, &rz_show_type, NULL, "Display device type" },
    { SCSI_NOAUTO, SCSI_NOAUTO, "noautosize", "NOAUTOSIZE", NULL, NULL, NULL, "Disables disk autosize on attach" },
    { SCSI_NOAUTO,           0, "autosize",   "AUTOSIZE",   NULL, NULL, NULL, "Enables disk autosize on attach" },
    { MTAB_XTD|MTAB_VUN, 0, "FORMAT", "FORMAT",
      &scsi_set_fmt, &scsi_show_fmt, NULL, "Set/Display unit format" },
    { 0 }
    };

static const char *drv_types[] = {
    "RZ23",
    "RZ23L",
    "RZ24",
    "RZ24L",
    "RZ25",
    "RZ25L",
    "RZ26",
    "RZ26L",
    "RZ55",
    "CDROM",
    "RRD40",
    "RRD42",
    "RRW11",
    "CDW900",
    "XR1001",
    "TZK50",
    "TZ30",
    "RZUSER"
    };

DEVICE rz_dev = {
    "RZ", rz_unit, rz_reg, rz_mod,
    9, DEV_RDX, 8, 1, DEV_RDX, 8,
    NULL, NULL, &rz_reset,
    NULL, &rz_attach, &scsi_detach,
    NULL, DEV_DISABLE | DEV_DEBUG | DEV_DISK | DEV_SECTORS,
    0, rz_debug, NULL, NULL, &rz_help, NULL, NULL,
    &rz_description
    };

#if (RZ_CTLRS > 1)
DEVICE rzb_dev = {
    "RZB", rzb_unit, rzb_reg, rz_mod,
    9, DEV_RDX, 8, 1, DEV_RDX, 8,
    NULL, NULL, &rz_reset,
    NULL, &rz_attach, &scsi_detach,
    NULL, DEV_DISABLE | DEV_DEBUG | DEV_DISK | DEV_SECTORS,
    0, rz_debug, NULL, NULL, &rz_help, NULL, NULL,
    &rz_description
    };
#endif

static CTLR *rz_ctxmap[] = {
    &rz_ctx
#if (RZ_CTLRS > 1)
    , &rzb_ctx
#endif
    };

/* Register names for Debug tracing */
static const char *rz_rd_regs[] =
    {"TX L", "TX H", "FIFO", "CMD ",
     "STAT", "INT ", "SEQ ", "FFLG",
     "CFG1", "RSVD", "RSVD", "CFG2",
     "CFG3", "RSVD", "RSVD", "RSVD" };
static const char *rz_wr_regs[] =
    {"TX L", "TX H", "FIFO", "CMD ",
     "DST ", "TMO ", "SYNP", "SYNO",
     "CFG1", "CLK ", "TEST", "CFG2",
     "CFG3", "RSVD", "RSVD", "FFOB" };

uint8 rz_fifo_rd (CTLR *rz)
{
if (rz->fifo_c) {
    rz->fifo_t &= 0xF;
    rz->fifo_c--;
    return rz->fifo[rz->fifo_t++];
    }
else
    return rz->fifo[rz->fifo_b];
}

void rz_fifo_wr (CTLR *rz, uint8 data)
{
if (rz->fifo_c < 16) {
    rz->fifo[rz->fifo_b++] = data;
    rz->fifo_b &= 0xF;
    rz->fifo_c++;
    }
else {
    rz->fifo[rz->fifo_b] = data;
    rz->stat |= STS_GE;                                 /* gross error */
    }
}

void rz_fifo_reset (CTLR *rz)
{
rz->fifo_c = 0;
rz->fifo_t = rz->fifo_b = 0;
rz->fifo[rz->fifo_b] = 0;
}

/* Does a target answer selection? */

static t_bool rz_present (CTLR *rz, uint32 tgt)
{
#if defined (RZ_ABSENT_UNATTACHED)
UNIT *uptr = rz->bus.dev[tgt];

if (tgt == (uint32) (rz->cfg1 & CFG1_MYID))             /* the chip itself */
    return FALSE;
if (((uptr->flags & UNIT_ATT) == 0) &&                  /* nothing attached */
    !rzdev_tab[GET_DTYPE (uptr->flags)].removeable)     /* and not a drive without media */
    return FALSE;
#endif
return TRUE;
}

/* IO dispatch routines, I/O addresses 177601x0 - 177601x7 */

static int32 rz_rd_c (CTLR *rz, int32 pa)
{
int32 data = 0;

#if defined (RZ_DMA_ADDR)
if (pa == RZ_DMA_ADDR) {
    return rz->dma;
    }
if (pa == RZ_DMA_DIR) {
    return rz->dir;
    }
#endif

switch ((pa >> 2) & 0xF) {                              /* case on PA<2:1> */

    case 0:                                             /* transfer counter LSB */
        data = rz->txc & 0xFF;
        break;

    case 1:                                             /* transfer counter MSB */
        data = (rz->txc >> 8) & 0xFF;
        break;

    case 2:                                             /* FIFO */
        data = rz_fifo_rd (rz);
        break;

    case 3:                                             /* command */
        data = rz->last_cmd;
        break;

    case 4:                                             /* status */
        data = rz->stat | rz->bus.phase;
        break;

    case 5:                                             /* interrupt */
        data = rz->intr;
        if (rz->stat & STS_INT) {
            rz->stat &= STS_CLR;
            rz->intr = 0;
#if defined (RZ_INT_LEVEL)
            RZ_CLR_INT (rz);                            /* the request follows the chip */
#endif
            }
        break;

    case 6:                                             /* sequence step */
        data = rz->seq;
        break;

    case 7:                                             /* FIFO flags/seq step */
        data = (rz->seq << 5) | rz->fifo_c;
        break;

    case 8:                                             /* config 1 */
        data = rz->cfg1;
        break;

    case 11:                                            /* config 2 */
        data = rz->cfg2;
        break;

    case 12:                                            /* config 3 */
        data = rz->cfg3;
        break;

    default:                                            /* NCR reserved */
        data = 0;
        break;
        }

sim_debug (DBG_REG, rz->dev, "rz_rd(PA=0x%08X [%s], data=0x%X) at %08X\n", pa, rz_rd_regs[(pa >> 2) & 0xF], data, fault_PC);

SET_IRQL;
return data;
}

int32 rz_rd (int32 pa)
{
return rz_rd_c (rz_ctxmap[0], pa);
}

static void rz_wr_c (CTLR *rz, int32 pa, int32 data, int32 access)
{
sim_debug (DBG_REG, rz->dev, "rz_wr(PA=0x%08X [%s], access=%d, data=0x%X) at %08X\n", pa, rz_wr_regs[(pa >> 2) & 0xF], access, data, fault_PC);

#if defined (RZ_DMA_ADDR)
if (pa == RZ_DMA_ADDR) {
    rz->dma = data;
    return;
    }
if (pa == RZ_DMA_DIR) {
    rz->dir = data;
    return;
    }
#endif

switch ((pa >> 2) & 0xF) {                              /* case on PA<2:1> */

    case 0:                                             /* transfer count LSB */
        rz->txi = (rz->txi & ~0xFF) | (data & 0xFF);
        break;

    case 1:                                             /* transfer count MSB */
        rz->txi = (rz->txi & ~0xFF00) | ((data << 8) & 0xFF00);
        break;

    case 2:                                             /* FIFO */
        rz_fifo_wr (rz, data);
        break;

    case 3:                                             /* command */
        rz_cmd (rz, data);
        break;

    case 4:                                             /* destination bus ID */
        rz->dest = data & 0x7;
        break;

    case 5:                                             /* select timeout - NI */
        break;

    case 6:                                             /* sync period - NI*/
        break;

    case 7:                                             /* sync offset - NI? */
        break;

    case 8:                                             /* config 1 */
        rz->cfg1 = data;
        break;

    case 9:                                             /* clock conversion - NI */
        break;

    case 10:                                            /* test - NI? */
        break;

    case 11:                                            /* config 2 */
        rz->cfg2 = data;
        break;

    case 12:                                            /* config 3 */
        rz->cfg3 = data;
        break;

    case 15:                                            /* FIFO bottom */
        break;

    default:                                            /* NCR reserved */
        break;
        }

SET_IRQL;
}

void rz_wr (int32 pa, int32 data, int32 access)
{
rz_wr_c (rz_ctxmap[0], pa, data, access);
}

#if (RZ_CTLRS > 1)
int32 rzb_rd (int32 pa)
{
return rz_rd_c (rz_ctxmap[1], pa);
}

void rzb_wr (int32 pa, int32 data, int32 access)
{
rz_wr_c (rz_ctxmap[1], pa, data, access);
}
#endif

/* Unit service routine

   The chip's INT output is its interrupt register being non-zero: there is
   no interrupt without a cause in the register.  Every command queues one of
   these events, and a command written while an earlier event is still queued
   shares it (sim_activate leaves a queued event where it is).  If software
   reads the register between the two, the later event finds the register
   empty and must not raise the interrupt again, or the driver takes an
   interrupt that has nothing to report. */

t_stat rz_svc (UNIT *uptr)
{
CTLR *rz = rz_ctxmap[uptr->cnum];

if (rz->intr != 0) {
    rz->stat |= STS_INT;
    RZ_SET_INT (rz);
    }
return SCPE_OK;
}

void rz_setint (CTLR *rz, uint32 flag)
{
rz->intr |= flag;
sim_activate (&rz->units[RZ_CTLR], 50);
}

/* The board's DMA engine moved fewer bytes than the chip asked for

   The chip's transfer counter decrements on a DMA acknowledge for data out
   and synchronous data in, and on the SCSI ACK for asynchronous data in
   (NCR 53C94/95/96 data sheet, Transfer Counter register): a board that
   stops answering, as the KA46's does for a map entry with the valid bit
   clear, leaves the count short and the command unfinished.  The chip
   therefore sets no terminal count and raises no interrupt; the command
   stays where it is until software resets the chip (or the bus), as on
   the real part.  The board reports the failure to the CPU itself (on the
   KA46, PAR_CTL<SCSI_MAP> and the SCSI interrupt).

   An asynchronous data in is acknowledged into the chip's 16-byte FIFO
   before the DMA engine drains it, so the count runs ahead of a stopped
   engine by what the FIFO holds; rz_dma_refused() (below) takes that into
   account.  Data out has no such slack: its count follows the engine. */

void rz_dma_stopped (CTLR *rz, uint32 cmd, int32 nshort)
{
sim_debug (DBG_CMD, rz->dev, "DMA stopped, %d bytes not moved\n", nshort);
rz->last_cmd = cmd;
}

/* The board's DMA engine took only some of the bytes of an asynchronous data in

   The bytes were read from the bus (got), the engine stored got - nshort of
   them.  The chip acknowledged the rest into its FIFO for as long as there
   was room in it (16 bytes) and decremented its transfer counter for each,
   so the counter ends nshort, or the room, whichever is less, below where
   the engine left it, and the bytes wait in the FIFO, where the FIFO flags
   count them and the microprocessor can read them out.

   A transfer information command is complete when the counter is zero, the
   FIFO is empty and the target asserts REQ for the next byte, or when the
   target changes phase (data sheet, Transfer Information).  A FIFO that
   holds bytes the engine did not take keeps the first from happening, so:

     - the FIFO filled and bytes remain, or the counter is zero and the
       target is still in the phase: the command waits for its DMA (terminal
       count set if the counter is zero), no interrupt;
     - the target changed phase (all it had to send was taken, by the
       engine or the FIFO): the command ends as usual, with terminal count if
       the counter is zero and the bus service interrupt, and the bytes still
       in the FIFO.

   Returns TRUE if the command goes on to end. */

static t_bool rz_dma_refused (CTLR *rz, uint32 cmd, uint32 got, int32 nshort, uint32 old_phase)
{
uint32 moved = got - nshort;
uint32 held = 16 - rz->fifo_c;
uint32 i;

if (held > (uint32) nshort)
    held = nshort;
for (i = 0; i < held; i++)
    rz_fifo_wr (rz, rz->buf[moved + i]);
rz->txc -= held;
sim_debug (DBG_CMD, rz->dev, "DMA refused, %d bytes not moved, %d in the FIFO\n", nshort, held);
if (((uint32) nshort > held) ||                         /* the FIFO is full and the target has more, */
    ((rz->txc == 0) && (rz->bus.phase == old_phase))) { /* or it is not finished with the phase */
    if (rz->txc == 0)
        rz->stat |= STS_TC;
    rz_dma_stopped (rz, cmd, nshort);
    return FALSE;
    }
return TRUE;
}

void rz_cmd (CTLR *rz, uint32 cmd)
{
uint32 ini = (rz->cfg1 & CFG1_MYID);
uint32 tgt = rz->dest;
uint32 state = scsi_state (&rz->bus, ini);
uint32 txc, i, old_phase;
int32 nshort;
UNIT *cuptr = &rz->units[RZ_CTLR];

if ((cmd & CMD_DISC) && (state != SCSI_DISC)) {         /* check cmd validity */
    sim_debug (DBG_CMD, rz->dev, "disconnected cmd when not disconnected\n");
    rz_setint (rz, INT_ILLCMD);
    return;
    }
if ((cmd & CMD_TARG) && (state != SCSI_TARG)) {
    sim_debug (DBG_CMD, rz->dev, "target cmd when not target\n");
    rz_setint (rz, INT_ILLCMD);
    return;
    }
if ((cmd & CMD_INIT) && (state != SCSI_INIT)) {
    sim_debug (DBG_CMD, rz->dev, "initiator cmd when not initiator\n");
    rz_setint (rz, INT_ILLCMD);
    return;
    }

switch (cmd & 0x7f) {

    case 0:                                         /* NOP */
        sim_debug (DBG_CMD, rz->dev, "NOP\n");
        if (cmd & 0x80) {                           /* DMA */
            rz->stat &= ~STS_TC;
            rz->txc = (rz->txi == 0) ? RZ_MAXFR : rz->txi;
            }
        break;

    case 1:                                         /* flush FIFO */
        sim_debug (DBG_CMD, rz->dev, "flush fifo\n");
        rz_fifo_reset (rz);
        rz->intr |= INT_FC;
        break;

    case 2:                                         /* reset chip */
        sim_debug (DBG_CMD, rz->dev, "sw reset\n");
        rz_sw_reset (rz);
        break;

    case 3:                                         /* reset SCSI */
        sim_debug (DBG_CMD, rz->dev, "SCSI reset\n");
        scsi_reset (&rz->bus);
        if ((rz->cfg1 & CFG1_SRD) == 0) {
            rz_setint (rz, INT_SCSIRST);
            }
        break;

    case 0x20:                                      /* send message */
        sim_debug (DBG_CMD, rz->dev, "send message\n");
        break;

    case 0x21:                                      /* send status */
        sim_debug (DBG_CMD, rz->dev, "send status\n");
        break;

    case 0x22:                                      /* send data */
        sim_debug (DBG_CMD, rz->dev, "send data\n");
        break;

    case 0x23:                                      /* disconnect sequence */
        sim_debug (DBG_CMD, rz->dev, "disconnect sequence\n");
        break;

    case 0x24:                                      /* terminate sequence */
        sim_debug (DBG_CMD, rz->dev, "terminate sequence\n");
        break;

    case 0x25:                                      /* target cmd complete sequence */
        sim_debug (DBG_CMD, rz->dev, "target cmd complete sequence\n");
        break;

    case 0x27:                                      /* disconnect */
        sim_debug (DBG_CMD, rz->dev, "disconnect\n");
        break;

    case 0x28:                                      /* rcv message seq */
        sim_debug (DBG_CMD, rz->dev, "rcv message seq\n");
        break;

    case 0x29:                                      /* rcv cmd */
        sim_debug (DBG_CMD, rz->dev, "rcv cmd\n");
        break;

    case 0x30:                                      /* rcv data */
        sim_debug (DBG_CMD, rz->dev, "rcv data\n");
        break;

    case 0x31:                                      /* rcv cmd seq */
        sim_debug (DBG_CMD, rz->dev, "rcv cmd seq\n");
        break;

    case 0x40:                                      /* reselect */
        sim_debug (DBG_CMD, rz->dev, "reselect\n");
        break;

    case 0x41:                                          /* select without ATN */
        sim_debug (DBG_CMD, rz->dev, "select without atn\n");
        rz->seq = 0;
        if (!scsi_arbitrate (&rz->bus, ini)) {
            rz->seq = 0;
            rz->intr |= INT_DIS;                        /* disconnect */
            sim_activate (cuptr, 100);
            break;
            }
        if (!rz_present (rz, tgt) || !scsi_select (&rz->bus, tgt)) {
            rz->seq = 0;
            rz->intr |= INT_DIS;                        /* disconnect */
            scsi_release (&rz->bus);
            sim_activate (cuptr, 100);
            break;
            }
        rz->seq = 2;
        for (i = 0; rz->fifo_c > 0; i++)
            rz->buf[i] = rz_fifo_rd (rz);
        scsi_write (&rz->bus, &rz->buf[0], i);
        if (scsi_state (&rz->bus, tgt) == SCSI_DISC) {
            rz->seq = 3;
            rz->intr |= INT_DIS;
            }
        else {
            rz->seq = 4;
            rz->intr |= (INT_BUSSV | INT_FC);
            }
        sim_activate (cuptr, 50);
        break;

    case 0x42:                                          /* select with ATN */
        sim_debug (DBG_CMD, rz->dev, "select with atn\n");
        rz->seq = 0;
        if (!scsi_arbitrate (&rz->bus, ini)) {
            rz->intr |= INT_DIS;                        /* disconnect */
            sim_activate (cuptr, 100);
            break;
            }
        scsi_set_atn (&rz->bus);
        if (!rz_present (rz, tgt) || !scsi_select (&rz->bus, tgt)) {
            rz->seq = 0;
            rz->intr |= INT_DIS;                        /* disconnect */
            scsi_release (&rz->bus);
            sim_activate (cuptr, 100);
            break;
            }
        for (i = 0; rz->fifo_c > 0; i++)
            rz->buf[i] = rz_fifo_rd (rz);
        scsi_write (&rz->bus, &rz->buf[0], i);
        rz->seq = 2;
        if (scsi_state (&rz->bus, tgt) == SCSI_DISC) {
            rz->seq = 3;
            rz->intr |= INT_DIS;
            }
        else {
            rz->seq = 4;
            rz->intr |= (INT_BUSSV | INT_FC);
            }
        sim_activate (cuptr, 50);
        break;

    case 0x43:                                          /* select with ATN and stop */
        sim_debug (DBG_CMD, rz->dev, "select with atn and stop\n");
        if (!scsi_arbitrate (&rz->bus, ini)) {
            rz->seq = 0;
            rz->intr |= INT_DIS;                        /* disconnect */
            sim_activate (cuptr, 100);
            break;
            }
        scsi_set_atn (&rz->bus);
        if (!rz_present (rz, tgt) || !scsi_select (&rz->bus, tgt)) {
            rz->seq = 0;
            rz->intr |= INT_DIS;                        /* disconnect */
            scsi_release (&rz->bus);
            sim_activate (cuptr, 100);
            break;
            }
        rz->buf[0] = rz_fifo_rd (rz);
        scsi_write (&rz->bus, &rz->buf[0], 1);          /* send one message byte */
        if (scsi_state (&rz->bus, tgt) == SCSI_DISC) {  /* disconnected? */
            rz->seq = 0;
            rz->intr |= INT_DIS;
            }
        else {
            scsi_set_atn (&rz->bus);                    /* keep ATN asserted */
            rz->seq = 1;
            rz->intr |= (INT_BUSSV | INT_FC);           /* continue */
            }
        sim_activate (cuptr, 50);
        break;

    case 0x44:                                          /* enable selection/reselection */
        sim_debug (DBG_CMD, rz->dev, "enable selection/reselection\n");
        break;

    case 0x46:                                          /* select with ATN3 */
        sim_debug (DBG_CMD, rz->dev, "select with atn3\n");
        scsi_set_atn (&rz->bus);
        if (!rz_present (rz, tgt) || !scsi_select (&rz->bus, tgt)) {
            rz->intr |= INT_DIS;                        /* disconnect */
            scsi_release (&rz->bus);
            }
        else
            rz->intr |= (INT_BUSSV | INT_FC);           /* selected */
        sim_activate (cuptr, 50);
        break;

    case 0x1A:                                          /* set ATN */
        sim_debug (DBG_CMD, rz->dev, "set atn\n");
        scsi_set_atn (&rz->bus);                        /* no interrupt (Am53CF94 data sheet) */
        break;

    case 0x1B:                                          /* reset ATN */
        sim_debug (DBG_CMD, rz->dev, "reset atn\n");
        scsi_release_atn (&rz->bus);
        break;

    case 0x10:
        sim_debug (DBG_CMD, rz->dev, "transfer information\n");
        if (cmd & 0x80) {                               /* DMA */
            rz->stat &= ~STS_TC;
            rz->txc = (rz->txi == 0) ? RZ_MAXFR : rz->txi;
            }
        old_phase = rz->bus.phase;
        switch (rz->bus.phase) {
            case SCSI_DATO:                             /* data out */
            case SCSI_CMD:                              /* command */
            case SCSI_MSGO:                             /* message out */
                if (rz->bus.phase == 6)
                    scsi_release_atn (&rz->bus);
                if (cmd & 0x80) {                       /* DMA */
                    nshort = RZ_DMA_READ (rz, rz->txc, &rz->buf[0]);
                    txc = ((uint32) nshort < rz->txc)?
                        scsi_write (&rz->bus, &rz->buf[0], rz->txc - nshort): 0;
                    RZ_DMA_DONE (rz, txc);
                    rz->txc -= txc;
                    if (nshort != 0) {                  /* board stopped the DMA */
                        rz_dma_stopped (rz, cmd, nshort);
                        return;
                        }
                    if (rz->txc == 0)
                        rz->stat |= STS_TC;
                    }
                else {
                    for (txc = 0; rz->fifo_c > 0; txc++)
                        rz->buf[txc] = rz_fifo_rd (rz);
                    txc = scsi_write (&rz->bus, &rz->buf[0], txc);
                    }
                break;

            case SCSI_DATI:                             /* data in */
            case SCSI_STS:                              /* status */
            case SCSI_MSGI:                             /* message in */
                if (cmd & 0x80) {                       /* DMA */
                    while ((rz->bus.phase == old_phase) && (rz->txc != 0)) {
                        txc = scsi_read (&rz->bus, &rz->buf[0], rz->txc);
                        nshort = RZ_DMA_WRITE (rz, txc, &rz->buf[0]);
                        RZ_DMA_DONE (rz, txc - nshort);
                        rz->txc -= txc - nshort;
                        if ((nshort != 0) &&            /* board stopped the DMA */
                            !rz_dma_refused (rz, cmd, txc, nshort, old_phase))
                            return;
                        }
                    if (rz->txc == 0)
                        rz->stat |= STS_TC;
                    }
                else {
                    txc = scsi_read (&rz->bus, &rz->buf[0], 1);
                    rz_fifo_wr (rz, rz->buf[0]);
                    }
                break;
                }
        rz->seq = 0;
        if (scsi_state (&rz->bus, tgt) == SCSI_DISC) {
            rz->intr |= INT_DIS;
            }
        else {
            if (rz->bus.req)
                rz->intr |= INT_BUSSV;
            if (rz->bus.phase == SCSI_MSGI)
                rz->intr |= INT_FC;
            }
            sim_activate (cuptr, 50);
        break;

    case 0x11:
        sim_debug (DBG_CMD, rz->dev, "initiator command complete\n");
        txc = 0;
        txc += scsi_read (&rz->bus, &rz->buf[0], 1);
        txc += scsi_read (&rz->bus, &rz->buf[1], 1);
        rz_fifo_wr (rz, rz->buf[0]);
        rz_fifo_wr (rz, rz->buf[1]);
        rz->seq = 0;
        rz->intr |= INT_FC;
        sim_activate (cuptr, 50);
        break;

    case 0x12:
        sim_debug (DBG_CMD, rz->dev, "message accepted\n");
        scsi_release (&rz->bus);
        rz->seq = 0;
        rz->intr |= INT_DIS;
        sim_activate (cuptr, 50);
        break;

    case 0x18:
        sim_debug (DBG_CMD, rz->dev, "transfer pad\n");
        if (cmd & 0x80) {                               /* DMA */
            rz->stat &= ~STS_TC;
            rz->txc = (rz->txi == 0) ? RZ_MAXFR : rz->txi;
            }
        old_phase = rz->bus.phase;
        switch (rz->bus.phase) {
            case SCSI_DATO:                             /* data out */
            case SCSI_CMD:                              /* command */
            case SCSI_MSGO:                             /* message out */
                if (rz->bus.phase == 6)
                    scsi_release_atn (&rz->bus);
                rz->buf[0] = 0;
                for (; ((rz->bus.phase == old_phase) && (rz->txc > 0)); rz->txc--)
                    scsi_write (&rz->bus, &rz->buf[0], 1);
                if (rz->txc == 0)
                    rz->stat |= STS_TC;
                break;

            case SCSI_DATI:                             /* data in */
            case SCSI_STS:                              /* status */
            case SCSI_MSGI:                             /* message in */
                for (; ((rz->bus.phase == old_phase) && (rz->txc > 0)); rz->txc--)
                    txc = scsi_read (&rz->bus, &rz->buf[0], 1);
                if (rz->txc == 0)
                    rz->stat |= STS_TC;
                break;
                }
        rz->seq = 0;
        if (scsi_state (&rz->bus, tgt) == SCSI_DISC)
            rz->intr |= INT_DIS;
        else {
            if (rz->bus.req)
                rz->intr |= INT_BUSSV;
            /* if (rz->bus.phase == SCSI_MSGI) */
                rz->intr |= INT_FC;
            }
            sim_activate (cuptr, 50);
        break;

    default:
        sim_debug (DBG_CMD, rz->dev, "unknown command %X\n", cmd);
        break;
        }
if (cmd > 0)
    rz->last_cmd = cmd;
}

void rz_sw_reset (CTLR *rz)
{
uint32 i;

for (i = 0; i < 9; i++)
    sim_cancel (&rz->units[i]);
rz->txc = 0;
rz->cfg1 = rz->cfg1 & 0x7;
rz->cfg2 = 0;
rz->cfg3 = 0;
rz->stat = 0;
rz->seq = 0;
rz->intr = 0;
rz->dest = 0;
rz_fifo_reset (rz);
RZ_CLR_INT (rz);
scsi_reset (&rz->bus);
}

t_stat rz_reset (DEVICE *dptr)
{
CTLR *rz = rz_ctxmap[0];
uint32 i;
uint32 dtyp;
UNIT *uptr;
t_stat r;

#if (RZ_CTLRS > 1)
if (dptr == &rzb_dev)
    rz = rz_ctxmap[1];
#endif

if (rz->buf == NULL)
    rz->buf = (uint8 *)calloc (RZ_MAXFR, sizeof(uint8));
if (rz->buf == NULL)
    return SCPE_MEM;
r = scsi_init (&rz->bus, RZ_MAXFR);                     /* init SCSI bus */
if (r != SCPE_OK)
    return r;
rz->bus.dptr = dptr;                                    /* set bus device */
for (i = 0; i < 8; i++) {
    uptr = dptr->units + i;
    if (i == (uint32) RZ_INITIATOR_ID (rz))             /* initiator ID? */
        uptr->flags = UNIT_DIS;                         /* disable unit */
    scsi_add_unit (&rz->bus, i, uptr);
    dtyp = GET_DTYPE (uptr->flags);
    scsi_set_unit (&rz->bus, uptr, &rzdev_tab[dtyp]);
    scsi_reset_unit (uptr);
    }
for (i = 0; i <= RZ_CTLR; i++)                          /* units know their controller */
    dptr->units[i].cnum = rz->cnum;
rz_sw_reset (rz);
return SCPE_OK;
}

/* Set unit type (and capacity if user defined) */

t_stat rz_set_type (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
CTLR *rz = rz_ctxmap[uptr->cnum];
uint32 cap;
uint32 max = sim_toffset_64? RZU_EMAXC: RZU_MAXC;
t_stat r;

if ((val < 0) || ((val != RZU_DTYPE) && cptr))
    return SCPE_ARG;
if (uptr->flags & UNIT_ATT)
    return SCPE_ALATT;
if (cptr) {
    cap = (uint32) get_uint (cptr, 10, 0xFFFFFFFF, &r);
    if ((sim_switches & SWMASK ('L')) == 0)
        cap = cap * 1954;
    if ((r != SCPE_OK) || (cap < RZU_MINC) || (cap > max))
        return SCPE_ARG;
    rzdev_tab[val].lbn = cap;
    }
uptr->flags = (uptr->flags & ~UNIT_DTYPE) | (val << UNIT_V_DTYPE);
uptr->capac = (t_addr)rzdev_tab[val].lbn;
scsi_set_unit (&rz->bus, uptr, &rzdev_tab[val]);
scsi_reset_unit (uptr);
return SCPE_OK;
}

/* Show unit type */

t_stat rz_show_type (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
fprintf (st, "%s", rzdev_tab[GET_DTYPE (uptr->flags)].name);
return SCPE_OK;
}

t_stat rz_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
fprintf (st, "NCR 53C94 SCSI Controller (%s)\n\n", dptr->name);
fprintf (st, "The %s controller simulates the NCR 53C94 SCSI controller connected\n", dptr->name);
fprintf (st, "to a bus with up to 7 target devices.\n");
if (dptr->flags & DEV_DISABLE)
    fprintf (st, "Initially the %s controller is disabled.\n", dptr->name);
else
    fprintf (st, "The %s controller cannot be disabled.\n", dptr->name);
fprintf (st, "SCSI target device %s%d is reserved for the initiator and cannot\n", dptr->name,
    RZ_INITIATOR_ID (dptr == &rz_dev? &rz_ctx: rz_ctxmap[RZ_CTLRS - 1]));
fprintf (st, "be enabled\n");
#if defined (RZ_ABSENT_UNATTACHED)
fprintf (st, "A target disk with no file attached is absent from the bus: it does not\n");
fprintf (st, "answer selection.  A CD-ROM or tape drive with no file attached answers as\n");
fprintf (st, "a drive with no medium loaded.\n");
#endif
fprintf (st, "Each target on the SCSI bus can be set to one of several types:\n");
fprint_set_help (st, dptr);
fprintf (st, "Configured options can be displayed with:\n\n");
fprint_show_help (st, dptr);
fprint_reg_help (st, dptr);
scsi_help (st, dptr, uptr, flag, cptr);
return SCPE_OK;
}

t_stat rz_attach (UNIT *uptr, CONST char *cptr)
{
return scsi_attach_ex (uptr, cptr, drv_types);
}

const char *rz_description (DEVICE *dptr)
{
return "NCR 53C94 SCSI controller";
}
