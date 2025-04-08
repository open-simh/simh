/*  mp-09.c: SWTP MP-09 M6809 CPU simulator

    Copyright (c) 2011-2012, William Beech

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
        WILLIAM A. BEECH BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
        IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
        CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

        Except as contained in this notice, the name of William A. Beech shall not
        be used in advertising or otherwise to promote the sale, use or other dealings
        in this Software without prior written authorization from William A. Beech.

    The following copyright notice applies to the SWTP 6809 source, binary, and documentation:
 
    Original code published in 2024, written by Richard F Lukes
    Copyright (c) 2024, Richard F Lukes
 
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
        THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
        IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
        CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
        Except as contained in this notice, the names of The Authors shall not be
        used in advertising or otherwise to promote the sale, use or other dealings
        in this Software without prior written authorization from the Authors.

    MODIFICATIONS:

        24 Apr 15 -- Modified to use simh_debug
        04 Apr 24 -- Richard Lukes - modified SWTPC mp-a2 emulator to create mp-09 emulator.

    NOTES:

        The MP-09 CPU Board includes the SWTP Dynamic Address Translation (DAT) logic which
        is used to create a 20-bit virtual address space (1MB of RAM).

        The MP-09 CPU Board contains the following devices [mp-09.c]:
            M6809 processor [m6809.c].
            SWTPC SBUG-E, or custom boot ROM at top of 16-bit address space [bootrom.c].
            Interface to the SS-50 bus and the MP-B3 Mother Board for I/O
                and memory boards [mp-b3.c].
*/

#include <stdio.h>
#include "swtp_defs.h"

#define UNIT_V_DAT   (UNIT_V_UF)     /* Dynamic Address Translation setting */
#define UNIT_DAT     (1 << UNIT_V_DAT)


/* local global variables */
unsigned char DAT_RAM[16];
unsigned char DAT_RAM_CACHE[16];

/* function prototypes */

t_stat CPU_BD_reset (DEVICE *dptr);
t_stat CPU_BD_deposit(t_value val, t_addr addr, UNIT *uptr, int32 switches);
t_stat CPU_BD_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches);
int32 DAT_Xlate(int32 logical_addr);
int32 CPU_BD_get_mbyte(int32 addr);
int32 CPU_BD_get_mword(int32 addr);
void CPU_BD_put_mbyte(int32 addr, int32 val);
void CPU_BD_put_mword(int32 addr, int32 val);

/* external routines */

/* MP-B3 bus routines */
extern int32 MB_get_mbyte(int32 addr);
extern int32 MB_get_mword(int32 addr);
extern void MB_put_mbyte(int32 addr, int32 val);
extern void MB_put_mword(int32 addr, int32 val);

/* MP-09 data structures

   CPU_BD_dev        MP-09 device descriptor
   CPU_BD_unit       MP-09 unit descriptor
   CPU_BD_reg        MP-09 register list
   CPU_BD_mod        MP-09 modifiers list
*/

UNIT CPU_BD_unit = { UDATA (NULL, 0, 0) };

REG CPU_BD_reg[] = {
    { NULL }
};

MTAB CPU_BD_mod[] = {
    { UNIT_DAT, UNIT_DAT, "DAT enabled", "DAT", NULL },
    { UNIT_DAT, 0, "DAT disabled", "NODAT", NULL },
    { 0 }
};

DEBTAB CPU_BD_debug[] = {
    { "ALL", DEBUG_all },
    { "FLOW", DEBUG_flow },
    { "READ", DEBUG_read },
    { "WRITE", DEBUG_write },
    { "LEV1", DEBUG_level1 },
    { "LEV2", DEBUG_level2 },
    { NULL }
};

DEVICE CPU_BD_dev = {
    "MP-09",                            //name
    &CPU_BD_unit,                       //units
    CPU_BD_reg,                         //registers
    CPU_BD_mod,                         //modifiers
    1,                                  //numunits
    16,                                 //aradix 
    8,                                  //awidth 
    1,                                  //aincr 
    16,                                 //dradix 
    8,                                  //dwidth
    &CPU_BD_examine,                    //examine 
    &CPU_BD_deposit,                    //deposit 
    &CPU_BD_reset,                      //reset
    NULL,                               //boot
    NULL,                               //attach 
    NULL,                               //detach
    NULL,                               //ctxt
    DEV_DEBUG,                          //flags 
    0,                                  //dctrl 
    CPU_BD_debug,                       /* debflags */
    NULL,                               //msize
    NULL                                //lname
};

/* reset */
t_stat CPU_BD_reset (DEVICE *dptr)
{
    int32 i;

    /* this is performed whether DAT is enabled or not */
    // initialize DAT RAM
    for (i=0; i<16; i++) {
        DAT_RAM[i] = (~i) & 0x0F;
        DAT_RAM_CACHE[i] = i;
    }
    return SCPE_OK;
}

