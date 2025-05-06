/*  dc4.c: SWTP DC-4 FDC Simulator

    Copyright (c) 2005-2012, William A. Beech

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
        WILLIAM A BEECH BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
        IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
        CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

        Except as contained in this notice, the name of William A. Beech shall not
        be used in advertising or otherwise to promote the sale, use or other dealings
        in this Software without prior written authorization from William A. Beech.

    MODIFICATIONS:

        23 Apr 15 -- Modified to use simh_debug
        04 Apr 24 -- Richard Lukes - Modified to work with swtp6809 emulator

    NOTES:

        The DC-4 is a 5-inch floppy controller which can control up
        to 4 daisy-chained 5-inch floppy drives.  The controller is based on
        the Western Digital 1797 Floppy Disk Controller (FDC) chip. This
        file only emulates the minimum DC-4 functionality to interface with
        the virtual disk file.

        The floppy controller is interfaced to the CPU by use of 5 memory addreses.
        These are SS-30 slot numbers 5 and 6 (0xE014-0xE01B) on a SWTPC 6809 computer.
        These are SS-30 slot numbers 5 and 6 (0x8014-0x801B) on a SWTPC 6800 computer.

        Address     Mode    Function
        -------     ----    --------

        0xE014      Read    Returns FDC interrupt status
        0xE014      Write   Selects the drive/head/motor control
        0xE018      Read    Returns status of FDC
        0xE018      Write   FDC command register
        0xE019      Read    Returns FDC track register
        0xE019      Write   Set FDC track register
        0xE01A      Read    Returns FDC sector register
        0xE01A      Write   Set FDC sector register
        0xE01B      Read    Read data
        0xE01B      Write   Write data

        Drive Select Read (0xE014):

        +---+---+---+---+---+---+----+----+
        | I | D | X | X | X | X | d1 | d0 |
        +---+---+---+---+---+---+----+----+

        I = Set indicates an interrupt request from the FDC pending.
        D = DRQ pending - same as bit 1 of FDC status register.
        d1,d0 = current drive selected

        Drive Select Write (0xE014):

        +---+---+---+---+---+---+---+---+
        | M | S | X | X | X | X | Device|
        +---+---+---+---+---+---+---+---+

        M = If this bit is 1, the one-shot is triggered/retriggered to 
            start/keep the motors on.
        S = Side select. If set, side one is selected otherwise side zero
            is selected.
        X = not used
        Device = value 0 thru 3, selects drive 0-3 to be controlled.

        Drive Status Read (0xE018):

        +---+---+---+---+---+---+---+---+
        | R | P | H | S | C | L | D | B |
        +---+---+---+---+---+---+---+---+

        B - When 1, the controller is busy.
        D - When 1, index mark detected (type I) or data request - read data 
            ready/write data empty (type II or III).
        H - When 1, track 0 (type I) or lost data (type II or III).
        C - When 1, crc error detected.
        S - When 1, seek (type I) or RNF (type II or III) error.
        H - When 1, head is currently loaded (type I) or record type/
            write fault (type II or III).
        P - When 1, indicates that diskette is write-protected.
        R - When 1, drive is not ready.

        Drive Control Write (0xE018) for type I commands:

        +---+---+---+---+---+---+---+---+
        | 0 | S2| S1| S0| H | V | R1| R0|
        +---+---+---+---+---+---+---+---+

        R0/R1 - Selects the step rate.
        V - When 1, verify on destination track.
        H - When 1, loads head to drive surface.
        S0/S1/S2 = 000 - home.
           001 - seek track in data register.
           010 - step without updating track register.
           011 - step and update track register.
           100 - step in without updating track register.
           101 - step in and update track register.
           110 - step out without updating track register.
           111 - step out and update track register.

        Drive Control Write (0xE018) for type II commands:

        +---+---+---+---+---+---+---+---+
        | 1 | 0 | T | M | S | E | B | A |
        +---+---+---+---+---+---+---+---+

        A - Zero for read, 1 on write deleted data mark else data mark.
        B - When 1, shifts sector length field definitions one place.
        E - When, delay operation 15 ms, 0 no delay.
        S - When 1, select side 1, 0 select side 0.
        M - When 1, multiple records, 0 for single record.
        T - When 1, write command, 0 for read.

        Drive Control Write (0xE018) for type III commands:

        +---+---+---+---+---+---+---+---+
        | 1 | 1 | T0| T1| 0 | E | 0 | 0 |
        +---+---+---+---+---+---+---+---+

        E - When, delay operation 15 ms, 0 no delay.
        T0/T1 - 00 - read address command.
                10 - read track command.
                11 - write track command.

        Tracks are numbered from 0 up to one minus the last track in the 1797!

        Track Register Read (0xE019):

        +---+---+---+---+---+---+---+---+
        |          Track Number         |
        +---+---+---+---+---+---+---+---+

        Reads the current 8-bit value from the track position.

        Track Register Write (0xE019):

        +---+---+---+---+---+---+---+---+
        |          Track Number         |
        +---+---+---+---+---+---+---+---+

        Writes the 8-bit value to the track register.

        Sectors are numbers from 1 up to the last sector in the 1797!

        Sector Register Read (0xE01A):

        +---+---+---+---+---+---+---+---+
        |         Sector Number         |
        +---+---+---+---+---+---+---+---+

        Reads the current 8-bit value from the sector position.

        Sector Register Write (0xE01A):

        +---+---+---+---+---+---+---+---+
        |         Sector Number         |
        +---+---+---+---+---+---+---+---+

        Writes the 8-bit value to the sector register.

        Data Register Read (0xE01B):

        +---+---+---+---+---+---+---+---+
        |             Data              |
        +---+---+---+---+---+---+---+---+

        Reads the current 8-bit value from the data register.

        Data Register Write (0xE01B):

        +---+---+---+---+---+---+---+---+
        |             Data              |
        +---+---+---+---+---+---+---+---+

        Writes the 8-bit value to the data register.

        A FLEX disk is defined as follows:

        Track   Sector  Use
        0       1       Boot sector
        0       2       Boot sector (cont)
        0       3       Unused (doesn't exist)
        0       4       System Identity Record (explained below)
        0       5       Unused
        0       6-last  Directory - 10 entries/sector (explained below)
        1       1       First available data sector
        last-1  last    Last available data sector

        FLEX System Identity Record

        Byte    Use
        0x00    Two bytes of zeroes (Clears forward link)
        0x10    Volume name in ASCII(11 bytes)
        0x1B    Volume number in binary (2 bytes)
        0x1D    Address of first free data sector (Track-Sector) (2 bytes)
        0x1F    Address of last free data sector (Track-Sector) (2 bytes)
        0x21    Total number of data sectors in binary (2 bytes)
        0x23    Current date (Month-Day-Year) in binary
        0x26    Highest track number on disk in binary (byte)
        0x27    Highest sector number on a track in binary (byte)

        The following unit registers are used by this controller emulation:

        dsk_unit[cur_drv].pos       unit current sector byte index into file
        dsk_unit[cur_drv].filebuf   unit current sector buffer
        dsk_unit[cur_drv].fileref   unit current attached file reference
        dsk_unit[cur_drv].capac     capacity
        dsk_unit[cur_drv].u3        unit current flags
        dsk_unit[cur_drv].u4        unit current track
        dsk_unit[cur_drv].u5        unit current sector
        dsk_unit[cur_drv].u6        sectors per track
        dsk_unit[cur_drv].up7       points to "Flex string" or NULL
        dsk_unit[cur_drv].wait      cylinder per disk
*/

