/* vax_sgec.c: DEC SGEC (DC541) Ethernet controller

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

   sgec         SGEC Ethernet controller

   The SGEC (second generation Ethernet controller, DEC DC541) is the
   Ethernet interface of the NVAX-based VAXstation 4000 Model 90/96 and of
   several VAX 4000 and MicroVAX 3100 systems.  The CPU sees sixteen
   longword CSRs; the chip moves frames to and from memory through lists of
   four-longword descriptors that it owns while their OW bit is set, and
   learns its station and multicast addresses from a "setup frame" queued
   on the transmit list.

   DEC describes the chip in the technical manuals of the CPU modules that
   carry it; the comments cite the KA680 CPU Module Technical Manual
   (EK-KA680-TM-001), chapter 10.3 "Programming the SGEC", as "KA680
   10.3.x".  Where that chapter contradicts itself, or is silent, the
   choice made here is said, with what decided it: the self-test of the
   KA49 console firmware (which exercises the chip in detail), NetBSD's
   driver (dev/ic/sgec.c) or what VMS's EZDRIVER does.  Choices that
   nothing checks are marked "guess".

   The chip is connected to a network with ATTACH SGEC (NAT, TAP, VDE or a
   host interface, through sim_ether).  Not attached, it behaves as a chip
   whose cable is unplugged: frames sent in normal mode complete with
   "loss of carrier" and "no carrier" and nothing arrives.  The loopback
   modes work either way.

   CSRs (register n at base + 4n; 8-14 are the chip's "virtual" CSRs,
   reached with the NICSR5<DN> handshake, KA680 10.3.2.2):

   0    <31:30> IPL 14 + n, <29> sync mode, <15:2> vector; <28:16> and
        <1:0> read as one
   1    transmit poll demand (write)
   2    receive poll demand (write)
   3    receive descriptor list address
   4    transmit descriptor list address
   5    status: <31> ID init done, <30> SF self-test failed, <29:26> SS
        self-test status, <25:24> TS transmit state, <23:22> RS receive
        state, <18:17> OM (copy of CSR6<9:8>), <16> DN virtual CSR access
        done, <7> BO boot message, <6> TW, <5> RW watchdogs, <4> ME memory
        error, <3> RU receive buffer unavailable, <2> RI, <1> TI (<7:1>
        write one to clear), <0> IS summary of <6:1>
   6    command: <31> RE reset, <30> IE, <28:25> BL burst limit, <20> BE
        boot message enable, <19> SE, <11> ST start transmission, <10> SR
        start reception, <9:8> OM 0 normal, 1 internal loopback, 2 external
        loopback, 3 diagnostic, <7> DC disable data chaining, <6> FC force
        collisions, <3> PB pass bad frames, <2:1> AF 1 promiscuous, 2 all
        multicasts
   7    system page table base (physical), for SVAPTE buffer addresses
   8    reserved
   9    watchdog timers: <31:16> receive, <15:0> transmit
   10   <20:16> chip revision, <15:0> missed frames (cleared when read)
   11   boot message verification field <31:0>
   12   boot message verification field <63:32>
   13   <7:0> boot message processor field
   14   diagnostic breakpoint
   15   monitor command

   Descriptors, longwords 0-3 (KA680 10.3.15, 10.3.16):

   0    <31> OW; receive: <30:16> frame length, <15:0> status; transmit:
        <29:16> TDR count, <15:0> status.  The only longword the chip writes.
   1    <31> CA chain, <30> VA virtual; receive: <29> VT; transmit: <29:28>
        DT (2 = setup frame, 3 = diagnostic), <27> AC no CRC, <26> FS, <25>
        LS, <24> IC interrupt on completion, <23> VT
   2    <30:16> buffer size, <8:0> byte offset in the first page (VA)
   3    buffer address; with CA, the address of the next descriptor; with
        VA, the address of the PTE that maps the buffer's first page:
        physical with VT set, system virtual (translated through CSR7)
        without it
*/

#include "vax_defs.h"
#include "sim_ether.h"
#include "vax_sgec.h"

/* CSR0 */

#define CSR0_RW         0xE000FFFC                      /* IPL, SA, vector */
#define CSR0_MBO        0x1FFF0003
#define CSR0_VEC        0x0000FFFC

/* CSR5 */

#define CSR5_ID         0x80000000                      /* init done */
#define CSR5_SF         0x40000000                      /* self-test failed */
#define CSR5_SS         0x3C000000                      /* self-test status */
#define CSR5_V_TS       24                              /* transmit state */
#define CSR5_V_RS       22                              /* receive state */
#define CSR5_V_OM       17                              /* operating mode */
#define CSR5_DN         0x00010000                      /* virtual CSR access done */
#define CSR5_MBO        0x0038FF00
#define CSR5_BO         0x00000080                      /* boot message */
#define CSR5_TW         0x00000040                      /* transmit watchdog */
#define CSR5_RW         0x00000020                      /* receive watchdog */
#define CSR5_ME         0x00000010                      /* memory error */
#define CSR5_RU         0x00000008                      /* receive buffer unavailable */
#define CSR5_RI         0x00000004                      /* receive interrupt */
#define CSR5_TI         0x00000002                      /* transmit interrupt */
#define CSR5_IS         0x00000001                      /* interrupt summary */
#define CSR5_W1C        (CSR5_BO | CSR5_TW | CSR5_RW | CSR5_ME | CSR5_RU | CSR5_RI | CSR5_TI)
#define CSR5_INT        (CSR5_TW | CSR5_RW | CSR5_ME | CSR5_RU | CSR5_RI | CSR5_TI)

#define ST_STOP         0                               /* process states */
#define ST_RUN          1
#define ST_SUSP         2

/* CSR6.  KA680 table 10-14 calls <24:21>, <18:12>, <5:4> and <0> "must be
   one"; its reset value (table 10-15, 03E0F000) and figure 10-7 have ones
   only in <24:21> and <15:12>, with <18:16>, <5:4> and <0> reserved.  The
   reset value and the figure are followed. */

#define CSR6_RE         0x80000000                      /* reset */
#define CSR6_IE         0x40000000                      /* interrupt enable */
#define CSR6_BL1        0x02000000                      /* burst limit 1, its value after reset */
#define CSR6_RW         0x5E180FCE                      /* IE, BL, BE, SE, ST, SR, OM, DC, FC, PB, AF */
#define CSR6_MBO        0x01E0F000
#define CSR6_BE         0x00100000                      /* boot message enable */
#define CSR6_ST         0x00000800                      /* start transmission */
#define CSR6_SR         0x00000400                      /* start reception */
#define CSR6_V_OM       8
#define CSR6_M_OM       3
#define  OM_NORM        0
#define  OM_ILBK        1
#define  OM_ELBK        2
#define CSR6_DC         0x00000080                      /* disable data chaining */
#define CSR6_FC         0x00000040                      /* force collisions */
#define CSR6_AF_PROM    0x00000002                      /* promiscuous */
#define CSR6_AF_ALLM    0x00000004                      /* all multicasts */

/* Other CSRs */

#define CSR_ADDR        0x3FFFFFFC                      /* CSR3, 4, 7 */
#define CSR9_INIT       0x04E204E2                      /* watchdogs RT = TT = 1250 (2 ms) */
#define CSR13_RW        0x000000FF
/* NICSR15 (monitor) reads 0000FFFF on a real KA49 at the console after a
   VMS shutdown (2026-10-20); neither the console's power-up tests nor
   VMS write it, so that is taken as its reset value (the KA680 manual,
   10.3.13, gives 00000FFF). */
#define CSR15_INIT      0x0000FFFF
/* NICSR10<20:16>, the chip revision.  The KA680 manual (1990-1991 editions)
   gives 00030000 as its value after reset, revision 3.  Which revision the
   KA49's chip has is not known (NICSR10 is a virtual CSR, read through the
   DN handshake, so a console EXAMINE does not show it).  VMS's EZDRIVER
   accepts only 3 and 4 (with any other value DECnet bugchecks NETDLLERR as
   it starts the line) and gives a revision 3 chip extra set-up data; the
   documented 3 is used. */
