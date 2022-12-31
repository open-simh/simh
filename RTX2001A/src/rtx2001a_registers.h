/* rtx2001a_registers.h: RTX2001A CPU simulator

   Copyright (c) 2022, Systasis Computer Systems, Inc.

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
   SYSTASIS COMPUTER SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Systasis Computer Systems shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Systasis Computer Systems.

    6-Sep-22   SYS      Initial version
*/

/*
Stack Related Registers, Internal Processor Registers, and Status/Control Registers, Holding/Counter Scratchpad registers

The reset states for the RTX 2001A are shown in RTX2001A DataSheet, Table 4

R-W   Indicates that the register can be either read from or written to
R     Indicates a read-only register
W     Indicates a write-only register
R/W   Indicates that the first register is read-only and the second register is write-only
*     Indicates that individual bits in the register may be read-only or write-only and that the bit map for that register should be consulted
N     Indicates that the register cannot be read from or written to

REGISTER  ASIC ADDR       INITIALIZATION VALUES   R W   COMMENTS
========  =========       =====================   ===   ==============================================================
TOP                       0000 0000 0000 0000     R-W   Holds the top element of the Parameter Stack
NEXT                      1111 1111 1111 1111     R-W   Holds the second element of the Parameter Stack
IR                        0000 0000 0000 0000     N     Latch containing the instruction currently being executed
I         01H, 02H, 03H   1111 1111 1111 1111     R-W   16 bits of the 21-bit top element of the Return Stack and
                                                          holds the count during streamed and loop instructions
                                                          See Table 11, pg 22
CR        03H             0100 0000 0000 1000     *     Interrupts disabled, BOOT=1, Byte Order=0
                                                          See Fig 3, pg 7
MD        04H             1111 1111 1111 1111     R-W   Holds the divisor during Step Divide operations
SR        06H             0000 0000 0000 0000     R-W   Holds the intermediate values used during Step Square Root operations
PC        07H             0000 0000 0000 0000     R-W   Holds the address of the next instruction to be fetched from Main Memory
IMR       08H             0000 0000 0000 0000     R-W   Holds enabled maskable interrupt assignments
                                                          See Fig 6, pg 8
SPR       09H             0000 0000 0000 0000     R-W   Holds the stack pointer value for each stack
                                                          See Fig 7, pg 9; Fig 6.4, pg 90
SUR       0AH             0000 0111 0000 0111     R-W   Holds holds the underflow limit values for the
                                                          Parameter and Return Stacks
                                                          See Fig 9, pg 9
SVR       0BH             1111 1111 1111 1111     W     Holds holds the overflow limit values for the
                                                          Parameter and Return Stacks
                                                          Each stack overflow limit set for max. stack size
                                                          See Fig 8, pg 9
IVR       0BH             0000 0010 0000 0000     R     Holds the current Interrupt Vector value
                                                      .. See Figure 4, pg. 8 and Table 7, pg. 15
IPR       0CH             0000 0000 0000 0000     R-W   Initialized for Code Page 0
DPR       0DH             0000 0000 0000 0000     R-W   Initialized for Data Page 0
UPR       0EH             0000 0000 0000 0000     R-W   Initialized for User Page 0
CPR       0FH             0000 0000 0000 0000     R-W   Initialized for Code Page 0
IBC       10H             0000 0000 0000 0000     *     Holds the Interrupt Vector base address and
                                                          processor configuration information
                                                          Interrupt Base=O, Counters on internal clocks,
                                                          no rounding, use CPR for data accesses
                                                          See Fig 5, pg 8
UBR       11H             0000 0000 0000 0000     R-W   User Base address set to 0
TC0/TP0   13H             0000 0000 0000 0000     R/W   Timer/Counter Registers:
TC1/TP1   14H                                     R/W       Hold the current count value for each of the three Timer/Counters
TC2/TP2   15H                                     R/W       All Timer/Counters set to timeout after 65536 counts
                                                      Timer Preload Registers:
                                                          Hold the initial 16-bit count values written to each timer
RX        16H             0000 0000 0000 0000     R-W   Scratchpad register for data storage
RH        17H             0000 0000 0000 0000     R-W   Scratchpad register for data storage and
                                                          a counting register which automatically
                                                          increments or decrements RX by one when
                                                          read or written by specialized instructions
                                                          (see Table 12, pg 22)
*/