#include <stdio.h>
#include "swtp_defs.h"

/* Unit settings*/

#define UNIT_V_DC4_READONLY    (UNIT_V_UF + 0)
#define UNIT_DC4_READONLY      (0x1 << UNIT_V_DC4_READONLY)

#define UNIT_V_DC4_SPT         (UNIT_V_UF + 1)
#define UNIT_DC4_SPT           (0xF << UNIT_V_DC4_SPT) 
#define UNIT_DC4_SPT_UNKNOWN   (0x1 << UNIT_V_DC4_SPT)
#define UNIT_DC4_SPT_FLEX_SIR  (0x2 << UNIT_V_DC4_SPT)
#define UNIT_DC4_SPT_10SPT     (0x3 << UNIT_V_DC4_SPT)
#define UNIT_DC4_SPT_20SPT     (0x4 << UNIT_V_DC4_SPT)
#define UNIT_DC4_SPT_30SPT     (0x5 << UNIT_V_DC4_SPT)
#define UNIT_DC4_SPT_36SPT     (0x6 << UNIT_V_DC4_SPT)
#define UNIT_DC4_SPT_72SPT     (0x8 << UNIT_V_DC4_SPT)
#define UNIT_DC4_SPT_255SPT    (0x9 << UNIT_V_DC4_SPT)

/* maximum of 4 disks, sector size is fixed at SECTOR_SIZE bytes */

#define NUM_DISK        4               /* standard 1797 maximum */
#define SECTOR_SIZE     256             /* standard FLEX sector is 256 bytes */

/* Flex OS SIR offsets */
#define MAXCYL          0x26            /* last cylinder # */
#define MAXSEC          0x27            /* last sector # */

/* 1797 status bits */
#define BUSY            0x01
#define DRQ             0x02
#define WRPROT          0x40
#define NOTRDY          0x80

/* drive register status bit */
#define FDCDRV_INTRQ    0x80
#define FDCDRV_DRQ      0x40
#define FDCDRV_D1       0x02
#define FDCDRV_D0       0x01

/* function prototypes */

t_stat dsk_reset(DEVICE *dptr);
t_stat dsk_config(UNIT *uptr, int32 val, CONST char *cptr, void *desc);
t_stat dsk_detach(UNIT *uptr);
t_stat dsk_attach(UNIT *uptr, CONST char *cptr);

/* SS-50 I/O address space functions */

int32 fdcdrv(int32 io, int32 data);
int32 fdccmd(int32 io, int32 data);
int32 fdctrk(int32 io, int32 data);
int32 fdcsec(int32 io, int32 data);
int32 fdcdata(int32 io, int32 data);

/* Local Variables */

int32 fdc_data_byte;                    /* fdc data register */

// this counter is used to emulate write sector VERIFY
// a write sector VERIFY, re-reads the sector, but
// only updates the CRC_ERROR and RECORD_NOT_FOUND flags
// (which don't exist in the emulation)
int32 read_w_drq_busy_counter = 0;      /* if DRQ=1,BUSY=1 then increment counter else zero it. Also zero whenever reading from data register on sector read command */

