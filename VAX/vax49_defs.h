/* vax49_defs.h: VAXstation 4000 Model 90 (KA49, NVAX) model-specific definitions

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

   This file covers the KA49 CPU module of the VAXstation 4000 Model 90, 90A
   and 96.  The module carries:

   - an NVAX (DC246) processor: the VAX base instruction set with F, D and G
     floating point, an on-chip virtual instruction cache (VIC), an 8 KB
     primary cache (Pcache), the control of a 256 KB write-back backup cache
     (Bcache) with ECC, and a large set of processor registers (IPRs) for
     cache control, error capture and diagnostics;
   - the NMC (DC244) memory controller with eight SIMM slots in two sets of
     four, 16 MB to 128 MB;
   - the NCA I/O adapter, which connects the NVAX bus (NDAL) to two
     I/O buses and supplies the interrupt vectors of the board devices;
   - the devices of the VAXstation 4000 series, most of them at addresses
     that differ from the Model 60's: a DC7085 (DZ) serial controller, a
     DS1287A clock with battery-backed RAM, a 53C94 SCSI controller with a
     scatter/gather DMA map, an SGEC Ethernet controller, an AM79C30A audio
     chip, and the board's configuration, LED, interrupt and reset registers.

   Every address and bit below is established either from the KA49 console
   firmware, from what the real machines showed when probed, from NetBSD's
   KA49 support or from DEC's documentation; nothing here is invented without
   saying so.  Comments mark the guesses.

   System memory map, 30-bit physical addressing (the power-up mode; the
   NVAX can be switched to 32-bit addressing by PAMODE, which the console
   does not do and this model does not implement):

        0000 0000 - 07FF FFFF   main memory, 16, 32, 64, 80 or 128 MB
        2000 8000 - 2000 803F   SGEC Ethernet controller CSRs
        2004 0000 - 200B FFFF   console ROM, 512 KB (four 128K x 8 flash chips)
        2100 0000 - 2100 03FC   external processor registers: register n is
                                the longword at 2100 0000 + 4n; the interval
                                timer lives here (ICCS 2100 0060, NICR 0064,
                                ICR 0068)
        2101 0000 - 2101 7FF0   NMC O-bit RAM window, one 24-bit entry every
                                16 bytes, paged by MOAMR
        2101 8000 - 2101 804C   NMC memory controller registers
        2102 0000 - 2102 0018   NCA I/O adapter registers
        2500 0000 - 2500 000F   DZ (DC7085) serial line controller
        2540 0000 - 2540 00FF   DS1287A clock and RAM, register n in bits
                                <9:2> of the longword at 2540 0000 + 4n
        2580 0000               configuration register (read only)
        2580 0004               diagnostic LED register
        25C0 0000 - 25C0 0013   board registers: SCSI DMA address, SCSI DMA
                                direction, interrupt mask, interrupt request
                                and clear, I/O reset
        2600 0080 - 2600 00BF   53C94 SCSI controller, register n at +4n
        2680 0000               AM79C30A audio
        2700 0000 - 2700 7FFF   SCSI scatter/gather map, 8192 longwords
        2780 0000 - 2780 007F   network address ROM, one byte per longword
*/

#ifdef FULL_VAX                                         /* subset VAX */
#undef FULL_VAX
#endif

#ifndef _VAX_49_DEFS_H_
#define _VAX_49_DEFS_H_ 1

/* Microcode constructs

   SID (IPR 3E) on an NVAX: <31:24> CPU type 19 (0x13), <13:9> microcode
   patch revision, <8> non-standard patch, <7:0> microcode revision.  The
   patch revision is set when the console loads its microcode patch through
   PCSCR, and the console picks one of three patches from a measurement of
   the processor speed: 27 on a 4000/90 (the banner then says 71 MHZ), 19 on
   a 4000/90A (83 MHZ), 11 on a 4000/96 (100 MHZ).  The simulator runs at
   whatever speed the host gives it, so the model sets the patch revision
   from SET CPU MODEL instead (see vax49_sysdev.c). */

#define VAX49_SID_TYPE  (0x13u << 24)                   /* NVAX CPU type 19 */
#define VAX49_SID_UREV  0x02                            /* microcode rev 2 */
#define VAX49_SID_V_PATCH 9                             /* patch rev <13:9> */
#define VAX49_SID_M_PATCH 0x1F

