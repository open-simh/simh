/*  mp-b3.c: SWTP SS-50/SS-30 MP-B3 Mother Board

    Copyright (c) 2011-2012, William A. Beech

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
        04 Apr 24 -- Richard Lukes - Modified to emulate MP-B3 motherboard for swtp6809 emulator

    NOTES:

*/

#include <stdio.h>
#include "swtp_defs.h"


#define UNIT_V_16BYTESPERSLOT    (UNIT_V_UF) /* MP-1M board enable */
#define UNIT_16BYTESPERSLOT      (1 << UNIT_V_16BYTESPERSLOT)

#define MASK_20BIT               0xFFFFF
#define MASK_16BIT               0xFFFF

/* function prototypes */

/* empty I/O device routine */
int32 nulldev(int32 io, int32 data);

/* SS-50 bus routines */
t_stat MB_reset (DEVICE *dptr);
t_stat MB_deposit(t_value val, t_addr addr, UNIT *uptr, int32 switches);
t_stat MB_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches);
int32 MB_get_mbyte(int32 addr);
int32 MB_get_mword(int32 addr);
void MB_put_mbyte(int32 addr, int32 val);
void MB_put_mword(int32 addr, int32 val);

/* BOOTROM memory access routines */
extern int32 BOOTROM_get_mbyte(int32 offset);

/* MP-1M memory access routines */
extern int32 mp_1m_get_mbyte(int32 addr);
extern void mp_1m_put_mbyte(int32 addr, int32 val);

/* SS-50 I/O address space functions */

/* MP-S serial I/O routines */
extern int32 sio0s(int32 io, int32 data);
extern int32 sio0d(int32 io, int32 data);
extern int32 sio1s(int32 io, int32 data);
extern int32 sio1d(int32 io, int32 data);

/* DC-4 FDC I/O routines */
extern int32 fdcdrv(int32 io, int32 data);
extern int32 fdccmd(int32 io, int32 data);
extern int32 fdctrk(int32 io, int32 data);
extern int32 fdcsec(int32 io, int32 data);
extern int32 fdcdata(int32 io, int32 data);

/*
MP-B3 configured with 4 address per SS-30 slot (x8).

This is the I/O configuration table.  There are 32 possible
device addresses, if a device is plugged into a port it's routine
address is here, 'nulldev' means no device is available
*/

struct idev {
        int32 (*routine)(int32, int32);
};

struct idev dev_table[32] = {
        {&sio0s},   {&sio0d},   {&sio1s},   {&sio1d},   /* Port 0 E000-E003 */
        {&sio0s},   {&sio0d},   {&sio1s},   {&sio1d},   /* Port 1 E004-E007 */
/* sio1x routines just return the last value read on the matching
   sio0x routine.  SWTBUG tests for the MP-C with most port reads! */
        {&nulldev}, {&nulldev}, {&nulldev}, {&nulldev}, /* Port 2 E008-E00B */
        {&nulldev}, {&nulldev}, {&nulldev}, {&nulldev}, /* Port 3 E00C-E00F */
        {&nulldev}, {&nulldev}, {&nulldev}, {&nulldev}, /* Port 4 E010-E013 */
        {&fdcdrv},  {&nulldev}, {&nulldev}, {&nulldev}, /* Port 5 E014-E017 */
        {&fdccmd},  {&fdctrk},  {&fdcsec},  {&fdcdata}, /* Port 6 E018-E01B */
        {&nulldev}, {&nulldev}, {&nulldev}, {&nulldev}  /* Port 7 E01C-E01F */
};

/* dummy i/o device */

int32 nulldev(int32 io, int32 data)
{
    if (io == 0) {
        return (0xFF);
    } else {
        return 0;
    }
}

/* Mother Board data structures

    MB_dev        Mother Board device descriptor
    MB_unit       Mother Board unit descriptor
    MB_reg        Mother Board register list
    MB_mod        Mother Board modifiers list
*/

UNIT MB_unit = {
    UDATA (NULL, 0, 0)
};

REG MB_reg[] = {
    { NULL }
};

MTAB MB_mod[] = {
    { UNIT_16BYTESPERSLOT, UNIT_16BYTESPERSLOT, "I/O port size of 16 bytes not implemented", "16BYTE", NULL, NULL },
    { UNIT_16BYTESPERSLOT, 0, "I/O port size of 4 bytes", "4BYTE", NULL, NULL },
    { 0 }
};

DEBTAB MB_debug[] = {
    { "ALL", DEBUG_all },
    { "FLOW", DEBUG_flow },
    { "READ", DEBUG_read },
    { "WRITE", DEBUG_write },
    { "LEV1", DEBUG_level1 },
    { "LEV2", DEBUG_level2 },
    { NULL }
};