int32 cur_dsk;                          /* Currently selected drive */
int32 prev_dsk;                         /* previously selected drive */

char flex_flag_str[5] = { 'F', 'L', 'E', 'X', 0 };

/* Floppy Disk Controller data structures

       dsk_dev        Mother Board device descriptor
       dsk_unit       Mother Board unit descriptor
       dsk_reg        Mother Board register list
       dsk_mod        Mother Board modifiers list
*/

UNIT dsk_unit[NUM_DISK] = {
        //{ UDATA (NULL, UNIT_FIX+UNIT_ATTABLE+UNIT_DISABLE, 0)  },
        //{ UDATA (NULL, UNIT_FIX+UNIT_ATTABLE+UNIT_DISABLE, 0)  },
        //{ UDATA (NULL, UNIT_FIX+UNIT_ATTABLE+UNIT_DISABLE, 0)  },
        //{ UDATA (NULL, UNIT_FIX+UNIT_ATTABLE+UNIT_DISABLE, 0)  }
        { UDATA (NULL, UNIT_FIX + UNIT_ATTABLE, 0)  },
        { UDATA (NULL, UNIT_FIX + UNIT_ATTABLE, 0)  },
        { UDATA (NULL, UNIT_FIX + UNIT_ATTABLE, 0)  },
        { UDATA (NULL, UNIT_FIX + UNIT_ATTABLE, 0)  }
};

REG dsk_reg[] = {
        { HRDATA (DISK, cur_dsk, 4) },
        { NULL }
};

MTAB dsk_mod[] = {
        { UNIT_DC4_READONLY, UNIT_DC4_READONLY, "RO", "RO", &dsk_config },
        { UNIT_DC4_READONLY, 0, "RW", "RW", &dsk_config },
        { UNIT_DC4_SPT, UNIT_DC4_SPT_UNKNOWN, "Unknown sectors per track", "UNKNOWN", &dsk_config },
        { UNIT_DC4_SPT, UNIT_DC4_SPT_FLEX_SIR, "Read Flex SIR for disk info", "FLEX", &dsk_config },
        { UNIT_DC4_SPT, UNIT_DC4_SPT_10SPT, "10 sectors per track", "10SPT", &dsk_config },
        { UNIT_DC4_SPT, UNIT_DC4_SPT_20SPT, "20 sectors per track", "20SPT", &dsk_config },
        { UNIT_DC4_SPT, UNIT_DC4_SPT_30SPT, "30 sectors per track", "30SPT", &dsk_config },
        { UNIT_DC4_SPT, UNIT_DC4_SPT_36SPT, "36 sectors per track", "36SPT", &dsk_config },
        { UNIT_DC4_SPT, UNIT_DC4_SPT_72SPT, "72 sectors per track", "72SPT", &dsk_config },
        { UNIT_DC4_SPT, UNIT_DC4_SPT_255SPT, "255 sectors per track", "255SPT", &dsk_config },
        { 0 }
};

DEBTAB dsk_debug[] = {
    { "ALL", DEBUG_all, "Debug all" },
    { "FLOW", DEBUG_flow, "Debug flow of control" },
    { "READ", DEBUG_read, "Debug device reads" },
    { "WRITE", DEBUG_write, "Debug device writes" },
    { "LEV1", DEBUG_level1, "Debug level 1" },
    { "LEV2", DEBUG_level2, "Debug level 2" },
    { NULL }
};

DEVICE dsk_dev = {
    "DC-4",                             //name
    dsk_unit,                           //units
    dsk_reg,                            //registers
    dsk_mod,                            //modifiers
    NUM_DISK,                           //numunits
    16,                                 //aradix
    8,                                  //awidth
    1,                                  //aincr
    16,                                 //dradix
    8,                                  //dwidth
    NULL,                               //examine
    NULL,                               //deposit
    &dsk_reset,                         //reset
    NULL,                               //boot
    &dsk_attach,                        //attach
    &dsk_detach,                        //detach
    NULL,                               //ctxt
    DEV_DEBUG,                          //flags
    0,                                  //dctrl
    dsk_debug,                          /* debug flags defined in DEBTAB */
    NULL,                               //msize
    NULL,                               //help
    NULL,                               //attach help
    NULL,                               //help context
    NULL                                //device description
};

/* Reset routine */

t_stat dsk_reset(DEVICE *dptr)
{
    int i;

    sim_printf("dsk_reset: call to reset routine\n");

    cur_dsk = 0;                        /* force initial SIR read in FLEX mode, use a drive # that can't be selected */
    prev_dsk = 5;

    for (i=0; i<NUM_DISK; i++) {
        /* no default for disk geometry, this is calculated when a file is attached */

        dptr->units[i].u3 = NOTRDY;        /* clear current flags */
        dptr->units[i].u4 = 0;             /* clear current cylinder # */
        dptr->units[i].u5 = 0;             /* clear current sector # */
        dptr->units[i].pos = 0;            /* clear current byte ptr */

        if (dptr->units[i].filebuf == NULL) {
            dptr->units[i].filebuf = malloc(SECTOR_SIZE); /* allocate buffer */
            if (dptr->units[i].filebuf == NULL) {
                sim_printf("dc-4_reset: Malloc error\n");
                return SCPE_MEM;
            } else {
                sim_debug(DEBUG_flow, dptr, "dsk_reset: allocated file buffer\n");
            }
        }
    }

    // initialize data register
    fdc_data_byte = 0;

    return SCPE_OK;

} /* dsk_reset() */

