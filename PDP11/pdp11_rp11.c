/* pdp11_rp11.c: RP11-C/RP02/RP03 disk pack device

   Copyright (c) 2022 Tony Lawrence

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
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.

   Inspired by PDP-11's RK/RL implementations by Robert Supnik.
*/

#if defined(VM_PDP11)  &&  !defined(UC15)

#include <assert.h>

#include "pdp11_defs.h"
#include "sim_disk.h"

/* Constants */

#define RPCONTR         uint16
#define RPWRDSZ         16
#define MAP_RDW(a,c,b)  (Map_ReadW (a, (c) << 1, b) >> 1)
#define MAP_WRW(a,c,b)  (Map_WriteW(a, (c) << 1, b) >> 1)

/* RP02 parameters; PR03 doubles # of cylinders (both total and spare) */
#define RP_NUMWD        256                             /* words/sector */
#define RP_NUMCY        203                             /* cylinders/drive */
#define RP_SPARE        3                               /* of those, spare */
#define RP_NUMSF        20                              /* surfaces/cylinder */
#define RP_NUMSC        10                              /* sectors/track */
#define RP_NUMTR        (RP_NUMCY * RP_NUMSF)           /* tracks/drive */
#define RP_NUMBL        (RP_NUMTR * RP_NUMSC)           /* blocks/drive */
#define RP_NUMDR        8                               /* drives/controller */
#define RP_MAXFR        (1 << 16)                       /* max transfer */
#define RP_SIZE(n)      (RP_NUMWD * (n))                /* words in n blocks */
#define RP_SIZE_RP02    RP_SIZE(RP_NUMBL)               /* RP02 capacity, words */
#define RP_SIZE_RP03    RP_SIZE(RP_NUMBL*2)             /* RP03 capacity, words */
#define RP_ROT_12       125                             /* Half rotation, 0.1ms */

/* Flags in the unit flags word */
#define UNIT_NOAUTO     DKUF_NOAUTOSIZE                 /* autosize disabled */
#define UNIT_V_RP03     (DKUF_V_UF + 0)
#define UNIT_RP03       (1 << UNIT_V_RP03)              /* RP03 vs RP02 (0) */
#define GET_DTYPE(x)    (((x) & UNIT_RP03) >> UNIT_V_RP03)

/* Controller / drive types */
#define RP_RP11C        "RP11C"
#define RP_RP02         "RP02"
#define RP_RP03         "RP03"

/* Parameters in the unit descriptor */
#define CYL             u3                              /* current cylinder */
#define FUNC            u4                              /* function */
#define HEAD            u5                              /* current track */
#define SEEKING         u6                              /* unit performing a seek */

/* 12 UNIBUS registers */
#define RP_IOLN         030

/* RP02/RP03 particulars */
static struct drv_typ {
    const char*         name;                           /* device type name */
    int32               cyl;                            /* cylinders */
    int32               size;                           /* #blocks */
    int32               spare;                          /* spare (out of cyl) */
    int32               seek_1;                         /* one track move, 0.1ms */
    int32               seek_ave;                       /* average seek, 0.1ms */
    int32               seek_max;                       /* maximal seek, 0.1ms */
} drv_tab[] = {
    { RP_RP02, RP_NUMCY,   RP_NUMBL,   RP_SPARE,   200, 500, 800 },
    { RP_RP03, RP_NUMCY*2, RP_NUMBL*2, RP_SPARE*2,  75, 290, 550 }
};

/* RPDS 776710, selected drive status, read-only except for the attention bits */
static BITFIELD rp_ds_bits[] = {
#define RPDS_ATTN       0000377                         /* attention (read/clear) */
    BITF(ATTN,8),
#define RPDS_WLK        0000400                         /* write locked */
    BIT(WLK),
#define RPDS_UNSAFE     0001000                         /* unsafe */
    BIT(UNSAFE),
#define RPDS_SEEK       0002000                         /* seek underway */
    BIT(SEEK),
#define RPDS_INC        0004000                         /* seek incomplete */
    BIT(INC),
#define RPDS_HNF        0010000                         /* header not found */
    BIT(HNF),
#define RPDS_RP03       0020000                         /* drive is RP03 */
    BIT(RP03=),
#define RPDS_ONLN       0040000                         /* unit online */
    BIT(ONLN),
#define RPDS_RDY        0100000                         /* unit ready */
    BIT(RDY),
    ENDBITS
};
#define RPDS_DKER(x)    ((x) & (RPDS_HNF | RPDS_INC) ? RPER_DRE : 0)

/* RPER 776712, error register, read-only */
static BITFIELD rp_er_bits[] = {
#define RPER_DRE        0000001                         /* drive error (HNF|INC) */
    BIT(DRE),
#define RPER_EOP        0000002                         /* end of pack (overrun) */
    BIT(EOP),
#define RPER_NXM        0000004                         /* nx memory */
    BIT(NXM),
#define RPER_WCE        0000010                         /* write check error */
    BIT(WCE),
#define RPER_TE         0000020                         /* timing error */
    BIT(TE),
#define RPER_CSE        0000040                         /* serial checksum error */
    BIT(CSE),
#define RPER_WPE        0000100                         /* word parity error */
    BIT(WPE),
#define RPER_LPE        0000200                         /* longitudinal parity error */
    BIT(LPE),
#define RPER_MODE       0000400                         /* mode error */
    BIT(MODE),
#define RPER_FMTE       0001000                         /* format error */
    BIT(FMTE),
#define RPER_PGE        0002000                         /* programming error */
    BIT(PGE), 
#define RPER_NXS        0004000                         /* nx sector */
    BIT(NXS),
#define RPER_NXT        0010000                         /* nx track */
    BIT(NXT),
#define RPER_NXC        0020000                         /* nx cylinder */
    BIT(NXC),
#define RPER_FUV        0040000                         /* unsafe violation */
    BIT(FUV),
#define RPER_WPV        0100000                         /* write lock violation */
    BIT(WPV),
    ENDBITS
};
#define RPER_REAL       0177776
/* hard errors: drawing 19 */
#define RPER_HARDERR    (RPER_WPV | RPER_FUV | RPER_NXC | RPER_NXT | \
                         RPER_NXS | RPER_PGE | RPER_NXM | RPER_DRE | RPER_MODE)
/* soft errors: drawing 19 */
#define RPER_SOFTERR    (RPER_LPE | RPER_WPE | RPER_CSE | RPER_WCE | \
                         RPER_EOP | RPER_TE  | RPER_FMTE)
#define RPER_HARD(x)    ((x) & RPER_HARDERR ? (RPCS_ERR | RPCS_HERR) : 0)
#define RPER_SOFT(x)    ((x) & RPER_SOFTERR ?  RPCS_ERR              : 0)