#ifndef RTX2001A_REGISTERS_H_
#define RTX2001A_REGISTERS_H_ 0

#include "hp3k_defs.h"
#include "rtx2001a_psb.h"
#include "rtx2001a_rsb.h"
#include "rtx2001a_debug.h"

/*
** CPU Stack Registers
*/
extern RTX_WORD TOP;  // Top Register
extern RTX_WORD NEXT; // Next Register

/*
** Instruction Register Latch
*/
extern RTX_WORD IR;

/*
** ASIC Bus Registers
*/
static enum {
    I = 0,       // Index Register, no latch
    I_ASIC = 1,  // Index Register, ASIC bus latch
    I_ALU = 2,   // Index Register, ALU latch
    _CR = 3,     // Configuration Register
    MD = 4,      // Doubleword Multiply/Divide Register
    SQ = 5,      // "pseudo-register" for step math operation
    SR = 6,      // Square Root Register
    PC = 7,      // Program Counter Register
    _IMR = 8,    // Interrupt Mask Register
    _SPR = 9,    // Stack Pointer Register
    _SUR = 0xA,  // Stack Underflow Register
    _SVR = 0xB,  // Stack Overflow Register, write-only
    _IVR = 0xB,  // Interrupt Vector Register, read-only
    _IPR = 0xC,  // Index Page Register
    _DPR = 0xD,  // Data Page Register
    _UPR = 0xE,  // User Page Register
    _CPR = 0xF,  // Code Page Register
    _IBC = 0x10, // Interrupt Base/Control Register
    _UBR = 0x11, // User Base Address Register
    TC0 = 0x13,  // Timer Counter Register 0, read-only
    TP0 = 0x13,  // Timer Preload Register 0, write-only
    TC1 = 0x14,  // Timer Counter Register 1, read-only
    TP1 = 0x14,  // Timer Preload Register 1, write-only
    TC2 = 0x15,  // Timer Counter Register 2, read-only
    TP2 = 0x15,  // Timer Preload Register 2, write-only
    RX = 0x16,   // Scratchpad/Counting register
    RH = 0x17,   // Scratchpad register
    RSP = 0x21,  // Return stack pointer
    PSP = 0x22,  // Parameter stack pointer
    ASIC_MAX = 0x25,
} ASIC_FILE;
extern RTX_WORD asic_file[ASIC_MAX];

/**
 * Configuration Register
 */
struct ConfigurationRegisterFields
{
    RTX_WORD cy : 1;   // 0: R/W Carry
    RTX_WORD ccy : 1;  // 1: R/W Complex Carry
    RTX_WORD bo : 1;   // 2: Byte Order 0=BE (Intel), 1=LE (Motorola)
    RTX_WORD boot : 1; // 3: Select Boot ROM
    RTX_WORD sid : 1;  // 4: Set Interrupt Disable 0=Enabled, 1=Disabled
    RTX_WORD mbz0 : 9; // 5-13 MBZ
    RTX_WORD rid : 1;  // 14: Read Interrupt Disable
    RTX_WORD il : 1;   // 15: Interrupt Latch
};

union ConfigurationRegister
{
    struct ConfigurationRegisterFields fields;
    RTX_WORD pr;
} cr;
typedef union ConfigurationRegister CR;
#define CY (cr.fields.cy)
#define CCY (cr.fields.ccy)
#define BO (cr.fields.bo)
#define BOOT (cr.fields.boot)
#define SID (cr.fields.sid)
#define RID (cr.fields.rid)
#define IL (cr.fields.il)

static BITFIELD cr_bits[] = { // Fig 3, pg 7
    BIT(CY),                  // 0: R/W Carry
    BIT(CCY),                 // 1: R/W Complex Carry
    BIT(BO),                  // 2: Byte Order 0=BE, 1=LE
    BIT(BOOT),                // 3: Select Boot ROM
    BIT(SID),                 // 4: Set Interrupt Disable 0=Enabled, 1=Disabled
    BITNCF(9),                // 5-13 MBZ
    BIT(RID),                 // 14: Read Interrupt Disable
    BIT(IL),                  // 15: Interrupt latch
    ENDBITS};