/* Console halt codes, SAVPSL<13:8> (NVAX table 14-2) */

#define CON_HLTPIN      0x0200                          /* external halt (BREAK) */
#define CON_PWRUP       0x0300                          /* power up */
#define CON_INTSTK      0x0400                          /* interrupt stack not valid */
#define CON_DBLMCK      0x0500                          /* double error (mchk in mchk) */
#define CON_HLTINS      0x0600                          /* HALT instruction */
#define CON_BADPSL      0x4000                          /* SAVPSL<14>: PSL not valid */
#define CON_MAPON       0x8000                          /* SAVPSL<15>: MAPEN was set */

/* NVAX machine check codes (ISTATE1<23:16>) */

#define MCHK_UNKNOWN    0x01                            /* unknown MSTATUS */
#define MCHK_INTID      0x02                            /* bad INT.ID value */
#define MCHK_CANTGET    0x03                            /* cannot get here */
#define MCHK_MOVC       0x04                            /* MOVC status */
#define MCHK_ASYNC      0x05                            /* asynchronous hardware error */
#define MCHK_SYNC       0x06                            /* synchronous hardware error */
#define MCHK_READ       MCHK_SYNC                       /* read of a non-existent address (guess) */
#define MCHK_WRITE      MCHK_ASYNC                      /* (writes do not machine check) */

/* NVAX processor registers beyond the architected set (NVAX chip
   specification, table 2-22) */

#define MT_CPUID        0x0E                            /* CPU ID, set by the console */
#define MT_MCESR        0x26                            /* machine check acknowledge (W) */
#define MT_SAVPC        0x2A                            /* console saved PC */
#define MT_SAVPSL       0x2B                            /* console saved PSL and halt code */
#define MT_INTSYS       0x7A                            /* interrupt system status */
#define MT_PMFCNT       0x7B                            /* performance monitor count */
#define MT_PCSCR        0x7C                            /* patchable control store control */
#define MT_ECR          0x7D                            /* Ebox control */
#define MT_MTBTAG       0x7E                            /* Mbox TB tag fill (W) */
#define MT_MTBPTE       0x7F                            /* Mbox TB PTE fill (W) */
#define MT_CCTL         0xA0                            /* Cbox: Bcache control */
#define MT_BCDECC       0xA2                            /* Bcache data ECC to write (W) */
#define MT_BCETSTS      0xA3                            /* Bcache error tag status */
#define MT_BCETIDX      0xA4                            /* Bcache error tag index */
#define MT_BCETAG       0xA5                            /* Bcache error tag */
#define MT_BCEDSTS      0xA6                            /* Bcache error data status */
#define MT_BCEDIDX      0xA7                            /* Bcache error data index */
#define MT_BCEDECC      0xA8                            /* Bcache error data ECC */
#define MT_CEFADR       0xAB                            /* fill error address */
#define MT_CEFSTS       0xAC                            /* fill error status */
#define MT_NESTS        0xAE                            /* NDAL error status */
#define MT_NEOADR       0xB0                            /* NDAL error output address */
#define MT_NEOCMD       0xB2                            /* NDAL error output command */
#define MT_NEDATHI      0xB4                            /* NDAL error data high */
#define MT_NEDATLO      0xB6                            /* NDAL error data low */
#define MT_NEICMD       0xB8                            /* NDAL error input command */
#define MT_VMAR         0xD0                            /* VIC memory address */
#define MT_VTAG         0xD1                            /* VIC tag */
#define MT_VDATA        0xD2                            /* VIC data */
#define MT_ICSR         0xD3                            /* Ibox control and status */
#define MT_BPCR         0xD4                            /* branch prediction control */
#define MT_BPC          0xD6                            /* backup PC (R) */
#define MT_BPCUNW       0xD7                            /* backup PC with unwind (R) */
#define MT_MP0BR        0xE0                            /* Mbox copies of the base and */
#define MT_MP0LR        0xE1                            /* length registers */
#define MT_MP1BR        0xE2
#define MT_MP1LR        0xE3
#define MT_MSBR         0xE4
#define MT_MSLR         0xE5
#define MT_MMAPEN       0xE6                            /* Mbox copy of MAPEN */
#define MT_PAMODE       0xE7                            /* physical address mode */
#define MT_MMEADR       0xE8                            /* memory management error address */
#define MT_MMEPTE       0xE9                            /* ... PTE address */
#define MT_MMESTS       0xEA                            /* ... status */
#define MT_TBADR        0xEC                            /* TB parity error address */
#define MT_TBSTS        0xED                            /* TB status */
#define MT_PCADR        0xF2                            /* Pcache parity error address */
#define MT_PCSTS        0xF4                            /* Pcache status */
#define MT_PCCTL        0xF8                            /* Pcache control */
#define MT_MAX          0xFF                            /* last register of the plain set */