/* attach */
t_stat dsk_attach(UNIT *uptr, CONST char *cptr)
{
    off_t fsize = 0;   /* size of attached file */
    int32 temp_cpd;    /* cylinders per disk */
    int32 temp_spt;    /* sectors per track */
    t_stat r;
    int32 err;
    int32 pos;

    r = attach_unit(uptr, cptr);
    sim_debug(DEBUG_flow, &dsk_dev, "dsk_attach: attach returns %d\n", r);
    if (r == SCPE_OK) {
        /* determine the size of the attached file and populate the UNIT capac value (capacity) */
        err = sim_fseek(uptr->fileref, 0, SEEK_END); /* seek to offset */
        if (err == -1) {
            sim_debug(DEBUG_flow, &dsk_dev, "dsk_attach: fseek returns %d\n", err);
            return (SCPE_IOERR);
        }
        fsize = sim_ftell(uptr->fileref);
        uptr->capac = fsize;

        if (uptr->up7 == flex_flag_str) {
                sim_printf("dsk_attach: Reading disk geometry from SIR of Flex disk named %s\n", cptr);
                sim_debug(DEBUG_flow, &dsk_dev, "dsk_attach: Reading disk geometry from SIR of Flex disk name %d\n", cptr);

	        // whenever a new file is attached - re-read the SIR
                pos = 0x200;       /* Location of Flex System Information Record (SIR) */

                sim_debug(DEBUG_read, &dsk_dev, "dsk_attach: Read pos = %ld ($%04X)\n", pos, (unsigned int) pos);
                err = sim_fseek(uptr->fileref, pos, SEEK_SET); /* seek to offset */
                if (err) {
                    sim_debug(DEBUG_read, &dsk_dev, "dsk_attach: Seek error read in SIR\n");
                    sim_printf("dsk_attach: Seek error read in SIR\n");
                    return SCPE_IOERR;
                } 
                err = sim_fread(uptr->filebuf, SECTOR_SIZE, 1, uptr->fileref); /* read in buffer */
                if (err != 1) {
                    sim_debug(DEBUG_read, &dsk_dev, "dsk_attach: Seek error read in SIR\n");
                    sim_printf("dsk_attach: File error read in SIR\n");
                    return SCPE_IOERR;
                }

                /* retrieve parameters from SIR */
                temp_spt = *((uint8 *)(uptr->filebuf) + MAXSEC) & 0xFF;
                temp_cpd = *((uint8 *)(uptr->filebuf) + MAXCYL) & 0xFF;

                /* zero based track numbering */
                temp_cpd++;

                sim_printf("dsk_attach: SIR was read. SPT=%d. CPD=%d. Capacity=%d\n", temp_spt, temp_cpd, fsize);
                sim_debug(DEBUG_flow, &dsk_dev, "dsk_attach: SIR was read. SPT=%d. CPD=%d. Capacity=%d\n", temp_spt, temp_cpd, fsize);

                /* confirm that geometry aligns with size */
                if (fsize == (temp_cpd * temp_spt * SECTOR_SIZE)) {
                    uptr->u6 = temp_spt;
                    uptr->wait = temp_cpd;
                    sim_printf("dsk_attach: sectors per track is %d, cylinders per disk is %d\n", temp_spt, temp_cpd);
                    sim_debug(DEBUG_flow, &dsk_dev, "dsk_attach: WARNING: sectors per track is %d, cylinders per disk is %d\n", temp_spt, temp_cpd);
                } else {
                    sim_printf("dsk_attach: Disk geometry does not align with file size!\n");
                    sim_debug(DEBUG_flow, &dsk_dev, "dsk_attach: WARNING: disk geometry does not align with file size!\n");
                }
        } else {
            /* if sectors per track != 0, calculate cylinders per disk based on file capacity */
            temp_spt = uptr->u6;
            if (temp_spt != 0) {
                temp_cpd = fsize / (temp_spt * SECTOR_SIZE);
                if (fsize == (temp_cpd * temp_spt * SECTOR_SIZE)) {
                    uptr->wait = temp_cpd;
                    sim_printf("dsk_attach: cylinders per disk calculated as %d\n", temp_cpd);
                    sim_debug(DEBUG_flow, &dsk_dev, "dsk_attach: WARNING: cylinders per disk calculated as %d\n", temp_cpd);
                    /* clear any pre-existing flags */
                    uptr->u3 = 0;
                } else {
                    uptr->wait = 0;
                    sim_printf("dsk_attach: cylinders per disk could not be determined. %d\n", temp_cpd);
                    sim_debug(DEBUG_flow, &dsk_dev, "dsk_attach: WARNING: cylinders per disk could not be determined. %d\n", temp_cpd);
                }
            }
        }
    }
    return(r);

} /* dsk_attach() */

/* detach */
t_stat dsk_detach(UNIT *uptr)
{
    t_stat r;

    uptr->capac = 0;
    r = detach_unit(uptr);
    sim_debug(DEBUG_flow, &dsk_dev, "dsk_detach: detach return %d\n", r);
    return(r);

} /* dsk_detach */