/*
** Interrupt Mask Register
*/
struct InterruptMaskRegisterFields
{
    RTX_WORD mbz : 1;  // 0: Always reads as O.
    RTX_WORD ei1 : 1;  // 1: External Input Pin 1
    RTX_WORD psu : 1;  // 2: Parameter Stack Underflow
    RTX_WORD rsu : 1;  // 3: Return Stack Underflow
    RTX_WORD psv : 1;  // 4: Parameter Stack Overflow
    RTX_WORD rsv : 1;  // 5: Return Stack overflow
    RTX_WORD ei2 : 1;  // 6: External Input Pin 2
    RTX_WORD t0 : 1;   // 7: Timer / Counter 0 Interrupt
    RTX_WORD t1 : 1;   // 8: Timer / Counter 1 Interrupt
    RTX_WORD t2 : 1;   // 9: Timer / Counter 2 Interrupt
    RTX_WORD ei3 : 1;  // 10: External Input Pin 3
    RTX_WORD ei4 : 1;  // 11: External Input Pin 4
    RTX_WORD ei5 : 1;  // 12: External Input Pin 5
    RTX_WORD swi : 1;  // 13:  Software Interrupt
    RTX_WORD mbz1 : 2; // 14-15: Always reads as O.
};

static BITFIELD imr_bits[] = { // Table 4.7, pg 51
    BITNC,                     // 0: Always reads as O.
    BIT(EI1),                  // 1: External Input Pin 1
    BIT(PSU),                  // 2: Parameter Stack Underflow
    BIT(RSU),                  // 3: Return Stack Underflow
    BIT(PSV),                  // 4: Parameter Stack Overflow
    BIT(RSV),                  // 5: Return Stack overflow
    BIT(EI2),                  // 6: External Input Pin 2
    BIT(T0),                   // 7: Timer / Counter 0 Interrupt
    BIT(T1),                   // 8: Timer / Counter 1 Interrupt
    BIT(T2),                   // 9: Timer / Counter 2 Interrupt
    BIT(EI3),                  // 10: External Input Pin 3
    BIT(EI4),                  // 11: External Input Pin 4
    BIT(EI5),                  // 12: External Input Pin 5
    BIT(SWI),                  // 13:  Software Interrupt
    BITNCF(2),                 // 14-15: Always reads as O.
    ENDBITS};

union InterruptMaskRegister
{
    struct InterruptMaskRegisterFields fields;
    RTX_WORD pr;
} imr;
typedef union InterruptMaskRegister IMR;

/**
 * Stack Pointer Register
 *
 * RTX2001A has 64 words in each stack
 */
struct StackPointerRegisterFields
{
    RTX_WORD psp : 6;
    RTX_WORD mbz0 : 2;
    RTX_WORD rsp : 6;
    RTX_WORD mbz1 : 2;
};

union StackPointerRegister
{
    struct StackPointerRegisterFields fields;
    RTX_WORD pr;
} spr;
typedef union StackPointerRegister SPR;
#define STACK_MASK 0x3F /* mask for stack pointer wrapping */
// define RSP (spr.fields.rsp)
// define PSP (spr.fields.psp)

static BITFIELD spr_bits[] = { // Fig 7, pg 9
    BITF(PSP, 6),              // 0-5: Parameter Stack Pointer.
    BITNCF(2),                 //
    BITF(RSP, 6),              // 8-13: Return Stack Pointer.
    BITNCF(2),
    ENDBITS};

/**
 * Stack Underflow Register
 */
struct StackUnderflowRegisterFields
{
    RTX_WORD psf : 1;  // 0: Parameter Stack Start Flag
    RTX_WORD ps : 1;   // 1: Parameter Substacks, 0: two 32 word stacks, 1: one 64 word stack
    RTX_WORD mbz0 : 1; // 2: Reserved
    RTX_WORD psu : 5;  // 3-7: Parameter Stack Underflow Limit, 0 - 31 words from stack bottom
    RTX_WORD rsf : 1;  // 8: Return Stack Start Flag
    RTX_WORD rs : 1;   // 9: Return Substacks, 0: two 32 word stacks, 1: one 64 word stack
    RTX_WORD mbz1 : 1; // 10: Reserved
    RTX_WORD rsu : 5;  // 11-15: Return Stack Underflow Limit, 0 - 31 words from stack bottom
};

