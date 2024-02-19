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

    MODIFICATIONS:

        24 Apr 15 -- Modified to use simh_debug
        19 Feb 24 -- Richard Lukes - Modified mp-8m.c to use for 1MB memory for swtp6809 emulator

    NOTES:

        These functions support 1MB of memory on an SS-50 system as a single 1MB memory card.
        Due to the way DAT works, the upper 8KB of each 64KB address space is allocated to I/O ($E000) and ROM ($F000) space.
        Therefore, the maximum usable RAM is 16 x 56KB = 896KB.

        The unit uses a statically allocated 1MB byte buffer. This makes for a fast implementation.
        No effort was made to make available memory variable in size (e.g. limit to only 32KB, 64KB, 96KB, 128KB, etc.).
*/

#include <stdio.h>
#include "swtp_defs.h"

/* there is only one of these devices */
#define ONE_MEGABYTE 0x100000

/* prototypes */

t_stat mp_1m_reset (DEVICE *dptr);
t_stat mp_1m_deposit(t_value val, t_addr addr, UNIT *uptr, int32 switches);
t_stat mp_1m_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches);
int32 mp_1m_get_mbyte(int32 addr);
int32 mp_1m_get_mword(int32 addr);
void mp_1m_put_mbyte(int32 addr, int32 val);
void mp_1m_put_mword(int32 addr, int32 val);

/* isbc064 Standard I/O Data Structures */

UNIT mp_1m_unit = { 
    UDATA (NULL, UNIT_FIX + UNIT_BINK, ONE_MEGABYTE)
};

MTAB mp_1m_mod[] = { 
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

/* Deposit routine */
t_stat mp_1m_deposit(t_value val, t_addr addr, UNIT *uptr, int32 switches)
{
    if (addr >= ONE_MEGABYTE) {
        return SCPE_NXM;
    } else {
        mp_1m_ram_memory_array[addr] = val & 0xFF;
        return SCPE_OK;
    }
}

/* Examine routine */
t_stat mp_1m_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches)
{
    if (addr >= ONE_MEGABYTE) {
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
        for (j=0; j < uptr->capac; j++) {    /* fill pattern for testing */
            i = j & 0xF;
            val = (0xA0 |  i) & 0xFF;
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
    if (addr >= ONE_MEGABYTE) {
        sim_debug (DEBUG_read, &mp_1m_dev, "mp_1m_get_mbyte: addr=%08X Out of range\n", addr);
        return 0xFF;
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
    //UNIT *uptr;

    //uptr = mp_1m_dev.units;
    sim_debug (DEBUG_write, &mp_1m_dev, "mp_1m_put_mbyte: addr=%05X, val=%02X", addr, val);

    if (addr >= ONE_MEGABYTE) {
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
    mp_1m_put_mbyte(addr+1, val & 0xFF);
}

/* end of mp-1m.c */