/* disk config */
t_stat dsk_config(UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
    sim_debug(DEBUG_flow, &dsk_dev, "dsk_config: val=%d\n", val);

    if ((val & UNIT_DC4_READONLY) != 0) {
        uptr->u3 |= WRPROT;             /* set write protect */
    } else {
        uptr->u3 &= (~WRPROT & 0xFF);   /* unset EPROM size */
    }

    if ((val & UNIT_DC4_SPT) == UNIT_DC4_SPT_FLEX_SIR) {
        uptr->up7 = flex_flag_str;
        sim_debug(DEBUG_flow, &dsk_dev, "dsk_config: disk geometry to be determined from Flex disk\n");
    }
    if ((val & UNIT_DC4_SPT) == UNIT_DC4_SPT_10SPT) {
        uptr->u6 = 10;
        sim_debug(DEBUG_flow, &dsk_dev, "dsk_config: sectors per track set to %d\n", uptr->u6);
    }
    if ((val & UNIT_DC4_SPT) == UNIT_DC4_SPT_20SPT) {
        uptr->u6 = 20;
        sim_debug(DEBUG_flow, &dsk_dev, "dsk_config: sectors per track set to %d\n", uptr->u6);
    }
    if ((val & UNIT_DC4_SPT) == UNIT_DC4_SPT_30SPT) {
        uptr->u6 = 30;
        sim_debug(DEBUG_flow, &dsk_dev, "dsk_config: sectors per track set to %d\n", uptr->u6);
    }
    if ((val & UNIT_DC4_SPT) == UNIT_DC4_SPT_36SPT) {
        uptr->u6 = 36;
        sim_debug(DEBUG_flow, &dsk_dev, "dsk_config: sectors per track set to %d\n", uptr->u6);
    }
    if ((val & UNIT_DC4_SPT) == UNIT_DC4_SPT_72SPT) {
        uptr->u6 = 72;
        sim_debug(DEBUG_flow, &dsk_dev, "dsk_config: sectors per track set to %d\n", uptr->u6);
    }
    if ((val & UNIT_DC4_SPT) == UNIT_DC4_SPT_255SPT) {
        uptr->u6 = 255;
        sim_debug(DEBUG_flow, &dsk_dev, "dsk_config: sectors per track set to %d\n", uptr->u6);
    }
    return(SCPE_OK);
} /* dsk_config */

/*
    I/O instruction handlers, called from the MP-B3 module when a
    read or write occur to addresses 0xE004-0xE007.
*/

/*
    DC-4 drive select register routine - this register is not part of the 1797
*/

int32 fdcdrv(int32 io, int32 data)
{
    static long pos;
    static int32 err;
    int32 cpd;
    int32 spt;
    int32 temp_return;
    

    sim_debug(DEBUG_flow, &dsk_dev, "fdcdrv: io=%02X, data=%02X\n", io, data);

    if (io) {
        /* write to DC-4 drive register */
        cur_dsk = data & 0x03;          /* only 2 drive select bits */
        if (cur_dsk != prev_dsk) { /* did the disk change? */
            prev_dsk = cur_dsk;
        }
        sim_debug(DEBUG_read, &dsk_dev, "fdcdrv: Drive selected %d cur_dsk=%d\n", data & 0x03, cur_dsk);
        sim_debug(DEBUG_write, &dsk_dev, "fdcdrv: Drive set to %d\n", cur_dsk);

        if ((dsk_unit[cur_dsk].flags & UNIT_DC4_READONLY) != 0) {
            dsk_unit[cur_dsk].u3 |= WRPROT; /* RO - set 1797 WPROT */
            sim_debug(DEBUG_write, &dsk_dev, "fdcdrv: Disk is write protected\n");
        } else {
            dsk_unit[cur_dsk].u3 &= ((~WRPROT) & 0xFF); /* RW - unset 1797 WPROT */
            sim_debug(DEBUG_write, &dsk_dev, "fdcdrv: Disk is not write protected\n");
        }
        if ((dsk_unit[cur_dsk].flags & UNIT_ATT) == 0) { /* device is not attached */
            /* set NOTRDY flag */
            dsk_unit[cur_dsk].u3 |= NOTRDY;
            sim_debug(DEBUG_write, &dsk_dev, "fdcdrv: Drive is NOT READY\n");
        }
    } else {
        /* read from DC-4 drive register */
        /* least significant 2 bits are the selected drive */
        temp_return = cur_dsk & 0x03;
        if (((dsk_unit[cur_dsk].u3 & BUSY) != 0) && ((dsk_unit[cur_dsk].u3 & DRQ) == 0)) {
            /* if BUSY indicates that there is a command and DRQ is unset, set DRQ now */
            dsk_unit[cur_dsk].u3 |= DRQ;     /* enable DRQ now */
        }
        sim_debug(DEBUG_read, &dsk_dev, "fdcdrv: Drive register read as %02X\n", temp_return );
        return temp_return;
    }
    return SCPE_OK;
} /* fdcdrv */

/* WD 1797 FDC command register routine */