/* Block-decoded processor registers: one register number per cache block,
   far above the plain set.  ML_IPR_VALID (used by MTPR and MFPR in
   vax_cpu1.c) lets these through to ReadIPR and WriteIPR. */

#define MT_BCTAG_BASE   0x01000000                      /* Bcache tag store, 32-byte blocks */
#define MT_BCTAG_END    0x01200000                      /* (room for a 2 MB Bcache) */
#define MT_BCFLU_BASE   0x01400000                      /* Bcache deallocate (W) */
#define MT_BCFLU_END    0x01600000
#define MT_PCTAG_BASE   0x01800000                      /* Pcache tags, 256 entries */
#define MT_PCTAG_END    0x01802000
#define MT_PCDAP_BASE   0x01C00000                      /* Pcache data parity, 1024 entries */
#define MT_PCDAP_END    0x01C02000

#define ML_IPR_VALID(prn) ((((uint32) (prn)) <= MT_MAX) || \
                           ((((uint32) (prn)) >= MT_BCTAG_BASE) && (((uint32) (prn)) < MT_BCTAG_END)) || \
                           ((((uint32) (prn)) >= MT_BCFLU_BASE) && (((uint32) (prn)) < MT_BCFLU_END)) || \
                           ((((uint32) (prn)) >= MT_PCTAG_BASE) && (((uint32) (prn)) < MT_PCTAG_END)) || \
                           ((((uint32) (prn)) >= MT_PCDAP_BASE) && (((uint32) (prn)) < MT_PCDAP_END)))

/* CPU */

#define CPU_MODEL_MODIFIERS \
                        { MTAB_XTD|MTAB_VDV, 0, "MODEL", "MODEL={90|90A|96}", \
                          &cpu_set_model, &cpu_show_model, NULL, "Set/Show the simulator CPU Model" },

/* Memory: two sets of four SIMMs, 4 MB or 16 MB SIMMs, each set
   homogeneous; set 0 must be filled first.  The five sizes below are the
   ones those rules allow (set 1 may hold 4 MB SIMMs behind a set 0 of 16 MB
   SIMMs, but the console's sizing code has no case for 16 MB SIMMs behind
   4 MB ones that would come out differently, so 80 MB is the one mixed
   size). */

#define MAXMEMWIDTH     27                              /* 128 MB */
#define MAXMEMSIZE      (1 << MAXMEMWIDTH)
#define MAXMEMWIDTH_X   27
#define MAXMEMSIZE_X    (1 << MAXMEMWIDTH_X)
#define INITMEMSIZE     (1 << 27)                       /* as the 4000/90s shipped */
#define MEMSIZE         (cpu_unit.capac)

/* Memory is reached directly, except while the console's cache test has
   the Bcache in one of its diagnostic modes (force hit, software ECC):
   then every memory reference goes through ReadReg and WriteReg, so that
   the model can serve it from the Bcache data RAM or keep the check bits
   the test writes (vax49_sysdev.c). */