union StackUnderflowRegister
{
    struct StackUnderflowRegisterFields fields;
    RTX_WORD pr;
} sur;
typedef union StackUnderflowRegister SUR;
#define PSF (sur.fields.psf)
#define PS (sur.fields.ps)
#define PSU (sur.fields.psu)
#define RSF (sur.fields.rsf)
#define RS (sur.fields.rs)
#define RSU (sur.fields.rsu)

static BITFIELD sur_bits[] = { // Fig 6.8, pg 93
    BIT(PSF),                  // 0: Parameter Stack Start Flag
    BIT(PS),                   // 1: Parameter Substacks, 0: two 32 word stacks, 1: one 64 word stack
    BITNC,                     // 2: Reserved
    BITF(PSU, 5),              // 3-7: Parameter Stack Underflow Limit, 0 - 31 words from stack bottom
    BIT(RSF),                  // 8: Return Stack Start Flag
    BIT(RS),                   // 9: Return Substacks, 0: two 32 word stacks, 1: one 64 word stack
    BITNC,                     // 10: Reserved
    BITF(RSU, 5),              // 11-15: Return Stack Underflow Limit, 0 - 31 words from stack bottom
    ENDBITS};

/**
 * Interrupt Vector Register
 */
struct InterruptVectorRegisterFields
{
    RTX_WORD mbz : 5; // 0-4: Reserved
    RTX_WORD ivb : 5; // 5-9: Interrupt Vector Base
    RTX_WORD vec : 5; // 10-15: Vector
};

union InterruptVectorRegister
{
    struct InterruptVectorRegisterFields fields;
    RTX_WORD pr;
} ivr;
typedef union InterruptVectorRegister IVR;

static BITFIELD ivr_bits[] = {
    BITNCF(5),    // 0-4: Reserved
    BITF(IVB, 5), // 5-9: Interrupt Vector Base
    BITF(VEC, 5), // 10-15: Vector
    ENDBITS};

/**
 * Stack Overflow Register
 */
struct StackOverflowRegisterFields
{
    RTX_WORD pvl : 6;  // 0-5: Parameter Stack Overflow Limit, 0 - 31 words from stack bottom
    RTX_WORD mbz0 : 2; // 6-7: Reserved
    RTX_WORD rvl : 6;  // 8-13: Return Stack Overflow Limit, 0 - 31 words from stack bottom
    RTX_WORD mbz1 : 2; // 14-15: Reserved
};

union StackOverflowRegister
{
    struct StackOverflowRegisterFields fields;
    RTX_WORD pr;
} svr;
typedef union StackOverflowRegister SVR;
#define PVL (svr.fields.pvl)
#define RVL (svr.fields.rvl)

static BITFIELD svr_bits[] = { // Fig 9, pg 8
    BITF(PVL, 6),              // 0-5: Parameter Stack Overflow Limit, 0 - 31 words from stack bottom
    BITNCF(2),                 // 6-7: Reserved
    BITF(RVL, 6),              // 8-13: Return Stack Overflow Limit, 0 - 31 words from stack bottom
    BITNCF(2),                 // 14-15: Reserved
    ENDBITS};

/**
 * Index Page Register
 */
struct IndexPageRegisterFields
{
    RTX_WORD pr : 5;    // 0-4: 5 most significant bits of the top element of the Return Stack (I) or current page address
    RTX_WORD mbz1 : 11; // 5-15: Reserved
};

static BITFIELD ipr_bits[] = { // Fig 10, pg 9
    BITF(IPR, 5),              // 0-4: 5 most significant bits of the top element of the Return Stack or current page address
    BITNCF(11),                // 5-15: Reserved
    ENDBITS};

union IndexPageRegister
{
    struct IndexPageRegisterFields fields;
    RTX_WORD pr;
} ipr;
typedef union IndexPageRegister IPR;

/**
 * Data/User/Code Page Registers
 */
struct PageRegisterFields
{
    RTX_WORD pr : 4;    // 0-3: 4 most significant bits of the top element of the Return Stack (I) or current page address
    RTX_WORD mbz1 : 12; // 4-15: Reserved
};