int32 fdccmd(int32 io, int32 data)
{
    static int32 val = 0;
    static long pos;
    static int32 err;
 
    sim_debug(DEBUG_flow, &dsk_dev, "fdccmd: io=%02X, data=%02X\n", io, data);

    /* check for NOTRDY */
    if ((dsk_unit[cur_dsk].flags & UNIT_ATT) == 0) { /* not attached */
        dsk_unit[cur_dsk].u3 |= NOTRDY; /* set not ready flag */
        sim_debug(DEBUG_flow, &dsk_dev, "fdccmd: Drive %d is not attached\n", cur_dsk);
    } else {
        /* drive is attached */
        dsk_unit[cur_dsk].u3 &= ~NOTRDY; /* clear not ready flag */
        sim_debug(DEBUG_flow, &dsk_dev, "fdccmd: Drive %d is attached\n", cur_dsk);
    }

    /* check for WRPROT */
    if ((dsk_unit[cur_dsk].flags & UNIT_DC4_READONLY) != 0) {
        dsk_unit[cur_dsk].u3 |= WRPROT;  /* RO - set write protect flag */
        sim_debug(DEBUG_flow, &dsk_dev, "fdccmd: Drive %d is write protected\n", cur_dsk);
    } else {
        dsk_unit[cur_dsk].u3 &= ~WRPROT; /* RW - unset write protect flag */
        sim_debug(DEBUG_flow, &dsk_dev, "fdccmd: Drive %d is not write protected\n", cur_dsk);
    }

    if (io) {

        /* write command to fdc */
	if ((dsk_unit[cur_dsk].u3 & BUSY) != 0) {
            /* do not write to command register if device is BUSY */
            sim_debug(DEBUG_write, &dsk_dev, "fdccmd: Cannot write to command register, device is BUSY\n", cur_dsk);
            return(SCPE_OK);
        }
	if ((dsk_unit[cur_dsk].u3 & NOTRDY) != 0) {
            /* do not write to command register if device is NOTRDY */
            sim_debug(DEBUG_write, &dsk_dev, "fdccmd: Cannot write to command register, device is NOT READY\n", cur_dsk);
            return(SCPE_OK);
        }

        switch(data) {
                                        /* restore command - type I command */

            /* ignored: head load flag, verify flag, stepping motor rate*/
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07:
            case 0x09:
            case 0x0A:
            case 0x0B:
            case 0x0C:
            case 0x0D:
            case 0x0E:
            case 0x0F:

                dsk_unit[cur_dsk].u4 = 0;   /* home the drive, track = 0 */
                dsk_unit[cur_dsk].u3 &= ~(BUSY | DRQ); /* clear flags */
                sim_debug(DEBUG_write, &dsk_dev, "fdccmd: restore of drive %d\n", cur_dsk);
                break;

                                        /* seek command - type I command */

            /* ignored: head load flag, verify flag, stepping motor rate*/
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x14:
            case 0x15:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x19:
            case 0x1A:
            case 0x1B:
            case 0x1C:
            case 0x1D:
            case 0x1E:
            case 0x1F:

                /* set track */
                dsk_unit[cur_dsk].u4 = fdc_data_byte;
                dsk_unit[cur_dsk].u3 &= ~(BUSY | DRQ); /* clear flags */
                sim_debug(DEBUG_write, &dsk_dev, "fdccmd: Seek of disk %d, track %d\n", cur_dsk, fdc_data_byte);
                break;

                                        /* read sector command - type II command (m=0, SSO=0) */
            case 0x80:
            case 0x84:
            case 0x88:
            case 0x8C:
                                        /* read sector command - type II command (m=0, SSO=1) */
            case 0x82:
            case 0x86:
            case 0x8A:
            case 0x8E:

                /* only execute command if disk is READY */
                if ((dsk_unit[cur_dsk].u3 & NOTRDY) != 0) {
                    sim_debug(DEBUG_write, &dsk_dev, "fdccmd: Read sector command, but, disk is NOT READY\n");
                } else {

                    sim_debug(DEBUG_write, &dsk_dev, "fdccmd: Read of disk %d, track %d, sector %d\n", 
                        cur_dsk, dsk_unit[cur_dsk].u4, dsk_unit[cur_dsk].u5);

                    /* sectors for track 0 are numbered 0,1,3,4, ... 10 */
                    if (dsk_unit[cur_dsk].u4 == 0) {
                        /* Track 0 Sector 0 --> pos = 0 */
                        /* Track 0 Sector 1 --> pos = 256 */
                        if (dsk_unit[cur_dsk].u5 == 0) {
                            pos = 0;
                        } else {
                            if (dsk_unit[cur_dsk].u5 == 1) {
                                pos = 0;
                            } else {
                                pos = (SECTOR_SIZE * (dsk_unit[cur_dsk].u5 - 1));
                            }
                        }
                    } else {
                        /* calculate file offset. u4 is track. u5 is sector. u6 is sectors per track. */
                        pos = (dsk_unit[cur_dsk].u6 * SECTOR_SIZE) * dsk_unit[cur_dsk].u4;
                        pos += (SECTOR_SIZE * (dsk_unit[cur_dsk].u5 - 1));
                    }
                    sim_debug(DEBUG_write, &dsk_dev, "fdccmd: Read pos = %ld ($%08X)\n",
                        pos, (unsigned int) pos);
                    err = sim_fseek(dsk_unit[cur_dsk].fileref, pos, SEEK_SET); /* seek to offset */
                    if (err) {
                        sim_printf("fdccmd: sim_fseek error = %d\n", err);
                        sim_debug(DEBUG_write, &dsk_dev, "fdccmd: sim_fseek error = %d\n", err);
                        return SCPE_IOERR;
                    } 
                    err = sim_fread(dsk_unit[cur_dsk].filebuf, SECTOR_SIZE, 1, dsk_unit[cur_dsk].fileref); /* read into sector buffer */
                    if (err != 1) {
    
                        /* display error information */
                        sim_printf("fdccmd: sim_fread error = %d\n", err);
                        sim_printf("fdccmd: errno = %d\n", errno);
                        sim_debug(DEBUG_write, &dsk_dev, "fdccmd: sim_fread error = %d\n", err);
                        return SCPE_IOERR;
                    }

                    /* set BUSY to indicate that type II command is in progress */
                    dsk_unit[cur_dsk].u3 |= BUSY; /* set BUSY */
                    dsk_unit[cur_dsk].u3 &= ~DRQ; /* unset DRQ */
                    dsk_unit[cur_dsk].pos = 0;    /* clear buffer pointer */
                }
                break;

                                        /* write sector command - type II command */
            case 0xA8:
            case 0xAC:

                /* only execute command if disk is READY */
                if ((dsk_unit[cur_dsk].u3 & NOTRDY) != 0) {
                    sim_debug (DEBUG_write, &dsk_dev, "fdccmd: Write sector command, but, disk is NOT READY\n");
                } else {

                    sim_debug (DEBUG_write, &dsk_dev, "fdccmd: Write of disk %d, track %d, sector %d\n",
                        cur_dsk, dsk_unit[cur_dsk].u4, dsk_unit[cur_dsk].u5);
                    if ((dsk_unit[cur_dsk].u3 & WRPROT) != 0) {
                        sim_printf("fdccmd: Drive %d is write-protected\n", cur_dsk);
                    } else {
                        /* calculate file offset. u4 is track. u5 is sector. u6 is sectors per track. */
                        pos = (dsk_unit[cur_dsk].u6 * SECTOR_SIZE) * dsk_unit[cur_dsk].u4;
                        pos += SECTOR_SIZE * (dsk_unit[cur_dsk].u5 - 1);
                        sim_debug(DEBUG_write, &dsk_dev, "fdccmd: Write pos = %ld ($%08X)\n", pos, (unsigned int) pos);
                        err = sim_fseek(dsk_unit[cur_dsk].fileref, pos, SEEK_SET); /* seek to offset */
                        if (err) {
                            sim_printf("fdccmd: sim_fseek error = %d\n", err);
                            sim_debug(DEBUG_write, &dsk_dev, "fdccmd: sim_fseek error = %d\n", err);
                            return SCPE_IOERR;
                        } 
                        /* set BUSY to indicate that type II command is in progress */
                        dsk_unit[cur_dsk].u3 |= BUSY; /* set BUSY */
                        dsk_unit[cur_dsk].u3 &= ~DRQ; /* unset DRQ */
                        dsk_unit[cur_dsk].pos = 0;    /* clear buffer pointer */
                    }
                }
                break;

            default:
                sim_printf("Unknown or unimplemented FDC command %02XH\n", data);
                sim_debug(DEBUG_write, &dsk_dev, "fdccmd: Unknown or unimplemented command %02X\n", cur_dsk, data);
        }
        return(SCPE_OK);

    } else {
        /* read status from fdc */

        val = dsk_unit[cur_dsk].u3;     /* set return value */

        if (((val & BUSY) != 0) && ((val & DRQ) == 0)) {
            /* if BUSY indicates that there is a command and DRQ is unset, set DRQ now */
            dsk_unit[cur_dsk].u3 = val | DRQ;     /* enable DRQ for next time */
        }

        /* handle write VERIFY - as done by Flex */
        if (((val & BUSY) !=0) && ((val & DRQ) != 0)) {
            /* Re-read the sector, with read sector command but don't read any data to update the CRC_ERROR and RECORD_NOT_FOUND flags (not emulated) */
            read_w_drq_busy_counter++;
            if (read_w_drq_busy_counter > 50) {
                /* wait for enough status reads to be confident that this a VERIFY */
                read_w_drq_busy_counter = 0; 
                /* reset BUSY and DRQ flags, we do not implement CRC_ERROR or RECORD_NOT_FOUND flags */
                dsk_unit[cur_dsk].u3 &= ~(BUSY | DRQ);
                val = dsk_unit[cur_dsk].u3;
                sim_debug (DEBUG_write, &dsk_dev, "fdccmd: detected write verify\n");
            }
        } else {
            read_w_drq_busy_counter = 0;
        }
        sim_debug (DEBUG_read, &dsk_dev, "fdccmd: Exit Drive %d status=%02X\n", cur_dsk, val);
        return val;
    }
} /* fdccmd */

