/* mp-1m.c: SWTP 1M Byte Memory Card emulator

    Copyright (c) 2011-2012, William A. Beech

        Permission is hereby granted, free of charge, to any person obtaining a
        copy of this software and associated documentation files (the "Software"),
        to deal in the Software without restriction, including without limitation
        the rights to use, copy, modify, merge, publish, distribute, sublicense,
        and/or sell copies of the Software, and to permit persons to whom the
        Software is furnished to do so, subject to the following conditions:

        The above copyright notice and this permission notice shall be included in
        all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS O
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
        WILLIAM A. BEECH BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
        IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
        CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

        Except as contained in this notice, the name of William A. Beech shall not be
        used in advertising or otherwise to promote the sale, use or other dealings
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
        04 Apr 24 -- Richard Lukes - Modified mp-8m.c to use for 1MB memory for swtp6809 emulator

    NOTES:

        These functions support 1MB of memory on an SS-50 system as a single 1MB memory card.
        Due to the way Dynamic Address Translation (DAT)  works, the upper 8KB of each 64KB address space is allocated
        to I/O ($E000) and ROM ($F000) space. Therefore, the maximum usable RAM is 16 x 56KB = 896KB.

        The unit uses a statically allocated 1MB byte buffer. This makes for a fast implementation.
        No effort was made to make available memory variable in size (e.g. limit to only 32KB, 64KB, 96KB, 128KB, etc.).
*/

#include <stdio.h>
#include "swtp_defs.h"

/* this is the maximum size of the physical address space */
#define ONE_MEGABYTE 0x100000

/* this is the value returned when reading a byte of non-existant memory */
#define BLANK_MEMORY_BYTE_VALUE 0xFF

#define UNIT_V_MSIZE    (UNIT_V_UF)              /* user defined options */
#define UNIT_MSIZE      (0xFF << UNIT_V_MSIZE)
#define UNIT_8KB        (0x01 << UNIT_V_MSIZE)   /* 8KB */
#define UNIT_16KB       (0x02 << UNIT_V_MSIZE)   /* 16KB */
#define UNIT_32KB       (0x04 << UNIT_V_MSIZE)   /* 32KB */
#define UNIT_56KB       (0x08 << UNIT_V_MSIZE)   /* 56KB */
#define UNIT_128KB      (0x10 << UNIT_V_MSIZE)   /* 128KB */
#define UNIT_256KB      (0x20 << UNIT_V_MSIZE)   /* 256KB */
#define UNIT_512KB      (0x30 << UNIT_V_MSIZE)   /* 512KB */
#define UNIT_1024KB     (0x40 << UNIT_V_MSIZE)   /* 1024KB */

/* prototypes */

t_stat mp_1m_reset (DEVICE *dptr);
t_stat mp_1m_config(UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat mp_1m_deposit(t_value val, t_addr addr, UNIT *uptr, int32 switches);
t_stat mp_1m_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches);
int32 mp_1m_get_mbyte(int32 addr);
int32 mp_1m_get_mword(int32 addr);
void mp_1m_put_mbyte(int32 addr, int32 val);
void mp_1m_put_mword(int32 addr, int32 val);

UNIT mp_1m_unit = { 
    UDATA (NULL, UNIT_FIX + UNIT_BUF + UNIT_BINK, ONE_MEGABYTE)
};

MTAB mp_1m_mod[] = { 
    { UNIT_MSIZE, UNIT_1024KB, "1MB of RAM", "1024KB", &mp_1m_config },
    { UNIT_MSIZE, UNIT_512KB, "512KB of RAM", "512KB", &mp_1m_config },
    { UNIT_MSIZE, UNIT_256KB, "256KB of RAM", "256KB", &mp_1m_config },
    { UNIT_MSIZE, UNIT_128KB, "128KB of RAM", "128KB", &mp_1m_config },
    { UNIT_MSIZE, UNIT_56KB, "56KB of RAM", "56KB", &mp_1m_config },
    { UNIT_MSIZE, UNIT_32KB, "32KB of RAM", "32KB", &mp_1m_config },
    { UNIT_MSIZE, UNIT_16KB, "16KB of RAM", "16KB", &mp_1m_config },
    { UNIT_MSIZE, UNIT_8KB, "8KB of RAM", "8KB", &mp_1m_config },
    { 0 }
};

DEBTAB mp_1m_debug[] = {
    { "ALL", DEBUG_all },
    { "FLOW", DEBUG_flow },
    { "READ", DEBUG_read },
    { "WRITE", DEBUG_write },
    { "LEV1", DEBUG_level1 },
    { "LEV2", DEBUG_level2 },
    { NULL }
};

DEVICE mp_1m_dev = {
    "MP-1M",                            //name
    &mp_1m_unit,                        //units
    NULL,                               //registers
    mp_1m_mod,                          //modifiers
    1,                                  //numunits
    16,                                 //aradix
    20,                                 //awidth
    1,                                  //aincr
    16,                                 //dradix
    8,                                  //dwidth
    &mp_1m_examine,                     //examine
    &mp_1m_deposit,                     //deposit
    &mp_1m_reset,                       //reset
    NULL,                               //boot
    NULL,                               //attach
    NULL,                               //detach
    NULL,                               //ctxt
    DEV_DEBUG,                          //flags
    0,                                  //dctrl
    mp_1m_debug,                        //debflags
    NULL,                               //msize
    NULL                                //lname
};

/* Pre-allocate 1MB array of bytes  */
uint8 mp_1m_ram_memory_array[ONE_MEGABYTE];

