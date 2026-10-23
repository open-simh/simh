/* vax49_sysdev.c: VAXstation 4000 Model 90 (KA49, NVAX) system-specific logic

   Copyright (c) 2026, Douglas P. Fields, Jr.
   This module incorporates code from SimH, Copyright (c) 1998-2008, Robert M Supnik,
   and from the KA46 model, Copyright (c) 2019, Matt Burke

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

   This module contains the KA49 system-specific registers and devices:

   sysd         system devices: the NVAX processor registers, the board
                registers (interrupt controller, configuration, LED, I/O
                reset, SCSI DMA address and direction), the NMC memory
                controller and its O-bit RAM, the NCA I/O adapter, the SCSI
                scatter/gather map, the network address ROM window, the
                machine check and the console halt.

   How the facts were established.  The KA49 has almost no published
   register-level documentation: the service manual names registers without
   addresses, NetBSD's port uses a handful of them.  Nearly everything here
   comes from the console firmware itself (its power-up tests are the
   specification of what the hardware must do) and from reading the
   registers of a real VAXstation 4000/90 at its console.  Where a behaviour
   is only consistent with the firmware and not proven by it, the comment
   says "guess".
*/

#include "vax_defs.h"
#include "vax_sgec.h"
#include "vax_lcspx.h"
#include "vax_tca.h"

/* The console ROM.  The simulator carries DEC's KA49 console firmware
   built in when the tree has it: VAX/ka49a.bin listed in sim_BuildROMs.c,
   which writes vax_ka49a_bin.h from it.  A tree without that header (or a
   build with DONT_USE_INTERNAL_ROM) loads the ROM at run time instead:
   LOAD -R <file> before BOOT, or, at the first BOOT, the file ka49a.bin
   in the current working directory.  The image is public (see
   ka49_rom_missing below for where to get it). */

#if !defined (DONT_USE_INTERNAL_ROM) && defined (__has_include)
#if __has_include ("vax_ka49a_bin.h")
#define KA49_INTERNAL_ROM       1
#endif
#endif

#if defined (KA49_INTERNAL_ROM)
#include "vax_ka49a_bin.h" /* Defines BOOT_CODE_FILENAME and BOOT_CODE_ARRAY, etc */
#else
#define BOOT_CODE_FILENAME "ka49a.bin"
#if !defined (BOOT_CODE_ARRAY)
#define BOOT_CODE_ARRAY NULL
#define BOOT_CODE_SIZE 0
#endif
#endif

t_stat vax49_boot (int32 flag, CONST char *ptr);
#if !defined (KA49_INTERNAL_ROM)
static void ka49_rom_missing (void);
#endif

/* Special boot command, overrides regular boot */

CTAB vax49_cmd[] = {
    { "BOOT", &vax49_boot, RU_BOOT,
      "bo{ot}                   boot simulator\n", NULL, &run_cmd_message },
    { NULL }
    };

/* CPU models.  The Model 90, 90A and 96 use the same module and firmware;
   what differs is the clock.  The console measures the processor speed at
   power-up and loads one of three microcode patches, which sets the patch
   revision in SID<13:9>; the banner's MHz figure and VMS's choice of
   HW_NAME both follow from that SID.  The simulator's speed is the host's,
   so the patch revision is the model's choice here and the measurement is
   ignored (SID and PCSCR report it whatever the console loaded).

   PCCTL<9> read as 1 on a real 4000/90 and 0 on a real 4000/96 (the
   VMS error log prints "pcache redundancy elements enabled" for one and not
   the other); it is a read-only property of the chip, recorded per model.
   CEFSTS and NEICMD with nothing latched differ between the two real
   machines too (see TBSTS_IDLE below) and are recorded per model.  The 90A
   values are guesses (the 90's). */

typedef struct {
    const char  *name;                                  /* SET CPU MODEL= name */
    const char  *sim_name;                              /* simulator name */
    uint32      patch;                                  /* SID<13:9> patch revision */
    uint32      pcctl_red;                              /* PCCTL<9> */
    uint32      cefsts;                                 /* CEFSTS, nothing latched */
    uint32      neicmd;                                 /* NEICMD, nothing latched */
    } KA49_MODEL;

static const KA49_MODEL ka49_models[] = {
    { "90",  "VAXstation 4000-90 (KA49)",  27, 1, 0x00006FA0, 0x00000040 },
    { "90A", "VAXstation 4000-90A (KA49)", 19, 1, 0x00006FA0, 0x00000040 },
    { "96",  "VAXstation 4000-96 (KA49)",  11, 0, 0x00007FE0, 0x00000000 },
    { NULL }
    };

int32 ka49_model = 0;                                   /* index in ka49_models */
int32 sys_model = 0;                                    /* no graphics console */

/* Configuration register (2580.0000, read only, word)

   <0>      TURBOchannel adapter present (an option ROM dump is attached to
            TCA, vax_tca.c; the real 4000/96 has one, the 4000/90 here none)
   <1>      LCSPX graphics present
   <2>      run the remaining power-up device tests (4 to 13); the real
            machine ran them, so the bit is set (its true meaning: guess)
   <3>      serial console on DZ line 3 (no graphics console)
   <4>      SPXg/gt graphics present
   <5>      DSW21 synchronous communications option present
   <7:6>    memory set 0 (0A-0D): <6> present, <7> 16 MB SIMMs (else 4 MB)
   <9:8>    memory set 1 (1E-1H): the same
   <13:10>  SIMM faults
   <15:14>  not driven: read as ones

   The register is a word; a longword read gives it in both halves.  The
   real 4000/90 read C3CEC3CE (console E/P/L 2580.0000, 2026-10-20).

   The model has the LCSPX graphics, as both real machines it was compared
   with do, so <1> is set, with <3> because the console is the serial line
   (no monitor); for
   128 MB the register holds 03CE (C3CE as read).  VMS needs <1>: with it clear, VMS
   reports HW_MODEL 378, "Unknown VAXstation 4000-90", and the licence
   manager calls the CPU configuration unsupported; with it set, HW_MODEL
   376 and "VAXstation 4000-90", as on the real machines (found by
   experiment).  SET LCSPX DISABLED clears <1> (vax_lcspx.c).  The
   console sizes memory from <9:6> alone and programs MEMCON0/1 from them,
   so they must agree with SET CPU memory size. */

#define CFG_TCA         0x0001
#define CFG_LCSPX       0x0002
#define CFG_TESTS       0x0004
#define CFG_SERCON      0x0008
#define CFG_SPXG        0x0010
#define CFG_DSW21       0x0020
#define CFG_M0_PRES     0x0040
#define CFG_M0_16MB     0x0080
#define CFG_M1_PRES     0x0100
#define CFG_M1_16MB     0x0200

/* NMC registers */

#define MEMCON_VALID    0x80000000                      /* bank valid */
#define MMCDSR_CONINIT  0x00000040                      /* "console initialised memory" (console's marker) */
#define MOAMR_V_PAGE    6                               /* O-bit RAM page select <14:6> */
#define MOAMR_M_PAGE    0x1FF
#define OBIT_PAGES      (MOAMR_M_PAGE + 1)
#define OBIT_ENTRIES    (OBITSIZE >> 4)                 /* one entry per 16 bytes */
#define OBIT_MASK       0x00FFFFFF                      /* 24 bits per entry */

/* SCSI scatter/gather map entries.  Written: <31> valid, <30> force bad
   parity, <17:0> page frame number.  Read: <28> parity bit, <27> valid,
   <26:9> page frame number (so a read gives the page's physical address),
   all other bits zero.  The parity bit makes the number of ones in valid,
   PFN and parity odd, and is inverted when the entry was written with <30>
   set; the console's map RAM test checks exactly that. */

#define SGM_ENTRIES     (SGMSIZE >> 2)
#define SGM_W_VALID     0x80000000
#define SGM_W_BADPAR    0x40000000
#define SGM_W_PFN       0x0003FFFF
#define SGM_R_PARITY    0x10000000
#define SGM_R_VALID     0x08000000
#define SGM_R_V_PFN     9

/* NVAX processor register details (NVAX chip specification; values seen
   on a real 4000/90 are in the comments) */

#define ICSR_ENABLE     0x00000001                      /* VIC enable */
#define ICSR_RW         (ICSR_ENABLE)                   /* <2> LOCK and <4:3> errors are W1C, never set here */
#define BPCR_W_ONLY     0x00000020                      /* reads 0: VMS writes ECC80024, the console then reads ECC80004 */
#define BPCR_INIT       0xECC80004                      /* value the console showed (power-up value: guess) */
#define PCCTL_RW        0x000001FF                      /* <0> D_ENABLE, <1> I_ENABLE, <2> FORCE_HIT, <3> BANK_SEL, <4> P_ENABLE, <7:5> PMM, <8> */
#define PCCTL_ONES      0xFFFFFC00                      /* <31:10> read as one */
#define PCCTL_V_RED     9                               /* <9> read-only, per model */
#define PCSTS_ONES      0xFFFFF800                      /* <31:11> read as one; <10:0> error bits, W1C, never set here */
#define PCSCR_ENABLE    0x00000200                      /* PCS enabled: a patch is loaded */
#define PCSCR_V_PATCH   24                              /* patch revision <28:24> */
#define PCSCR_M_PATCH   0x1F
#define PAMODE_32       0x00000001                      /* 32-bit physical addressing */

/* TBSTS, CEFSTS, NEOCMD and NEICMD with no error latched.  Their lock and
   error bits are clear (the console clears CEFSTS<4:0> and TBSTS<0> by
   writing ones); the other bits are fields of the last TB, Cbox fill and
   NDAL transactions, which the model does not track.  It returns what the
   real machines show in every VMS error-log subpacket and in SDA's
   register block of a crash dump: on a 4000/90 (2026-10-10 to 2026-10-21)
   TBSTS 800001D0, CEFSTS 00006FA0, NEOCMD 0000FF14 and NEICMD 00000040
   (00000046 in the entries before 2026-10-20); on a 4000/96 the same but
   CEFSTS 00007FE0 and NEICMD 00000000 (per model, ka49_models). */

#define TBSTS_IDLE      0x800001D0
#define NEOCMD_IDLE     0x0000FF14

/* CEFSTS is one register, the Cbox's record of its last fill: <4:0> are the
   error bits (<1> LOCK, <3> fill failed due to a read data error, <4> lost
   error; write one to clear), the rest is state that the fill leaves behind
   and that stays when the error bits are cleared.  A fill that fails
   replaces the whole register, as a real 4000/90 showed (2026-10-26):

        idle (VMS, nothing latched)     00006FA0
        read of absent I/O space        0001920A, 00019200 once <4:0> are
                                        cleared (the console's D/I AC 1F),
                                        and still 00019200 under VMS later

   CEFADR holds the address of the fill: for I/O space the physical
   address, aligned to 32 bytes (alignment: a guess; 3C000000 and 3C000004
   both gave it), with <31:30> set (FC000000 under VMS). */