#define CSR10_REV       0x00030000
/* The self-test takes 25 ms on the chip (KA680 10.3.18.1).  The model ends
   it after a few instructions: the KA49 console waits for NICSR5<ID> with
   its own delay loops and accepts ID at once, and VMS and NetBSD poll for
   it.  25 ms is the device's only delay that SIMH converts from wall-clock
   time with its calibration, and it is the likely cause of the one
   failure seen: on a busy host the console's NI interrupt test once
   failed its initialisation (error 4C). */
#define SGEC_SELFTEST   20                              /* self-test, instructions */
#define SGEC_VLATCH     5000                            /* virtual CSR read latch, instructions */

/* Descriptors */

#define DES_OW          0x80000000                      /* owned by the SGEC */
#define DES_V_LEN       16                              /* receive frame length */
#define DES_M_LEN       0x7FFF

#define RDES0_ES        0x8000                          /* error summary */
#define RDES0_LE        0x4000                          /* length error */
#define RDES0_V_DT      12                              /* data type */
#define RDES0_BO        0x0400                          /* buffer overflow */
#define RDES0_FS        0x0200                          /* first segment */
#define RDES0_LS        0x0100                          /* last segment */
#define RDES0_TL        0x0080                          /* frame too long */
#define RDES0_FT        0x0020                          /* frame type (Ethernet) */
#define RDES0_TN        0x0008                          /* translation not valid */
#define RDES1_CA        0x80000000                      /* chain address */
#define RDES1_VA        0x40000000                      /* virtual address */
#define RDES1_VT        0x20000000                      /* PTE address is physical */

#define TDES0_ES        0x8000                          /* error summary */
#define TDES0_LE        0x1000                          /* length error */
#define TDES0_LO        0x0800                          /* loss of carrier */
#define TDES0_NC        0x0400                          /* no carrier */
#define TDES0_EC        0x0100                          /* excessive collisions */
#define TDES0_HF        0x0080                          /* heartbeat fail */
#define TDES0_TN        0x0004                          /* translation not valid */
#define TDES1_CA        0x80000000                      /* chain address */
#define TDES1_VA        0x40000000                      /* virtual address */
#define TDES1_V_DT      28                              /* data type */
#define TDES1_M_DT      3
#define  DT_NORM        0
#define  DT_SETUP       2
#define  DT_DIAG        3
#define TDES1_AC        0x08000000                      /* no CRC */
#define TDES1_FS        0x04000000                      /* first segment */
#define TDES1_IF        0x04000000                      /* setup frame: inverse filtering (the FS bit) */
#define TDES1_LS        0x02000000                      /* last segment */
#define TDES1_HP        0x02000000                      /* setup frame: hash filtering (the LS bit) */
#define TDES1_IC        0x01000000                      /* interrupt on completion */
#define TDES1_VT        0x00800000                      /* PTE address is physical */
#define SDES0_SE        0x2000                          /* setup frame not 128 bytes */

#define SGEC_MAXFR      1518                            /* longest frame, with the FCS */
#define SGEC_MINFR      60                              /* shortest frame, without the FCS */
#define SGEC_SETUPSZ    128                             /* setup frame */
#define SGEC_NFILT      16                              /* perfect filter slots */
#define SGEC_DELAY      50                              /* instructions to a DMA start */
#define SGEC_MAXDES     1024                            /* descriptors walked per pass */
#define SGEC_QUE_MAX    512                             /* frames waiting for a buffer */

/* Debug flags */

#define DBG_REG         0x0001                          /* CSR accesses */
#define DBG_TX          0x0002                          /* transmit list */
#define DBG_RX          0x0004                          /* receive list */
#define DBG_INT         0x0008                          /* interrupts */
#define DBG_SET         0x0010                          /* setup frames */
#define DBG_PCK         0x0020                          /* packet headers */
#define DBG_DAT         0x0040                          /* packet data */
#define DBG_ETH         0x0080                          /* sim_ether */

DEBTAB sgec_debug[] = {
    { "REG",  DBG_REG,  "CSR reads and writes" },
    { "TX",   DBG_TX,   "transmit descriptors" },
    { "RX",   DBG_RX,   "receive descriptors" },
    { "INT",  DBG_INT,  "interrupt requests" },
    { "SET",  DBG_SET,  "setup frames and the address filter" },
    { "PCK",  DBG_PCK,  "packet headers" },
    { "DAT",  DBG_DAT,  "packet data" },
    { "ETH",  DBG_ETH,  "the Ethernet transport" },
    { NULL, 0 }
    };

/* Statistics (SHOW SGEC STATISTICS) */

typedef struct {
    uint32      tx_frames;                              /* frames sent */
    uint32      tx_bytes;
    uint32      tx_loop;                                /* ... looped back */
    uint32      tx_err;                                 /* ... completed with an error */
    uint32      tx_host_err;                            /* ... the host could not send */
    uint32      setup;                                  /* setup frames */
    uint32      rx_frames;                              /* frames received */
    uint32      rx_bytes;
    uint32      rx_filtered;                            /* ... rejected by the filter */
    uint32      rx_missed;                              /* ... lost for want of a buffer */
    uint32      rx_err;                                 /* ... truncated or not mapped */
    uint32      rx_stopped;                             /* ... arrived with reception stopped */
    uint32      boot_msg;                               /* boot messages recognised */
    } SGEC_STATS;

uint32 sgec_csr[SGEC_NCSR] = { 0 };                     /* CSRs as stored */
uint32 sgec_rxcur = 0;                                  /* current receive descriptor */
uint32 sgec_txcur = 0;                                  /* current transmit descriptor */
int32 sgec_txst = ST_STOP;                              /* transmit state */
int32 sgec_rxst = ST_STOP;                              /* receive state */
int32 sgec_csr3w = 0;                                   /* CSR3 written since reset */
int32 sgec_csr4w = 0;                                   /* CSR4 written since reset */
int32 sgec_irq = 0;                                     /* request asserted */
uint8 sgec_filt[SGEC_NFILT * 6];                        /* perfect filter */
uint8 sgec_hash[64];                                    /* hash filter (512 bits) */
int32 sgec_hashmode = 0;                                /* hash filtering */
int32 sgec_invfilt = 0;                                 /* inverse filtering */
int32 sgec_re = 0;                                      /* CSR6<RE> reads as one (software reset) */
int32 sgec_vrg = -1;                                    /* virtual CSR latched by a read */
uint32 sgec_vval = 0;                                   /* ... its value */
double sgec_vtime = 0;                                  /* ... when */
uint32 sgec_crctab[256];
uint8 sgec_frame[SGEC_MAXFR + 4];                       /* frame being sent */
uint8 sgec_rxbuf[SGEC_MAXFR + 4];                       /* frame being received */
SGEC_STATS sgec_stats;
ETH_DEV *sgec_eth = NULL;                               /* the transport, when attached */
ETH_QUE sgec_readq;                                     /* frames from the network */
ETH_PACK sgec_rdpkt;
ETH_PACK sgec_wrpkt;
int32 sgec_must_poll = 1;                               /* transport cannot wake us */
uint32 sgec_qloss = 0;                                  /* queue losses already counted */

extern int32 tmxr_poll;

