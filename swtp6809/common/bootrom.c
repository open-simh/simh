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

    MODIFICATIONS:

        23 Apr 15 -- Modified to use simh_debug
        19 Feb 24 -- Richard Lukes - Modified for swtp6809 emulator

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

#define DONT_USE_INTERNAL_ROM

#if !defined(DONT_USE_INTERNAL_ROM)
#include "swtp_sbuge_bin.h"
#endif /* DONT_USE_INTERNAL_ROM */

#define UNIT_V_MSIZE    (UNIT_V_UF)     /* ROM Size */
#define UNIT_MSIZE      (0x7 << UNIT_V_MSIZE)
#define UNIT_NONE       (0 << UNIT_V_MSIZE) /* No EPROM */
#define UNIT_2704       (1 << UNIT_V_MSIZE) /* 2704 mode */
#define UNIT_2708       (2 << UNIT_V_MSIZE) /* 2708 mode */
#define UNIT_2716       (3 << UNIT_V_MSIZE) /* 2716 mode */
#define UNIT_2732       (4 << UNIT_V_MSIZE) /* 2732 mode */
#define UNIT_2764       (5 << UNIT_V_MSIZE) /* 2764 mode */

#define BOOTROM_2K 0x0800
#define BOOTROM_4K 0x1000
#define BOOTROM_8K 0x2000

/* function prototypes */

t_stat BOOTROM_attach (UNIT *uptr, CONST char *cptr);
t_stat BOOTROM_config (UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat BOOTROM_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches);
t_stat BOOTROM_reset (DEVICE *dptr);
//t_stat BOOTROM_svc (UNIT *uptr);

int32  BOOTROM_get_mbyte(int32 address);

/* SIMH Standard I/O Data Structures */

UNIT BOOTROM_unit = {
#if defined(DONT_USE_INTERNAL_ROM)
    UDATA (NULL, UNIT_ATTABLE+UNIT_BINK+UNIT_ROABLE+UNIT_RO, 0),
#else /* !defined(DONT_USE_INTERNAL_ROM) */
    UDATA (NULL, UNIT_ATTABLE+UNIT_BINK+UNIT_ROABLE+UNIT_RO+((BOOT_CODE_SIZE>>9)<<UNIT_V_MSIZE), BOOT_CODE_SIZE),
#endif
    KBD_POLL_WAIT };

MTAB BOOTROM_mod[] = {
    { UNIT_MSIZE, UNIT_NONE, "None", "NONE", &BOOTROM_config },
    { UNIT_MSIZE, UNIT_2704, "2704", "2704", &BOOTROM_config },
    { UNIT_MSIZE, UNIT_2708, "2708", "2708", &BOOTROM_config },
    { UNIT_MSIZE, UNIT_2716, "2716", "2716", &BOOTROM_config },
    { UNIT_MSIZE, UNIT_2732, "2732", "2732", &BOOTROM_config },
    { UNIT_MSIZE, UNIT_2764, "2764", "2764", &BOOTROM_config },
    { 0 }
};

DEBTAB BOOTROM_debug[] = {
    { "ALL", DEBUG_all },
    { "FLOW", DEBUG_flow },
    { "READ", DEBUG_read },
    { "WRITE", DEBUG_write },
    { "LEV1", DEBUG_level1 },
    { "LEV2", DEBUG_level2 },
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
    NULL                            /* lname */
};

/* global variables */

/* MP-09 actually has 4 x 2KB EPROM sockets at $E000, $E800, $F000, $F800 */
/* This will be emulated as a single BOOTROM having a high address of $FFFF and variable start address depending on size */
/* Available sizes are None=0, 512B=2704, 1KB=2708, 2KB=2716, 4KB=2732, 8KB=2764 */

/* Pre-allocate 8KB array of bytes to accomodate largest BOOTROM */
uint8 BOOTROM_memory_array[BOOTROM_8K];

t_stat BOOTROM_examine(t_value *eval_array, t_addr addr, UNIT *uptr, int32 switches)
{
    if (addr >= BOOTROM_8K) {
        return SCPE_NXM;
    }
    if (eval_array != NULL) {
        *eval_array = BOOTROM_memory_array[addr] & 0xFF;
    }
    return SCPE_OK;

} /* BOOTROM_examin() */

/* BOOTROM_attach - attach file to EPROM unit */
t_stat BOOTROM_attach (UNIT *uptr, CONST char *cptr)
{
    t_stat r;
    t_addr image_size, capac;
    int i;

//RICHARD WAS HERE
printf("calling BOOTROM_attach\n");
    sim_debug (DEBUG_flow, &BOOTROM_dev, "BOOTROM_attach: cptr=%s\n", cptr);
    if ((r = attach_unit (uptr, cptr)) != SCPE_OK) {
        sim_debug (DEBUG_flow, &BOOTROM_dev, "BOOTROM_attach: Error\n");
        return r;
    }
    image_size = (t_addr)sim_fsize_ex (BOOTROM_unit.fileref);
    if (image_size <= 0) {
        sim_printf("BOOTROM_attach: File error\n");
        return SCPE_IOERR;
    } 
    for (capac = 0x200, i=1; capac < image_size; capac <<= 1, i++);
    if (i > (UNIT_2764>>UNIT_V_MSIZE)) {
        detach_unit (uptr);
        return SCPE_ARG;
    }
    uptr->flags &= ~UNIT_MSIZE;
    uptr->flags |= (i << UNIT_V_MSIZE);
    sim_debug (DEBUG_flow, &BOOTROM_dev, "BOOTROM_attach: Done\n");
//RICHARD WAS HERE
    return (BOOTROM_reset (NULL));
    return SCPE_OK;
} /* BOOTROM_attach() */