#define CEFSTS_W1C      0x0000001F                      /* <4:0> error bits */
#define CEFSTS_IOREAD   0x0001920A                      /* fill of absent I/O space failed */
#define CEFSTS_MEMREAD  0x0000020A                      /* fill of absent memory failed */
#define CEFSTS_MEMWRITE 0x0000018A                      /* ... the ownership read of a write: <8> read done for a write, <7> O-read */
#define CEFADR_IO       0xC0000000                      /* <31:30> set for I/O space */
#define CEFADR_ALIGN    0xFFFFFFE0                      /* fill address: 32 bytes */

/* NCA error status, CESR (2102.0000), the bits this model sets.  A read or
   write that nothing answers on the CP2 bus is a master transaction
   timeout: <15> with the summary <31>.  CIOEAR2 (2102.0014) holds the
   address, PA<28:0>. */

#define CESR_ERRSUM     0x80000000                      /* <31> error summary */
#define CESR_CP2_MTT    0x00008000                      /* <15> CP2 master transaction timeout */
#define MESR_ERRSUM     0x80000000                      /* NMC MESR <31> error summary */
#define MESR_LOSTHARD   0x40000000                      /* <30> lost NDAL hard error */
#define MESR_NXM        0x00100000                      /* <20> non-existent memory address */
#define CIOEAR_ADDR     0x1FFFFFFF                      /* PA<28:0> */
#define CIOEAR_ID1      0x20000000                      /* <31:29> = 1: the commander ID of a write with the Bcache on (a guess, see io_write_error) */
#define IOERR_DELAY     5760                            /* us: 144000 NDAL cycles of 40 ns, the CP2 timeout (estimate; the hard error came within 10 ms) */

/* CCTL: Bcache control.  The bits the console's cache test uses, as the
   test shows them to behave:

   <0>      Bcache enable
   <4:3>    size (10 = 256 KB, the KA49's)
   <6>      force hit: every memory reference hits in the Bcache, whatever
            the tag says, so the Bcache data RAM is read and written alone.
            The test turns the memory controller's banks off (MEMCON<31>)
            while it works in this mode and writes, for example, at
            0222.2220, which does not exist on a 16 MB machine.
   <7>      software ECC: memory writes store the check bits from BCDECC
            instead of computing them, and reads latch a syndrome in BCEDECC
   <8>      tag diagnostic mode: tag store writes store the check bits as
            written instead of computing them (guess: the console uses it
            only together with a test that needs exactly that)
   <30>     SW_ETM, <31> HW_ETM: error transition modes (NetBSD).  HW_ETM
            is status, set by the Cbox on an error and cleared by writing
            one: VMS writes 80000011 at start-up, and the real 4000/90's
            error log then shows CCTL 00000011, where a stored bit 31 made
            the model's say "HARDWARE ERROR TRANSITION MODE" (2026-10-20).
            The model never sets it.

   The model has no Bcache in normal operation: memory is always coherent.
   It keeps just enough state for the console's Bcache tests: the tag
   store, a Bcache data RAM for force-hit mode, and in software-ECC mode
   the check bits of each quadword written. */

#define CCTL_ENABLE     0x00000001
#define CCTL_FORCEHIT   0x00000040
#define CCTL_SWECC      0x00000080
#define CCTL_TAGDIAG    0x00000100
#define CCTL_HWETM      0x80000000                      /* hardware error transition mode: status, W1C */
#define BCDATA_SIZE     (256 * 1024)                    /* Bcache data RAM */
#define CACHEABLE_END   0x20000000                      /* PA<29> = 0: memory space */

#define BCTAG_ENTRIES   ((MT_BCTAG_END - MT_BCTAG_BASE) >> 5)
#define BCTAG_STORED    0x1FFDFE00                      /* bits a tag entry holds: <28:18>, <16:9> */
#define BCTAG_ECC       0x0001F800                      /* <16:11> check bits */
#define BCECC_ENTRIES   (256 * 1024 / 8)                /* one check word per Bcache quadword */
#define PCTAG_ENTRIES   ((MT_PCTAG_END - MT_PCTAG_BASE) >> 5)
#define PCDAP_ENTRIES   ((MT_PCDAP_END - MT_PCDAP_BASE) >> 3)

/* Debug flags */

#define DBG_IPR         0x0001                          /* processor registers */
#define DBG_REG         0x0002                          /* board registers */
#define DBG_LED         0x0004                          /* LED codes */
#define DBG_MCHK        0x0008                          /* machine checks */
#define DBG_HALT        0x0010                          /* console halts */
#define DBG_INT         0x0020                          /* interrupt controller */
#define DBG_NXM         0x0040                          /* references to absent I/O space */

DEBTAB sysd_debug[] = {
    { "IPR",  DBG_IPR,  "processor register reads and writes" },
    { "REG",  DBG_REG,  "board register reads and writes" },
    { "LED",  DBG_LED,  "diagnostic LED codes" },
    { "MCHK", DBG_MCHK, "machine checks" },
    { "HALT", DBG_HALT, "console halts" },
    { "INT",  DBG_INT,  "interrupt mask, request and clear" },
    { "NXM",  DBG_NXM,  "references to non-existent I/O addresses" },
    { NULL, 0 }
    };

extern int32 tmr_int;
extern int32 iccs_rd (void);
extern void iccs_wr (int32 val);
extern int32 icr_rd (void);
extern int32 nicr_rd (void);
extern void nicr_wr (int32 val);
extern int32 rom_rd (int32 pa);
extern int32 nvr_rd (int32 pa);
extern void nvr_wr (int32 pa, int32 val, int32 lnt);
extern int32 nar_rd (int32 pa);
extern int32 dz_rd (int32 pa);
extern void dz_wr (int32 pa, int32 val, int32 lnt);
extern int32 dz_int_level (void);
extern int32 rz_rd (int32 pa);
extern void rz_wr (int32 pa, int32 val, int32 lnt);
extern uint32 *rom;
extern UNIT clk_unit;

/* Processor registers */

int32 ka_savpc = 0;                                     /* SAVPC */
int32 ka_savpsl = 0;                                    /* SAVPSL */
int32 ka_cpuid = 0;                                     /* CPUID */
int32 ka_mchk_busy = 0;                                 /* machine check not yet acknowledged */
int32 ka_intsys = 0;                                    /* INTSYS */
int32 ka_pmfcnt = 0;                                    /* PMFCNT */
int32 ka_pcscr = 0;                                     /* PCSCR as written */
int32 ka_ecr = 0;                                       /* ECR */
int32 ka_cctl = 0;                                      /* CCTL */
int32 ka_bcdecc = 0;                                    /* BCDECC */
int32 ka_bcedecc = 0;                                   /* BCEDECC */
int32 ka_bcetsts = 0, ka_bcedsts = 0;                   /* Bcache error status */
int32 ka_cefsts = 0x00006FA0;                           /* Cbox fill status (the "90" idle value) */
int32 ka_cefadr = 0;                                    /* Cbox fill error address */
int32 ka_nests = 0;                                     /* NDAL error status */
int32 ka_vmar = 0, ka_vtag = 0, ka_vdata = 0;           /* VIC diagnostic access */
int32 ka_icsr = 0;                                      /* ICSR */
int32 ka_bpcr = BPCR_INIT;                              /* BPCR */
int32 ka_pamode = 0;                                    /* PAMODE */
int32 ka_pcctl = 0;                                     /* PCCTL */
uint32 *ka_bctag = NULL;                                /* Bcache tag store */
uint32 *ka_bcecc = NULL;                                /* check bits of quadwords written in SW ECC mode */
uint8 *ka_bcecc_set = NULL;                             /* ... and whether there are any */
uint32 *ka_bcdata = NULL;                               /* Bcache data RAM, force-hit mode */
uint32 ka_pctag[PCTAG_ENTRIES];                         /* Pcache tags */
uint32 ka_pcdap[PCDAP_ENTRIES];                         /* Pcache data parity */
int32 ka_cdiag = 0;                                     /* memory references routed through ReadReg/WriteReg */

/* Board registers */

int32 int_req[IPL_HLVL] = { 0 };                        /* interrupt requests, IPL 14-17 */
int32 int_mask = 0;                                     /* INT_MSK */
int32 ka_cfg = 0;                                       /* configuration register */
int32 ka_led = 0;                                       /* LED register (complement of the display) */
int32 ka_dmaadr = 0;                                    /* SCSI DMA window address */
int32 ka_dmadir = 0;                                    /* SCSI DMA direction */
int32 ka_ioreset = 0;                                   /* last I/O reset value */
int32 ka_softerr = 0;                                   /* soft error interrupt request */
int32 ka_harderr = 0;                                   /* hard error interrupt request */
int32 ka_xipr44 = 0;                                    /* external IPR 44 (2100.0110) */

/* NMC */

int32 nmc_memcon[8] = { 0 };                            /* MEMCON0-7 */
int32 nmc_mesr = 0;                                     /* MESR */
int32 nmc_mear = 0;                                     /* MEAR as latched by a memory error */
int32 nmc_mmcdsr = 0;                                   /* MMCDSR */
int32 nmc_moamr = 0;                                    /* MOAMR */
uint32 nmc_last = 0;                                    /* last NMC register address, for MEAR */
uint32 *nmc_obit = NULL;                                /* O-bit RAM */

/* NCA */

int32 nca_cesr = 0;                                     /* CESR */
int32 nca_cmcdsr = 0;                                   /* CMCDSR */
int32 nca_cioear2 = 0;                                  /* CIOEAR2: CP2 I/O error address */

/* SCSI scatter/gather map, as written */

uint32 *ka_sgmap = NULL;

t_stat sysd_reset (DEVICE *dptr);
t_stat sysd_show_led (FILE *st, UNIT *uptr, int32 val, CONST void *desc);
t_stat sysd_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr);
const char *sysd_description (DEVICE *dptr);
int32 con_halt (int32 code, int32 cc);
void ka49_set_cdiag (void);

t_stat io_write_svc (UNIT *uptr);

UNIT sysd_unit[] = {
    { UDATA (NULL, 0, 0) },
    { UDATA (&io_write_svc, 0, 0) }                     /* a posted write that nothing answered */
    };

#define IOWR_UNIT       (&sysd_unit[1])