extern int32 ka_cdiag;
#define ADDR_IS_MEM(x)  ((((uint32) (x)) < MEMSIZE) && !ka_cdiag)
#define MEM_MODIFIERS   { UNIT_MSIZE, (1u << 24), NULL, "16M", &cpu_set_size, NULL, NULL, "Set Memory to 16M bytes" }, \
                        { UNIT_MSIZE, (1u << 25), NULL, "32M", &cpu_set_size, NULL, NULL, "Set Memory to 32M bytes" }, \
                        { UNIT_MSIZE, (1u << 26), NULL, "64M", &cpu_set_size, NULL, NULL, "Set Memory to 64M bytes" }, \
                        { UNIT_MSIZE, (1u << 26) + (1u << 24), NULL, "80M", &cpu_set_size, NULL, NULL, "Set Memory to 80M bytes" }, \
                        { UNIT_MSIZE, (1u << 27), NULL, "128M", &cpu_set_size, NULL, NULL, "Set Memory to 128M bytes" }

/* Console ROM: 512 KB of flash, a 32-bit path, byte n of each longword
   from flash chip n */

#define ROMAWIDTH       19                              /* ROM addr width */
#define ROMSIZE         (1u << ROMAWIDTH)               /* ROM length */
#define ROMAMASK        (ROMSIZE - 1)                   /* ROM addr mask */
#define ROMBASE         0x20040000                      /* ROM base */
#define ADDR_IS_ROM(x)  ((((uint32) (x)) >= ROMBASE) && \
                        (((uint32) (x)) < (ROMBASE + ROMSIZE)))

/* External processor registers.  The NVAX passes MTPR and MFPR of registers
   it does not implement itself to the system; on the KA49 those registers
   also appear in I/O space, register n at XIPRBASE + 4n.  The console uses
   both views of the interval timer (MTPR to ICCS for its speed measurement,
   the memory-mapped ICCS for its millisecond delay), which is what shows
   that they are one timer. */

#define XIPRBASE        0x21000000
#define XIPRSIZE        0x400

/* NMC memory controller */

#define OBITBASE        0x21010000                      /* O-bit RAM window */
#define OBITSIZE        0x8000
#define NMCBASE         0x21018000                      /* MEMCON0-7, MEAR, MESR, MMCDSR, MOAMR */
#define NMCSIZE         0x50

/* NCA I/O adapter */

#define NCABASE         0x21020000                      /* CESR, CMCDSR, CSEAR1/2, CIOEAR1/2, CNEAR */
#define NCASIZE         0x1C

/* Serial line controller */

#define DZSIZE          0x10                            /* DZ length */
#define DZBASE          0x25000000                      /* DZ base */

/* Time-of-year clock and battery-backed RAM (DS1287A): 64 registers, each
   in bits <9:2> of its own longword.  Registers 0-13 are the clock, 14-63
   the RAM that holds the console's settings. */

#define NVRAWIDTH       8                               /* NVR addr width */
#define NVRSIZE         (1u << NVRAWIDTH)               /* NVR length: 64 longwords */
#define NVRAMASK        (NVRSIZE - 1)                   /* NVR addr mask */
#define NVRBASE         0x25400000                      /* NVR base */
#define ADDR_IS_NVR(x)  ((((uint32) (x)) >= NVRBASE) && \
                        (((uint32) (x)) < (NVRBASE + NVRSIZE)))

/* Configuration and LED registers */

#define CFGBASE         0x25800000
#define LEDBASE         0x25800004
#define CFGSIZE         0x8

/* Board registers ("VS_REGS") */

#define KABASE          0x25C00000
#define KASIZE          0x14

/* SCSI controller and scatter/gather map */

#define RZBASE          0x26000080                      /* 53C94 registers */
#define RZSIZE          0x40
#define SGMBASE         0x27000000                      /* scatter/gather map */
#define SGMSIZE         0x8000

/* Network address ROM (vax_nar.c) */

#define NARAWIDTH       5                               /* NAR addr width */
#define NARSIZE         (1u << NARAWIDTH)               /* NAR length */
#define NARAMASK        (NARSIZE - 1)                   /* NAR addr mask */
#define NARBASE         0x27800000                      /* NAR base */
#define NARSPAN         (NARSIZE << 2)                  /* one byte per longword */

/* LCSPX graphics option (vax_lcspx.c): a 32 MB window, option ROM at
   39BC.0000 */

#define LCSPX_BASE      0x38000000

/* Other address spaces */

#define ADDR_IS_IO(x)   (0)
#define ADDR_IS_CDG(x)  (0)

/* Machine specific reserved operand tests (mostly NOPs) */