/* BOOTROM_config = None, 2704, 2708, 2716, 2732 or 2764 */
t_stat BOOTROM_config (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
//RICHARD WAS HERE
printf("calling BOOTROM_config\n");
    sim_debug (DEBUG_flow, &BOOTROM_dev, "BOOTROM_config: val=%d\n", val);
    if ((val < UNIT_NONE) || (val > UNIT_2764)) { /* valid param? */
        sim_debug (DEBUG_flow, &BOOTROM_dev, "BOOTROM_config: Parameter error\n");
        return SCPE_ARG;
    }
    if (val == UNIT_NONE)
        BOOTROM_unit.capac = 0;         /* set EPROM size */
    else
        BOOTROM_unit.capac = 0x200 << ((val >> UNIT_V_MSIZE) - 1); /* set EPROM size */
    if (BOOTROM_unit.filebuf) {         /* free buffer */
        //free (BOOTROM_unit.filebuf);
        BOOTROM_unit.filebuf = NULL;
    }
    sim_debug (DEBUG_flow, &BOOTROM_dev, "BOOTROM_config: BOOTROM_unit.capac=%d\n",
            BOOTROM_unit.capac);
    sim_debug (DEBUG_flow, &BOOTROM_dev, "BOOTROM_config: Done\n");
    return SCPE_OK;
} /* BOOTROM config */

/* BOOTROM reset */
t_stat BOOTROM_reset (DEVICE *dptr)
{
    t_addr i,j;
    unsigned long c;
    long count;
    FILE *fp;
    uint8 *pa;

//RICHARD WAS HERE
printf("calling BOOTROM_reset\n");
    /* initialize array to $FF */
    for (i=0; i<BOOTROM_8K; i++) {
        BOOTROM_memory_array[i] = 0xFF;
    }
    sim_debug (DEBUG_flow, &BOOTROM_dev, "BOOTROM_reset: \n");

    if ((BOOTROM_unit.flags & UNIT_MSIZE) == 0) { /* if none selected */
        BOOTROM_unit.capac = 0;         /* set EPROM size to 0 */
        sim_debug (DEBUG_flow, &BOOTROM_dev, "BOOTROM_reset: Done1\n");
        return SCPE_OK;
    }                               /* if attached */

    if (BOOTROM_unit.filebuf == NULL) { /* no buffer allocated */
        //BOOTROM_unit.filebuf = calloc(1, BOOTROM_unit.capac); /* allocate EPROM buffer */
        BOOTROM_unit.filebuf = BOOTROM_memory_array; /* allocate EPROM buffer */
        if (BOOTROM_unit.filebuf == NULL) {
            sim_debug (DEBUG_flow, &BOOTROM_dev, "BOOTROM_reset: Malloc error\n");
            return SCPE_MEM;
        }
    }

//RICHARD WAS HERE - need to understand INTERNAL_ROM
#if !defined(DONT_USE_INTERNAL_ROM)
    if (!BOOTROM_unit.filename) {
        if (BOOTROM_unit.capac < BOOT_CODE_SIZE) {
            return SCPE_ARG;
        }
        memcpy (BOOTROM_unit.filebuf, BOOT_CODE_ARRAY, BOOT_CODE_SIZE);
        return SCPE_OK;
    }
#endif

//RICHARD WAS HERE - I think this needs to move to attach
    fp = fopen(BOOTROM_unit.filename, "rb"); /* open EPROM file */
    if (fp == NULL) {
        printf("\tUnable to open ROM file %s\n",BOOTROM_unit.filename);
        printf("\tNo ROM image loaded!!!\n");
        return SCPE_OK;
    }

    j=0;
    pa = BOOTROM_unit.filebuf;
    count = fread( &c, 1, 1, fp);
    while (count != 0) {
	pa[j++] = c & 0xFF;
        count = fread( &c, 1, 1, fp);

        /* warn if ROM image is larger than the specified ROM size */
        if (j > BOOTROM_unit.capac) {
            printf("\tImage is too large - Load truncated!!!\n");
            j--;
            break;
        }
    }
    fclose(fp);

    /* warn if loaded ROM image is smaller than the specified ROM size */
    if (j < BOOTROM_unit.capac) {
        printf("\tImage is smaller than specified ROM size!!!\n");
    }

    printf("\t%d bytes of ROM image %s loaded\n", j, BOOTROM_unit.filename);
    sim_debug (DEBUG_flow, &BOOTROM_dev, "BOOTROM_reset: Done2\n");
    return SCPE_OK;
} /* BOOTROM_reset() */

/*  get a byte from memory - from specified memory address */
int32 BOOTROM_get_mbyte(int32 address)
{
    int32 val;
    uint8 *pa;

    if (BOOTROM_unit.filebuf == NULL) {
        sim_debug (DEBUG_read, &BOOTROM_dev, "BOOTROM_get_mbyte: EPROM not configured\n");
        return 0xFF;
    }
    sim_debug (DEBUG_read, &BOOTROM_dev, "BOOTROM_get_mbyte: address=%04X\n", address);
    if ((t_addr)(0xFFFF - address) > BOOTROM_unit.capac) {
        sim_debug (DEBUG_read, &BOOTROM_dev, "BOOTROM_get_mbyte: EPROM reference beyond ROM size\n");
        return 0xFF;
    }
    pa = BOOTROM_unit.filebuf;
    /* the following code is needed to calculate offsets so address $FFFF references the last byte of the ROM */
    val = 0xFF;
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
    sim_debug (DEBUG_read, &BOOTROM_dev, "BOOTROM_get_mbyte: Normal val=%02X\n", val);
    return val;
} /* BOOTROM_get_mbyte() */

/* end of bootrom.c */
