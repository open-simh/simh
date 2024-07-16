/*  altairz80_defs.h: MITS Altair simulator definitions

    Copyright (c) 2002-2024, Peter Schorn

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
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    PETER SCHORN BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    Except as contained in this notice, the name of Peter Schorn shall not
    be used in advertising or otherwise to promote the sale, use or other dealings
    in this Software without prior written authorization from Peter Schorn.

    Based on work by Charles E Owen (c) 1997
*/

#ifndef ALTAIRZ80_DEFS_H_
#define ALTAIRZ80_DEFS_H_    0

#include "sim_defs.h"                                       /* simulator definitions                        */

#define MAXBANKSIZE             65536                       /* maximum memory size, a power of 2            */
#define MAXBANKSIZELOG2         16                          /* log2 of MAXBANKSIZE                          */
#define MAXBANKS                16                          /* max number of memory banks, a power of 2     */
#define MAXBANKSLOG2            4                           /* log2 of MAXBANKS                             */
#define MAXMEMORY               (MAXBANKS * MAXBANKSIZE)    /* maximum, total memory size                   */
#define ADDRMASK                (MAXBANKSIZE - 1)           /* address mask                                 */
#define ADDRMASKEXTENDED        (MAXMEMORY - 1)             /* extended address mask                        */
#define BANKMASK                (MAXBANKS - 1)              /* bank mask                                    */
#define MEMORYSIZE              (cpu_unit.capac)            /* actual memory size                           */
#define MEMORYMASK              (cpu_unit.capac - 1)        /* actual memory size mask                      */
#define KB                      1024                        /* kilo byte                                    */
#define KBLOG2                  10                          /* log2 of KB                                   */
#define ALTAIR_ROM_LOW          0xff00                      /* start address of regular Altair ROM          */
#define RESOURCE_TYPE_MEMORY    1
#define RESOURCE_TYPE_IO        2
#define MAX_INT_VECTORS         32                          /* maximum number of interrupt vectors          */

#define NUM_OF_DSK              16                          /* NUM_OF_DSK must be power of two              */
#define LDA_INSTRUCTION         0x3e                        /* op-code for LD A,<8-bit value> instruction   */
#define UNIT_NO_OFFSET_1        0x37                        /* LD A,<unitno>                                */
#define UNIT_NO_OFFSET_2        0xb4                        /* LD a,80h | <unitno>                          */

#define CPU_INDEX_8080          4                           /* index of default PC register */

typedef enum {
    CHIP_TYPE_8080 = 0,
    CHIP_TYPE_Z80,
    CHIP_TYPE_8086,
    CHIP_TYPE_M68K,     /* must come after 8080, Z80 and 8086 */
    NUM_CHIP_TYPE,      /* must be last */
} ChipType;

/* simulator stop codes */
#define STOP_IBKPT              1                           /* breakpoint   (program counter)               */
#define STOP_MEM                2                           /* breakpoint   (memory access)                 */
#define STOP_INSTR              3                           /* breakpoint   (instruction access)            */
#define STOP_OPCODE             4                           /* invalid operation encountered (8080, Z80, 8086) */
#define STOP_HALT               5                           /* HALT                                         */

#define UNIT_CPU_V_OPSTOP       (UNIT_V_UF+0)               /* stop on invalid operation                    */
#define UNIT_CPU_OPSTOP         (1 << UNIT_CPU_V_OPSTOP)
#define UNIT_CPU_V_BANKED       (UNIT_V_UF+1)               /* banked memory is used                        */
#define UNIT_CPU_BANKED         (1 << UNIT_CPU_V_BANKED)
#define UNIT_CPU_V_ALTAIRROM    (UNIT_V_UF+2)               /* ALTAIR ROM exists                            */
#define UNIT_CPU_ALTAIRROM      (1 << UNIT_CPU_V_ALTAIRROM)
#define UNIT_CPU_V_VERBOSE      (UNIT_V_UF+3)               /* warn if ROM is written to                    */
#define UNIT_CPU_VERBOSE        (1 << UNIT_CPU_V_VERBOSE)
#define UNIT_CPU_V_MMU          (UNIT_V_UF+4)               /* use MMU and slower CPU                       */
#define UNIT_CPU_MMU            (1 << UNIT_CPU_V_MMU)
#define UNIT_CPU_V_STOPONHALT   (UNIT_V_UF+5)               /* stop simulation on HALT                      */
#define UNIT_CPU_STOPONHALT     (1 << UNIT_CPU_V_STOPONHALT)
#define UNIT_CPU_V_SWITCHER     (UNIT_V_UF+6)               /* switcher 8086 <--> 8080/Z80 enabled          */
#define UNIT_CPU_SWITCHER       (1 << UNIT_CPU_V_SWITCHER)
#define UNIT_CPU_V_PO           (UNIT_V_UF+7)               /* enable programmed output messages            */
#define UNIT_CPU_PO             (1 << UNIT_CPU_V_PO)

#define ADDRESS_FORMAT          "[0x%08x]"

typedef struct {
    uint32 mem_base;    /* Memory Base Address */
    uint32 mem_size;    /* Memory Address space requirement */
    uint32 io_base;     /* I/O Base Address */
    uint32 io_size;     /* I/O Address Space requirement */
} PNP_INFO;

extern ChipType chiptype;

#endif