static BITFIELD pr_bits[] = { // Fig 10, pg 9
    BITF(PR, 4),              // 0-3: 4 most significant bits of the top element of the Return Stack or current page address
    BITNCF(12),               // 4-15: Reserved
    ENDBITS};

union DataPageRegister
{
    struct PageRegisterFields fields;
    RTX_WORD pr;
} dpr;
typedef union DataPageRegister DPR;

/**
 * User Page & Base Address Registers
 */
union UserPageRegister
{
    struct PageRegisterFields fields;
    RTX_WORD pr;
} upr;
typedef union UserPageRegister UPR;

union CodePageRegister
{
    struct PageRegisterFields fields;
    RTX_WORD pr;
} cpr;
typedef union CodePageRegister CPR;

/**
 * Interrupt Base/Control Register
 */
struct InterruptBaseControlRegisterFields
{
    RTX_WORD sef : 1;    // 0: RO Stack Error flag
    RTX_WORD psu : 1;    // 1: RO Parameter Stack Underflow flag
    RTX_WORD rsu : 1;    // 2: RO Return Stack Underflow flag
    RTX_WORD psv : 1;    // 3: RO Parameter Stack Overflow flag
    RTX_WORD rsv : 1;    // 4: RO Return Stack Overflow flag
    RTX_WORD dprsel : 1; // 5: RW Data Page Register, 0=CPR, 1=DPR
    RTX_WORD mbz : 1;    // 6: Reserved
    RTX_WORD cycext : 1; // 7: RW Enable Extended Cycle Length
    RTX_WORD tc : 2;     // 8-9 RW Select Timer/Counter Input Signals
    RTX_WORD ivb : 5;    // 10-15 RW Interrupt Vector Base, MA10 - MA15
};

union InterruptBaseControlRegister
{
    struct InterruptBaseControlRegisterFields fields;
    RTX_WORD pr;
} ibc;
typedef union InterruptBaseControlRegister IBC;

#define SEF (ibc.fields.sef)
#define IBC_PSU (ibc.fields.psu)
#define IBC_RSU (ibc.fields.rsu)
#define IBC_PSV (ibc.fields.psv)
#define IBC_RSV (ibc.fields.rsv)
#define DPRSEL (ibc.fields.dprsel)
#define CYCEXT (ibc.fields.cycext)
#define TC (ibc.fields.tc)
#define IVB (ibc.fields.ivb)

static BITFIELD ibc_bits[] = { // Fig 5, pg 7
    BIT(SEF),                  // 0: RO Stack Error flag
    BIT(PSU),                  // 1: RO Parameter Stack Underflow flag
    BIT(RSU),                  // 2: RO Return Stack Underflow flag
    BIT(PSV),                  // 3: RO Parameter Stack Overflow flag
    BIT(RSV),                  // 4: RO Return Stack Overflow flag
    BIT(DPRSEL),               // 5: RW Data Page Register, 0=CPR, 1=DPR
    BITNC,                     // 6: Reserved
    BIT(CYCEXT),               // 7: RW Enable Extended Cycle Length
    BITF(TC, 2),               // 8-9 RW Select Timer/Counter Input Signals
    BITF(IVB, 5),              // 10-15 RW Interrupt Vector Base, MA10 - MA15
    ENDBITS};

struct UserBaseAddressRegisterFields
{
    RTX_WORD mbz : 1;  // 0: Reserved
    RTX_WORD page : 5; // 1-5: User Memory address page
    RTX_WORD adr : 10; // 6-15: User Memory address
};

union UserBaseAddressRegister
{
    struct UserBaseAddressRegisterFields fields;
    RTX_WORD pr;
} ubr;
typedef union UserBaseAddressRegister UBR;

static BITFIELD ubr_bits[] = { // Fig 13, pg 10
    BITNC,                     // 0: Reserved
    BITF(PAGE, 5),             // 1-5: User Memory Address Page
    BITF(ADR, 10),             // 6-15: User Memory Address
    ENDBITS};