/* Deposit routine */
t_stat CPU_BD_deposit(t_value val, t_addr addr, UNIT *uptr, int32 switches)
{
    if (addr > ADDRMASK) {
        return SCPE_NXM;
    } else {
        CPU_BD_put_mbyte(addr, val);
        return SCPE_OK;
    }
}

/* Examine routine */
t_stat CPU_BD_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches)
{
    if (addr > ADDRMASK) {
        return SCPE_NXM;
    }
    if (eval_array != NULL) {
        *eval_array = CPU_BD_get_mbyte(addr);
    }
    return SCPE_OK;
}

/* Perform adress translation from 16-bit logical address to 20-bit physical address using DAT mapping RAM */
int32 DAT_Xlate(int32 logi_addr)
{

    /* if DAT is enabled perform the Dynamic Address Translation */
    if (CPU_BD_unit.flags & UNIT_DAT) {

        int32 DAT_index;    /* which of the 16 mapping registers to index */
        int32 DAT_byte;     /* the lookup value from DAT RAM */
        int32 DAT_block;    /* A13-A14-A15-A16 */

        /* translation from 16-bit logical address to 20-bit physical address */
        DAT_index = (logi_addr & 0xF000) >> 12;
        DAT_byte = DAT_RAM_CACHE[DAT_index];
        if (DAT_index >= 0xE) {
            /* for logical addresses $E000-$FFFF */
            // Bank address (A17-A20) is 0b0000
            return((logi_addr & 0xFFF) + ((DAT_byte & 0x0F)<<12));
        } else {
            // Bank address (A17-A20) is the high order 4-bits of DAT_byte
            return((logi_addr & 0xFFF) + (DAT_byte<<12));
        }

   } else {
       /* DAT is disabled */
       return(logi_addr);
   }
}

/*  get a byte from memory */
int32 CPU_BD_get_mbyte(int32 addr)
{
    int32 val = 0;
    int32 phy_addr;

    sim_debug (DEBUG_read, &CPU_BD_dev, "CPU_BD_get_mbyte: addr=%04X\n", addr);
    switch(addr & 0xF000) {
        case 0xE000:
        case 0xF000:
	    /* ROM is mapped to logical address $E000-$FFFF at all times! Unaffected by DAT */
            val = MB_get_mbyte(addr);
            sim_debug (DEBUG_read, &CPU_BD_dev, "CPU_BD_get_mbyte: mp_b3 val=%02X\n", val);
            break;
        default:
	    /* access the resources on the motherboard - 16-bit addressing */
	    /* 56K of RAM from 0000-DFFF */
	    /* 8K of I/O space from E000-EFFF */
	    /* 2K of scratchpad RAM from F000-F7FF ??? */
            if (CPU_BD_unit.flags & UNIT_DAT) {
	        phy_addr = DAT_Xlate(addr);
            } else {
	        phy_addr = addr;
            }
            val = MB_get_mbyte(phy_addr);
            sim_debug (DEBUG_read, &CPU_BD_dev, "CPU_BD_get_mbyte: mp_b3 val=%02X\n", val);
            break;
    }
    return val;
}

/*  get a word from memory */
int32 CPU_BD_get_mword(int32 addr)
{
    int32 val;

    sim_debug (DEBUG_read, &CPU_BD_dev, "CPU_BD_get_mword: addr=%04X\n", addr);
    val = (CPU_BD_get_mbyte(addr) << 8);
    val |= CPU_BD_get_mbyte(addr+1);
    val &= 0xFFFF;
    sim_debug (DEBUG_read, &CPU_BD_dev, "CPU_BD_get_mword: val=%04X\n", val);
    return val;
}

/*  put a byte to memory */
void CPU_BD_put_mbyte(int32 addr, int32 val)
{
    int32 phy_addr;

    sim_debug (DEBUG_write, &CPU_BD_dev, "CPU_BD_put_mbyte: addr=%04X, val=%02X\n", addr, val);

    if ((addr & 0xFFF0) == 0xFFF0) {
        /* this is performed whether DAT is enabled or not */
        DAT_RAM[addr & 0x0F] = val;
        DAT_RAM_CACHE[addr & 0x0F] = (val & 0xF0) | (~val & 0x0F);
    } else {
        switch(addr & 0xF800) {
            case 0xF800:
	        /* do not write to ROM area */
	        break;
            default:
	        phy_addr = DAT_Xlate(addr);
                MB_put_mbyte(phy_addr, val);
	        break;
        }
    }
}

/*  put a word to memory */
void CPU_BD_put_mword(int32 addr, int32 val)
{
    sim_debug (DEBUG_write, &CPU_BD_dev, "CPU_BD_put_mword: addr=%04X, val=%04X\n", addr, val);
    CPU_BD_put_mbyte(addr, val >> 8);
    CPU_BD_put_mbyte(addr+1, val & 0xFF);
}

/* end of mp-09.c */