t_stat sgec_svc (UNIT *uptr);
t_stat sgec_stsvc (UNIT *uptr);
t_stat sgec_reset (DEVICE *dptr);
t_stat sgec_attach (UNIT *uptr, CONST char *cptr);
t_stat sgec_detach (UNIT *uptr);
t_stat sgec_setmac (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat sgec_showmac (FILE *st, UNIT *uptr, int32 val, CONST void *desc);
t_stat sgec_showfilt (FILE *st, UNIT *uptr, int32 val, CONST void *desc);
t_stat sgec_setstats (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat sgec_showstats (FILE *st, UNIT *uptr, int32 val, CONST void *desc);
t_stat sgec_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr);
t_stat sgec_help_attach (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr);
const char *sgec_description (DEVICE *dptr);
static void sgec_update_int (void);
static void sgec_chip_reset (void);
static void sgec_set_filter (void);
static void sgec_rx_pump (void);
void sgec_read_callback (int status);

#define UNIT_V_NOHB     (UNIT_V_UF + 0)                 /* no heartbeat from the transceiver */
#define UNIT_NOHB       (1u << UNIT_V_NOHB)

/* No heartbeat by default: the real 4000/90's transceiver sends no SQE
   test, so its SGEC reports heartbeat failure on every frame (VMS's LANCP
   "Collision detect check failure" equals the frames sent, and the
   console's own SHOW MOP counts HrtBtErr), 2026-10-19 and 2026-10-20.
   The report is harmless; SET SGEC HEARTBEAT gives a transceiver with
   one. */

UNIT sgec_unit[] = {
    { UDATA (&sgec_svc, UNIT_IDLE | UNIT_ATTABLE | UNIT_NOHB, 0) }, /* DMA engines, network */
    { UDATA (&sgec_stsvc, UNIT_DIS, 0) }                /* self-test */
    };

REG sgec_reg[] = {
    { BRDATAD (CSR,   sgec_csr,   16, 32, SGEC_NCSR, "CSRs as stored") },
    { HRDATAD (RXCUR, sgec_rxcur, 32, "current receive descriptor") },
    { HRDATAD (TXCUR, sgec_txcur, 32, "current transmit descriptor") },
    { DRDATAD (TXST,  sgec_txst,   2, "transmit state (0 stopped, 1 running, 2 suspended)") },
    { DRDATAD (RXST,  sgec_rxst,   2, "receive state (0 stopped, 1 running, 2 suspended)") },
    { FLDATAD (CSR3W, sgec_csr3w,  0, "CSR3 written since the reset") },
    { FLDATAD (CSR4W, sgec_csr4w,  0, "CSR4 written since the reset") },
    { FLDATAD (INT,   sgec_irq,    0, "interrupt request") },
    { FLDATAD (RE,    sgec_re,     0, "CSR6<RE> reads as one") },
    { BRDATAD (FILTER, sgec_filt, 16,  8, SGEC_NFILT * 6, "perfect filter, 16 addresses") },
    { BRDATAD (HASH,  sgec_hash,  16,  8, 64, "hash filter") },
    { FLDATAD (HASHMODE, sgec_hashmode, 0, "hash filtering") },
    { FLDATAD (INVFILT, sgec_invfilt, 0, "inverse filtering") },
    { DRDATAD (VRG,   sgec_vrg,    8, "virtual CSR latched by a read (-1 none)"), REG_HRO },
    { HRDATAD (VVAL,  sgec_vval,  32, "latched virtual CSR value"), REG_HRO },
    { NULL }
    };

MTAB sgec_mod[] = {
    { MTAB_XTD|MTAB_VDV|MTAB_VALR, 0, "MAC", "MAC=xx-xx-xx-xx-xx-xx",
      &sgec_setmac, &sgec_showmac, NULL, "station address (the network address ROM)" },
    { MTAB_XTD|MTAB_VDV|MTAB_NMO, 0, "ETH", NULL,
      NULL, &eth_show, NULL, "host interfaces that can be attached" },
    { MTAB_XTD|MTAB_VDV|MTAB_NMO, 0, "FILTER", NULL,
      NULL, &sgec_showfilt, NULL, "the address filter loaded by the last setup frame" },
    { MTAB_XTD|MTAB_VDV|MTAB_NMO, 0, "STATISTICS", "STATISTICS",
      &sgec_setstats, &sgec_showstats, NULL, "frame counts (SET clears them)" },
    { UNIT_NOHB, 0, "HEARTBEAT", "HEARTBEAT", NULL, NULL, NULL,
      "the transceiver sends the heartbeat (SQE test): TDES0<HF> clear" },
    { UNIT_NOHB, UNIT_NOHB, "NOHEARTBEAT", "NOHEARTBEAT", NULL, NULL, NULL,
      "no heartbeat: TDES0<HF> on every frame sent" },
    { 0 }
    };

DEVICE sgec_dev = {
    "SGEC", sgec_unit, sgec_reg, sgec_mod,
    2, 16, 32, 4, 16, 32,
    NULL, NULL, &sgec_reset,
    NULL, &sgec_attach, &sgec_detach,
    NULL, DEV_DEBUG | DEV_ETHER, 0, sgec_debug, NULL, NULL, &sgec_help,
    &sgec_help_attach, NULL, &sgec_description
    };

/* Physical memory, for descriptors, buffers and page tables */

static t_bool sgec_rdl (uint32 pa, uint32 *val)
{
if (((pa & 3) != 0) || !ADDR_IS_MEM (pa))
    return FALSE;
*val = (uint32) ReadL (pa);
return TRUE;
}

static t_bool sgec_wrl (uint32 pa, uint32 val)
{
if (((pa & 3) != 0) || !ADDR_IS_MEM (pa))
    return FALSE;
WriteL (pa, (int32) val);
return TRUE;
}

/* Ethernet CRC-32: the frame check sequence (complemented, sent low byte
   first) and, without the complement, the hash filter's index */

static void sgec_crc_init (void)
{
uint32 c, i, k;

for (i = 0; i < 256; i++) {
    c = i;
    for (k = 0; k < 8; k++)
        c = (c & 1)? (c >> 1) ^ 0xEDB88320: (c >> 1);
    sgec_crctab[i] = c;
    }
}

static uint32 sgec_crc (const uint8 *p, uint32 n)
{
uint32 c = 0xFFFFFFFF;

while (n--)
    c = sgec_crctab[(c ^ *p++) & 0xFF] ^ (c >> 8);
return c;
}

/* Append the FCS to a frame of len bytes; returns the new length */

static uint32 sgec_add_fcs (uint8 *fr, uint32 len)
{
uint32 fcs = sgec_crc (fr, len) ^ 0xFFFFFFFF;

fr[len++] = fcs & 0xFF;
fr[len++] = (fcs >> 8) & 0xFF;
fr[len++] = (fcs >> 16) & 0xFF;
fr[len++] = (fcs >> 24) & 0xFF;
return len;
}

/* A buffer: physical, or (VA) described by a vector of PTEs, the first of
   which maps the buffer's first page (KA680 10.3.15.2, 10.3.16.2).  The PTE
   address is physical (PAPTE) when VT is set and system virtual (SVAPTE)
   when it is clear: the KA680 manual's tables 10-29 and 10-34 say so, the
   bit descriptions under them say the opposite.  VMS decides it: its
   descriptors carry VT set with physical addresses of PTE vectors.  The PTE
   is a VAX PTE (valid <31>, page frame <20:0>, 512-byte pages); that the
   chip ignores the protection and modify bits is a guess (VMS's PTEs have
   M both set and clear). */

typedef struct {
    uint32      addr;                                   /* DES3 */
    uint32      offset;                                 /* byte offset (VA) */
    t_bool      va;                                     /* virtual */
    t_bool      papte;                                  /* PTE address physical */
    } SGEC_BUF;

/* The physical address of a PTE whose address the descriptor gives */

static t_bool sgec_pte_pa (uint32 ptea, t_bool papte, uint32 *pa)
{
uint32 spte;

if (papte) {
    *pa = ptea;
    return TRUE;
    }
if ((ptea & 0xC0000000) != 0x80000000)                  /* SVAPTE: system space only */
    return FALSE;
if (!sgec_rdl ((sgec_csr[7] & CSR_ADDR) + (((ptea & 0x3FFFFFFF) >> 9) << 2), &spte))
    return FALSE;
if ((spte & 0x80000000) == 0)
    return FALSE;
*pa = ((spte & 0x1FFFFF) << 9) | (ptea & 0x1FF);
return TRUE;
}

/* Physical address of byte n of a buffer; FALSE if it cannot be mapped */

static t_bool sgec_buf_pa (SGEC_BUF *b, uint32 n, uint32 *pa)
{
uint32 off, ptepa, pte;

if (!b->va) {
    *pa = b->addr + n;
    return ADDR_IS_MEM (*pa);
    }
off = (b->offset & 0x1FF) + n;
if (!sgec_pte_pa (b->addr + ((off >> 9) << 2), b->papte, &ptepa) ||
    !sgec_rdl (ptepa, &pte) || ((pte & 0x80000000) == 0))
    return FALSE;
*pa = ((pte & 0x1FFFFF) << 9) | (off & 0x1FF);
return ADDR_IS_MEM (*pa);
}

static t_bool sgec_buf_read (SGEC_BUF *b, uint8 *dst, uint32 len)
{
uint32 i, pa = 0;

for (i = 0; i < len; i++) {
    if ((i == 0) || (((pa + 1) & 0x1FF) == 0)) {        /* first byte, new page */
        if (!sgec_buf_pa (b, i, &pa))
            return FALSE;
        }
    else pa = pa + 1;
    dst[i] = (uint8) ReadB (pa);
    }
return TRUE;
}

static t_bool sgec_buf_write (SGEC_BUF *b, const uint8 *src, uint32 len)
{
uint32 i, pa = 0;

for (i = 0; i < len; i++) {
    if ((i == 0) || (((pa + 1) & 0x1FF) == 0)) {
        if (!sgec_buf_pa (b, i, &pa))
            return FALSE;
        }
    else pa = pa + 1;
    WriteB (pa, src[i]);
    }
return TRUE;
}

/* Memory error: the chip stops both processes and reports ME */

static void sgec_memerr (uint32 pa)
{
sim_debug (DBG_TX | DBG_RX, &sgec_dev, "memory error at %08X\n", pa);
sgec_csr[5] |= CSR5_ME;
sgec_txst = sgec_rxst = ST_STOP;
}

/* Acquire a descriptor: read the four longwords at *cur, following chain
   descriptors.  A chain descriptor the chip does not own, or one that
   points to itself, ends the list (KA680 10.3.15.2: the self-pointing one
   counts as host-owned whatever its OW; that OW is checked on the others
   too is an inference).  Returns 1 if the descriptor at *cur (updated
   past any chain descriptors) is owned by the chip, 0 if not, -1 on a
   memory error. */

static int32 sgec_acquire (uint32 *cur, uint32 *d)
{
int32 i;

for (i = 0; i < SGEC_MAXDES; i++) {
    if (!sgec_rdl (*cur, &d[0]) || !sgec_rdl (*cur + 4, &d[1]) ||
        !sgec_rdl (*cur + 8, &d[2]) || !sgec_rdl (*cur + 12, &d[3])) {
        sgec_memerr (*cur);
        return -1;
        }
    if ((d[0] & DES_OW) == 0)
        return 0;
    if ((d[1] & RDES1_CA) == 0)                         /* CA is <31> in both kinds */
        return 1;
    if ((d[3] & ~3u) == *cur)                           /* points to itself */
        return 0;
    *cur = d[3] & ~3u;
    }
return 0;
}

/* Setup frame (KA680 10.3.17): perfect filtering, 16 slots of 8 bytes with
   an address in the first 6 of each; or, with the setup descriptor's HP
   bit (the LS position), a 512-bit hash table of multicast addresses in
   bytes 0-63 and one physical address in bytes 64-69.  IF (the FS
   position) inverts a perfect filter. */

static void sgec_load_setup (const uint8 *sf, t_bool hash, t_bool inv)
{
sgec_hashmode = hash;
sgec_invfilt = inv && !hash;
memset (sgec_filt, 0, sizeof (sgec_filt));
memset (sgec_hash, 0, sizeof (sgec_hash));
if (hash) {
    memcpy (sgec_hash, sf, 64);
    memcpy (sgec_filt, sf + 64, 6);
    }
else {
    int32 i;
    for (i = 0; i < SGEC_NFILT; i++)
        memcpy (sgec_filt + i * 6, sf + (i << 3), 6);
    }
sim_debug (DBG_SET, &sgec_dev, "setup frame (%s%s): first address %02X-%02X-%02X-%02X-%02X-%02X\n",
           hash? "hash": "perfect", sgec_invfilt? ", inverse": "", sgec_filt[0], sgec_filt[1],
           sgec_filt[2], sgec_filt[3], sgec_filt[4], sgec_filt[5]);
sgec_set_filter ();
}

/* Address filter (KA680 10.3.7 AF, 10.3.17).  There is no special case for
   the broadcast address: it is a multicast address.  After a reset the
   table is all zeros, so only frames to 00-00-00-00-00-00 pass until the
   first setup frame (10.3.17.1). */

static t_bool sgec_accept (const uint8 *dst)
{
int32 i, mc = dst[0] & 1;
uint32 h;

if (sgec_csr[6] & CSR6_AF_PROM)
    return TRUE;
if (mc && (sgec_csr[6] & CSR6_AF_ALLM))
    return TRUE;
if (sgec_hashmode) {
    if (!mc)
        return memcmp (dst, sgec_filt, 6) == 0;
    h = sgec_crc (dst, 6) & 0x1FF;                      /* CRC without the final complement */
    return (sgec_hash[h >> 3] >> (h & 7)) & 1;
    }
for (i = 0; i < SGEC_NFILT; i++) {
    if (memcmp (dst, sgec_filt + i * 6, 6) == 0)
        return !sgec_invfilt;
    }
return sgec_invfilt && !(sgec_csr[6] & (CSR6_AF_PROM | CSR6_AF_ALLM));
}

/* Tell the transport what to pass: a superset of what the chip accepts
   (the perfect addresses; every multicast in hash and all-multicast
   modes; everything when promiscuous or inverse filtering).  The chip's
   own filter is applied to every frame afterwards. */

static void sgec_set_filter (void)
{
ETH_MAC addrs[SGEC_NFILT];
int32 i, j, n = 0, nslot = sgec_hashmode? 1: SGEC_NFILT;
ETH_BOOL allm, prom;

if (sgec_eth == NULL)
    return;
for (i = 0; i < nslot; i++) {
    for (j = 0; j < n; j++) {
        if (memcmp (addrs[j], sgec_filt + i * 6, 6) == 0)
            break;
        }
    if (j == n)
        memcpy (addrs[n++], sgec_filt + i * 6, 6);
    }
allm = sgec_hashmode || ((sgec_csr[6] & CSR6_AF_ALLM) != 0);
prom = sgec_invfilt || ((sgec_csr[6] & CSR6_AF_PROM) != 0);
eth_filter (sgec_eth, n, addrs, allm, prom);
}

/* Boot message (KA680 10.3.6 BO, 10.3.7 BE, 10.3.12).  The manual gives
   only the registers; the frame recognised is a guess from the DNA MOP
   specification: a remote console BOOT message (protocol 60-02, after the
   2-byte length the function code 6, the 8-byte verification field whose
   first byte is VRF<7:0>, the processor byte).  The chip sets BO, which
   raises no interrupt, and pulses its BOOT_L pin, whose use on the KA49
   is not known and is not modelled; the frame is then filtered as any
   other. */

static void sgec_check_boot (const uint8 *fr, uint32 len)
{
uint32 i;
uint8 vrf[8];

if (((sgec_csr[6] & CSR6_BE) == 0) || (len < 26) ||
    (fr[12] != 0x60) || (fr[13] != 0x02) || (fr[16] != 6))
    return;
for (i = 0; i < 4; i++) {
    vrf[i] = (sgec_csr[11] >> (i * 8)) & 0xFF;
    vrf[i + 4] = (sgec_csr[12] >> (i * 8)) & 0xFF;
    }
if ((memcmp (fr + 17, vrf, 8) == 0) && (fr[25] == (sgec_csr[13] & 0xFF))) {
    sim_debug (DBG_RX, &sgec_dev, "boot message recognised\n");
    sgec_csr[5] |= CSR5_BO;
    sgec_stats.boot_msg++;
    }
}

/* Receive one frame (len bytes, the FCS included) into the receive list
   (KA680 10.3.18.4).  Returns FALSE, without consuming the frame, when the
   receiver is suspended and still has no descriptor; the caller decides
   whether the frame waits or is lost.  The data type in RDES0 is the
   operating mode (the chip cannot tell a looped frame from one off the
   wire, KA680 table 10-28 note). */

static t_bool sgec_receive (const uint8 *fr, uint32 len, uint32 dtype)
{
uint32 d[4], nd[4], cur, nxt, n, done = 0, rdes0, ftype;
int32 r;
SGEC_BUF b;

if (sgec_rxst == ST_STOP) {
    sgec_stats.rx_stopped++;
    return TRUE;
    }
sgec_check_boot (fr, len);
if (!sgec_accept (fr)) {
    sim_debug (DBG_RX, &sgec_dev, "frame to %02X-%02X-%02X-%02X-%02X-%02X rejected by the filter\n",
               fr[0], fr[1], fr[2], fr[3], fr[4], fr[5]);
    sgec_stats.rx_filtered++;
    return TRUE;
    }
cur = sgec_rxcur;
r = sgec_acquire (&cur, d);
if (r < 0)
    return TRUE;
sgec_rxcur = cur;
if (r == 0) {                                           /* still no buffer */
    if (sgec_rxst != ST_SUSP) {
        sim_debug (DBG_RX, &sgec_dev, "descriptor %08X not owned: receive suspended\n", cur);
        sgec_csr[5] |= CSR5_RU;
        sgec_rxst = ST_SUSP;
        }
    return FALSE;
    }
sgec_rxst = ST_RUN;
ftype = ((uint32) fr[12] << 8) | fr[13];
for (;;) {
    b.addr = d[3];
    b.offset = d[2] & 0x1FF;
    b.va = (d[1] & RDES1_VA) != 0;
    b.papte = (d[1] & RDES1_VT) != 0;
    n = (d[2] >> 16) & 0x7FFF;
    rdes0 = (dtype << RDES0_V_DT) | ((done == 0)? RDES0_FS: 0);
    if (n > len - done)
        n = len - done;
    if (!sgec_buf_write (&b, fr + done, n)) {           /* not mapped: the frame is lost */
        rdes0 |= RDES0_ES | RDES0_TN | RDES0_LS;
        sgec_stats.rx_err++;
        done = len;
        }
    else {
        done = done + n;
        if ((done < len) && (sgec_csr[6] & CSR6_DC)) {  /* one buffer per frame: truncate */
            rdes0 |= RDES0_BO;
            done = len;
            }
        if (done < len) {                               /* data chaining: the next buffer */
            nxt = cur + 16;
            r = sgec_acquire (&nxt, nd);
            if (r < 0)
                return TRUE;
            if (r == 0) {                               /* none: truncated */
                rdes0 |= RDES0_ES | RDES0_LE;
                sgec_stats.rx_err++;
                done = len;
                }
            else {
                sgec_wrl (cur, rdes0);                  /* release this one */
                sim_debug (DBG_RX, &sgec_dev, "descriptor %08X: %d bytes, RDES0 %04X\n", cur, n, rdes0);
                cur = nxt;
                memcpy (d, nd, sizeof (d));
                continue;
                }
            }
        rdes0 |= RDES0_LS;
        }
    if (ftype > 1500)
        rdes0 |= RDES0_FT;
    if (len > SGEC_MAXFR)
        rdes0 |= RDES0_ES | RDES0_TL;
    sgec_wrl (cur, ((len & DES_M_LEN) << DES_V_LEN) | rdes0);   /* status, OW cleared */
    sim_debug (DBG_RX, &sgec_dev, "descriptor %08X: %d bytes, frame of %d, RDES0 %04X\n",
               cur, n, len, rdes0);
    break;
    }
sgec_rxcur = cur + 16;
sgec_csr[5] |= CSR5_RI;
sgec_stats.rx_frames++;
sgec_stats.rx_bytes += len;
nxt = sgec_rxcur;                                       /* look ahead for the next frame */
r = sgec_acquire (&nxt, nd);
if (r == 0) {
    sgec_rxcur = nxt;
    sgec_csr[5] |= CSR5_RU;
    sgec_rxst = ST_SUSP;
    sim_debug (DBG_RX, &sgec_dev, "descriptor %08X not owned: receive suspended\n", nxt);
    }
else if (r > 0)
    sgec_rxcur = nxt;
return TRUE;
}

/* A frame looped back to the receiver: lost if there is no buffer */

static void sgec_loopback (uint8 *fr, uint32 len, uint32 mode)
{
sgec_stats.tx_loop++;
if (!sgec_receive (fr, len, mode)) {
    sgec_csr[10] = (sgec_csr[10] & ~0xFFFFu) | ((sgec_csr[10] + 1) & 0xFFFF);
    sgec_stats.rx_missed++;
    }
}

/* Frames from the network, from the read queue into the receive list.  A
   frame that finds no buffer waits in the queue (bounded): the host can
   deliver frames much faster than a wire, and a guest that is merely slow
   to return buffers should not lose them.  Frames lost when the queue
   overflows are counted as missed (NICSR10). */

void sgec_read_callback (int status)
{
if (DBG_PCK & sgec_dev.dctrl)
    eth_packet_trace_ex (sgec_eth, sgec_rdpkt.msg, sgec_rdpkt.len, "sgec-recvd", DBG_DAT & sgec_dev.dctrl, DBG_PCK);
ethq_insert (&sgec_readq, ETH_ITM_NORMAL, &sgec_rdpkt, status);
}

static void sgec_rx_pump (void)
{
ETH_ITEM *item;
uint32 len, mode, lost;

lost = (uint32) sgec_readq.loss - sgec_qloss;
if (lost) {
    sgec_qloss = (uint32) sgec_readq.loss;
    sgec_csr[10] = (sgec_csr[10] & ~0xFFFFu) | ((sgec_csr[10] + lost) & 0xFFFF);
    sgec_stats.rx_missed += lost;
    }
mode = (sgec_csr[6] >> CSR6_V_OM) & CSR6_M_OM;
while (sgec_readq.count > 0) {
    item = &sgec_readq.item[sgec_readq.head];
    if ((sgec_rxst == ST_STOP) || (mode == OM_ILBK)) {  /* not listening to the wire */
        sgec_stats.rx_stopped++;
        ethq_remove (&sgec_readq);
        continue;
        }
    len = item->packet.len;
    if (len > ETH_MAX_PACKET)
        len = ETH_MAX_PACKET;
    memcpy (sgec_rxbuf, item->packet.msg, len);
    if (len < SGEC_MINFR) {                             /* pad, as a sender on a wire */
        memset (sgec_rxbuf + len, 0, SGEC_MINFR - len);
        len = SGEC_MINFR;
        }
    len = sgec_add_fcs (sgec_rxbuf, len);
    if (!sgec_receive (sgec_rxbuf, len, (mode == OM_ELBK)? OM_ELBK: OM_NORM))
        break;                                          /* no buffer: wait */
    ethq_remove (&sgec_readq);
    }
}

/* Send a frame (len bytes, the FCS included) on the network.  Returns
   the TDES0 status bits. */

static uint32 sgec_send (uint8 *fr, uint32 len)
{
t_stat r;

if (sgec_eth == NULL)                                   /* no cable */
    return TDES0_ES | TDES0_LO | TDES0_NC;
sgec_wrpkt.len = len - 4;                               /* the host network carries no FCS */
if (sgec_wrpkt.len > ETH_MAX_PACKET) {                  /* longer than the host allows */
    sgec_wrpkt.len = ETH_MAX_PACKET;
    sgec_stats.tx_host_err++;
    }
memcpy (sgec_wrpkt.msg, fr, sgec_wrpkt.len);
if (DBG_PCK & sgec_dev.dctrl)
    eth_packet_trace_ex (sgec_eth, sgec_wrpkt.msg, sgec_wrpkt.len, "sgec-write", DBG_DAT & sgec_dev.dctrl, DBG_PCK);
r = eth_write (sgec_eth, &sgec_wrpkt, NULL);
if (r != SCPE_OK)                                       /* the host failed: the frame */
    sgec_stats.tx_host_err++;                           /* is lost, as on a busy wire */
return (sgec_unit[0].flags & UNIT_NOHB)? TDES0_HF: 0;
}

/* Transmit process (KA680 10.3.18.5): runs the transmit list until a
   descriptor the chip does not own, then suspends.  TI is set by a frame
   with IC and whenever the process suspends (the end of the list or an
   error); that rule gives exactly the interrupt counts the KA49 console's
   interrupt test demands (two during initialisation, one for a
   looped-back frame). */

static void sgec_tx_suspend (uint32 cur, uint32 tdes0)
{
if (tdes0)
    sgec_wrl (cur, tdes0);
sgec_txst = ST_SUSP;
sgec_csr[5] |= CSR5_TI;
}

static void sgec_transmit (void)
{
uint32 d[4], cur, n, len = 0, tdes0, dt, mode;
uint8 sf[SGEC_SETUPSZ];
t_bool inframe = FALSE, err = FALSE, ac = FALSE;
int32 r;
SGEC_BUF b;

while ((sgec_txst == ST_RUN) && ((sgec_csr[6] & CSR6_ST) != 0)) {
    cur = sgec_txcur;
    r = sgec_acquire (&cur, d);
    sgec_txcur = cur;
    if (r < 0)
        break;
    if (r == 0) {                                       /* end of the list */
        sim_debug (DBG_TX, &sgec_dev, "descriptor %08X not owned: transmit suspended%s\n",
                   cur, inframe? " in the middle of a frame": "");
        sgec_tx_suspend (cur, 0);                       /* (a frame cut short is not */
        break;                                          /* reported: drivers never do it) */
        }
    dt = (d[1] >> TDES1_V_DT) & TDES1_M_DT;
    n = (d[2] >> 16) & 0x7FFF;
    b.addr = d[3];
    b.offset = d[2] & 0x1FF;
    b.va = (d[1] & TDES1_VA) != 0;
    b.papte = (d[1] & TDES1_VT) != 0;
    if ((dt != DT_NORM) && inframe) {                   /* setup or diagnostic inside a frame */
        sgec_txcur = cur + 16;
        sgec_tx_suspend (cur, TDES0_ES | TDES0_LE);
        sgec_stats.tx_err++;
        break;
        }
    if (dt == DT_SETUP) {                               /* setup frame: always physical */
        sgec_txcur = cur + 16;
        sgec_stats.setup++;
        if (n != SGEC_SETUPSZ)
            tdes0 = TDES0_ES | SDES0_SE;
        else {
            b.va = FALSE;
            if (sgec_buf_read (&b, sf, SGEC_SETUPSZ)) {
                sgec_load_setup (sf, (d[1] & TDES1_HP) != 0, (d[1] & TDES1_IF) != 0);
                tdes0 = 0;
                }
            else {
                sgec_memerr (d[3]);
                break;
                }
            }
        sgec_wrl (cur, tdes0);
        if (d[1] & TDES1_IC)
            sgec_csr[5] |= CSR5_TI;
        continue;
        }
    if (dt == DT_DIAG) {
        /* A diagnostic descriptor loads code into the SGEC's internal
           processor's RAM, a software patch used with the breakpoint
           register NICSR14 (KA680 10.3.13.1); VMS gives one to a
           revision 3 chip.  The internal processor is not modelled: the
           descriptor completes and nothing is sent. */
        sgec_txcur = cur + 16;
        sgec_wrl (cur, 0);
        if (d[1] & TDES1_IC)
            sgec_csr[5] |= CSR5_TI;
        continue;
        }
    if (((d[1] & TDES1_FS) != 0) == inframe) {          /* FS and LS out of order */
        sim_debug (DBG_TX, &sgec_dev, "descriptor %08X: FS/LS out of order\n", cur);
        sgec_txcur = cur + 16;
        sgec_tx_suspend (cur, TDES0_ES | TDES0_LE);
        sgec_stats.tx_err++;
        break;
        }
    if (d[1] & TDES1_FS) {                              /* a new frame */
        inframe = TRUE;
        len = 0;
        err = FALSE;
        ac = (d[1] & TDES1_AC) != 0;
        }
    if (len + n > SGEC_MAXFR)                           /* too long: keep what fits */
        n = SGEC_MAXFR - len;
    if (!sgec_buf_read (&b, sgec_frame + len, n))       /* (a zero size skips the buffer) */
        err = TRUE;
    len = len + n;
    sgec_txcur = cur + 16;
    if ((d[1] & TDES1_LS) == 0) {                       /* more to come */
        sgec_wrl (cur, 0);
        continue;
        }
    inframe = FALSE;
    if (err) {                                          /* a buffer not mapped */
        sim_debug (DBG_TX, &sgec_dev, "descriptor %08X: translation not valid\n", cur);
        sgec_tx_suspend (cur, TDES0_ES | TDES0_TN);
        sgec_stats.tx_err++;
        break;
        }
    if (len == 0) {                                     /* suppressed frame */
        sgec_wrl (cur, 0);
        continue;
        }
    if ((len < SGEC_MINFR + 4) || !ac) {                /* pad and FCS (always if short) */
        if (len < SGEC_MINFR) {
            memset (sgec_frame + len, 0, SGEC_MINFR - len);
            len = SGEC_MINFR;
            }
        len = sgec_add_fcs (sgec_frame, len);
        }
    mode = (sgec_csr[6] >> CSR6_V_OM) & CSR6_M_OM;
    if (mode == OM_ILBK) {                              /* internal loopback */
        tdes0 = TDES0_LO | TDES0_NC;                    /* (meaningless; the console expects them) */
        if (sgec_csr[6] & CSR6_FC)                      /* forced collisions: 16 attempts */
            tdes0 |= TDES0_EC;
        else sgec_loopback (sgec_frame, len, OM_ILBK);
        }
    else {
        tdes0 = sgec_send (sgec_frame, len);
        if (mode == OM_ELBK)                            /* external loopback: also heard */
            sgec_loopback (sgec_frame, len, OM_ELBK);
        }
    if (tdes0 & (TDES0_LE | TDES0_LO | TDES0_NC | TDES0_EC | TDES0_TN)) {
        tdes0 |= TDES0_ES;
        sgec_stats.tx_err++;
        }
    sim_debug (DBG_TX, &sgec_dev, "descriptor %08X: frame of %d bytes, TDES0 %04X\n", cur, len, tdes0);
    sgec_wrl (cur, tdes0);
    sgec_stats.tx_frames++;
    sgec_stats.tx_bytes += len;
    if (d[1] & TDES1_IC)
        sgec_csr[5] |= CSR5_TI;
    }
}

/* Interrupt request: an event bit of CSR5 <6:1> and IE (KA680 10.3.18.2) */

static void sgec_update_int (void)
{
int32 irq = ((sgec_csr[5] & CSR5_INT) != 0) && ((sgec_csr[6] & CSR6_IE) != 0);

if (irq != sgec_irq)
    sim_debug (DBG_INT, &sgec_dev, "request %s (CSR5 %08X)\n", irq? "set": "cleared", sgec_csr[5]);
sgec_irq = irq;
if (irq)
    SGEC_SET_INT;
else
    SGEC_CLR_INT;
}

int32 sgec_vector (void)
{
return (int32) (sgec_csr[0] & CSR0_VEC);
}

/* End of the self-test, which always passes; no interrupt (KA680
   10.3.6.1) */

t_stat sgec_stsvc (UNIT *uptr)
{
sgec_csr[5] |= CSR5_ID;
return SCPE_OK;
}

/* The DMA engines and the network.  Attached, the unit is woken by the
   transport's reader thread when frames arrive, or polled at the clock
   rate when the transport cannot do that; it also polls while frames
   wait in the queue for a receive buffer. */

t_stat sgec_svc (UNIT *uptr)
{
if (sgec_eth != NULL) {
    while (eth_read (sgec_eth, &sgec_rdpkt, &sgec_read_callback))
        ;
    }
if (sgec_txst == ST_RUN)
    sgec_transmit ();
if (sgec_readq.count > 0)
    sgec_rx_pump ();
sgec_update_int ();
if ((sgec_eth != NULL) && (sgec_must_poll || (sgec_readq.count > 0)))
    sim_clock_coschedule (uptr, tmxr_poll);
return SCPE_OK;
}

static void sgec_kick (void)
{
sim_activate_abs (&sgec_unit[0], SGEC_DELAY);
}

/* Receiver: try to acquire a descriptor (start, poll demand) */

static void sgec_rx_poll (void)
{
uint32 d[4], cur = sgec_rxcur;
int32 r = sgec_acquire (&cur, d);

if (r < 0)
    return;
sgec_rxcur = cur;
sgec_rxst = r? ST_RUN: ST_SUSP;
if (r && (sgec_readq.count > 0))
    sgec_kick ();
}

/* CSR read.  The virtual CSRs 8-14 are read twice by the handshake (read,
   wait for DN, read again; KA680 10.3.2.2); DN is always set here, and the
   first read latches the value so that the second returns the same one
   (NICSR10's missed-frame count clears when read). */

static uint32 sgec_vread (int32 rg)
{
uint32 val;

switch (rg) {

    case 8:                                             /* reserved */
        val = 0;
        break;

    case 10:                                            /* revision, missed frames */
        val = CSR10_REV | (sgec_csr[10] & 0xFFFF);
        sgec_csr[10] = 0;                               /* the count clears when read */
        break;

    default:
        val = sgec_csr[rg];
        break;
        }
return val;
}

int32 sgec_rd (int32 pa)
{
int32 rg = (pa >> 2) & (SGEC_NCSR - 1);
uint32 val;

switch (rg) {

    case 0:
        val = (sgec_csr[0] & CSR0_RW) | CSR0_MBO;
        break;

    case 1: case 2:                                     /* poll demands (the real machine) */
        val = 0xFFFFFFFF;
        break;

    case 5:
        val = (sgec_csr[5] & (CSR5_ID | CSR5_SF | CSR5_SS | CSR5_W1C)) |
              ((uint32) sgec_txst << CSR5_V_TS) | ((uint32) sgec_rxst << CSR5_V_RS) |
              (((sgec_csr[6] >> CSR6_V_OM) & CSR6_M_OM) << CSR5_V_OM) |
              CSR5_DN | CSR5_MBO;
        if (val & CSR5_INT)
            val |= CSR5_IS;
        break;

    case 6:
        val = (sgec_csr[6] & CSR6_RW) | CSR6_MBO | (sgec_re? CSR6_RE: 0);
        break;

    case 8: case 9: case 10: case 11: case 12: case 13: case 14:
        if ((sgec_vrg == rg) && ((sim_gtime () - sgec_vtime) < SGEC_VLATCH))
            val = sgec_vval;                            /* the handshake's second read */
        else {
            val = sgec_vval = sgec_vread (rg);
            sgec_vtime = sim_gtime ();
            }
        sgec_vrg = (sgec_vrg == rg)? -1: rg;
        break;

    default:
        val = sgec_csr[rg];
        break;
        }
sim_debug (DBG_REG, &sgec_dev, "CSR%d read %08X, PC %08X\n", rg, val, fault_PC);
return (int32) val;
}

/* CSR write.  The write rules of KA680 10.3 are followed for the commands
   (a start command starts a stopped process only; list addresses are
   taken only while the process is stopped or suspended); the mode fields
   of CSR6 and the watchdogs take effect whenever written (guess: the
   KA49 console changes the address filter and the operating mode while
   the processes run and expects the change to take effect). */

void sgec_wr (int32 pa, int32 val, int32 lnt)
{
int32 rg = (pa >> 2) & (SGEC_NCSR - 1);
uint32 v = (uint32) val;
uint32 old;

if (lnt < L_LONG) {                                     /* merge a partial write */
    int32 sc = (pa & 3) << 3;
    uint32 mask = ((lnt == L_WORD)? 0xFFFFu: 0xFFu) << sc;
    v = ((v << sc) & mask) | ((uint32) sgec_rd (pa & ~3) & ~mask);
    if (rg == 5)                                        /* do not clear what was not written */
        v = v & mask;
    }
sim_debug (DBG_REG, &sgec_dev, "CSR%d write %08X, PC %08X\n", rg, v, fault_PC);
if ((rg >= 8) && (rg <= 14))
    sgec_vrg = -1;
switch (rg) {

    case 0:
        sgec_csr[0] = v & CSR0_RW;
        break;

    case 1:                                             /* transmit poll demand */
        if (sgec_txst != ST_STOP) {
            sgec_txst = ST_RUN;
            sgec_kick ();
            }
        break;

    case 2:                                             /* receive poll demand */
        if (sgec_rxst == ST_SUSP)
            sgec_rx_poll ();
        break;

    case 3:                                             /* receive list */
        sgec_csr[3] = v & CSR_ADDR;
        if (sgec_rxst != ST_RUN) {
            sgec_rxcur = sgec_csr[3];
            sgec_csr3w = 1;
            }
        break;

    case 4:                                             /* transmit list */
        sgec_csr[4] = v & CSR_ADDR;
        if (sgec_txst != ST_RUN) {
            sgec_txcur = sgec_csr[4];
            sgec_csr4w = 1;
            }
        break;

    case 5:                                             /* status: write one to clear */
        sgec_csr[5] = sgec_csr[5] & ~(v & CSR5_W1C);
        break;

    case 6:
        if (v & CSR6_RE) {                              /* reset */
            sgec_chip_reset ();
            sgec_re = 1;
            break;
            }
        sgec_re = 0;
        old = sgec_csr[6];
        sgec_csr[6] = v & CSR6_RW;
        if (v & CSR6_ST) {                              /* start transmission */
            if ((sgec_txst == ST_STOP) && sgec_csr4w) {
                sgec_txst = ST_RUN;
                sgec_kick ();
                }
            }
        else sgec_txst = ST_STOP;
        if (v & CSR6_SR) {                              /* start reception */
            if ((sgec_rxst == ST_STOP) && sgec_csr3w)
                sgec_rx_poll ();
            }
        else sgec_rxst = ST_STOP;
        if ((old ^ sgec_csr[6]) & (CSR6_AF_PROM | CSR6_AF_ALLM))
            sgec_set_filter ();
        break;

    case 7:                                             /* system page table */
        sgec_csr[7] = v & CSR_ADDR;
        break;

    case 8: case 10:                                    /* reserved; read only */
        break;

    case 11: case 12:                                   /* boot message, while BE is clear */
        if ((sgec_csr[6] & CSR6_BE) == 0)
            sgec_csr[rg] = v;
        break;

    case 13:
        if ((sgec_csr[6] & CSR6_BE) == 0)
            sgec_csr[13] = v & CSR13_RW;
        break;

    default:                                            /* 9, 14, 15 */
        sgec_csr[rg] = v;
        break;
        }
sgec_update_int ();
}

/* Chip reset (CSR6<RE>, or the hardware reset).  The KA680 manual,
   10.3.18.1 and table 10-39: the registers clear except NICSR3, 4 and 7
   (unpredictable; kept here), NICSR5<DN> (1), NICSR6<BL> (1), NICSR6<RE>
   (1 after a software reset) and NICSR9 (RT = TT = 1250); the address
   filter is all zeros; then the self-test runs for 25 ms and sets
   NICSR5<ID> (here it always passes).  Both processes stop; frames waiting
   for the receiver are dropped. */

static void sgec_chip_reset (void)
{
sim_cancel (&sgec_unit[0]);
sim_cancel (&sgec_unit[1]);
sgec_csr[0] = 0;
sgec_csr[5] = 0;
sgec_csr[6] = CSR6_BL1;
sgec_csr[9] = CSR9_INIT;
sgec_csr[10] = 0;
sgec_csr[11] = sgec_csr[12] = sgec_csr[13] = sgec_csr[14] = 0;
sgec_csr[15] = CSR15_INIT;
sgec_re = 0;
sgec_vrg = -1;
sim_activate (&sgec_unit[1], SGEC_SELFTEST);
sgec_txst = sgec_rxst = ST_STOP;
sgec_csr3w = sgec_csr4w = 0;
sgec_rxcur = sgec_csr[3];
sgec_txcur = sgec_csr[4];
sgec_hashmode = sgec_invfilt = 0;
memset (sgec_filt, 0, sizeof (sgec_filt));
memset (sgec_hash, 0, sizeof (sgec_hash));
if (sgec_eth != NULL) {
    ethq_clear (&sgec_readq);
    sgec_qloss = 0;
    sgec_set_filter ();
    if (sgec_must_poll)
        sim_clock_coschedule (&sgec_unit[0], tmxr_poll);
    }
}

t_stat sgec_reset (DEVICE *dptr)
{
sgec_crc_init ();
memset (sgec_csr, 0, sizeof (sgec_csr));
sgec_chip_reset ();
sgec_update_int ();
return SCPE_OK;
}

/* Attach to a network through sim_ether */

t_stat sgec_attach (UNIT *uptr, CONST char *cptr)
{
t_stat r;
ETH_MAC mac;
char *tptr;

if (uptr != &sgec_unit[0])
    return SCPE_NOATT;
if (uptr->flags & UNIT_ATT)
    return SCPE_ALATT;
tptr = (char *) malloc (strlen (cptr) + 1);
sgec_eth = (ETH_DEV *) calloc (1, sizeof (ETH_DEV));
if ((tptr == NULL) || (sgec_eth == NULL)) {
    free (tptr);
    free (sgec_eth);
    sgec_eth = NULL;
    return SCPE_MEM;
    }
strcpy (tptr, cptr);
r = eth_open (sgec_eth, cptr, &sgec_dev, DBG_ETH);
if (r == SCPE_OK) {
    SGEC_GET_MAC (mac);
    r = eth_check_address_conflict (sgec_eth, mac);
    if (r != SCPE_OK)
        eth_close (sgec_eth);
    }
if (r == SCPE_OK) {
    r = ethq_init (&sgec_readq, SGEC_QUE_MAX);
    if (r != SCPE_OK)
        eth_close (sgec_eth);
    }
if (r != SCPE_OK) {
    free (tptr);
    free (sgec_eth);
    sgec_eth = NULL;
    return r;
    }
sgec_must_poll = (eth_set_async (sgec_eth, 0) != SCPE_OK);
ethq_clear (&sgec_readq);
sgec_qloss = 0;
uptr->filename = tptr;
uptr->flags |= UNIT_ATT;
sgec_set_filter ();
sim_clock_coschedule (uptr, tmxr_poll);
return SCPE_OK;
}

t_stat sgec_detach (UNIT *uptr)
{
if ((uptr->flags & UNIT_ATT) == 0)
    return SCPE_OK;
sim_cancel (uptr);
eth_close (sgec_eth);
free (sgec_eth);
sgec_eth = NULL;
ethq_destroy (&sgec_readq);
free (uptr->filename);
uptr->filename = NULL;
uptr->flags &= ~UNIT_ATT;
if (sgec_txst == ST_RUN)                                /* finish what was started */
    sgec_kick ();
return SCPE_OK;
}

/* The station address lives in the board's network address ROM, which
   the model owns; the SGEC itself learns it only from setup frames */

t_stat sgec_setmac (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
ETH_MAC mac;
t_stat r;

if (cptr == NULL)
    return SCPE_ARG;
if (sgec_unit[0].flags & UNIT_ATT)
    return SCPE_ALATT;
r = eth_mac_scan_ex (mac, cptr, uptr);
if (r != SCPE_OK)
    return r;
SGEC_SET_MAC (mac);
return SCPE_OK;
}

t_stat sgec_showmac (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
ETH_MAC mac;
char buf[20];

SGEC_GET_MAC (mac);
eth_mac_fmt (mac, buf);
fprintf (st, "MAC=%s", buf);
return SCPE_OK;
}

t_stat sgec_showfilt (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
char buf[20];
int32 i;

fprintf (st, "Address filter (%s%s), mode %s\n", sgec_hashmode? "hash": "perfect",
         sgec_invfilt? ", inverse": "",
         (sgec_csr[6] & CSR6_AF_PROM)? "promiscuous": (sgec_csr[6] & CSR6_AF_ALLM)? "all multicast": "normal");
for (i = 0; i < (sgec_hashmode? 1: SGEC_NFILT); i++) {
    eth_mac_fmt (sgec_filt + i * 6, buf);
    fprintf (st, "  [%2d] %s\n", i, buf);
    }
if (sgec_hashmode) {
    fprintf (st, "  hash");
    for (i = 0; i < 64; i++)
        fprintf (st, "%s%02X", (i & 15)? "": "\n   ", sgec_hash[i]);
    fprintf (st, "\n");
    }
return SCPE_OK;
}

t_stat sgec_setstats (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
memset (&sgec_stats, 0, sizeof (sgec_stats));
return SCPE_OK;
}

t_stat sgec_showstats (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
const char *fmt = "  %-26s%u\n";

fprintf (st, "SGEC statistics:\n");
fprintf (st, fmt, "Frames sent:", sgec_stats.tx_frames);
fprintf (st, fmt, "Bytes sent:", sgec_stats.tx_bytes);
fprintf (st, fmt, "Frames looped back:", sgec_stats.tx_loop);
fprintf (st, fmt, "Send errors (TDES0<ES>):", sgec_stats.tx_err);
fprintf (st, fmt, "Host send failures:", sgec_stats.tx_host_err);
fprintf (st, fmt, "Setup frames:", sgec_stats.setup);
fprintf (st, fmt, "Frames received:", sgec_stats.rx_frames);
fprintf (st, fmt, "Bytes received:", sgec_stats.rx_bytes);
fprintf (st, fmt, "Rejected by the filter:", sgec_stats.rx_filtered);
fprintf (st, fmt, "Missed (no buffer):", sgec_stats.rx_missed);
fprintf (st, fmt, "Receive errors:", sgec_stats.rx_err);
fprintf (st, fmt, "Arrived, reception off:", sgec_stats.rx_stopped);
fprintf (st, fmt, "Boot messages:", sgec_stats.boot_msg);
if (sgec_eth != NULL) {
    fprintf (st, fmt, "Waiting for a buffer:", (uint32) sgec_readq.count);
    fprintf (st, fmt, "Most ever waiting:", (uint32) sgec_readq.high);
    eth_show_dev (st, sgec_eth);
    }
return SCPE_OK;
}

t_stat sgec_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
fprintf (st, "SGEC (DC541) Ethernet controller (%s)\n\n", dptr->name);
fprintf (st, "The SGEC is the Ethernet controller of the NVAX-based VAXstation 4000\n");
fprintf (st, "Model 90 and 96 (VMS device EZA0).  Attached to a network (ATTACH SGEC),\n");
fprintf (st, "it sends and receives frames; not attached, frames sent in normal mode\n");
fprintf (st, "complete with loss of carrier and no carrier, as on a machine whose cable\n");
fprintf (st, "is unplugged.  Internal and external loopback work either way, through the\n");
fprintf (st, "chip's own address filter.\n\n");
fprintf (st, "The station address is the one in the network address ROM, which the\n");
fprintf (st, "console and the operating system read and give to the chip in a setup\n");
fprintf (st, "frame; SET SGEC MAC=xx-xx-xx-xx-xx-xx changes it (before ATTACH).  Two\n");
fprintf (st, "simulators on the same network need different addresses.\n\n");
fprintf (st, "The transmit status reports heartbeat failure on every frame sent, as\n");
fprintf (st, "the transceiver of the real VAXstation 4000/90 it was compared with does\n");
fprintf (st, "(it sends no SQE test; VMS counts \"collision detect check failure\", which\n");
fprintf (st, "is harmless).  SET SGEC HEARTBEAT reports a transceiver that sends one.\n\n");
fprint_set_help (st, dptr);
fprint_show_help (st, dptr);
fprint_reg_help (st, dptr);
sgec_help_attach (st, dptr, uptr, flag, cptr);
return SCPE_OK;
}

t_stat sgec_help_attach (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
fprintf (st, "\nTo reach a network, attach the SGEC to a host transport:\n\n");
fprintf (st, "   sim> SHOW SGEC ETH                          (what can be attached)\n");
fprintf (st, "   sim> ATTACH SGEC nat:tcp=2323:10.0.2.15:23  (SLiRP NAT, no privileges)\n");
fprintf (st, "   sim> ATTACH SGEC tap:tap0\n\n");
return eth_attach_help (st, dptr, uptr, flag, cptr);
}

const char *sgec_description (DEVICE *dptr)
{
return "SGEC Ethernet controller";
}