REG sysd_reg[] = {
    { HRDATAD (SAVPC,   ka_savpc,   32, "console saved PC") },
    { HRDATAD (SAVPSL,  ka_savpsl,  32, "console saved PSL, halt code <13:8>") },
    { HRDATAD (CPUID,   ka_cpuid,   32, "CPU ID") },
    { HRDATAD (PCSCR,   ka_pcscr,   32, "patchable control store control, as written") },
    { HRDATAD (ECR,     ka_ecr,     32, "Ebox control") },
    { HRDATAD (CCTL,    ka_cctl,    32, "Bcache control") },
    { HRDATAD (BCDECC,  ka_bcdecc,  32, "Bcache data ECC to write") },
    { HRDATAD (BCEDECC, ka_bcedecc, 32, "Bcache error data ECC") },
    { HRDATAD (ICSR,    ka_icsr,    32, "Ibox control and status") },
    { HRDATAD (BPCR,    ka_bpcr,    32, "branch prediction control") },
    { HRDATAD (PAMODE,  ka_pamode,  32, "physical address mode") },
    { HRDATAD (PCCTL,   ka_pcctl,   32, "Pcache control, writable bits") },
    { HRDATAD (CFG,     ka_cfg,     16, "configuration register") },
    { HRDATAD (LED,     ka_led,      8, "LED register (the display shows its complement)") },
    { HRDATAD (INTMSK,  int_mask,    8, "interrupt mask") },
    { HRDATAD (INTREQ,  int_req[1], 16, "IPL 15 interrupt requests") },
    { HRDATAD (DMAADR,  ka_dmaadr,  32, "SCSI DMA window address") },
    { HRDATAD (DMADIR,  ka_dmadir,   1, "SCSI DMA direction") },
    { BRDATAD (MEMCON,  nmc_memcon, 16, 32, 8, "NMC memory configuration") },
    { HRDATAD (MESR,    nmc_mesr,   32, "NMC memory error status") },
    { HRDATAD (MEAR,    nmc_mear,   32, "NMC memory error address, as latched by a memory error") },
    { HRDATAD (MMCDSR,  nmc_mmcdsr, 32, "NMC control, diagnostic and status") },
    { HRDATAD (MOAMR,   nmc_moamr,  32, "NMC O-bit RAM access mode") },
    { HRDATAD (CESR,    nca_cesr,   32, "NCA error status") },
    { HRDATAD (CIOEAR2, nca_cioear2, 32, "NCA CP2 I/O error address") },
    { HRDATAD (CEFSTS,  ka_cefsts,  32, "Cbox fill error status") },
    { HRDATAD (CEFADR,  ka_cefadr,  32, "Cbox fill error address") },
    { HRDATAD (CMCDSR,  nca_cmcdsr, 32, "NCA control, diagnostic and status") },
    { NULL }
    };

MTAB sysd_mod[] = {
    { MTAB_XTD|MTAB_VDV|MTAB_NMO, 0, "LED", NULL,
      NULL, &sysd_show_led, NULL, "Display the diagnostic LED code" },
    { 0 },
    };

DEVICE sysd_dev = {
    "SYSD", sysd_unit, sysd_reg, sysd_mod,
    2, 16, 16, 1, 16, 8,
    NULL, NULL, &sysd_reset,
    NULL, NULL, NULL,
    NULL, DEV_DEBUG, 0, sysd_debug, NULL, NULL, &sysd_help, NULL, NULL,
    &sysd_description
    };

/* Interrupt controller helpers */

/* The TURBOchannel adapter's request (vax_tca.c): a level that follows the
   adapter's CSR, INT_REQ<0>; INT_CLR does not remove it */

void ka49_tca_int (int32 level)
{
if (level)
    int_req[IPL_SC] = int_req[IPL_SC] | INT_TCA;
else int_req[IPL_SC] = int_req[IPL_SC] & ~INT_TCA;
SET_IRQL;
}

static int32 int_mask_expanded (void)                  /* INT_MSK as int_req[1] bits */
{
int32 m = int_mask & (INT_TCA | INT_SC | INT_DZRX | INT_AUD);  /* sources with a known vector */

if (int_mask & INT_DZRX)                                /* one mask bit for both DZ directions */
    m = m | INT_DZTX;
return m;
}

/* The 53C94's request follows the chip's INT output: it goes away when
   the processor reads the chip's interrupt register, with no write to
   INT_CLR (vax4xx_rz94.c sets and clears INT_REQ<1> itself, RZ_INT_LEVEL
   in vax49_defs.h).

   The DZ's requests are latched.  The DC7085 asks for service whenever it
   re-evaluates its state (a register access, a character sent, its poll):
   vax4xx_dz.c then sets or clears DZRX and DZTX in int_req, as it does for
   every VAXstation.  A latched request counts only while the chip still
   asserts it by the DZ11 rules (receive: RIE and RDONE, or the silo alarm;
   transmit: TIE and TRDY; dz_int_level), the acknowledge removes it, and
   INT_CLR does not remove a request that the chip still asserts.  Two
   programs fix this behaviour, though not the circuit behind it (guess):

   - the console's DZ handlers write INT_CLR = 08 and go on serving the
     chip, and its interrupt-driven loopback test works only if a transmit
     request that is still asserted is seen again after the receive
     handler has cleared bit 3;
   - VMS's DZ driver sets TIE while the console's polled output has left
     line 3 enabled in TCR with TRDY set, and its transmit interrupt
     routine, having nothing to send, returns without touching the chip:
     a request that followed the chip as a level would interrupt it for
     ever (it did, in the first VMS boot of this model). */

static int32 dz_pending (void)                          /* latched DZ requests still asserted */
{
int32 lvl = dz_int_level ();
int32 r = 0;

if ((lvl & 1) && (int_req[IPL_DZRX] & INT_DZRX))
    r = r | INT_DZRX;
if ((lvl & 2) && (int_req[IPL_DZTX] & INT_DZTX))
    r = r | INT_DZTX;
return r;
}

static int32 int_pending (void)                         /* IPL 15 requests, as int_req[1] bits */
{
return (int_req[IPL_SC] & ~(INT_DZRX | INT_DZTX)) | dz_pending ();
}

static int32 int_req_reg (void)                         /* INT_REQ as the CPU reads it */
{
int32 p = int_pending ();
int32 r = p & 0xFF;

if (p & INT_DZTX)
    r = r | INT_DZRX;                                   /* the DZ is one bit */
return r;
}

/* Find highest priority outstanding interrupt */

int32 eval_int (void)
{
int32 ipl = PSL_GETIPL (PSL);
int32 i, t;

static const int32 sw_int_mask[IPL_SMAX] = {
    0xFFFE, 0xFFFC, 0xFFF8, 0xFFF0,                     /* 0 - 3 */
    0xFFE0, 0xFFC0, 0xFF80, 0xFF00,                     /* 4 - 7 */
    0xFE00, 0xFC00, 0xF800, 0xF000,                     /* 8 - B */
    0xE000, 0xC000, 0x8000                              /* C - E */
    };

if (hlt_pin)                                            /* hlt pin int */
    return IPL_HLTPIN;
if ((ipl < IPL_MEMERR) && ka_harderr)                   /* hard error */
    return IPL_MEMERR;
if ((ipl < IPL_CRDERR) && ka_softerr)                   /* soft error */
    return IPL_CRDERR;
if ((ipl < 0x17) && int_req[0x17 - IPL_HMIN])           /* IRQ_H<3>: no sources */
    return 0x17;
if ((ipl < IPL_CLK) && (tmr_int || int_req[IPL_CLK - IPL_HMIN]))
    return IPL_CLK;                                     /* interval timer */
if ((ipl < 0x15) && (int_pending () & int_mask_expanded ()))
    return 0x15;                                        /* board devices */
if ((ipl < 0x14) && int_req[0x14 - IPL_HMIN])           /* SGEC */
    return 0x14;
if (ipl >= IPL_SMAX)                                    /* ipl >= sw max? */
    return 0;
if ((t = SISR & sw_int_mask[ipl]) == 0)                 /* eligible req */
    return 0;
for (i = IPL_SMAX; i > ipl; i--) {                      /* check swre int */
    if ((t >> i) & 1)                                   /* req != 0? int */
        return i;
    }
return 0;
}

/* Return vector for highest priority hardware interrupt at IPL lvl.

   The NVAX runs an interrupt acknowledge and the NCA answers with the
   vector of the source; a vector of 0 is a passive release (nothing to
   take).  The 53C94 and DZ requests are levels that follow the chips
   (vax4xx_rz94.c), so the acknowledge clears nothing there; the DZ's
   latched request is removed by the acknowledge. */

int32 get_vector (int32 lvl)
{
int32 pend;

switch (lvl) {

    case IPL_MEMERR:                                    /* hard error */
        ka_harderr = 0;
        return SCB_MEMERR;

    case IPL_CRDERR:                                    /* soft error */
        ka_softerr = 0;
        return SCB_CRDERR;

    case IPL_CLK:                                       /* interval timer */
        tmr_int = 0;
        return SCB_INTTIM;

    case 0x14:                                          /* SGEC: its programmed vector; */
        if (int_req[IPL_NI] & INT_NI)                   /* the request is a level, cleared */
            return sgec_vector ();                      /* through its CSR5 */
        return 0;

    case 0x15:                                          /* board devices */
        pend = int_pending () & int_mask_expanded ();
        if (pend & INT_TCA)                             /* TCA: level, follows its CSR */
            return VEC_TCA;
        if (pend & INT_SC)                              /* 53C94: level, follows the chip */
            return VEC_SC;
        if (pend & INT_DZRX) {                          /* DZ: latched requests */
            CLR_INT (DZRX);
            return VEC_DZRX;
            }
        if (pend & INT_DZTX) {
            CLR_INT (DZTX);
            return VEC_DZTX;
            }
        if (pend & INT_AUD)
            return VEC_AUD;
        return 0;                                       /* passive release */

    default:
        if ((lvl < IPL_HMIN) || (lvl > IPL_MEMERR))
            ABORT (STOP_UIPL);                          /* unknown intr */
        return 0;                                       /* passive release */
        }
}

/* Configuration register value for the current memory size */

static int32 cfg_for_memory (void)
{
int32 v = CFG_TESTS | CFG_SERCON;                       /* serial console, run the tests */

if (lcspx_present ())                                   /* the graphics option */
    v = v | CFG_LCSPX;
if (tca_present ())                                     /* the TURBOchannel adapter */
    v = v | CFG_TCA;

switch ((uint32) (MEMSIZE >> 20)) {                     /* MB */

    case 16:                                            /* set 0, 4 MB SIMMs */
        return v | CFG_M0_PRES;

    case 32:                                            /* both sets, 4 MB SIMMs */
        return v | CFG_M0_PRES | CFG_M1_PRES;

    case 64:                                            /* set 0, 16 MB SIMMs */
        return v | CFG_M0_PRES | CFG_M0_16MB;

    case 80:                                            /* set 0 16 MB, set 1 4 MB */
        return v | CFG_M0_PRES | CFG_M0_16MB | CFG_M1_PRES;

    case 128:                                           /* both sets, 16 MB SIMMs */
    default:
        return v | CFG_M0_PRES | CFG_M0_16MB | CFG_M1_PRES | CFG_M1_16MB;
        }
}

/* Bcache tag check bits.  In normal operation the Cbox computes the check
   bits <16:11> of a tag entry from the rest of the entry.  The NVAX's
   check-bit matrix is not in the documents available to this project; the
   console's tag test writes six values and compares what comes back, which
   fixes the code for bits 9, 10, 20, 21 and 22 and its value for zero.  The
   other columns are left at zero: a guess that the console never sees. */

static uint32 bctag_ecc (uint32 tag)
{
static const struct { int32 bit; uint32 col; } cols[] = {
    {  9, 0x0000B000 },
    { 10, 0x0001E800 },
    { 20, 0x00009800 },
    { 21, 0x0001C000 },
    { 22, 0x0000C800 },
    { -1, 0 }
    };
uint32 ecc = 0x00009000;                                /* check bits of a zero tag */
int32 i;

for (i = 0; cols[i].bit >= 0; i++) {
    if (tag & (1u << cols[i].bit))
        ecc = ecc ^ cols[i].col;
    }
return ecc;
}