#define ML_PA_TEST(r)
#define ML_LR_TEST(r)
#define ML_SBR_TEST(r)
#define ML_PXBR_TEST(r)
#define LP_AST_TEST(r)
#define LP_MBZ84_TEST(r)
#define LP_MBZ92_TEST(r)

/* The NVAX keeps only ASTLVL<2:0> and takes no fault on a larger value */

#define MT_AST_TEST(r)  r = (r) & 07

/* Common CSR flags */

#define CSR_V_GO        0                               /* go */
#define CSR_V_IE        6                               /* interrupt enable */
#define CSR_V_DONE      7                               /* done */
#define CSR_V_BUSY      11                              /* busy */
#define CSR_V_ERR       15                              /* error */
#define CSR_GO          (1u << CSR_V_GO)
#define CSR_IE          (1u << CSR_V_IE)
#define CSR_DONE        (1u << CSR_V_DONE)
#define CSR_BUSY        (1u << CSR_V_BUSY)
#define CSR_ERR         (1u << CSR_V_ERR)

/* Timers */

#define TMR_CLK         0                               /* 100Hz clock */

/* I/O system definitions */

#define MT_MAXFR        (1 << 16)                       /* magtape max rec */

#define DEV_V_4XX       (DEV_V_UF + 0)                  /* KA4xx I/O */
#define DEV_4XX         (1u << DEV_V_4XX)

#define DEV_RDX         16                              /* default device radix */

/* Device information block */

#define VEC_DEVMAX      4                               /* max device vec */

typedef struct {
    int32               rom_index;                      /* option ROM index */
    uint8               *rom_array;                     /* option ROM code */
    t_addr              rom_size;                       /* option ROM size */
    } DIB;

/* Interrupts

   The NVAX has four interrupt request inputs, IRQ_H<3:0>, at IPL 17-14,
   and takes an interrupt vector from the system during the acknowledge.
   On the KA49:

   - IPL 14 (IRQ_H<0>): the SGEC, whose vector and IPL come from its own
     CSR0 (the console programs vector F0, VMS 104).
   - IPL 15 (IRQ_H<1>): the board devices, through an 8-bit interrupt
     request register (INT_REQ, 25C0.000C), gated by an 8-bit mask register
     (INT_MSK, 25C0.0008).  INT_REQ shows requests whether or not they are
     masked.  Vectors are fixed per source: SCSI 148, DZ receive 150, DZ
     transmit 154, audio 158.  The DZ has a single INT_REQ bit (3) for both
     directions; the acknowledge returns 150 or 154 from what the DZ wants.
   - IPL 16: the interval timer (vector C0); IRQ_H<2> has no source.
   - IPL 17 (IRQ_H<3>): unused.
   - IPL 1A soft errors (vector 54) and IPL 1D hard errors (vector 60).

   int_req[] holds one word per level 14-17.  At IPL 15 the bit numbers
   are the INT_REQ bit numbers, except that the DZ transmitter has a bit
   of its own above the register (INT_V_DZTX) so that the acknowledge can
   tell the two DZ vectors apart; INT_REQ bit 3 reads as the OR of the two.

   Within IPL 15 the priority order of the sources is not known; the model
   takes them in vector order (SCSI, DZ receive, DZ transmit, audio). */

#define INT_V_NI        0                               /* IPL 14: SGEC */

#define INT_V_SC        1                               /* IPL 15: 53C94, INT_REQ<1> */
#define INT_V_DZRX      3                               /* DZ receive, INT_REQ<3> */
#define INT_V_AUD       4                               /* audio (guess: INT_REQ<4>) */
#define INT_V_SGMERR    7                               /* SCSI map error, INT_REQ<7>, polled */
#define INT_V_DZTX      8                               /* DZ transmit, reads as INT_REQ<3> */

#define INT_NI          (1u << INT_V_NI)
#define INT_SC          (1u << INT_V_SC)
#define INT_DZRX        (1u << INT_V_DZRX)
#define INT_AUD         (1u << INT_V_AUD)
#define INT_SGMERR      (1u << INT_V_SGMERR)
#define INT_DZTX        (1u << INT_V_DZTX)