/* WD 1797 FDC track register routine */

int32 fdctrk(int32 io, int32 data)
{
//sim_printf("\nfdctrk: io=%d, data=%d\n", io, data);

    sim_debug(DEBUG_flow, &dsk_dev, "fdctrk: io=%02X, data=%02X\n", io, data);

    if (io) {
        /* write to track register */
        /* do not load when device is BUSY or NOT READY */
        if (((dsk_unit[cur_dsk].u3 & BUSY) == 0) && ((dsk_unit[cur_dsk].u3 & NOTRDY) == 0)) {
// RICHARD WAS HERE
// SEEK ERROR!
            if (data >= dsk_unit[cur_dsk].wait) {
sim_printf("Seek error! cur_dsk=%d, tracks per disk is %d, requested track is %d\n", cur_dsk, dsk_unit[cur_dsk].wait, data);
            }
            dsk_unit[cur_dsk].u4 = data & 0xFF;
            sim_debug (DEBUG_write, &dsk_dev, "fdctrk: Drive %d track set to %d\n", cur_dsk, dsk_unit[cur_dsk].u4);
        }
        return(SCPE_OK);
    } else {
        /* read from to track register */
        sim_debug (DEBUG_read, &dsk_dev, "fdctrk: Drive %d track read as %d\n", cur_dsk, dsk_unit[cur_dsk].u4);
        return(dsk_unit[cur_dsk].u4);
    }
} /* fdctrk */