/* Bcache data check bits, as far as the console's software-ECC tests need
   them.  With CCTL<7> set, a quadword write stores BCDECC as its check
   bits, and a read latches into BCEDECC the syndrome (stored check bits
   XOR the check bits the data should have) of the last quadword of the
   32-byte block in wrap order, that is the quadword just before the one
   addressed.  The tests use zero data only; for zero data the check bits
   depend on the parity of the quadword's address (bits <17:3>): 02000200
   for even parity, 02C003C0 for odd.  That much is what the console
   expects; the rest of the function, for non-zero data, is a guess that
   keeps the syndrome of a correctly written quadword zero. */

static uint32 bcecc_check (uint32 qa, uint32 lo, uint32 hi)
{
uint32 a = (qa >> 3) & 0x7FFF;
uint32 par = 0;
uint32 chk;

while (a) {                                             /* parity of address <17:3> */
    par = par ^ (a & 1);
    a = a >> 1;
    }
chk = par? 0x02C003C0: 0x02000200;
lo = lo ^ (lo >> 16);                                   /* fold the data (guess) */
hi = hi ^ (hi >> 16);
return chk ^ ((lo & 0x3FF) << 6) ^ ((hi & 0x3FF) << 22);
}

static uint32 bcecc_index (uint32 pa)
{
return (pa >> 3) & (BCECC_ENTRIES - 1);
}

static uint32 *cdiag_word (uint32 pa);

static void bcecc_latch_read (uint32 pa)
{
uint32 qa = (pa & ~0x1Fu) | ((pa + 0x18) & 0x18);       /* previous quadword in the block */
uint32 idx = bcecc_index (qa);
uint32 *lop = cdiag_word (qa);
uint32 *hip = cdiag_word (qa + 4);
uint32 good = bcecc_check (qa, lop? *lop: 0, hip? *hip: 0);
uint32 stored = ka_bcecc_set[idx]? ka_bcecc[idx]: good;

ka_bcedecc = (int32) (stored ^ good);
}

static void bcecc_note_write (uint32 pa)
{
uint32 idx = bcecc_index (pa);

ka_bcecc[idx] = (uint32) ka_bcdecc;
ka_bcecc_set[idx] = 1;
}

/* Cache diagnostic modes (force hit, software ECC): memory references are
   routed through ReadReg/WriteReg (ADDR_IS_MEM in vax49_defs.h tests
   ka_cdiag) so that the model sees them.  Entering software-ECC mode
   forgets every stored check word: what was written before was written
   with correct check bits. */

void ka49_set_cdiag (void)
{
int32 swecc = ((ka_cctl & CCTL_SWECC) != 0);

if (swecc && ((ka_cdiag & CCTL_SWECC) == 0) && (ka_bcecc_set != NULL))
    memset (ka_bcecc_set, 0, BCECC_ENTRIES);
ka_cdiag = ka_cctl & (CCTL_FORCEHIT | CCTL_SWECC);
}

/* The longword a memory-space reference reaches in a cache diagnostic
   mode: the Bcache data RAM under force hit (indexed by the address bits
   below the cache size), memory otherwise; NULL if neither exists. */

static uint32 *cdiag_word (uint32 pa)
{
if (ka_cctl & CCTL_FORCEHIT)
    return &ka_bcdata[(pa & (BCDATA_SIZE - 1)) >> 2];
if (pa < MEMSIZE)
    return (uint32 *) &M[pa >> 2];
return NULL;
}

/* References above the top of memory.  A real 4000/90 (2026-10-26, a VMS
   user-mode program at 08000000 on a 128 MB machine):

   read     machine check 06; MESR 80100000 (<31> summary, <20>
            non-existent memory), CEFSTS 0000020A, CEFADR the address, MEAR
            the address plus 4 (02000001 for 08000000: PA<29:2> in <27:0>,
            commander nibble 0; one entry in three read 02000006).  VMS
            retries the instruction three times, each a machine check that
            latches the same values again (VMS clears them in between).
   write    posted, then the ownership read of the write miss fails: MESR
            C0100000 (<30> lost NDAL hard error as well), CEFSTS 0000018A,
            CCTL<31> (hardware error transition mode), MEAR the address plus
            18 (02000006), and the hard error interrupt: VMS bugchecked
            ASYNCWRTER.  Before the store's next instruction, so the program
            did not get to print that it was done.

   Everything is latched before the machine check, which does not return.
   No lost-error bits are modelled. */

static void mem_read_error (uint32 pa)
{
nmc_mesr = nmc_mesr | MESR_ERRSUM | MESR_NXM;
nmc_mear = (int32) (((pa >> 2) + 1) & 0x0FFFFFFF);
ka_cefsts = CEFSTS_MEMREAD;
ka_cefadr = (int32) (pa & CEFADR_ALIGN);
}

static void mem_write_error (uint32 pa)
{
nmc_mesr = nmc_mesr | MESR_ERRSUM | MESR_LOSTHARD | MESR_NXM;
nmc_mear = (int32) (((pa >> 2) + 6) & 0x0FFFFFFF);
ka_cefsts = CEFSTS_MEMWRITE;
ka_cefadr = (int32) (pa & CEFADR_ALIGN);
ka_cctl = ka_cctl | CCTL_HWETM;
ka_harderr = 1;
SET_IRQL;
}

static int32 mem_rd (uint32 pa)
{
uint32 *wp = cdiag_word (pa);

if (wp == NULL) {                                       /* no memory there */
    sim_debug (DBG_NXM, &sysd_dev, "read of non-existent memory %08X, PC %08X\n", pa, fault_PC);
    mem_read_error (pa);
    MACH_CHECK (MCHK_READ);
    }
if ((ka_cctl & CCTL_ENABLE) && (ka_cctl & CCTL_SWECC))
    bcecc_latch_read (pa);
return (int32) *wp;
}

static void mem_wr (uint32 pa, int32 val, int32 lnt)
{
uint32 *wp = cdiag_word (pa);

if (wp == NULL) {                                       /* no memory there */
    sim_debug (DBG_NXM, &sysd_dev, "write of non-existent memory %08X, PC %08X\n", pa, fault_PC);
    mem_write_error (pa);
    return;
    }
if (lnt == L_BYTE) {
    int32 sc = (pa & 3) << 3;
    *wp = (*wp & ~(0xFFu << sc)) | (((uint32) val & 0xFF) << sc);
    }
else if (lnt == L_WORD)
    *wp = (pa & 2)? (*wp & 0xFFFF) | ((uint32) val << 16): (*wp & ~0xFFFFu) | ((uint32) val & 0xFFFF);
else
    *wp = (uint32) val;
if (ka_cctl & CCTL_SWECC)
    bcecc_note_write (pa);
}

/* Read KA49 specific IPR's */

int32 ReadIPR (int32 rg)
{
int32 val = 0;
uint32 prn = (uint32) rg;

if (prn >= MT_BCTAG_BASE) {                             /* block-decoded registers */
    if (prn < MT_BCTAG_END)
        val = ka_bctag[(prn - MT_BCTAG_BASE) >> 5];
    else if ((prn >= MT_PCTAG_BASE) && (prn < MT_PCTAG_END))
        val = ka_pctag[(prn - MT_PCTAG_BASE) >> 5];
    else if ((prn >= MT_PCDAP_BASE) && (prn < MT_PCDAP_END))
        val = ka_pcdap[(prn - MT_PCDAP_BASE) >> 3];
    else
        RSVD_OPND_FAULT(ReadIPR);                       /* Bcache deallocate is write only */
    sim_debug (DBG_IPR, &sysd_dev, "MFPR %08X = %08X\n", prn, val);
    return val;
    }

switch (rg) {

    case MT_CPUID:                                      /* CPUID */
        val = ka_cpuid;
        break;

    case MT_ICCS:                                       /* interval timer */
        val = iccs_rd ();
        break;

    case MT_NICR:
        val = nicr_rd ();
        break;

    case MT_ICR:
        val = icr_rd ();
        break;

    case MT_SAVPC:                                      /* SAVPC */
        val = ka_savpc;
        break;

    case MT_SAVPSL:                                     /* SAVPSL */
        val = ka_savpsl;
        break;

    case MT_SID:                                        /* SID */
        val = VAX49_SID_TYPE | (ka49_models[ka49_model].patch << VAX49_SID_V_PATCH) | VAX49_SID_UREV;
        break;

    case MT_INTSYS:                                     /* INTSYS: nothing pending to report (guess) */
        val = 0;
        break;

    case MT_PMFCNT:
        val = ka_pmfcnt;
        break;

    case MT_PCSCR:                                      /* report the model's patch, not the one loaded */
        val = ka_pcscr;
        if (val & PCSCR_ENABLE)
            val = (val & ~(PCSCR_M_PATCH << PCSCR_V_PATCH)) |
                  (ka49_models[ka49_model].patch << PCSCR_V_PATCH);
        break;

    case MT_ECR:
        val = ka_ecr;
        break;

    case MT_CCTL:
        val = ka_cctl;
        break;

    case MT_BCETSTS:                                    /* error capture: nothing captured */
        val = ka_bcetsts;
        break;

    case MT_BCEDSTS:
        val = ka_bcedsts;
        break;

    case MT_BCEDECC:                                    /* syndrome latched in software-ECC mode */
        val = ka_bcedecc;
        break;

    case MT_BCETIDX: case MT_BCETAG: case MT_BCEDIDX:
    case MT_NEOADR:
    case MT_NEDATHI: case MT_NEDATLO:
        val = 0;
        break;

    case MT_CEFADR:                                     /* address of the last failed fill */
        val = ka_cefadr;
        break;

    case MT_NEOCMD:                                     /* last NDAL commands */
        val = NEOCMD_IDLE;
        break;

    case MT_NEICMD:
        val = ka49_models[ka49_model].neicmd;
        break;

    case MT_CEFSTS:
        val = ka_cefsts;
        break;

    case MT_NESTS:
        val = ka_nests;
        break;

    case MT_VMAR:
        val = ka_vmar;
        break;

    case MT_VTAG:
        val = ka_vtag;
        break;

    case MT_VDATA:
        val = ka_vdata;
        break;

    case MT_ICSR:
        val = ka_icsr & ICSR_RW;
        break;

    case MT_BPCR:
        val = ka_bpcr & ~BPCR_W_ONLY;
        break;

    case MT_BPC: case MT_BPCUNW:
        val = 0;
        break;

    case MT_MP0BR:                                      /* Mbox copies */
        val = P0BR;
        break;

    case MT_MP0LR:
        val = P0LR;
        break;

    case MT_MP1BR:
        val = P1BR;
        break;

    case MT_MP1LR:
        val = P1LR;
        break;

    case MT_MSBR:
        val = SBR;
        break;

    case MT_MSLR:
        val = SLR;
        break;

    case MT_MMAPEN:
        val = mapen & 1;
        break;

    case MT_PAMODE:
        val = ka_pamode;
        break;

    case MT_MMEADR: case MT_MMEPTE: case MT_MMESTS:
    case MT_TBADR: case MT_PCADR:
        val = 0;
        break;

    case MT_TBSTS:
        val = TBSTS_IDLE;
        break;

    case MT_PCSTS:
        val = PCSTS_ONES;
        break;

    case MT_PCCTL:
        val = PCCTL_ONES | (ka49_models[ka49_model].pcctl_red << PCCTL_V_RED) | (ka_pcctl & PCCTL_RW);
        break;

    default:                                            /* MCESR, MTBTAG, MTBPTE, BCDECC are */
        sim_debug (DBG_IPR, &sysd_dev, "MFPR %02X: not readable\n", rg);
        RSVD_OPND_FAULT(ReadIPR);                       /* write only; the rest do not exist */
        }

sim_debug (DBG_IPR, &sysd_dev, "MFPR %02X = %08X\n", rg, val);
return val;
}

