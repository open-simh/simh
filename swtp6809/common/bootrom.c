/*  bootrom.c: Boot ROM simulator for Motorola processors

    Copyright (c) 2010-2012, William A. Beech

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

        23 Apr 15 -- Modified to use simh_debug
        04 Apr 24 -- Richard Lukes - Modified for swtp6809 emulator

    NOTES:

        These functions support a single simulated 2704 to 2764 EPROM device on 
        an 8-bit computer system..  This device allows the buffer to be loaded from
        a binary file containing the emulated EPROM code.

        These functions support a simulated 2704(0.5KB), 2708(1KB), 2716(2KB), 2732(4KB) or 2764(8KB) EPROM
        device at the top of the 16-bit address space. The base address of the ROM varies depends on the size.
        For example, The 2764 BOOTROM is mapped from $E000 to $FFFF less the I/O range reserved by the MP-B3 motherboard.
        The 2716 BOOTROM is mapped from $F800 to $FFFF.
        The last byte of the ROM is always stored at $FFFF

        The device type is stored as a binary number in the first three unit flag bits.

        This device uses a statically allocated buffer to hold the EPROM image. All bytes are initialized to $FF.
        A call to BOOTROM_attach will load the buffer with the EPROM image.

*/

#include <stdio.h>
#include "swtp_defs.h"

#define UNIT_V_MSIZE    (UNIT_V_UF + 0)        /* ROM Size */
#define UNIT_MSIZE      (0x2F << UNIT_V_MSIZE)
#define UNIT_NONE       (0 << UNIT_V_MSIZE)  /* No EPROM */
#define UNIT_2704       (1 << UNIT_V_MSIZE)  /* 2704 mode */
#define UNIT_2708       (2 << UNIT_V_MSIZE)  /* 2708 mode */
#define UNIT_2716       (3 << UNIT_V_MSIZE)  /* 2716 mode */
#define UNIT_2732       (4 << UNIT_V_MSIZE)  /* 2732 mode */
#define UNIT_2764       (5 << UNIT_V_MSIZE)  /* 2764 mode */

/* Maximum size of bootrom is 8KB from $E000-$FFFF */
#define MAX_BOOTROM_SIZE (8*1024)

// this value is used when referencing BOOTROM memory that is not populated (i.e. not loaded by BOOTROM attach)
#define DEFAULT_NO_ROM_BYTE_VALUE 0xFF

/* function prototypes */

t_stat BOOTROM_attach(UNIT *uptr, CONST char *cptr);
t_stat BOOTROM_config(UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat BOOTROM_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches);
t_stat BOOTROM_reset(DEVICE *dptr);

int32  BOOTROM_get_mbyte(int32 address);

/* SIMH Standard I/O Data Structures */

UNIT BOOTROM_unit = {
    UDATA (NULL, UNIT_ATTABLE+UNIT_BINK+UNIT_ROABLE+UNIT_RO, 0),
    KBD_POLL_WAIT };

MTAB BOOTROM_mod[] = {
    { UNIT_MSIZE, UNIT_NONE, "NONE", "NONE", &BOOTROM_config },
    { UNIT_MSIZE, UNIT_2704, "2704", "2704", &BOOTROM_config },
    { UNIT_MSIZE, UNIT_2708, "2708", "2708", &BOOTROM_config },
    { UNIT_MSIZE, UNIT_2716, "2716", "2716", &BOOTROM_config },
    { UNIT_MSIZE, UNIT_2732, "2732", "2732", &BOOTROM_config },
    { UNIT_MSIZE, UNIT_2764, "2764", "2764", &BOOTROM_config },
    { 0 }
};

DEBTAB BOOTROM_debug[] = {
    { "ALL", DEBUG_all, "Debug all"},
    { "FLOW", DEBUG_flow, "Debug flow of control" },
    { "READ", DEBUG_read, "Debug device reads" },
    { "WRITE", DEBUG_write, "Debug device writes" },
    { "LEV1", DEBUG_level1, "Debug level 1" },
    { "LEV2", DEBUG_level2, "Debug level 2" },
    { NULL }
};