#define IPL_HMIN        0x14                            /* lowest hwre level */
#define IPL_HMAX        0x17                            /* highest hwre level */
#define IPL_HLVL        (IPL_HMAX - IPL_HMIN + 1)       /* # hardware levels */
#define IPL_SMAX        0xF                             /* highest swre level */
#define IPL_CLK         0x16                            /* interval timer */
#define IPL_NI          (0x14 - IPL_HMIN)               /* int_req index */
#define IPL_SC          (0x15 - IPL_HMIN)
#define IPL_DZRX        (0x15 - IPL_HMIN)
#define IPL_DZTX        (0x15 - IPL_HMIN)
#define IPL_AUD         (0x15 - IPL_HMIN)
#define IPL_SGMERR      (0x15 - IPL_HMIN)

#define VEC_SC          0x148                           /* fixed board vectors */
#define VEC_DZRX        0x150
#define VEC_DZTX        0x154
#define VEC_AUD         0x158

/* Device vectors */

#define VEC_QBUS        0                               /* Not a Qbus system */
#define VEC_Q           0

/* Interrupt macros */

#define IREQ(dv)        int_req[IPL_##dv]
#define SET_INT(dv)     int_req[IPL_##dv] = int_req[IPL_##dv] | (INT_##dv)
#define CLR_INT(dv)     int_req[IPL_##dv] = int_req[IPL_##dv] & ~(INT_##dv)
#define IORETURN(f,v)   ((f)? (v): SCPE_OK)             /* cond error return */

/* System model: 0 = no graphics console (the serial line 3 is the console,
   which is all this model supports so far); the DZ uses it to give lines 0
   and 1 to a keyboard and mouse on a graphics workstation. */

extern int32 sys_model;

/* Machine specific definitions - DZ */

#define DZ_L3C          1                               /* line 3 console */

/* Machine specific definitions - SGEC: the request goes to IRQ_H<0> (IPL
   14), whatever IPL the SGEC's CSR0 names */

#define SGECBASE        0x20008000                      /* SGEC CSRs */
#define SGEC_SET_INT    SET_INT (NI)
#define SGEC_CLR_INT    CLR_INT (NI)

/* The station address is the network address ROM's (vax_nar.c) */

extern uint8 nar_mac[6];
extern DEVICE nar_dev;
t_stat nar_reset (DEVICE *dptr);
#define SGEC_GET_MAC(m) memcpy ((m), nar_mac, 6)
#define SGEC_SET_MAC(m) (memcpy (nar_mac, (m), 6), nar_reset (&nar_dev))

/* Machine specific definitions - RZ94: the board's DMA engine (the SQWF)
   moves the data, through the SCSI scatter/gather map, from the window
   address in its own DMA address register; the 53C94's request reaches
   INT_REQ<1> as a level that follows the chip's INT output */

#define RZ_SCSI_ID      6                               /* initiator SCSI id */
#define RZ_DMA_READ(rz,bc,buf)  sqwf_read (bc, buf)
#define RZ_DMA_WRITE(rz,bc,buf) sqwf_write (bc, buf)
#define RZ_DMA_DONE(rz,n)       sqwf_done (n)
#define RZ_INT_LEVEL    1
#define RZ_ABSENT_UNATTACHED 1                          /* no drive behind an empty unit */

/* Function prototypes for I/O */

int32 Map_ReadB (uint32 ba, int32 bc, uint8 *buf);
int32 Map_ReadW (uint32 ba, int32 bc, uint16 *buf);
int32 Map_WriteB (uint32 ba, int32 bc, uint8 *buf);
int32 Map_WriteW (uint32 ba, int32 bc, uint16 *buf);
int32 sqwf_read (uint32 bc, uint8 *buf);
int32 sqwf_write (uint32 bc, uint8 *buf);
void sqwf_done (uint32 n);

/* Function prototypes for system-specific unaligned support */

int32 ReadIOU (uint32 pa, int32 lnt);
int32 ReadRegU (uint32 pa, int32 lnt);
void WriteIOU (uint32 pa, int32 val, int32 lnt);
void WriteRegU (uint32 pa, int32 val, int32 lnt);

t_stat auto_config (const char *name, int32 nctrl);

/* Function prototypes for virtual and physical memory interface (inlined) */

#include "vax_mmu.h"

#endif