/* Write KA49 specific IPR's */

void WriteIPR (int32 rg, int32 val)
{
uint32 prn = (uint32) rg;

if (prn >= MT_BCTAG_BASE) {                             /* block-decoded registers */
    sim_debug (DBG_IPR, &sysd_dev, "MTPR %08X, %08X\n", val, prn);
    if (prn < MT_BCTAG_END) {                           /* Bcache tag */
        uint32 t = ((uint32) val) & BCTAG_STORED;
        if ((ka_cctl & CCTL_TAGDIAG) == 0)              /* normal: Cbox computes the check bits */
            t = (t & ~BCTAG_ECC) | bctag_ecc (t & ~BCTAG_ECC);
        ka_bctag[(prn - MT_BCTAG_BASE) >> 5] = t;
        }
    else if ((prn >= MT_BCFLU_BASE) && (prn < MT_BCFLU_END))
        ;                                               /* deallocate: no Bcache data to write back */
    else if ((prn >= MT_PCTAG_BASE) && (prn < MT_PCTAG_END))
        ka_pctag[(prn - MT_PCTAG_BASE) >> 5] = val;
    else if ((prn >= MT_PCDAP_BASE) && (prn < MT_PCDAP_END))
        ka_pcdap[(prn - MT_PCDAP_BASE) >> 3] = val;
    else
        RSVD_OPND_FAULT(WriteIPR);
    return;
    }

sim_debug (DBG_IPR, &sysd_dev, "MTPR %08X, %02X\n", val, rg);
switch (rg) {

    case MT_CPUID:                                      /* CPUID */
        ka_cpuid = val;
        break;

    case MT_ICCS:                                       /* interval timer */
        iccs_wr (val);
        break;

    case MT_NICR:
        nicr_wr (val);
        break;

    case MT_ICR:                                        /* read only */
        break;

    case MT_MCESR:                                      /* machine check acknowledge */
        ka_mchk_busy = 0;
        break;

    case MT_SAVPC:
        ka_savpc = val;
        break;

    case MT_SAVPSL:
        ka_savpsl = val;
        break;

    case MT_INTSYS:
        ka_intsys = val;
        break;

    case MT_PMFCNT:
        ka_pmfcnt = val;
        break;

    case MT_PCSCR:                                      /* the microcode patch load: accepted */
        ka_pcscr = val;
        break;

    case MT_ECR:
        ka_ecr = val;
        break;

    case MT_MTBTAG: case MT_MTBPTE:                     /* TB fill: the TB is SIMH's */
        break;

    case MT_CCTL:                                       /* <31> HW_ETM: write one to clear */
        ka_cctl = (val & ~CCTL_HWETM) | (ka_cctl & CCTL_HWETM & ~val);
        ka49_set_cdiag ();
        break;

    case MT_BCDECC:
        ka_bcdecc = val;
        break;

    case MT_BCETSTS:                                    /* W1C error status */
        ka_bcetsts = ka_bcetsts & ~val;
        break;

    case MT_BCEDSTS:
        ka_bcedsts = ka_bcedsts & ~val;
        break;

    case MT_CEFSTS:                                     /* <4:0> write one to clear */
        ka_cefsts = ka_cefsts & ~(val & CEFSTS_W1C);
        break;

    case MT_NESTS:
        ka_nests = ka_nests & ~val;
        break;

    case MT_VMAR:
        ka_vmar = val;
        break;

    case MT_VTAG:
        ka_vtag = val;
        break;

    case MT_VDATA:
        ka_vdata = val;
        break;

    case MT_ICSR:
        ka_icsr = val & ICSR_RW;
        break;

    case MT_BPCR:
        ka_bpcr = val;
        break;

    case MT_PAMODE:
        ka_pamode = val & PAMODE_32;
        if (ka_pamode)
            sim_printf ("%%SIM-INFO: PAMODE set to 32-bit physical addressing, which this model does not implement\n");
        break;

    case MT_TBSTS: case MT_PCSTS:                       /* W1C error status, never set */
        break;

    case MT_PCCTL:
        ka_pcctl = val & PCCTL_RW;
        break;

    default:
        RSVD_OPND_FAULT(WriteIPR);
        }
return;
}

/* External processor registers in I/O space (2100.0000 + 4n).  The
   interval timer (n = 18, 19, 1A) is the one the console uses for every
   delay; n = 44 (2100.0110) is written 0 once at power-up and never read
   (its purpose: unknown).  Any other register reads as absent. */

int32 xipr_rd (int32 pa)
{
int32 n = (pa - XIPRBASE) >> 2;

switch (n) {

    case MT_ICCS:
        return iccs_rd ();

    case MT_NICR:
        return nicr_rd ();

    case MT_ICR:
        return icr_rd ();

    case 0x44:
        return ka_xipr44;
        }
sim_debug (DBG_NXM, &sysd_dev, "read of absent external IPR %02X (%08X), PC %08X\n", n, pa, fault_PC);
MACH_CHECK (MCHK_READ);
return 0;
}

void xipr_wr (int32 pa, int32 val, int32 lnt)
{
int32 n = (pa - XIPRBASE) >> 2;

switch (n) {

    case MT_ICCS:
        iccs_wr (val);
        break;

    case MT_NICR:
        nicr_wr (val);
        break;

    case 0x44:
        ka_xipr44 = val;
        break;

    default:
        sim_debug (DBG_NXM, &sysd_dev, "write of absent external IPR %02X (%08X) = %08X\n", n, pa, val);
        break;
        }
}

/* NMC O-bit RAM: 2048 entries of 24 bits visible at a time, one every 16
   bytes from 2101.0000, in 512 pages selected by MOAMR<14:6>.  The console
   clears all of it at power-up and the CACHE test writes and reads every
   entry, so it is plain storage here. */

static uint32 obit_index (int32 pa)
{
return (((nmc_moamr >> MOAMR_V_PAGE) & MOAMR_M_PAGE) * OBIT_ENTRIES) +
       (((pa - OBITBASE) >> 4) & (OBIT_ENTRIES - 1));
}

int32 obit_rd (int32 pa)
{
return nmc_obit[obit_index (pa)] & OBIT_MASK;
}

void obit_wr (int32 pa, int32 val, int32 lnt)
{
nmc_obit[obit_index (pa)] = val & OBIT_MASK;
}

/* The hard error request (ERR_H, SCB 0x60 at IPL 1D) stays up while a
   hard error is latched and unserved.  The vector acknowledge takes it
   down (get_vector) and so does clearing the error: a real 4000/90's
   console cleared the latch of a failed write with write-one-to-clear
   stores and saw no ?33 UNXINT when it then lowered the IPL (2026-10-26).
   Only writes raise it; a failed read is reported by its machine check. */

static void hard_error_check (void)
{
if (((nca_cesr & CESR_ERRSUM) == 0) && ((nmc_mesr & MESR_ERRSUM) == 0)) {
    ka_harderr = 0;
    SET_IRQL;
    }
}

/* NMC registers.  MEAR on a real 4000/90 read 18406010 with no error
   latched: (2101.8040 >> 2) with 1 in the top nibble, i.e. the address of
   the last NMC access (the read of MEAR itself), tagged with the commander
   ID.  The model reproduces that (interpretation). */

int32 nmc_rd (int32 pa)
{
int32 rg = (pa - NMCBASE) >> 2;
int32 val;

nmc_last = (uint32) pa & ~3u;
switch (rg) {

    case 0: case 1: case 2: case 3:                     /* MEMCON0-7 */
    case 4: case 5: case 6: case 7:
        val = nmc_memcon[rg];
        break;

    case 0x10:                                          /* MEAR: the error address while a memory error is latched */
        if (nmc_mesr & MESR_NXM)
            val = nmc_mear;
        else
            val = (int32) (0x10000000 | ((nmc_last & 0x3FFFFFFF) >> 2));
        break;

    case 0x11:                                          /* MESR */
        val = nmc_mesr;
        break;

    case 0x12:                                          /* MMCDSR */
        val = nmc_mmcdsr;
        break;

    case 0x13:                                          /* MOAMR */
        val = nmc_moamr;
        break;

    default:
        val = 0;
        break;
        }
sim_debug (DBG_REG, &sysd_dev, "NMC read %08X = %08X\n", pa, val);
return val;
}

void nmc_wr (int32 pa, int32 val, int32 lnt)
{
int32 rg = (pa - NMCBASE) >> 2;

if (lnt < L_LONG) {                                     /* merge partial writes */
    int32 sc = (pa & 3) << 3;
    int32 mask = (lnt == L_WORD)? 0xFFFF: 0xFF;
    int32 cur = nmc_rd (pa & ~3);
    val = ((val & mask) << sc) | (cur & ~(mask << sc));
    }
nmc_last = (uint32) pa & ~3u;
sim_debug (DBG_REG, &sysd_dev, "NMC write %08X = %08X\n", pa, val);
switch (rg) {

    case 0: case 1: case 2: case 3:                     /* MEMCON0-7 */
    case 4: case 5: case 6: case 7:
        nmc_memcon[rg] = val;
        break;

    case 0x11:                                          /* MESR: write 1 to clear */
        nmc_mesr = nmc_mesr & ~val;
        hard_error_check ();
        break;

    case 0x12:                                          /* MMCDSR */
        nmc_mmcdsr = val;
        break;

    case 0x13:                                          /* MOAMR */
        nmc_moamr = val;
        break;

    default:                                            /* MEAR is read only */
        break;
        }
}

/* MMCDSR at power-up.  A real 4000/90 read 08942E40 at its console, the
   console having set bit 6 (its "memory initialised" marker); the rest is
   taken as the power-up state (a guess: VMS reprograms the register to
   08E6C600 and the console was entered after VMS had run). */

#define MMCDSR_INIT     0x08942E00

/* NCA registers.  CMCDSR read 0000F108 on a real 4000/90, which is what
   the console's power-up code (clear <1:0>, set <8> and <3>) leaves of a
   reset value of 0000F000 (the reset value is therefore a guess). */

#define CMCDSR_INIT     0x0000F000

int32 nca_rd (int32 pa)
{
int32 rg = (pa - NCABASE) >> 2;

switch (rg) {

    case 0:                                             /* CESR */
        return nca_cesr;

    case 1:                                             /* CMCDSR */
        return nca_cmcdsr;

    case 5:                                             /* CIOEAR2 */
        return nca_cioear2;

    default:                                            /* the other error addresses: nothing latched */
        return 0;
        }
}