DEVICE BOOTROM_dev = {
    "BOOTROM",                      /* name */
    &BOOTROM_unit,                  /* units */
    NULL,                           /* registers */
    BOOTROM_mod,                    /* modifiers */
    1,                              /* numunits */
    16,                             /* aradix */
    16,                             /* awidth */
    1,                              /* aincr */
    16,                             /* dradix */
    8,                              /* dwidth */
    &BOOTROM_examine,               /* examine */
    NULL,                           /* deposit */
    &BOOTROM_reset,                 /* reset */
    NULL,                           /* boot */
    &BOOTROM_attach,                /* attach */
    NULL,                           /* detach */
    NULL,                           /* ctxt */
    DEV_DEBUG,                      /* flags */
    0,                              /* dctrl */
    BOOTROM_debug,                  /* debflags */
    NULL,                           /* msize */
    NULL,                           /* help */
    NULL,                           /* attach help */
    NULL,                           /* help context */
    NULL                            /* device description */
};

/* global variables */

/* MP-09 actually has 4 x 2KB EPROM sockets at $E000, $E800, $F000, $F800 */
/* This will be emulated as a single BOOTROM having a high address of $FFFF and variable start address depending on size */
/* Available sizes are None=0, 512B=2704, 1KB=2708, 2KB=2716, 4KB=2732, 8KB=2764 */

/* Pre-allocate 8KB array of bytes to accomodate largest BOOTROM */
uint8 BOOTROM_memory[MAX_BOOTROM_SIZE];

/* BOOTROM_examine routine */
t_stat BOOTROM_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches)
{
    sim_debug(DEBUG_flow, &BOOTROM_dev, "BOOTROM_examine: addr=%08x\n", addr);

    if (addr >= uptr->capac || addr >= MAX_BOOTROM_SIZE) {
        return SCPE_NXM;
    }
    if (eval_array != NULL) {
       *eval_array = BOOTROM_memory[addr];
    }
    return SCPE_OK;

} /* BOOTROM_examine() */

/* BOOTROM_attach - attach file to EPROM unit */
t_stat BOOTROM_attach(UNIT *uptr, CONST char *cptr)
{
    t_stat r;
    t_addr image_size;
    int i,j;
    FILE *fp;
    uint8 byte_val;
    size_t items_read;

    if (BOOTROM_unit.filebuf == NULL) { /* no buffer allocated */
        BOOTROM_unit.filebuf = BOOTROM_memory;
    }
    if ((BOOTROM_unit.flags & UNIT_MSIZE) == 0) { /* if none selected */
        BOOTROM_unit.capac = 0;         /* set EPROM size to 0 */
        return SCPE_OK;
    }

    sim_debug(DEBUG_flow, &BOOTROM_dev, "BOOTROM_attach: cptr=%s\n", cptr);
    if ((r = attach_unit(uptr, cptr)) != SCPE_OK) {
        sim_debug(DEBUG_flow, &BOOTROM_dev, "BOOTROM_attach: Error %d\n", r);
        return r;
    }

    image_size = (t_addr) sim_fsize_ex(uptr->fileref);
    if (image_size <= 0) {
        sim_printf("BOOTROM_attach: File error\n");
        detach_unit(uptr);
        return SCPE_IOERR;
    } else {
        if (image_size > MAX_BOOTROM_SIZE) {
            sim_printf("BOOTROM_attach: Error. File size exceeds ROM capacity\n");
            detach_unit(uptr);
            return SCPE_ARG;
        }
    }

    /* open EPROM file */
    fp = fopen(BOOTROM_unit.filename, "rb");
    if (fp == NULL) {
        printf("Bootrom: Unable to open ROM file %s\n",BOOTROM_unit.filename);
        printf("Bootrom: No ROM image loaded!!!\n");
        return SCPE_OK;
    }

    /* load EPROM file */
    j = 0;
    items_read = sim_fread(&byte_val, (size_t)1, (size_t)1, fp);
    while (items_read != 0) {
        BOOTROM_memory[j++] = byte_val;
        items_read = sim_fread(&byte_val, (size_t)1, (size_t)1,fp);
        if (j > BOOTROM_unit.capac) {
            printf("Bootrom: Image is too large - Load truncated!!!\n");
            j--;
            break;
        }
    }
    fclose(fp);
    printf("Bootrom: %d bytes of ROM image %s loaded\n", j, BOOTROM_unit.filename);

    sim_debug(DEBUG_flow, &BOOTROM_dev, "BOOTROM_attach: Done\n");
    return SCPE_OK;

} /* BOOTROM_attach() */