/* WD 1797 FDC sector register routine */

int32 fdcsec(int32 io, int32 data)
{
    sim_debug(DEBUG_flow, &dsk_dev, "fdcsec: io=%02X, data=%02X\n", io, data);

    if (io) {
        /* write to sector register */
        /* do not load when device is BUSY or NOT READY */
        if (((dsk_unit[cur_dsk].u3 & BUSY) == 0) && ((dsk_unit[cur_dsk].u3 & NOTRDY) == 0)) {
            dsk_unit[cur_dsk].u5 = data & 0xFF;
            sim_debug(DEBUG_write, &dsk_dev, "fdcsec: Drive %d sector set to %d\n", cur_dsk, dsk_unit[cur_dsk].u5);
        }
        return(SCPE_OK);
    } else {
        /* read sector register */
        sim_debug (DEBUG_read, &dsk_dev, "fdcsec: Drive %d sector read as %d\n", cur_dsk, dsk_unit[cur_dsk].u5);
        return(dsk_unit[cur_dsk].u5);
   }
} /* fdcsec */

/* WD 1797 FDC data register routine */

int32 fdcdata(int32 io, int32 data)
{
    int32 val;
    static int32 err;

    sim_debug(DEBUG_flow, &dsk_dev, "fdcdata: io=%02X, data=%02X\n", io, data);

    if (io) {
        /* write byte to fdc */
        if ((dsk_unit[cur_dsk].u3 & BUSY) == 0) {
            /* NOT BUSY - write to data register fdc_data_byte */
            sim_debug(DEBUG_write, &dsk_dev, "fdcdata: Writing %02X to data register\n", data);
            fdc_data_byte = data;
        } else {
            /* BUSY - a command is being processed */
            if (dsk_unit[cur_dsk].pos < SECTOR_SIZE) { /* copy bytes to buffer */

                /* reset the counter used for write verify */
                read_w_drq_busy_counter = 0;

                sim_debug (DEBUG_flow, &dsk_dev, "fdcdata: Writing byte pos=%d val=%02X\n", dsk_unit[cur_dsk].pos, data);
                *((uint8 *)(dsk_unit[cur_dsk].filebuf) + dsk_unit[cur_dsk].pos) = data; /* byte into buffer */
                dsk_unit[cur_dsk].pos++;    /* increment buffer pointer */

                /* if this is last byte in sector then update statuses */
                if (dsk_unit[cur_dsk].pos >= SECTOR_SIZE) { /* done? */
                    err = sim_fwrite(dsk_unit[cur_dsk].filebuf, SECTOR_SIZE, 1, dsk_unit[cur_dsk].fileref); /* write it */
                    if (err != 1) {
                        /* display error information */
                        sim_printf("fdcdata: sim_fwrite error = %d\n", err);
                        sim_printf("fdcdata: errno = %d\n", errno);
                        sim_debug(DEBUG_write, &dsk_dev, "fdcdata: sim_fwrite error = %d\n", err);
                        return SCPE_IOERR;
                    }
                    dsk_unit[cur_dsk].u3 &= ~(BUSY | DRQ); /* reset flags */
                    dsk_unit[cur_dsk].pos = 0;             /* reset counter */
                    sim_debug(DEBUG_write, &dsk_dev, "fdcdata: Sector write complete\n");
                }
            }
        }
        return(SCPE_OK);
    } else {
        /* read byte from fdc */

        if ((dsk_unit[cur_dsk].u3 & BUSY) == 0) {
            /* NOT BUSY - read from data register fdc_data_byte */
            val = fdc_data_byte;
            sim_debug(DEBUG_read, &dsk_dev, "fdcdata: Reading data register value of %02X\n", val);
        } else {
            /* BUSY - a read command is being processed */
            if (dsk_unit[cur_dsk].pos < SECTOR_SIZE) { /* copy bytes from buffer */

                /* reset the counter used for write verify */
                read_w_drq_busy_counter = 0;

                /* read a byte into the sector buffer */
                val = *((uint8 *)(dsk_unit[cur_dsk].filebuf) + dsk_unit[cur_dsk].pos) & 0xFF;
                sim_debug(DEBUG_read, &dsk_dev, "fdcdata: Reading byte pos=%d val=%02X\n", dsk_unit[cur_dsk].pos, val);
                dsk_unit[cur_dsk].pos++;        /* step counter */

                /* if this is last byte in sector then update statuses */
                if (dsk_unit[cur_dsk].pos >= SECTOR_SIZE) {  /* done? */
                    dsk_unit[cur_dsk].u3 &= ~(BUSY | DRQ);   /* clear flags */
                    dsk_unit[cur_dsk].pos = 0;               /* reset step counter */
                    sim_debug(DEBUG_read, &dsk_dev, "fdcdata: Sector read complete\n");
                }
            }
        }
        return(val);
    }
} /* fdcdata() */

/* end of dc-4.c */