void nca_wr (int32 pa, int32 val, int32 lnt)
{
int32 rg = (pa - NCABASE) >> 2;

if (lnt < L_LONG) {
    int32 sc = (pa & 3) << 3;
    int32 mask = (lnt == L_WORD)? 0xFFFF: 0xFF;
    int32 cur = nca_rd (pa & ~3);
    val = ((val & mask) << sc) | (cur & ~(mask << sc));
    }
switch (rg) {

    case 0:                                             /* CESR: write 1 to clear */
        nca_cesr = nca_cesr & ~val;
        hard_error_check ();
        break;

    case 1:                                             /* CMCDSR */
        nca_cmcdsr = val;
        break;
        }
}

/* Configuration and LED registers (2580.0000, 2580.0004).  The LED
   register holds the complement of the code the eight LEDs display: the
   console writes MCOMB #^XFE to show FE.  How the LED register reads back
   is not known; the model returns what was written. */

/* A write to the LED register goes out over the I/O bus and takes the
   NVAX about 1.5 microseconds.  The console's IT test depends on it: it
   counts the interval timer's 1 ms interrupts during 100000 iterations of
   a two-instruction loop that writes the LED register, and accepts 128 to
   187 of them, so one iteration must take 1.28 to 1.87 microseconds.  The
   simulator executes the loop in a few host nanoseconds, so the model
   charges each LED write 1.55 microseconds of simulated time (in calibrated
   instruction units, the time base of the interval timer). */

#define LED_WRITE_USECS 1.55

extern int32 tmr_poll;                                  /* instructions per 10 ms */

#define CFG_UNDRIVEN    0xC000                          /* <15:14> read as ones */

int32 cfg_rd (int32 pa)
{
int32 w;

if (pa & 4)                                             /* LED */
    return ka_led & 0xFF;
w = CFG_UNDRIVEN | (ka_cfg & 0x3FFF);
return (w << 16) | w;                                   /* the word in both halves */
}

void cfg_wr (int32 pa, int32 val, int32 lnt)
{
if ((pa & 4) == 0)                                      /* configuration: read only */
    return;
val = val & 0xFF;
if (tmr_poll > 0)                                       /* bus write latency */
    sim_interval = sim_interval - (int32) ((double) tmr_poll * LED_WRITE_USECS / 10000.0);
if (val != ka_led)
    sim_debug (DBG_LED, &sysd_dev, "LED %02X (register %02X), PC %08X\n", (~val) & 0xFF, val, fault_PC);
ka_led = val;
}

/* Board registers (25C0.0000)

   +00  SCSI DMA window address (22 bits), the SQWF's address counter: a
        read gives the running address (00001600 on a real 4000/90, read
        at its console)
   +04  SCSI DMA direction: 0 = data in (to memory), 1 = data out; the real
        machine read 00001601 with the address at 1600, so a read returns
        the address with the direction in bit 0 (guess)
   +08  INT_MSK (byte): read as FFFF00xx on a real 4000/90
   +0C  INT_REQ (byte, read) / INT_CLR (write ones to clear): read as
        FFFFFFxx
   +10  IORESET (byte, write): the console writes 3 then 0 at power-up and
        in INIT; which devices each bit resets is not known, so the model
        takes no action */

int32 ka_rd (int32 pa)
{
int32 rg = (pa - KABASE) >> 2;
int32 val;

switch (rg) {

    case 0:                                             /* DMA address */
        val = ka_dmaadr & 0x003FFFFF;
        break;

    case 1:                                             /* DMA direction */
        val = (ka_dmaadr & 0x003FFFFE) | (ka_dmadir & 1);
        break;

    case 2:                                             /* INT_MSK */
        val = (int32) (0xFFFF0000u | (uint32) (int_mask & 0xFF));
        break;

    case 3:                                             /* INT_REQ */
        val = (int32) (0xFFFFFF00u | (uint32) int_req_reg ());
        break;

    default:                                            /* IORESET */
        val = 0;
        break;
        }
sim_debug (DBG_REG, &sysd_dev, "board read %08X = %08X\n", pa, val);
return val;
}

void ka_wr (int32 pa, int32 val, int32 lnt)
{
int32 rg = (pa - KABASE) >> 2;

sim_debug (DBG_REG, &sysd_dev, "board write %08X = %08X (length %d)\n", pa, val, lnt);
switch (rg) {

    case 0:                                             /* DMA address */
        ka_dmaadr = val & 0x003FFFFF;                   /* the 53C94's DMA starts here */
        break;

    case 1:                                             /* DMA direction */
        ka_dmadir = val & 1;
        break;

    case 2:                                             /* INT_MSK */
        if ((pa & 3) == 0) {
            int_mask = val & 0xFF;
            sim_debug (DBG_INT, &sysd_dev, "INT_MSK = %02X\n", int_mask);
            SET_IRQL;
            }
        break;

    case 3:                                             /* INT_CLR */
        if ((pa & 3) == 0) {
            int32 clr = val & 0xFF;
            sim_debug (DBG_INT, &sysd_dev, "INT_CLR %02X (requests %02X)\n", clr, int_req_reg ());
            if (clr & INT_DZRX) {                       /* the DZ bit: both directions, */
                int32 lvl = dz_int_level ();            /* except a request still asserted */
                clr = clr & ~INT_DZRX;
                if ((lvl & 1) == 0)
                    clr = clr | INT_DZRX;
                if ((lvl & 2) == 0)
                    clr = clr | INT_DZTX;
                }
            clr = clr & ~(INT_SC | INT_TCA);            /* the 53C94 and TCA requests follow their source */
            int_req[IPL_SC] = int_req[IPL_SC] & ~clr;
            SET_IRQL;
            }
        break;

    case 4:                                             /* IORESET */
        ka_ioreset = val & 0xFF;
        break;
        }
}

/* SCSI scatter/gather map (2700.0000) */

static uint32 sgm_read_form (uint32 w)
{
uint32 pfn = w & SGM_W_PFN;
uint32 v = (w & SGM_W_VALID)? 1: 0;
uint32 ones = v;
uint32 t = pfn;
uint32 r;

while (t) {
    ones = ones + (t & 1);
    t = t >> 1;
    }
r = (pfn << SGM_R_V_PFN) | (v? SGM_R_VALID: 0);
if (((ones & 1) == 0) != ((w & SGM_W_BADPAR) != 0))     /* odd parity, unless forced bad */
    r = r | SGM_R_PARITY;
return r;
}

int32 sgm_rd (int32 pa)
{
return (int32) sgm_read_form (ka_sgmap[((pa - SGMBASE) >> 2) & (SGM_ENTRIES - 1)]);
}

void sgm_wr (int32 pa, int32 val, int32 lnt)
{
uint32 idx = ((pa - SGMBASE) >> 2) & (SGM_ENTRIES - 1);

if (lnt < L_LONG) {                                     /* merge into the entry as written (guess) */
    int32 sc = (pa & 3) << 3;
    uint32 mask = (lnt == L_WORD)? 0xFFFF: 0xFF;
    val = (int32) ((((uint32) val & mask) << sc) | (ka_sgmap[idx] & ~(mask << sc)));
    }
ka_sgmap[idx] = (uint32) val & (SGM_W_VALID | SGM_W_BADPAR | SGM_W_PFN);
}

/* SCSI DMA through the scatter/gather map (the SQWF).  The 53C94's DMA
   address is a 22-bit window address: bits <21:9> select one of the 8192
   map entries, bits <8:0> the byte in that 512-byte page.  An entry that
   is not valid stops the transfer and sets the map error request, INT_REQ
   bit 7, which the console polls after every SCSI transfer.

   Map_ReadB    -       fetch byte buffer from memory
   Map_ReadW    -       fetch word buffer from memory
   Map_WriteB   -       store byte buffer into memory
   Map_WriteW   -       store word buffer into memory

   Each returns the number of bytes not transferred. */

static t_bool sgm_map_addr (uint32 wa, uint32 *ma)
{
uint32 ent = ka_sgmap[(wa >> 9) & (SGM_ENTRIES - 1)];

if (ent & SGM_W_VALID) {
    *ma = ((ent & SGM_W_PFN) << 9) | (wa & 0x1FF);
    if (*ma < MEMSIZE)
        return TRUE;
    }
SET_INT (SGMERR);                                       /* map error */
return FALSE;
}

int32 Map_ReadB (uint32 ba, int32 bc, uint8 *buf)
{
int32 i;
uint32 ma = 0;

for (i = 0; i < bc; i++) {
    if ((i == 0) || (((ba + i) & 0x1FF) == 0)) {        /* new page? */
        if (!sgm_map_addr (ba + i, &ma))
            return (bc - i);
        }
    buf[i] = (uint8) ((M[ma >> 2] >> ((ma & 3) << 3)) & 0xFF);
    ma = ma + 1;
    }
return 0;
}

int32 Map_ReadW (uint32 ba, int32 bc, uint16 *buf)
{
int32 i;
uint32 ma = 0;

ba = ba & ~1u;
bc = bc & ~1;
for (i = 0; i < bc; i = i + 2) {
    if ((i == 0) || (((ba + i) & 0x1FF) == 0)) {        /* new page? */
        if (!sgm_map_addr (ba + i, &ma))
            return (bc - i);
        }
    buf[i >> 1] = (uint16) ((M[ma >> 2] >> ((ma & 2) << 3)) & 0xFFFF);
    ma = ma + 2;
    }
return 0;
}

int32 Map_WriteB (uint32 ba, int32 bc, uint8 *buf)
{
int32 i;
uint32 ma = 0;

for (i = 0; i < bc; i++) {
    if ((i == 0) || (((ba + i) & 0x1FF) == 0)) {        /* new page? */
        if (!sgm_map_addr (ba + i, &ma))
            return (bc - i);
        }
    M[ma >> 2] = (M[ma >> 2] & ~(0xFFu << ((ma & 3) << 3))) | (((uint32) buf[i]) << ((ma & 3) << 3));
    ma = ma + 1;
    }
return 0;
}

int32 Map_WriteW (uint32 ba, int32 bc, uint16 *buf)
{
int32 i;
uint32 ma = 0;

ba = ba & ~1u;
bc = bc & ~1;
for (i = 0; i < bc; i = i + 2) {
    if ((i == 0) || (((ba + i) & 0x1FF) == 0)) {        /* new page? */
        if (!sgm_map_addr (ba + i, &ma))
            return (bc - i);
        }
    M[ma >> 2] = (M[ma >> 2] & ~(0xFFFFu << ((ma & 2) << 3))) | (((uint32) buf[i >> 1]) << ((ma & 2) << 3));
    ma = ma + 2;
    }
return 0;
}

/* The SQWF, the board's DMA engine for the 53C94 (vax4xx_rz94.c calls
   these through RZ_DMA_READ, RZ_DMA_WRITE and RZ_DMA_DONE): it moves the
   data between the chip and memory through the scatter/gather map, from
   the window address in the DMA address register, and its address counter
   moves on by every byte that crosses the SCSI bus.  The console relies on
   that: a transfer longer than the 53C94's 64 KB count is done as several
   DMA commands, between which the console reloads only the 53C94's count
   (ROM 1B368, 1B124); the address register is rewritten only after a
   phase change. */