/* mp_1m_config */
t_stat mp_1m_config (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{

    sim_debug (DEBUG_flow, &mp_1m_dev, "mp_1m_config: val=%d\n", val);
    if ((val > UNIT_1024KB) || (val < UNIT_8KB)) { /* valid param? */
        sim_debug (DEBUG_flow, &mp_1m_dev, "mp_1m_config: Parameter error\n");
        return SCPE_ARG;
    }

    /* set RAM size. All RAM starts with a base address of 00000H */
    switch ( val ) {
        case UNIT_8KB:
            mp_1m_unit.capac = 8 * 1024;
            break;
        case UNIT_16KB:
            mp_1m_unit.capac = 16 * 1024;
            break;
        case UNIT_32KB:
            mp_1m_unit.capac = 32 * 1024;
            break;
        case UNIT_56KB:
            mp_1m_unit.capac = 56 * 1024;
            break;
        case UNIT_128KB:
            mp_1m_unit.capac = 128 * 1024;
            break;
        case UNIT_256KB:
            mp_1m_unit.capac = 256 * 1024;
            break;
        case UNIT_512KB:
            mp_1m_unit.capac = 512 * 1024;
            break;
        case UNIT_1024KB:
            mp_1m_unit.capac = 1024 * 1024;
            break;
        default:
            /* what to do? default to 1024KB */
            mp_1m_unit.capac = 1024 * 1024;
    }
    sim_debug (DEBUG_flow, &mp_1m_dev, "mp_1m_config: mp_1m_unit.capac=%d\n", mp_1m_unit.capac);
    sim_debug (DEBUG_flow, &mp_1m_dev, "mp_1m_config: Done\n");

    return SCPE_OK;
} /* mp-1m config */

/* Deposit routine */
t_stat mp_1m_deposit(t_value val, t_addr addr, UNIT *uptr, int32 switches)
{
    if (addr >= mp_1m_unit.capac) {
        return SCPE_NXM;
    } else {
        mp_1m_ram_memory_array[addr] = val & 0xFF;
        return SCPE_OK;
    }
}

/* Examine routine */
t_stat mp_1m_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches)
{
    if (addr >= mp_1m_unit.capac) {
        return SCPE_NXM;
    }
    if (eval_array != NULL) {
        *eval_array = mp_1m_ram_memory_array[addr];
    }
    return SCPE_OK;
}

/* Reset routine */

t_stat mp_1m_reset (DEVICE *dptr)
{
    int32 i, j, val;
    UNIT *uptr;

    sim_debug (DEBUG_flow, &mp_1m_dev, "mp_1m_reset: \n");
    uptr = mp_1m_dev.units;
    sim_debug (DEBUG_flow, &mp_1m_dev, "MP-1M %d unit.flags=%08X\n", i, uptr->flags);

    // capacity
    uptr->capac = ONE_MEGABYTE;
    // starting address
    uptr->u3 = 0;

    if (uptr->filebuf == NULL) {
        uptr->filebuf = &mp_1m_ram_memory_array;
        if (uptr->filebuf == NULL) {
            printf("mp_1m_reset: Malloc error\n");
            return SCPE_MEM;
        }
        for (j=0; j < uptr->capac; j++) {    /* populate memory with fill pattern */
            i = j & 0xF;
            val = (0xA0 |  i) & BYTEMASK;
            *((uint8 *)(uptr->filebuf) + j) = val;
        }
        sim_debug (DEBUG_flow, &mp_1m_dev, "MP-1M %d initialized at [%05X-%05XH]\n", i, uptr->u3, uptr->u3 + uptr->capac - 1);
    }
    sim_debug (DEBUG_flow, &mp_1m_dev, "mp_1m_reset: Done\n");
    return SCPE_OK;
}

/*
    I/O instruction handlers, called from the mp-b3 module when an
    external memory read or write is issued.
*/

/*  get a byte from memory */

int32 mp_1m_get_mbyte(int32 addr)
{
    //UNIT *uptr;
    int32 val;

    //uptr = mp_1m_dev.units;
    sim_debug (DEBUG_read, &mp_1m_dev, "mp_1m_get_mbyte: addr=%05X", addr);
    if (addr >= mp_1m_unit.capac) {
        sim_debug (DEBUG_read, &mp_1m_dev, "mp_1m_get_mbyte: addr=%08X Out of range\n", addr);
        return BLANK_MEMORY_BYTE_VALUE;
    } else {
        val = mp_1m_ram_memory_array[addr];
        sim_debug (DEBUG_read, &mp_1m_dev, " addr=%05x val=%02X\n", addr, val);
        return val;
    }
}

/*  get a word from memory */

int32 mp_1m_get_mword(int32 addr)
{
    int32 val;

    val = (mp_1m_get_mbyte(addr) << 8);
    val |= mp_1m_get_mbyte(addr+1);
    return val;
}

/*  put a byte into memory */

void mp_1m_put_mbyte(int32 addr, int32 val)
{
    sim_debug (DEBUG_write, &mp_1m_dev, "mp_1m_put_mbyte: addr=%05X, val=%02X", addr, val);

    if (addr >= mp_1m_unit.capac) {
        sim_debug (DEBUG_write, &mp_1m_dev, "mp_1m_put_mbyte: Address out of range, addr=%08x\n", addr);
    } else {
        mp_1m_ram_memory_array[addr] = val;
        sim_debug (DEBUG_write, &mp_1m_dev, "\n");
    }
}

/*  put a word into memory */

void mp_1m_put_mword(int32 addr, int32 val)
{
    mp_1m_put_mbyte(addr, val >> 8);
    mp_1m_put_mbyte(addr+1, val & BYTEMASK);
}

/* end of mp-1m.c */