/* RPCS 776714, command/status register */
static const char* rp_funcs[] = {
    "RESET", "WRITE", "READ", "WCHK", "SEEK", "WRNOSEEK", "HOME", "RDNOSEEK"
};

static BITFIELD rp_cs_bits[] = {
/* CSR_GO */                                            /* the GO! bit */
    BIT(GO),
#define RPCS_V_FUNC     1
#define RPCS_M_FUNC     7
#define RPCS_FUNC       (RPCS_M_FUNC << RPCS_V_FUNC)    /* function */
#define  RPCS_RESET     0
#define  RPCS_WRITE     1
#define  RPCS_READ      2
#define  RPCS_WCHK      3
#define  RPCS_SEEK      4
#define  RPCS_WR_NOSEEK 5
#define  RPCS_HOME      6
#define  RPCS_RD_NOSEEK 7
    BITFNAM(FUNC,3,rp_funcs),
#define RPCS_V_MEX      4
#define RPCS_M_MEX      3
#define RPCS_MEX        (RPCS_M_MEX << RPCS_V_MEX)      /* memory extension */
    BITF(MEX,2),
/* CSR_IE */                                            /* interrupt enable */
    BIT(IE),
/* CSR_DONE */                                          /* controller ready */
    BIT(DONE),
#define RPCS_V_DRV      8
#define RPCS_M_DRV      7
#define RPCS_DRV        (RPCS_M_DRV << RPCS_V_DRV)      /* drive id */
    BITFFMT(DRV,3,%u),
#define RPCS_HDR        0004000                         /* header operation */
    BIT(HDR),
#define RPCS_MODE       0010000                         /* 0=PDP-11; 1=PDP-10/-15 or format */
    BIT(MODE),
#define RPCS_AIE        0020000                         /* attention interrupt enable */
    BIT(AIE),
#define RPCS_HERR       0040000                         /* hard error */
    BIT(HERR),
#define RPCS_ERR        CSR_ERR                         /* error (hard or soft) */
    BIT(ERR),
#define RPCS_REAL       0037776                         /* bits kept here */
#define RPCS_RW         0037576                         /* read/write */
#define GET_FUNC(x)     (((x) & RPCS_FUNC) >> RPCS_V_FUNC)
#define GET_DRIVE(x)    (((x) & RPCS_DRV) >> RPCS_V_DRV)
    ENDBITS
};

/* RPWC 776716, two's complement word count */
/* For PDP-11 must be even for data, and in multiples of 3 for format */
static BITFIELD rp_wc_bits[] = {
    BITFFMT(WC,16,%u),
    ENDBITS
};

/* RPBA 776720, bus address */
static BITFIELD rp_ba_bits[] = {
#define RPBA_IMP        0177776                         /* implemented */
    BITF(BA,16),
    ENDBITS
};

/* RPCA 776722, cylinder address */
static BITFIELD rp_ca_bits[] = {
#define RPCA_IMP        0000777                         /* implemented */
    BITFFMT(CYL,9,%u),
    ENDBITS
};

/* RPDA 776724, disk address (track/sector) */
static BITFIELD rp_da_bits[] = {
#define RPDA_IMPL       0017777                         /* implemented */
#define RPDA_RW         0017417                         /* bits here */
#define RPDA_M_SECT     017
#define RPDA_SECT       RPDA_M_SECT                     /* sector */
    BITFFMT(SECT,4,%u),
#define RPDA_V_SOT      4
#define RPDA_SOT        (RPDA_M_SECT << RPDA_V_SOT)     /* current sect on track */
    BITFFMT(SOT,4,%u),
#define RPDA_V_TRACK    8
#define RPDA_M_TRACK    037
#define RPDA_TRACK      (RPDA_M_TRACK << RPDA_V_TRACK)  /* track */
    BITFFMT(SURF,5,%u),
#define GET_SECT(x)     ((x) & RPDA_SECT)
#define GET_TRACK(x)    (((x) & RPDA_TRACK) >> RPDA_V_TRACK)
#define GET_DA(c,h,s)   (((c) * RP_NUMSF + (h)) * RP_NUMSC + (s))
    ENDBITS
};

/* RPM1 776726 maintenance 1, read-only, not implemented */

/* RPM2 776730 maintenance 2, read-only, not implemented */

/* RPM1 776732 maintenance 3, write-only, not implemented */

/* SUCA 776734 selected unit cylinder address, read-only */
static BITFIELD rp_suca_bits[] = {
    BITFFMT(CYL,9,%u),
    ENDBITS
};

/* SILO 776736 silo memory, not implemented */

/* Maintenance Write Lockout Address (LOA) (the switches on the maint. panel) */
static const char* offon[] = { "OFF", "ON" };
static BITFIELD rp_wloa_bits[] = {
#define RPWLOA_IMPL     01777
#define RPWLOA_CYL2     0377                            /* cyls locked (x2) */
    BITFFMT(CYL2,8,%u),
#define RPWLOA_V_DRV    8
#define RPWLOA_M_DRV    3
#define RPWLOA_DRV      (RPWLOA_M_DRV << RPWLOA_V_DRV)  /* drive(s) locked */
    BITFFMT(DRV,3,%u),
#define GET_WLOACYL(x)  ((((x) & RPWLOA_CYL2) << 1) | 1)
#define GET_WLOADRV(x)  (((x) & RPWLOA_DRV) >> RPWLOA_V_DRV)
    BITNCF(4),
#define RPWLOA_ON       0100000
    BITFNAM(PROTECT,1,offon),
    ENDBITS
};

/* Data buffer and device registers */

static RPCONTR* rpxb = NULL;                            /* xfer buffer */
static int32 rpds = 0;                                  /* drive status */
static int32 rper = 0;                                  /* error status */
static int32 rpcs = 0;                                  /* control/status */
static int32 rpwc = 0;                                  /* word count */
static int32 rpba = 0;                                  /* memory address */
static int32 rpca = 0;                                  /* cylinder address */
static int32 rpda = 0;                                  /* disk address */
static int32 suca = 0;                                  /* current cylinder address */
static int32 wloa = 0;                                  /* write lockout address */
static int32 not_impl = 0;                              /* dummy register value */

/* Debug detail levels */

#define RPDEB_OPS       001                             /* transactions */
#define RPDEB_RRD       002                             /* reg reads */
#define RPDEB_RWR       004                             /* reg writes */
#define RPDEB_TRC       010                             /* trace */
#define RPDEB_INT       020                             /* interrupts */
#define RPDEB_DAT      0100                             /* transfer data */

static DEBTAB rp_deb[] = {
    { "OPS",       RPDEB_OPS, "transactions" },
    { "RRD",       RPDEB_RRD, "register reads" },
    { "RWR",       RPDEB_RWR, "register writes" },
    { "INTERRUPT", RPDEB_INT, "interrupts" },
    { "TRACE",     RPDEB_TRC, "trace" },
    { "DATA",      RPDEB_DAT, "transfer data" },
    { NULL, 0 }
};