int32 sqwf_read (uint32 bc, uint8 *buf)
{
return Map_ReadB ((uint32) ka_dmaadr, (int32) bc, buf);
}

int32 sqwf_write (uint32 bc, uint8 *buf)
{
return Map_WriteB ((uint32) ka_dmaadr, (int32) bc, buf);
}

void sqwf_done (uint32 n)
{
ka_dmaadr = (ka_dmaadr + (int32) n) & 0x003FFFFF;
}

/* Network address ROM (2780.0000): each byte sits in the low byte of its
   longword and again in bits <23:16>; bits <15:8> and <31:24> read as ones
   (a real 4000/90 read FF08FF08 for a byte 08).  The contents are
   vax_nar.c's, built from SET NAR MAC=. */

int32 ka_nar_rd (int32 pa)
{
int32 b = nar_rd (pa) & 0xFF;

return (int32) (0xFF00FF00u | ((uint32) b << 16) | (uint32) b);
}

/* Audio placeholder (2680.0000).  The AM79C30A is not modelled.  The
   console's AUD test only plays tones through it (it writes the chip's
   indirect registers and reads the interrupt register once), so the model
   accepts the writes and reads the eight direct registers as zero: an idle
   chip with no interrupt pending. */

#define AUD_BASE        0x26800000
#define AUD_SIZE        0x20

int32 aud_rd (int32 pa)
{
return 0;
}

void aud_wr (int32 pa, int32 val, int32 lnt)
{
return;
}

/* Read/write I/O register space

   These routines are the 'catch all' for address space map.  Any
   address that doesn't explicitly belong to memory, I/O, or ROM
   is given to these routines for processing.
*/

struct reglink {                                        /* register linkage */
    uint32      low;                                    /* low addr */
    uint32      high;                                   /* high addr */
    int32       (*read)(int32 pa);                      /* read routine */
    void        (*write)(int32 pa, int32 val, int32 lnt); /* write routine */
    };

struct reglink regtable[] = {
    { SGECBASE, SGECBASE+SGEC_CSRSIZE, &sgec_rd, &sgec_wr },
    { ROMBASE, ROMBASE+ROMSIZE, &rom_rd, NULL },
    { XIPRBASE, XIPRBASE+XIPRSIZE, &xipr_rd, &xipr_wr },
    { OBITBASE, OBITBASE+OBITSIZE, &obit_rd, &obit_wr },
    { NMCBASE, NMCBASE+NMCSIZE, &nmc_rd, &nmc_wr },
    { NCABASE, NCABASE+NCASIZE, &nca_rd, &nca_wr },
    { DZBASE, DZBASE+DZSIZE, &dz_rd, &dz_wr },
    { RZBASE, RZBASE+RZSIZE, &rz_rd, &rz_wr },
    { AUD_BASE, AUD_BASE+AUD_SIZE, &aud_rd, &aud_wr },
    { NVRBASE, NVRBASE+NVRSIZE, &nvr_rd, &nvr_wr },
    { CFGBASE, CFGBASE+CFGSIZE, &cfg_rd, &cfg_wr },
    { KABASE, KABASE+KASIZE, &ka_rd, &ka_wr },
    { SGMBASE, SGMBASE+SGMSIZE, &sgm_rd, &sgm_wr },
    { NARBASE, NARBASE+NARSPAN, &ka_nar_rd, NULL },
    { LCSPX_BASE, LCSPX_BASE+LCSPX_WINSIZE, &lcspx_rd, &lcspx_wr },
    { TCA_SLOT, TCA_SLOT+TCA_SLOTSIZE, &tca_slot_rd, &tca_slot_wr },
    { TCA_TRIG, TCA_TRIG+TCA_TRIGSIZE, &tca_trig_rd, &tca_trig_wr },
    { TCA_FIFO, TCA_FIFO+TCA_FIFOSIZE, &tca_fifo_rd, &tca_fifo_wr },
    { TCA_MAP, TCA_MAP+TCA_MAPSIZE, &tca_map_rd, &tca_map_wr },
    { TCA_BASE, TCA_BASE+TCA_WINSIZE, &tca_rd, &tca_wr },
    { TCA_CSR, TCA_CSR+TCA_CSRSIZE, &tca_csr_rd, &tca_csr_wr },
    { 0, 0, NULL, NULL }
    };

/* What the board latches when a read of I/O space is not answered was
   measured on a real 4000/90 (2026-10-26, a read of 3C000000 at the console
   and from a VMS user-mode program): CESR <31> and <15> (CP2 master
   transaction timeout), CIOEAR2 the address PA<28:0>, CEFSTS 0001920A and
   CEFADR the fill address.  The read raises no hard error interrupt: the
   machine check is the report. */

static void io_read_error (uint32 pa)
{
nca_cesr = nca_cesr | CESR_ERRSUM | CESR_CP2_MTT;
nca_cioear2 = (int32) (pa & CIOEAR_ADDR);
ka_cefsts = CEFSTS_IOREAD;
ka_cefadr = (int32) ((pa & CEFADR_ALIGN) | CEFADR_IO);
}

/* ReadReg - read register space

   Inputs:
        pa      =       physical address
        lnt     =       length (BWLQ) - ignored
   Output:
        longword of data

   A read of an address that nothing answers is a machine check on the
   NVAX: the NCA reports a bus timeout and the read never completes.  The
   console relies on this when it probes for options; returning ones (as
   the KA46 model does) would make absent devices look present. */

int32 ReadReg (uint32 pa, int32 lnt)
{
struct reglink *p;

if (pa < CACHEABLE_END)                                 /* cache diagnostic mode: memory */
    return mem_rd (pa);
for (p = &regtable[0]; p->low != 0; p++) {
    if ((pa >= p->low) && (pa < p->high) && p->read)
        return p->read (pa);
    }
sim_debug (DBG_NXM, &sysd_dev, "read of non-existent address %08X, PC %08X\n", pa, fault_PC);
io_read_error (pa);                                     /* latch before the machine check: it does not return */
MACH_CHECK (MCHK_READ);
return 0;
}

/* ReadRegU - read register space, unaligned

   Inputs:
        pa      =       physical address
        lnt     =       length in bytes (1, 2, or 3)
   Output:
        returned data, not shifted
*/

int32 ReadRegU (uint32 pa, int32 lnt)
{
return ReadReg (pa & ~03, L_LONG);
}

/* WriteReg - write register space

   Inputs:
        pa      =       physical address
        val     =       data to write, right justified in 32b longword
        lnt     =       length (BWLQ)
   Outputs:
        none

   A write to an address that nothing answers is not a machine check on
   the NVAX (writes are posted); the hardware reports it later with a hard
   error interrupt.

   What a real 4000/90 did with a store to 3C000000 (2026-10-26): the
   console's deposit and a VMS user-mode program's store both completed at
   once and silently; some milliseconds later the NCA latched CESR <31,15>
   (CP2 master transaction timeout) and CIOEAR2, raised the hard error and
   VMS bugchecked (ASYNCWRTER) in the same 10 ms tick, after the program
   had printed that it was done.  INTSYS read 0 at the console with the
   error latched, so the request is not visible there (the model's INTSYS
   is always 0).  CIOEAR2 read 3C000000 after the VMS write, PA<28:0> with
   <29> set, where every read gave 1C000000, and the console's write left
   1C000000: modelled as a function of the Bcache enable (CCTL<0>, set under
   VMS, clear at the console), the one thing measured to differ.  That is a
   guess; the alternative is that the console's write never updated
   CIOEAR2.  A second write that is not answered while the first is
   pending is not reported (no lost-error bits are modelled). */

static void io_write_error (uint32 pa)
{
if (sim_is_active (IOWR_UNIT))
    return;
IOWR_UNIT->u3 = (int32) pa;
sim_activate_after (IOWR_UNIT, IOERR_DELAY);
}

t_stat io_write_svc (UNIT *uptr)
{
uint32 pa = (uint32) uptr->u3;

nca_cesr = nca_cesr | CESR_ERRSUM | CESR_CP2_MTT;
nca_cioear2 = (int32) (pa & CIOEAR_ADDR);
if (ka_cctl & CCTL_ENABLE)
    nca_cioear2 = nca_cioear2 | CIOEAR_ID1;
ka_harderr = 1;
SET_IRQL;
sim_debug (DBG_NXM, &sysd_dev, "hard error: write of non-existent address %08X, CESR %08X\n", pa, nca_cesr);
return SCPE_OK;
}

void WriteReg (uint32 pa, int32 val, int32 lnt)
{
struct reglink *p;

if (pa < CACHEABLE_END) {                               /* cache diagnostic mode: memory */
    mem_wr (pa, val, lnt);
    return;
    }
for (p = &regtable[0]; p->low != 0; p++) {
    if ((pa >= p->low) && (pa < p->high) && p->write) {
        p->write (pa, val, lnt);
        return;
        }
    }
sim_debug (DBG_NXM, &sysd_dev, "write of non-existent address %08X = %08X, PC %08X\n", pa, val, fault_PC);
io_write_error (pa);
return;
}

/* WriteRegU - write register space, unaligned

   Inputs:
        pa      =       physical address
        val     =       data to write, right justified in 32b longword
        lnt     =       length (1, 2, or 3)
   Outputs:
        none
*/

void WriteRegU (uint32 pa, int32 val, int32 lnt)
{
int32 sc = (pa & 03) << 3;
int32 dat = ReadReg (pa & ~03, L_LONG);

dat = (dat & ~(insert[lnt] << sc)) | ((val & insert[lnt]) << sc);
WriteReg (pa & ~03, dat, L_LONG);
return;
}

/* Machine check

   The NVAX machine check frame, as pushed on the interrupt stack:

        (SP)+0    byte count, 0x18
        (SP)+4    ISTATE1: <31:29> ASTLVL, <23:16> machine check code,
                  <7:0> CPUID
        (SP)+8    INT.SYS
        (SP)+12   SAVEPC
        (SP)+16   VA
        (SP)+20   Q
        (SP)+24   ISTATE2: <31:28> Rn, <25:24> mode, <23:16> opcode,
                  <7> VR
        (SP)+28   PC
        (SP)+32   PSL

   The handler acknowledges with MTPR #0, #MCESR.  A machine check while
   the processor is still taking one is a double error: console halt with
   code 05.  VA is the virtual address of the reference, saved before the
   frame is written (each write of the frame moves mchk_va); ISTATE2 gives
   the opcode and the mode the reference was made in; Rn, VR and Q are
   left zero (the model has no microcode state to report). */