static REG cpu_reg[] = {
    {HRDATAD(TOP, TOP, 16, "Parameter Stack top element"), REG_RO},
    {HRDATAD(NEXT, NEXT, 16, "Parameter Stack second element"), REG_RO},
    {HRDATAD(IR, IR, 16, "Instruction Register"), REG_HIDDEN},
    {HRDATAD(I, asic_file[I], 16, "Index Register"), REG_DEPOSIT | 1 << REG_V_UF},
    {HRDATADF(CR, cr.pr, 16, "Configuration Register", cr_bits), REG_DEPOSIT | (3 << REG_V_UF)},
    // 5
    {HRDATAD(MD, asic_file[MD], 16, "Multiply/Divide Register"), REG_DEPOSIT | (4 << REG_V_UF)},
    {HRDATAD(SR, asic_file[SR], 16, "Square Root Register"), REG_DEPOSIT | (6 << REG_V_UF)},
    {HRDATAD(PC, asic_file[PC], 16, "Program Counter Register"), REG_DEPOSIT | (7 << REG_V_UF)},
    {HRDATADF(IMR, imr.pr, 16, "Interrupt Mask Register", imr_bits), REG_DEPOSIT | (8 << REG_V_UF)},
    {HRDATADF(SPR, spr.pr, 16, "Stack Pointer Register", spr_bits), REG_RO | (9 << REG_V_UF)},
    // 10
    {HRDATADF(SUR, sur.pr, 16, "Stack Underflow Register", sur_bits), REG_DEPOSIT | (0xA << REG_V_UF)},
    {HRDATADF(SVR, svr.pr, 16, "Stack Overflow Register", svr_bits), REG_DEPOSIT | (0xB << REG_V_UF)},
    {HRDATADF(IVR, ivr.pr, 16, "Interrupt Vector Register", ivr_bits), REG_RO | (0xB << REG_V_UF)},
    {HRDATADF(IPR, ipr.pr, 16, "Index Page Register", pr_bits), REG_DEPOSIT | (0xC << REG_V_UF)},
    {HRDATADF(DPR, dpr.pr, 16, "Data Page Register", pr_bits), REG_DEPOSIT | (0xD << REG_V_UF)},
    // 15
    {HRDATADF(UPR, upr.pr, 16, "User Page Register", pr_bits), REG_DEPOSIT | (0xE << REG_V_UF)},
    {HRDATADF(CPR, cpr.pr, 16, "Code Page Register", pr_bits), REG_DEPOSIT | (0xE << REG_V_UF)},
    {HRDATADF(IBC, ibc.pr, 16, "Interrupt Base/Control Register", ibc_bits), REG_DEPOSIT | (0x10 << REG_V_UF)},
    {HRDATADF(UBR, ubr.pr, 16, "User Base Address Register", ubr_bits), REG_DEPOSIT | (0x11 << REG_V_UF)},
    {HRDATAD(TC0, asic_file[TC0], 16, "Timer Counter 0 Register"), REG_RO | (0x13 << REG_V_UF)},
    // 20
    {HRDATAD(TP0, asic_file[TP0], 16, "Timer Preload 0 Register"), REG_DEPOSIT | (0x14 << REG_V_UF)},
    {HRDATAD(TC1, asic_file[TC1], 16, "Timer Counter 1 Register"), REG_RO | (0x15 << REG_V_UF)},
    {HRDATAD(TP1, asic_file[TP1], 16, "Timer Preload 1 Register"), REG_DEPOSIT | (0x16 << REG_V_UF)},
    {HRDATAD(TC2, asic_file[TC2], 16, "Timer Counter 2 Register"), REG_RO | (0x17 << REG_V_UF)},
    {HRDATAD(TP2, asic_file[TP2], 16, "Timer Preload 2 Register"), REG_DEPOSIT | (0x18 << REG_V_UF)},
    {HRDATAD(RX, asic_file[RX], 16, "Scratchpad/Counting register"), REG_DEPOSIT | (0x19 << REG_V_UF)},
    {HRDATAD(RH, asic_file[RH], 16, "Scratchpad register"), REG_DEPOSIT | (0x20 << REG_V_UF)},
    {HRDATAD(RSP, spr.pr, 16, "Return Stack Pointer"), REG_DEPOSIT | (0x21 << REG_V_UF)},
    {HRDATAD(PSP, spr.pr, 16, "Parameter Stack Pointer"), REG_DEPOSIT | (0x22 << REG_V_UF)},
    {NULL}};

#endif