static struct {
    const char* name;
    int32*      valp;
    BITFIELD*   bits;
} rp_regs[] = {
    { "RPDS", &rpds,     rp_ds_bits   },
    { "RPER", &rper,     rp_er_bits   },
    { "RPCS", &rpcs,     rp_cs_bits   },
    { "RPWC", &rpwc,     rp_wc_bits   },
    { "RPBA", &rpba,     rp_ba_bits   },
    { "RPCA", &rpca,     rp_ca_bits   },
    { "RPDA", &rpda,     rp_da_bits   },
    { "RPM1", &not_impl, NULL         },
    { "RPM2", &not_impl, NULL         },
    { "RPM3", &not_impl, NULL         },
    { "SUCA", &suca,     rp_suca_bits },
    { "SILO", &not_impl, NULL         }
};

/* Forward decls */

static t_stat rp_rd (int32 *data, int32 PA, int32 access);
static t_stat rp_wr (int32 data, int32 PA, int32 access);
static int32  rp_inta (void);
static t_stat rp_svc (UNIT *uptr);
static t_stat rp_reset (DEVICE *dptr);
static void   rp_go (void);
static void   rp_set_done (int32 error);
static void   rp_clr_done (void);
static t_stat rp_boot (int32 unitno, DEVICE *dptr);
static t_stat rp_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr);
static t_stat rp_attach (UNIT *uptr, CONST char *cptr);
static t_stat rp_detach (UNIT *uptr);
static t_stat rp_set_type (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
static t_stat rp_show_type (FILE *st, UNIT *uptr, int32 val, CONST void *desc);
static t_stat rp_set_wloa (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
static t_stat rp_set_ctrl (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
static t_stat rp_show_ctrl (FILE *st, UNIT *uptr, int32 val, CONST void *desc);
static const char *rp_description (DEVICE *dptr);

/* RP11 data structures

   rp_reg       RP register list
   rp_unit      RP unit list
   rp_mod       RP modifier list
   rp_dev       RP device descriptor
*/

static DIB rp_dib = {
    IOBA_AUTO/*base address*/, RP_IOLN/*addresses used*/,
    rp_rd, rp_wr,
    1/*# of vectors*/, IVCL(RP), VEC_AUTO,
    { rp_inta }, RP_IOLN/*addresses per device*/,
};

static REG rp_reg[] = {
    /* registers */
    { ORDATADF(RPCS, rpcs, 16, "control/status",        rp_cs_bits) },
    { ORDATADF(RPCA, rpca, 16, "cylinder address",      rp_ca_bits) },
    { ORDATADF(RPDA, rpda, 16, "disk address",          rp_da_bits) },
    { ORDATADF(RPBA, rpba, 16, "memory address",        rp_ba_bits) },
    { ORDATADF(RPWC, rpwc, 16, "word count",            rp_wc_bits) },
    { ORDATADF(RPDS, rpds, 16, "drive status",          rp_ds_bits) },
    { ORDATADF(RPER, rper, 16, "error status",          rp_er_bits) },
    { ORDATADF(SUCA, suca, 16, "cylinder address",      rp_suca_bits) },
    { ORDATADF(WLOA, wloa, 16, "write lockout address", rp_wloa_bits) },

    /* standard stuff */
    { FLDATAD (INT,  IREQ(RP), INT_V_RP,   "interrupt pending flag") },
    { FLDATAD (ERR,  rpcs,     CSR_V_ERR,  "error flag (CSR<15>)") },
    { FLDATAD (DONE, rpcs,     CSR_V_DONE, "device done flag (CSR<7>)") },
    { FLDATAD (IE,   rpcs,     CSR_V_IE,   "interrupt enable flag (CSR<6>)") },
    { ORDATA  (DEVADDR, rp_dib.ba,  32), REG_HRO },
    { ORDATA  (DEVVEC,  rp_dib.vec, 16), REG_HRO },
    { NULL }
};

static UNIT rp_unit[RP_NUMDR] = {
    { UDATA(rp_svc,
            UNIT_FIX | UNIT_ATTABLE | UNIT_DISABLE | UNIT_ROABLE | UNIT_RP03,
            RP_SIZE_RP03) },
    { UDATA(rp_svc,
            UNIT_FIX | UNIT_ATTABLE | UNIT_DISABLE | UNIT_ROABLE | UNIT_RP03,
            RP_SIZE_RP03) },
    { UDATA(rp_svc,
            UNIT_FIX | UNIT_ATTABLE | UNIT_DISABLE | UNIT_ROABLE | UNIT_RP03,
            RP_SIZE_RP03) },
    { UDATA(rp_svc,
            UNIT_FIX | UNIT_ATTABLE | UNIT_DISABLE | UNIT_ROABLE | UNIT_RP03,
            RP_SIZE_RP03) },
    { UDATA(rp_svc,
            UNIT_FIX | UNIT_ATTABLE | UNIT_DISABLE | UNIT_ROABLE | UNIT_RP03,
            RP_SIZE_RP03) },
    { UDATA(rp_svc,
            UNIT_FIX | UNIT_ATTABLE | UNIT_DISABLE | UNIT_ROABLE | UNIT_RP03,
            RP_SIZE_RP03) },
    { UDATA(rp_svc,
            UNIT_FIX | UNIT_ATTABLE | UNIT_DISABLE | UNIT_ROABLE | UNIT_RP03,
            RP_SIZE_RP03) },
    { UDATA(rp_svc,
            UNIT_FIX | UNIT_ATTABLE | UNIT_DISABLE | UNIT_ROABLE | UNIT_RP03,
            RP_SIZE_RP03) }
};

static MTAB rp_mod[] = {
    { MTAB_VDV, 0,
        NULL,           "MASSBUS", 
        rp_set_ctrl,    NULL,           NULL,
        "Switch controller type to MASSBUS/RP04+/RM03+" },
    { MTAB_VDV, 0,
        "TYPE",         NULL, 
        NULL,           rp_show_ctrl,   NULL,
        "Display controller type" },
    { MTAB_VDV | MTAB_VALR, 0,
        NULL,           "PROTECT",
        rp_set_wloa,    NULL,           NULL,
        "Set write lockout mode/address" },
    { MTAB_VUN, 0,
        "WRITEENABLED", "WRITEENABLED", 
        set_writelock,  show_writelock, NULL,
        "Write enable disk drive" },
    { MTAB_VUN, 1,
        NULL,           "LOCKED", 
        set_writelock,  NULL,           NULL,
        "Write lock disk drive" },
    { MTAB_VUN, 0,
        "TYPE",         NULL,
        NULL,           rp_show_type,   NULL,
        "Display device type" },
    { MTAB_VUN, 0/*RP02*/,
        NULL,           RP_RP02,
        rp_set_type,    NULL,           NULL,
        "Set " RP_RP02 " disk type" },
    { MTAB_VUN, UNIT_RP03,
        NULL,           RP_RP03,
        rp_set_type,    NULL,   NULL,
        "Set " RP_RP03 " disk type"},
    { UNIT_NOAUTO, 0,
        "autosize",     "AUTOSIZE", 
        NULL,           NULL,           NULL,
        "Set type based on file size at attach" },
    { UNIT_NOAUTO, UNIT_NOAUTO,
        "noautosize",   "NOAUTOSIZE",   NULL,
        NULL,           NULL,
        "Disable disk autosize on attach" },
    { MTAB_VUN | MTAB_VALR, 0,
        "FORMAT",       "FORMAT={AUTO|SIMH|VHD|RAW}",
        sim_disk_set_fmt, sim_disk_show_fmt, NULL,
        "Set/Display disk format" },
    { MTAB_VDV | MTAB_VALR, 010,
        "ADDRESS",      "ADDRESS",
        set_addr,       show_addr,      NULL,
        "Bus address" },
    { MTAB_VDV | MTAB_VALR, 0,
        "VECTOR",       "VECTOR",
        set_vec,        show_vec,       NULL,
        "Interrupt vector" },
    { 0 }
};

DEVICE rr_dev = {
    "RP", rp_unit, rp_reg, rp_mod, RP_NUMDR,
    8/*address radix*/, 24/*address width*/, 1/*address increment*/,
    8/*data radix*/, RPWRDSZ/*data width*/,
    NULL/*examine()*/, NULL/*deposit()*/,
    rp_reset, rp_boot, rp_attach, rp_detach,
    &rp_dib,
    DEV_DIS | DEV_DISABLE | DEV_UBUS | DEV_Q18 | DEV_DEBUG | DEV_DISK,
    0/*debug control*/, rp_deb,
    NULL/*msize()*/, NULL/*logical name*/,
    rp_help, NULL/*attach_help()*/, NULL/*help_ctx*/,
    rp_description,
};
#define rp_dev rr_dev

/* I/O dispatch routine, I/O addresses 17776710 - 17776736

   17776710     RPDS    read-only except for attention bits
   17776712     RPER    read-only
   17776714     RPCS    read/write
   17776716     RPWC    read/write
   17776720     RPBA    read/write
   17776722     RPCA    read/write
   17776724     RPDA    read/write
   17776726     RPM1    read-only,  unimplemented
   17776730     RPM2    read-only,  unimplemented
   17776732     RPM3    write-only, unimplemented
   17776734     SUCA    read-only
   17776736     SILO    read/write, unimplemented

   Some documentation says that RP11C actually responds to UNIBUS address range
   starting 17776700, but with no usable registers located in the first 4 words.
*/

static t_stat rp_rd (int32 *data, int32 PA, int32 access)
{
    /* offset by base then decode <4:1> */
    int rn = ((PA - rp_dib.ba) >> 1) & 017;
    UNIT* uptr;

    switch (rn) {
    case 0:                                             /* RPDS */
    case 1:                                             /* RPER */
    case 2:                                             /* RPCS */
        /* RPDS */
        uptr = rp_dev.units + GET_DRIVE(rpcs);          /* selected unit */
        rpds &= RPDS_ATTN;                              /* attention bits */
        if (!(uptr->flags & UNIT_DIS)) {                /* not disabled? */
            if (GET_DTYPE(uptr->flags))
                rpds |= RPDS_RP03;
            if (uptr->flags & UNIT_ATT) {               /* attached? */
                rpds |= RPDS_ONLN;
                if (uptr->flags & UNIT_WPRT)            /* write locked? */
                    rpds |= RPDS_WLK;
                if (uptr->SEEKING)                      /* still seeking? */
                    rpds |= RPDS_SEEK;
                else if (!sim_is_active(uptr))          /* idle? */
                    rpds |= RPDS_RDY;
            }
        }

        /* RPER */
        rper &= RPER_REAL;
        rper |= RPDS_DKER(rpds); 

        /* RPCS */
        rpcs &= RPCS_REAL;
        rpcs |= RPER_HARD(rper) | RPER_SOFT(rper);

        *data = *rp_regs[rn].valp;
        break;

    case 3:                                             /* RPWC */
        *data = rpwc;
        break;

    case 4:                                             /* RPBA */
        *data = rpba;
        break;

    case 5:                                             /* RPCA */
        *data = rpca;
        break;

    case 6:                                             /* RPDA */
        rpda &= RPDA_RW;
        rpda |= (rand() % RP_NUMSC) << RPDA_V_SOT;      /* a random sect */
        *data = rpda;                
        break;

    case 10:                                            /* SUCA */
        *data = suca;
        break;

    default:                                            /* not implemented */
        *data = 0;
        return SCPE_OK;
    }
    sim_debug(RPDEB_RRD, &rp_dev, ">>RP  read: %s=%#o\n", rp_regs[rn].name, *data);
    sim_debug_bits(RPDEB_RRD, &rp_dev, rp_regs[rn].bits, *data, *data, 1);
    return SCPE_OK;
}

#define RP_DATIOB(r, d)  (PA & 1 ? ((d) << 8) | ((r) & 0377) : ((r) & ~0377) | (d))

static t_stat rp_wr (int32 data, int32 PA, int32 access)
{
    /* offset by base then decode <4:1> */
    int rn = ((PA - rp_dib.ba) >> 1) & 017;
    int32 n, old_val = *rp_regs[rn].valp;

    switch (rn) {
    case 0:                                             /* RPDS */
        if (access != WRITEB  ||  !(PA & 1)) {
            rpds &= ~(data & RPDS_ATTN);                /* clr attention bits */
            if (!(rpds & RPDS_ATTN)  &&  (rpcs & RPCS_AIE)
                &&  (!(rpcs & CSR_IE)  ||  !(rpcs & CSR_DONE))) {
                sim_debug(RPDEB_INT, &rp_dev, "rp_wr(ATT:CLR_INT)\n");
                CLR_INT(RP);                            /* clr int request */
            }
        }
        break;

    case 1:                                             /* RPER: read-only */
        break;

    case 2:                                             /* RPCS */
        if (access == WRITEB)
            data = RP_DATIOB(rpcs, data);
        if (!(data & (RPCS_AIE | CSR_IE))) {            /* int disable? */
            sim_debug(RPDEB_INT, &rp_dev, "rp_wr(CSR:CLR_INT)\n");
            CLR_INT(RP);                                /* clr int request */
        } else if (((data & CSR_IE)
                    &&  (rpcs & (CSR_DONE | CSR_IE)) == CSR_DONE)  ||
                   ((data & RPCS_AIE)
                    &&  !(rpcs & RPCS_AIE)  &&  (rpds & RPDS_ATTN))) {
            sim_debug(RPDEB_INT, &rp_dev, "rp_wr(CSR:SET_INT)\n");
            SET_INT(RP);                                /* set int request */
        }
        rpcs &= ~RPCS_RW;
        rpcs |= data & RPCS_RW;
        n = GET_DRIVE(rpcs);
        if (n != GET_DRIVE(old_val)) {
            UNIT* uptr = rp_dev.units + n;              /* new selected unit */
            suca = uptr->CYL;
            n = 1;
        } else
            n = 0;                                      /* same old */
        if (!(rpcs & CSR_DONE)) {                       /* not ready? */
            if ((data & CSR_GO)  ||  n)                 /* GO or de-selected? */
                rper |= RPER_PGE;
        } else if (data & CSR_GO)                       /* new function? */
            rp_go();
        break;

    case 3:                                             /* RPWC */
        if (access == WRITEB)
            data = RP_DATIOB(rpwc, data);
        rpwc = data;
        break;

    case 4:                                             /* RPBA */
        if (access == WRITEB)
            data = RP_DATIOB(rpba, data);
        rpba = data & RPBA_IMP;
        break;

    case 5:                                             /* RPCA */
        if (access == WRITEB)
            data = RP_DATIOB(rpca, data);
        rpca = data & RPCA_IMP;
        break;

    case 6:                                             /* RPDA */
        if (access == WRITEB)
            data = RP_DATIOB(rpda, data);
        rpda &= ~RPDA_RW;
        rpda |= data & RPDA_RW;
        break;

    case 10:                                            /* SUCA: read-only */
        break;

    default:
        return SCPE_OK;
    }
    sim_debug(RPDEB_RWR, &rp_dev, ">>RP write: %s=%#o\n", rp_regs[rn].name, data);
    /* note that this is post-op; so e.g. it won't ever show the GO bit as 1 */
    sim_debug_bits(RPDEB_RWR, &rp_dev, rp_regs[rn].bits, old_val, *rp_regs[rn].valp, 1);
    return SCPE_OK;
}

/* Initiate new function */

static void rp_go (void)
{
    int32 i, cyl, head, sect, func, type;
    int rd, wr;
    UNIT* uptr;

    assert(rpcs & CSR_DONE);

    func = GET_FUNC(rpcs);                              /* get function */
    if (func == RPCS_RESET) {                           /* control reset? */
        rpds = 0;
        rper = 0;
        rpcs = CSR_DONE;
        rpwc = 0;
        rpba = 0;
        rpca = 0;
        rpda = 0;
        suca = rp_dev.units[0].CYL;
        sim_debug(RPDEB_INT, &rp_dev, "rp_go(RESET:CLR_INT)\n");
        CLR_INT(RP);                                   /* clr int request */
        return;
    }

    rp_clr_done();                                      /* clear done */
    rper  = 0;                                          /* clear errors */
    rpcs &= ~(CSR_ERR | RPCS_HERR);                     /* clear summary */
    i = GET_DRIVE(rpcs);                                /* get drive no */
    uptr = rp_dev.units + i;                            /* selected unit */

    if (!(uptr->flags & UNIT_ATT)) {                    /* not attached? */
        rp_set_done(RPER_PGE);
        return;
    }

    i = 0;                                              /* errors detected */
    rd = func == RPCS_READ   ||  func == RPCS_RD_NOSEEK  ||  func == RPCS_WCHK;
    wr = func == RPCS_WRITE  ||  func == RPCS_WR_NOSEEK;

    if (wr  &&  (uptr->flags & UNIT_WPRT))              /* write and locked? */
        i |= RPER_WPV;
    if (uptr->SEEKING  ||  sim_is_active(uptr))         /* still busy? */
        i |= RPER_PGE;
    if (rpcs & RPCS_HDR) {                              /* format and ... */
        if (!(rpcs & RPCS_MODE))                        /* ... not 18b? */
            i |= RPER_MODE;
        else if (!(rd | wr)  ||                         /* ... or not R/W? or ... */
            (rd  &&  -((int16) rpwc) != 3)  ||          /* rd hdr: wc m.b. 3 */
            (wr  &&  -((int16) rpwc) % 3)) {            /* wr hdr: wc m.b. mult of 3 */
            i |= RPER_PGE;
        }
    } else if (rd | wr) {                               /* regular R/W and ... */
        if (rpcs & RPCS_MODE)                           /* ... 18b? */
            i |= RPER_MODE;
#if 0   /* per doc, rpwc must be even; but DOS/Batch uses odd wc xfers (?!) */
        else if (rpwc & 1)                              /* ... or odd wc? */
            i |= RPER_PGE;
#endif
    }
    sect = GET_SECT(rpda);
    if (sect >= RP_NUMSC)
        i |= RPER_NXS;
    type = GET_DTYPE(uptr->flags);                      /* get drive type */
    if (func == RPCS_HOME) {
        head = 0;
        cyl  = 0;
    } else if (func == RPCS_RD_NOSEEK  ||  func == RPCS_WR_NOSEEK) {
        head = uptr->HEAD;
        cyl  = uptr->CYL;
    } else {
        head = GET_TRACK(rpda);
        cyl  = rpca;
        if (head >= RP_NUMSF)
            i |= RPER_NXT;
        if (cyl >= drv_tab[type].cyl)
            i |= RPER_NXC;
    }
    if ((wloa & RPWLOA_ON)  &&  wr
        &&  !(i & (RPER_WPV | RPER_NXC | RPER_NXT | RPER_NXS))) {
        if (GET_DRIVE(rpcs) <= GET_WLOADRV(wloa))       /* unit protected? */
            i |= RPER_WPV;
        else if (cyl <= GET_WLOACYL(wloa))              /* cyl protected? */
            i |= RPER_WPV;
    }
    if (i) {
        rp_set_done(i);                                 /* set done */
        return;
    }

    /* seek time */
    if (func == RPCS_HOME)
        i  = drv_tab[type].seek_ave / 2;
    else if (!(i = abs(cyl - uptr->CYL)))
        i  = drv_tab[type].seek_1 / 2;
    else if (i <= 2)
        i *= drv_tab[type].seek_1;
    else if (i <= (3 * drv_tab[type].cyl) / 4)
        i  = drv_tab[type].seek_ave;
    else
        i  = drv_tab[type].seek_max;
    if (func == RPCS_SEEK  ||  func == RPCS_HOME) {     /* seek? */
        uptr->SEEKING = 1;                              /* start seeking */
        rp_set_done(0);                                 /* set done */
        sim_activate(uptr,  i / 10);                    /* schedule */
    } else
        sim_activate(uptr, (i + RP_ROT_12) / 10);       /* I/O takes longer */

    uptr->CYL  = cyl;                                   /* put on cylinder */
    uptr->HEAD = head;                                  /* save head too */
    uptr->FUNC = func;                                  /* save func */
    return;
}

/* Complete seek */

static t_stat rp_seek_done (UNIT *uptr, int cancel)
{
    int32 n;

    assert(uptr->SEEKING);
    assert(uptr->FUNC == RPCS_SEEK  ||  uptr->FUNC == RPCS_HOME);
    n = (int32)(uptr - rp_dev.units);               /* get drv number */
    if (n == GET_DRIVE(rpcs))
        suca = cancel ? 0 : uptr->CYL;              /* update cyl shown */
    uptr->SEEKING = 0;                              /* set seek done */
    assert((1 << n) | RPDS_ATTN);
    rpds |= 1 << n;                                 /* set attention */
    if (rpcs & RPCS_AIE) {                          /* att ints enabled? */
        sim_debug(RPDEB_INT, &rp_dev, "rp_seek_done(SET_INT)\n");
        SET_INT(RP);
    }
    return SCPE_OK;
}

/* Service a unit

   If seek in progress, complete seek command
   Else complete data transfer command

   The unit control block contains the function and disk address for
   the current command.

   Some registers must be revalidated because they could have been
   modified in between go() and now.
*/

static t_stat rp_svc (UNIT *uptr)
{
    int32 func, cyl, head, sect, da, err, wc, n;
    t_seccnt todo, done;
    t_stat ioerr;
    uint32 ma;
    int rd;

    if (uptr->SEEKING)                                  /* seek? */
        return rp_seek_done(uptr, 0);

    func = uptr->FUNC;
    assert(func  &&  ~(rpcs & CSR_DONE));

    if (!(uptr->flags & UNIT_ATT)) {                    /* attached? */
        rp_set_done(RPER_PGE);
        return SCPE_UNATT;
    }
    sect = GET_SECT(rpda);                              /* get sect */
    if (sect >= RP_NUMSC) {                             /* bad sector? */
        rp_set_done(RPER_NXS);
        return SCPE_OK;
    }

    rd = func == RPCS_READ  ||  func == RPCS_RD_NOSEEK  ||  func == RPCS_WCHK;
    wc = 0200000 - rpwc;                                /* get wd cnt */
    cyl = uptr->CYL;
    head = uptr->HEAD;
    n = GET_DTYPE(uptr->flags);                         /* get drive type */
    assert(cyl < drv_tab[n].cyl  &&  head < RP_NUMSF);
    da = GET_DA(cyl, head, sect);                       /* form full disk addr */
    assert(da < drv_tab[n].size);
    n = drv_tab[n].size - da;                           /* sectors available */
    err = 0;                                            /* errors detected */
    if (rpcs & RPCS_HDR) {                              /* header ops? */
        if (!(rpcs & RPCS_MODE))
            err |= RPER_MODE;                           /* must be in 18b mode */
        else if ((rd  &&  wc != 3)  ||  (!rd  &&  wc % 3)) /* DEC-11-HRPCA-C-D 3.8 */
            err |= RPER_PGE;
        else if (rd)                                    /* a typo in doc??? */
            n  = 3;                                     /* can only read 3 wds */
        else
            n *= 3;                                     /* 3 wds per sector */
    } else {                                            /* no: regular R/W */
        if (rpcs & RPCS_MODE)
            err |= RPER_MODE;                           /* must be in PDP-11 mode */
#if 0   /* per doc, wc must be even; but DOS/Batch uses odd wc xfers (?!) */
        else if (wc & 1)                                /* must be even */
            err |= RPER_PGE;
#endif
        else
            n *= RP_NUMWD;                              /* can do this many */
    }
    if (err) {
        rp_set_done(err);
        return SCPE_OK;
    }
    if (wc > n)
        wc = n;
    assert(wc);

    /* A note on error handling:
     * RP11 processes data words between drive and memory absolutely sequentially
     * (not by the sector like the SIMH API provides).  Therefore, controller
     * errors should be asserted to follow that scenario.
     *
     * 1.  When reading from disk, an I/O error must be deferred until all words
     * (read from the disk so far) have been verified not to cause NXM.  If any
     * word did, then NXM gets reported, and the I/O error gets discarded
     * (because the real controller would have stopped the operation right then
     * and there, and would not have encountered the (later) I/O condition).
     *
     * 2.  When writing, I/O errors take precedence, provided that words keep
     * passing the address check in extraction from memory.  But no NXM should
     * be reported for any words that reside after the completed I/O boundary in
     * case of a short write to disk.
     *
     * 3.  Disk pack overrun is strictly a run-off of an otherwise successful
     * completion, which has left a residual word counter non-zero, because had
     * an earlier error stopped the disk operation, the overrun situation could
     * not have been experienced (the end of pack would not have been reached).
     */

    ma = ((rpcs & RPCS_MEX) << (16 - RPCS_V_MEX)) | rpba; /* get mem addr */

    if (rd) {                                           /* read */
        if (rpcs & RPCS_HDR) {                          /* format? */
            /* Sector header is loaded in the 36-bit Buffer Register(BR):
               17 0-bits, 9-bit cyl, 5-bit track, spare bit, 4-bit sect */
            rpxb[0] = 0;                                /* BR<35:20> */
            rpxb[1] = (cyl << 6) | (head << 1);         /* BR<19:04> */
            rpxb[2] = sect;                             /* BR<03:00> */
            ioerr = 0;
            done = 1;                                   /* 1 sector done */
        } else {                                        /* normal read */
            DEVICE* dptr = find_dev_from_unit(uptr);
            todo = (wc + (RP_NUMWD - 1)) / RP_NUMWD;    /* sectors to read */
            ioerr = sim_disk_rdsect(uptr, da, (uint8*) rpxb, &done, todo);
            n = done * RP_NUMWD;                        /* words read */
            sim_disk_data_trace(uptr, (uint8*) rpxb, da, n * sizeof(*rpxb), "rp_read",
                                RPDEB_DAT & (dptr->dctrl | uptr->dctrl), RPDEB_OPS);
            if (done >= todo)
                ioerr = 0;                              /* good stuff */
            else if (ioerr)
                wc = n;                                 /* short, adj wc */
            else {
                todo -= done;                           /* to clear ... */
                todo *= RP_NUMWD * sizeof(*rpxb);       /* ... bytes */
                memset(rpxb + n, 0, todo);
            }
        }
        if (func == RPCS_WCHK) {
            uint32 a = ma;
            for (n = 0;  n < wc;  ++n) {                /* loop thru buf */
                RPCONTR data;
                if (MAP_RDW(a, 1, &data)) {             /* mem wd */
                    err |= RPER_NXM;                    /* NXM? set flg */
                    break;
                }
                a += 2;
                if (ioerr)
                    continue;
                if (data != rpxb[n])                    /* match to disk? */
                    err |= RPER_WCE;                    /* no, err */
            }
            n %= wc;
        } else if ((n = MAP_WRW(ma, wc, rpxb))) {       /* store buf */
            err |= RPER_NXM;                            /* NXM? set flag */
            wc -= n;                                    /* adj wd cnt */
        }
        if (!n  &&  ioerr) {                            /* all wrds ok but I/O? */
            err |= RPER_FMTE;                           /* report as FMTE */
            if (func == RPCS_WCHK)
                err |= RPER_WCE;
        }
    } else {                                            /* write */
        if ((n = MAP_RDW(ma, wc, rpxb)))                /* get buf */
            wc -= n;                                    /* adj wd cnt */
        if (wc  &&  !(rpcs & RPCS_HDR)) {               /* regular write? */
            DEVICE* dptr = find_dev_from_unit(uptr);
            int32 m = (wc + (RP_NUMWD - 1)) & ~(RP_NUMWD - 1); /* clr to */
            memset(rpxb + wc, 0, (m - wc) * sizeof(*rpxb)); /* end of blk */
            sim_disk_data_trace(uptr, (uint8*) rpxb, da, m * sizeof(*rpxb), "rp_write",
                                RPDEB_DAT & (dptr->dctrl | uptr->dctrl), RPDEB_OPS);
            todo = m / RP_NUMWD;                        /* sectors to write */
            ioerr = sim_disk_wrsect(uptr, da, (uint8*) rpxb, &done, todo);
            if (done < todo) {
                wc = done * RP_NUMWD;                   /* words written */
                err |= RPER_FMTE;                       /* report as FMTE */
                if (!ioerr)
                    ioerr = 1;                          /* just in case */
            } else if (n)
                err |= RPER_NXM;                        /* NXM? set flg */
            else
                ioerr = 0;                              /* good stuff */
        } else {
            ioerr = 0;                                  /* good stuff */
            done = wc / 3;
            if (n)
                err |= RPER_NXM;                        /* NXM? set flg */
        }
    }

    if (wc) {                                           /* any xfer? */
        rpwc += wc;
        rpwc &= 0177777;
        ma   += wc << 1;
        rpba  = ma & RPBA_IMP;
        rpcs &= ~RPCS_MEX;
        rpcs |= (ma >> (16 - RPCS_V_MEX)) & RPCS_MEX;

        assert(done);
        rd = func == RPCS_RD_NOSEEK  ||  func == RPCS_WR_NOSEEK;
        if (!rd  ||  --done) {                          /* w/SEEK or 2+ sects? */
            da += done;                                 /* update DA */
            n = GET_DTYPE(uptr->flags);                 /* drive type */
            assert(da <= drv_tab[n].size);
            head = da / RP_NUMSC;                       /* new head (w/cyl) */
            cyl = head / RP_NUMSF;                      /* new cyl */
            if (cyl == drv_tab[n].cyl) {                /* at the end? */
                cyl  = drv_tab[n].cyl - 1;              /* last cyl and ... */
                head = RP_NUMSF       - 1;              /* ... head keep on */
            } else
                head %= RP_NUMSF;                       /* wrap up head */
            n = (int32)(uptr - rp_dev.units);           /* get drv number */
            if (!rd)                                    /* w/SEEK I/O? */
                uptr->HEAD = head;                      /* yes: select new head */
            else if (uptr->CYL != cyl  ||               /* no: arm moved or */
                     (rpwc  &&  !(err | ioerr))) {      /* boundary exceeded? */
                assert((1 << n) & RPDS_ATTN);
                rpds |= 1 << n;                         /* set attention */
                if (rpcs & RPCS_AIE) {                  /* att ints enabled? */
                    sim_debug(RPDEB_INT, &rp_dev, "rp_svc(SET_INT)\n");
                    SET_INT(RP);                        /* request interrupt */
                }
            }
            uptr->CYL = cyl;                            /* update new cyl */
            if (n == GET_DRIVE(rpcs))
                suca = uptr->CYL;                       /* let it out */
        }
    } else
        assert(err);

    if (rpwc  &&  !(err | ioerr))
        err |= RPER_EOP;                                /* disk pack overrun */
    rp_set_done(err);

    if (ioerr) {                                        /* I/O error? */
        sim_perror("RP I/O error");
        return SCPE_IOERR;
    }
    return SCPE_OK;
}

/* Interrupt state change routines

   rp_clr_done          clear done
   rp_set_done          set done and possibly errors
   rp_inta              interrupt acknowledge
*/

static void rp_clr_done (void)
{
    rpcs &= ~CSR_DONE;                                  /* clear done */
    if ((rpcs & CSR_IE)  &&  (!(rpcs & RPCS_AIE)  ||  !(rpds & RPDS_ATTN))) {
        sim_debug(RPDEB_INT, &rp_dev, "rp_clr_done(CLR_INT)\n");
        CLR_INT(RP);                                    /* clear int req */
    }
    return;
}

static void rp_set_done (int32 error)
{
    rper |= error;
    rpcs |= CSR_DONE;                                   /* set done */
    if (rpcs & CSR_IE) {                                /* int enable? */
        sim_debug(RPDEB_INT, &rp_dev, "rp_set_done(SET_INT)\n");
        SET_INT(RP);                                    /* request int */
    }
    return;
}

static int32 rp_inta (void)
{
    sim_debug(RPDEB_INT, &rp_dev, "rp_inta()\n");
    assert(((rpcs & RPCS_AIE)  &&  (rpds & RPDS_ATTN))  ||
            ((rpcs & CSR_IE)  &&  (rpcs & CSR_DONE)));
    rpcs &= ~RPCS_AIE;                                  /* AIE is one-shot */
    return rp_dib.vec;                                  /* return vector */
}

/* Device reset */

static t_stat rp_reset (DEVICE *dptr)
{
    int i;

    /* some sanity check first */
    assert(sizeof(rp_regs)/sizeof(rp_regs[0]) == RP_IOLN/2);

    /* clear everything now */
    rpds = 0;
    rper = 0;
    rpcs = CSR_DONE;
    rpwc = 0;
    rpba = 0;
    rpca = 0;
    rpda = 0;
    suca = 0;
    assert(dptr == &rp_dev);
    sim_debug(RPDEB_INT, &rp_dev, "rp_reset(CLR_INT)\n");
    CLR_INT(RP);
    for (i = 0;  i < RP_NUMDR;  ++i) {
        UNIT* uptr = rp_dev.units + i;
        sim_cancel(uptr);
        uptr->CYL = 0;
        uptr->FUNC = 0;
        uptr->HEAD = 0;
        uptr->SEEKING = 0;
    }
    if (rpxb == NULL)
        rpxb = (RPCONTR*) calloc(RP_MAXFR, sizeof (*rpxb));
    if (rpxb == NULL)
        return SCPE_MEM;
    return auto_config(NULL, 0);
}

/* Attach/detach routines */

static t_stat rp_attach (UNIT *uptr, CONST char *cptr)
{
    static const char* rp_types[] = { RP_RP03, RP_RP02, NULL };
    int type = GET_DTYPE(uptr->flags);
    return sim_disk_attach_ex2(uptr, cptr,
                               RP_NUMWD * sizeof(*rpxb), sizeof (*rpxb),
                               TRUE, 0, drv_tab[type].name,
                               0, 0, rp_types, 0);
}

static t_stat rp_detach (UNIT *uptr)
{
    if (uptr->SEEKING)
        rp_seek_done(uptr, 1/*cancel*/);
    else if (sim_is_active(uptr))
        rp_set_done(RPER_TE);
    sim_cancel(uptr);
    uptr->CYL = 0;
    uptr->FUNC = 0;
    uptr->HEAD = 0;
    return sim_disk_detach(uptr);
}

/* Set / show drive type */

static t_stat rp_set_type (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
    if ((val & ~UNIT_RP03)  ||  cptr)
        return SCPE_ARG;
    if (uptr->flags & UNIT_ATT)
        return SCPE_ALATT;
    uptr->capac  = RP_SIZE(drv_tab[GET_DTYPE(val)].size);
    uptr->flags &= ~UNIT_RP03;
    uptr->flags |= val;
    return SCPE_OK;
}

static t_stat rp_show_type (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
    fputs(drv_tab[GET_DTYPE(uptr->flags)].name, st);
    return SCPE_OK;
}

/* Set WLOA */

static t_stat rp_set_wloa (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
    DEVICE* dptr = find_dev_from_unit(uptr);
    if (!cptr  ||  !*cptr)
        return SCPE_ARG;
    if (strcasecmp(cptr, "OFF") == 0) {
        wloa &= ~RPWLOA_ON;
        return SCPE_OK;
    }
    if (strncasecmp(cptr, "ON", 2) != 0)
        return SCPE_ARG;
    cptr += 2;
    if (*cptr == ';') {
        char* end;
        long val;
        errno = 0;
        val = strtol(++cptr, &end, 0);
        if (errno  ||  !end  ||  *end  ||  end == cptr  ||  (val & ~RPWLOA_IMPL))
            return SCPE_ARG;
        wloa &= ~RPWLOA_IMPL;
        wloa |= val;
    } else if (!*cptr)
        return SCPE_2FARG;
    wloa |= RPWLOA_ON;
    return SCPE_OK;
}

/* Device bootstrap */

static t_stat rp_boot (int32 unitno, DEVICE *dptr)
{
    return SCPE_NOFNC;
}

/* Misc */

#define RP_DESCRIPTION  RP_RP11C "/" RP_RP02 "/" RP_RP03 " disk pack device"

static t_stat rp_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
    size_t i;
    static const char text[] =
    /*567901234567890123456789012345678901234567890123456789012345678901234567890*/
    RP_DESCRIPTION "\n\n"
    "Implementation does not include any maintenance registers or disk\n"
    "formatting operations yet supports the Write Lockout Address (LOA)\n"
    "register, which can be set with a PROTECT command:\n\n"
    "    sim> SET RP PROTECT=ON;0407\n\n"
    "to turn the protection on (in this case, the entire units 0 and 1,\n"
    "and 7x2=14 first cylinders of unit 2 will become write-locked).\n"
    "The current setting can be obtained by examining the WLOA register in\n"
    "the device:\n\n"
    "    sim> EXAMINE RP WLOA\n"
    "    WLOA:   100407  PROTECT=ON DRV=1 CYL2=7\n\n"
    "To remove the lockout:\n\n"
    "    sim> SET RP PROTECT=OFF\n"
    "    sim> EXAMINE RP WLOA\n"
    "    WLOA:   000407  PROTECT=OFF DRV=1 CYL2=7\n\n"
    "Note that it does not clear the address but turns the feature off.\n\n"
    "A detailed description of this device can be found in the\n"
    "\"PDP-11 Peripherals Handbook\" (1975, 1976) and in the technical manual\n"
    "\"RP11-C Disk Pack Drive Controller Maintenance Manual\" (1974)\n"
    "(DEC-11-HRPCA-C-D).\n\n"
    "Disk drive parameters:\n\n"
    "        Cylinders    Heads  Sects/Trk    Capacity     Average access\n"
    "      Total   Spare                   Nominal  Usable    time, ms\n";
    fputs(text, st);
    for (i = 0;  i < sizeof(drv_tab)/sizeof(drv_tab[0]);  ++i) {
        uint32 spare = GET_DA(drv_tab[i].spare, RP_NUMSF, RP_NUMSC);
        uint32 total = drv_tab[i].size;
        fprintf(st, "%.6s: %5u   %5u  %5u  %5u"
                "    %5.1fMB  %5.1fMB   %5u.%1u\n", drv_tab[i].name,
                drv_tab[i].cyl, drv_tab[i].spare, RP_NUMSF, RP_NUMSC,
                RP_SIZE(total - spare) / .5e6, RP_SIZE(total) / .5e6,
                (drv_tab[i].seek_ave + RP_ROT_12)/10,
                (drv_tab[i].seek_ave + RP_ROT_12)%10);
    }
    fprint_set_help (st, dptr);
    fprint_show_help(st, dptr);
    fprintf(st,
    "\nThe " RP_RP11C " is disabled in a Qbus system with more than 256KB of memory.\n");
    fprint_reg_help (st, dptr);
    return SCPE_OK;
}

static const char *rp_description (DEVICE *dptr)
{
    return RP_DESCRIPTION;
}

/* Show / switch controller type */

static t_stat rp_show_ctrl (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
    fputs(RP_RP11C, st);
    return SCPE_OK;
}

t_stat set_dev_enbdis (DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);

#undef rp_dev
static t_stat rp_set_ctrl (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
    DEVICE* dptr = find_dev_from_unit(uptr);
    t_stat rv;
    int i;

    if (cptr)
        return SCPE_ARG;
    if ((rv = set_dev_enbdis(dptr, 0, 0/*disable*/, NULL)) != SCPE_OK)
        return rv;
    for (i = 0;  sim_devices[i];  ++i) {
        extern DEVICE rp_dev;
        if (sim_devices[i] == dptr) {
            sim_devices[i]  = &rp_dev;
            if ((rv = set_dev_enbdis(sim_devices[i], 0, 1/*enable*/, NULL)) != SCPE_OK)
                sim_devices[i] = dptr;
            return rv;
        }
    }
    return SCPE_NXDEV;
}

#elif !defined(UC15)
#error "RP11C can only be used in PDP-11 configuration"
#endif /*VM_PDP11 && !UC15*/