DEVICE MB_dev = {
    "MP-B3",                            //name
    &MB_unit,                           //units
    MB_reg,                             //registers
    MB_mod,                             //modifiers
    1,                                  //numunits
    16,                                 //aradix
    20,                                 //awidth
    1,                                  //aincr
    16,                                 //dradix
    8,                                  //dwidth
    &MB_examine,                        //examine
    &MB_deposit,                        //deposit
    &MB_reset,                          //reset
    NULL,                               //boot
    NULL,                               //attach
    NULL,                               //detach
    NULL,                               //ctxt
    DEV_DEBUG,                          //flags
    0,                                  //dctrl
    MB_debug,                           /* debflags */
    NULL,                               //msize
    NULL,                               //logical name
    NULL,                               //help routine
    NULL,                               //attach help routine
    NULL,                               //help context
    NULL                                //device description
};

/* reset */
t_stat MB_reset (DEVICE *dptr)
{
    return SCPE_OK;
}

/* Deposit routine */
t_stat MB_deposit(t_value val, t_addr addr, UNIT *uptr, int32 switches)
{
    if (addr > MASK_20BIT) {
        /* exceed 20-bit address space */
        return SCPE_NXM;
    } else {
        MB_put_mbyte(addr, val);
        return SCPE_OK;
    }
}

/* Examine routine */
t_stat MB_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches)
{
    if (addr > MASK_20BIT) {
        return SCPE_NXM;
    }
    if (eval_array != NULL) {
        *eval_array = MB_get_mbyte(addr);
    }
    return SCPE_OK;
}

/*  get a byte from memory */

int32 MB_get_mbyte(int32 addr20)
{
    int32 val;
    int32 addr16;

    addr16 = addr20 & MASK_16BIT;

    // 20-bit physical addresses
    sim_debug (DEBUG_read, &MB_dev, "MB_get_mbyte: addr=%05X\n", addr20);
    switch (addr20 & 0x0F800) {
        case 0x0E000:
	    /* reserved I/O space from $E000-$E01F */
            /* read byte value from device */
            if (addr16 < 0xE020) {
                val = (dev_table[addr16 - 0xE000].routine(0, 0)) & 0xFF;
                break;
            }
        case 0x0E800:
        case 0x0F000:
        case 0x0F800:
            /* Up to 8KB of boot ROM from $E000-$FFFF */
            val = BOOTROM_get_mbyte(addr16);
            sim_debug (DEBUG_read, &MB_dev, "MB_get_mbyte: EPROM=%02X\n", val);
            break;
	default:
	    /* all the rest is RAM */
            val = mp_1m_get_mbyte(addr20);
            if (MB_dev.dctrl & DEBUG_read) {
            	printf("MB_get_mbyte: mp_1m add=%05x val=%02X\n", addr20, val);
            }
            break;
    }
    sim_debug (DEBUG_read, &MB_dev, "MB_get_mbyte: I/O addr=%05X val=%02X\n", addr20, val);
    return val;
}

/*  get a word from memory */

int32 MB_get_mword(int32 addr20)
{
    int32 val;

    sim_debug (DEBUG_read, &MB_dev, "MB_get_mword: addr=%05X\n", addr20);
    val = (MB_get_mbyte(addr20) << 8);
    val |= MB_get_mbyte(addr20+1);
    val &= MASK_16BIT;
    sim_debug (DEBUG_read, &MB_dev, "MB_get_mword: val=%05X\n", val);
    return val;
}

/*  put a byte to memory */

void MB_put_mbyte(int32 addr20, int32 val)
{
    int32 retval;
    int32 addr16;

    addr16 = addr20 & MASK_16BIT;

    // 20-bit physical addresses
    sim_debug (DEBUG_write, &MB_dev, "MB_put_mbyte: addr=%05X, val=%02X\n", addr20, val);

    switch(addr16 & 0xF000) {
        case 0xE000:
	    /* I/O and ROM space */
            if (addr16 < 0xE020) {
	        /* reserved I/O space from $E000-$E01F */
                /* write byte value to device */
                retval = dev_table[addr16 - 0xE000].routine(1, val);
            }
	    break;
        case 0xF000:
	    /* ROM space - READ ONLY! */
	    break;
	default:
	    /* RAM */
	    mp_1m_put_mbyte(addr20, val);
	    break;
    }
}
    
/*  put a word to memory */

void MB_put_mword(int32 addr20, int32 val)
{
    sim_debug (DEBUG_write, &MB_dev, "MB_ptt_mword: addr=%05X, val=%04X\n", addr20, val);
    MB_put_mbyte(addr20, val >> 8);
    MB_put_mbyte(addr20+1, val & 0xFF);
}

/* end of mp-b3.c */