int32 machine_check (int32 p1, int32 opc, int32 cc, int32 delta)
{
int32 istate1, istate2, acc, saved_pc = PC;
uint32 va = (uint32) mchk_va;                           /* before the frame's writes move it */

if (in_ie) {
    in_ie = 0;
    sim_debug (DBG_MCHK, &sysd_dev, "double error: machine check code %02X at PC %08X\n", p1, PC);
    return con_halt (CON_DBLMCK, cc);                   /* double machine check */
    }
sim_debug (DBG_MCHK, &sysd_dev, "machine check code %02X, VA %08X, PC %08X\n", p1, va, PC);
istate1 = ((ASTLVL & 07) << 29) | ((p1 & 0xFF) << 16) | (ka_cpuid & 0xFF);
istate2 = (((PSL >> PSL_V_CUR) & 3) << 24) | ((opc & 0xFF) << 16);
cc = intexc (SCB_MCHK, cc, 0, IE_SVE);                  /* take exception */
acc = ACC_MASK (KERN);                                  /* in kernel mode */
in_ie = 1;
SP = SP - 28;                                           /* push 7 words */
Write (SP, 0x18, L_LONG, WA);                           /* # bytes */
Write (SP + 4, istate1, L_LONG, WA);                    /* ISTATE1 */
Write (SP + 8, ka_intsys, L_LONG, WA);                  /* INT.SYS */
Write (SP + 12, saved_pc, L_LONG, WA);                  /* SAVEPC */
Write (SP + 16, va, L_LONG, WA);                        /* VA */
Write (SP + 20, 0, L_LONG, WA);                         /* Q */
Write (SP + 24, istate2, L_LONG, WA);                   /* ISTATE2 */
in_ie = 0;
ka_mchk_busy = 1;
return cc;
}

/* Console entry: SAVPC and SAVPSL receive the PC and PSL (with the halt
   code in SAVPSL<13:8>, MAPEN in <15>, "PSL not valid" in <14>); the
   processor turns off memory management and starts the console ROM at
   2004.0000 on the interrupt stack at IPL 1F. */

int32 con_halt (int32 code, int32 cc)
{
int32 temp;

ka_savpc = PC;                                          /* save PC */
ka_savpsl = ((PSL | cc) & 0xFFFF00FF) | code;           /* PSL, param */
temp = (PSL >> PSL_V_CUR) & 0x7;                        /* get is'cur */
if (temp > 4)                                           /* invalid? */
    ka_savpsl = ka_savpsl | CON_BADPSL;
else STK[temp] = SP;                                    /* save stack */
if (mapen)                                              /* mapping on? */
    ka_savpsl = ka_savpsl | CON_MAPON;
mapen = 0;                                              /* turn off map */
SP = IS;                                                /* set SP from IS */
PSL = PSL_IS | PSL_IPL1F;                               /* PSL = 41F0000 */
sim_debug (DBG_HALT, &sysd_dev, "console halt code %02X, SAVPC %08X, SAVPSL %08X\n", (code >> 8) & 0x3F, ka_savpc, ka_savpsl);
JUMP (ROMBASE);                                         /* PC = 20040000 */
return 0;                                               /* new cc = 0 */
}

/* Special boot command - linked into SCP by initial reset

   Syntax: BOOT {CPU}

*/

t_stat vax49_boot (int32 flag, CONST char *ptr)
{
char gbuf[CBUFSIZE];

get_glyph (ptr, gbuf, 0);                           /* get glyph */
if (gbuf[0] && strcmp (gbuf, "CPU"))
    return SCPE_ARG;                                /* Only can specify CPU device */
return run_cmd (flag, "CPU");
}

/* Bootstrap: power up.  The console reads SAVPSL<13:8> = 3 as "power-up"
   and runs the whole self-test; anything else takes its restart path. */

t_stat cpu_boot (int32 unitno, DEVICE *dptr)
{
t_stat r;

PC = ROMBASE;
PSL = PSL_IS | PSL_IPL1F;
ka_savpc = 0;
ka_savpsl = PSL_IS | PSL_IPL1F | CON_PWRUP;
if (rom == NULL)
    return SCPE_IERR;
if (*rom == 0) {                                        /* no boot? */
    r = cpu_load_bootcode (BOOT_CODE_FILENAME, BOOT_CODE_ARRAY, BOOT_CODE_SIZE, TRUE, 0);
    if (r != SCPE_OK) {
#if !defined (KA49_INTERNAL_ROM)
        ka49_rom_missing ();
#endif
        return r;
        }
    }
return SCPE_OK;
}

#if !defined (KA49_INTERNAL_ROM)
/* No console ROM built in, none loaded, and no ka49a.bin to load: say what
   is needed and where it is published.  Any KA49-A image runs (V1.0-006,
   V1.3-0BC and V1.4-0D0 pass the power-up tests); V1.4 is the one that
   real machines ship with after DEC's last update. */

static void ka49_rom_missing (void)
{
sim_printf ("\n");
sim_printf ("The KA49 console ROM is not built into this simulator and none is loaded.\n");
sim_printf ("Load it before BOOT with LOAD -R <file>, or put it in the current working\n");
sim_printf ("directory as %s.  It is DEC's KA49-A console firmware, %u bytes;\n", BOOT_CODE_FILENAME, (uint32) ROMSIZE);
sim_printf ("the V1.4-0D0-V4.4 image has the sim_BuildROMs checksum 0xFD87B044 and the\n");
sim_printf ("sha256 47487fb808d85e426da6298299fc2014c0dd1638199716b31f80d4173f2f8b11.\n");
sim_printf ("It is published in the Oxide VAX PROM archive,\n");
sim_printf ("   http://www.oxide.org/vaxproms/KA49-A_V1.4-0D0-V4.4.bin.gz\n");
sim_printf ("and is the last %u bytes of cougar.sys in DEC's V1.4 firmware update kit\n", (uint32) ROMSIZE);
sim_printf ("(the same archive's cougar_fw.zip, or Cougar/v1.4 in the DECROMs collection,\n");
sim_printf ("   https://github.com/larsbrinkhoff/DECROMs).\n\n");
}
#endif

/* SYSD reset */

t_stat sysd_reset (DEVICE *dptr)
{
int32 i;

if (nmc_obit == NULL)                                   /* O-bit RAM */
    nmc_obit = (uint32 *) calloc (OBIT_PAGES * OBIT_ENTRIES, sizeof (uint32));
if (ka_sgmap == NULL)                                   /* scatter/gather map */
    ka_sgmap = (uint32 *) calloc (SGM_ENTRIES, sizeof (uint32));
if (ka_bctag == NULL)                                   /* Bcache tags */
    ka_bctag = (uint32 *) calloc (BCTAG_ENTRIES, sizeof (uint32));
if (ka_bcecc == NULL)                                   /* Bcache data check words */
    ka_bcecc = (uint32 *) calloc (BCECC_ENTRIES, sizeof (uint32));
if (ka_bcecc_set == NULL)
    ka_bcecc_set = (uint8 *) calloc (BCECC_ENTRIES, sizeof (uint8));
if (ka_bcdata == NULL)                                  /* Bcache data RAM */
    ka_bcdata = (uint32 *) calloc (BCDATA_SIZE >> 2, sizeof (uint32));
if ((nmc_obit == NULL) || (ka_sgmap == NULL) || (ka_bctag == NULL) ||
    (ka_bcecc == NULL) || (ka_bcecc_set == NULL) || (ka_bcdata == NULL))
    return SCPE_MEM;

if (sim_switches & SWMASK ('P')) {                      /* power up */
    ka_cpuid = 0;
    ka_pcscr = 0;
    ka_ecr = 0;
    ka_cctl = 0;
    ka_bcdecc = ka_bcedecc = 0;
    ka_icsr = 0;
    ka_bpcr = BPCR_INIT;
    ka_pamode = 0;
    ka_pcctl = 0;
    ka_cefsts = ka49_models[ka49_model].cefsts;
    ka_cefadr = 0;
    ka_led = 0;
    for (i = 0; i < 8; i++)
        nmc_memcon[i] = 0;
    nmc_mesr = 0;
    nmc_mear = 0;
    nmc_mmcdsr = MMCDSR_INIT;
    nmc_moamr = 0;
    nca_cesr = 0;
    nca_cmcdsr = CMCDSR_INIT;
    nca_cioear2 = 0;
    }
ka_cdiag = 0;
ka49_set_cdiag ();
ka_mchk_busy = 0;
ka_softerr = ka_harderr = 0;
sim_cancel (IOWR_UNIT);
for (i = 0; i < IPL_HLVL; i++)
    int_req[i] = 0;
int_mask = 0;
ka_dmaadr = ka_dmadir = 0;
ka_cfg = cfg_for_memory ();
sim_vm_cmd = vax49_cmd;
return SCPE_OK;
}

t_stat sysd_show_led (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
fprintf (st, "LED code %02X (register %02X)\n", (~ka_led) & 0xFF, ka_led & 0xFF);
return SCPE_OK;
}

t_stat sysd_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
fprintf (st, "KA49 system devices (SYSD)\n\n");
fprintf (st, "SYSD holds the NVAX processor registers that the KA49 console uses, the\n");
fprintf (st, "board's interrupt controller, configuration register, diagnostic LEDs,\n");
fprintf (st, "I/O reset and SCSI DMA registers, the NMC memory controller, the NCA I/O\n");
fprintf (st, "adapter and the SCSI scatter/gather map.\n\n");
fprintf (st, "The eight diagnostic LEDs show the progress of the power-up self-test;\n");
fprintf (st, "the last code written is displayed by:\n\n");
fprintf (st, "   sim> SHOW SYSD LED\n\n");
fprintf (st, "and every change of code is traced by SET SYSD DEBUG=LED.\n\n");
fprint_reg_help (st, dptr);
return SCPE_OK;
}

const char *sysd_description (DEVICE *dptr)
{
return "system devices";
}

t_stat auto_config (const char *name, int32 nctrl)
{
return SCPE_OK;
}

t_stat build_dib_tab (void)
{
return SCPE_OK;
}

t_stat cpu_set_model (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
char gbuf[CBUFSIZE];
int32 i;

if ((cptr == NULL) || (!*cptr))
    return SCPE_ARG;
cptr = get_glyph (cptr, gbuf, 0);
for (i = 0; ka49_models[i].name != NULL; i++) {
    if (strcmp (gbuf, ka49_models[i].name) == 0) {
        ka49_model = i;
        ka_cefsts = ka49_models[i].cefsts;
        strcpy (sim_name, ka49_models[i].sim_name);
        return SCPE_OK;
        }
    }
return SCPE_ARG;
}

t_stat cpu_print_model (FILE *st)
{
fprintf (st, "%s", ka49_models[ka49_model].sim_name);
return SCPE_OK;
}

t_stat cpu_model_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
fprintf (st, "Initial memory size is 128MB; the choices are 16, 32, 64, 80 and 128MB.\n\n");
fprintf (st, "The CPU model is one of:\n\n");
fprintf (st, "   sim> SET CPU MODEL=90                 VAXstation 4000 Model 90 (default)\n");
fprintf (st, "   sim> SET CPU MODEL=90A                VAXstation 4000 Model 90A\n");
fprintf (st, "   sim> SET CPU MODEL=96                 VAXstation 4000 Model 96\n\n");
fprintf (st, "The model sets the microcode patch revision in the SID, which the console\n");
fprintf (st, "and VMS use to tell the three apart.\n\n");
fprintf (st, "The simulator is booted with the BOOT command:\n\n");
fprintf (st, "   sim> BOOT\n\n");
return SCPE_OK;
}