/* BOOTROM_config = None, 2704, 2708, 2716, 2732 or 2764 */
t_stat BOOTROM_config (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{

    sim_debug(DEBUG_flow, &BOOTROM_dev, "BOOTROM_config: val=%d\n", val);
    if ((val < UNIT_NONE) || (val > UNIT_2764)) { /* valid param? */
        sim_debug(DEBUG_flow, &BOOTROM_dev, "BOOTROM_config: Parameter error\n");
        return SCPE_ARG;
    }
    if (val == UNIT_NONE) {
        BOOTROM_unit.capac = 0;         /* set EPROM size */
    } else {
        BOOTROM_unit.capac = 0x100 << (val >> UNIT_V_MSIZE); /* set EPROM size */
    }

    if (!BOOTROM_unit.filebuf) {         /* point buffer to static array */
        BOOTROM_unit.filebuf = BOOTROM_memory;
    }

    sim_debug(DEBUG_flow, &BOOTROM_dev, "BOOTROM_config: BOOTROM_unit.capac=%d\n", BOOTROM_unit.capac);
    sim_debug(DEBUG_flow, &BOOTROM_dev, "BOOTROM_config: Done\n");

    return SCPE_OK;
} /* BOOTROM config */

/* BOOTROM reset */
t_stat BOOTROM_reset (DEVICE *dptr)
{
    t_addr i;

    sim_debug(DEBUG_flow, &BOOTROM_dev, "BOOTROM_reset: \n");

    /* allocate filebuf */
    if (BOOTROM_unit.filebuf == NULL) { /* no buffer allocated */
        //BOOTROM_unit.filebuf = calloc(1, BOOTROM_unit.capac); /* allocate EPROM buffer */
        BOOTROM_unit.filebuf = BOOTROM_memory;
        if (BOOTROM_unit.filebuf == NULL) {
            return SCPE_MEM;
        }
    }
    return SCPE_OK;

} /* BOOTROM_reset() */

/*  get a byte from memory - from specified memory address */
int32 BOOTROM_get_mbyte(int32 address)
{
    int32 val;
    uint8 *pa;

    if (BOOTROM_unit.filebuf == NULL) {
        sim_debug(DEBUG_read, &BOOTROM_dev, "BOOTROM_get_mbyte: EPROM not configured\n");
        return DEFAULT_NO_ROM_BYTE_VALUE;
    }
    sim_debug(DEBUG_read, &BOOTROM_dev, "BOOTROM_get_mbyte: address=%04X\n", address);
    if ((t_addr)(0xFFFF - address) > BOOTROM_unit.capac) {
        sim_debug(DEBUG_read, &BOOTROM_dev, "BOOTROM_get_mbyte: EPROM reference beyond ROM size\n");
        return DEFAULT_NO_ROM_BYTE_VALUE;
    }

    pa = BOOTROM_unit.filebuf;
    /* the following code is needed to calculate offsets so address $FFFF references the last byte of the ROM */
    val = DEFAULT_NO_ROM_BYTE_VALUE;
    switch (BOOTROM_unit.capac) {
        /* 2764 - $E000-$FFFF */
        case 0x2000:
            val = pa[address - 0xE000];
            break;
        /* 2732 - $F000-$FFFF */
        case 0x1000:
            if (address >=0xF000) {
                val = pa[address - 0xF000];
            }
            break;
        /* 2716 - $F800-$FFFF */
        case 0x0800:
            if (address >= 0xF800) {
                val = pa[address - 0xF800];
            }
            break;
        /* 2708 - $FC00-$FFFF */
        case 0x0400:
            if (address >= 0xFC00) {
                val = pa[address - 0xFC00];
            }
            break;
        /* 2704 - $FE00-$FFFF*/
        case 0x0200:
            if (address >= 0xFE00) {
                val = pa[address - 0xFE00];
            }
            break;
        default:
            break;
    }
    val &= 0xFF;
    sim_debug(DEBUG_read, &BOOTROM_dev, "BOOTROM_get_mbyte: Normal val=%02X\n", val);
    return val;
} /* BOOTROM_get_mbyte() */

/* end of bootrom.c */
