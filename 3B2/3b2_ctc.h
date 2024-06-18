/* 3b2_ctc.h: CM195H 23MB Cartridge Tape Controller CIO Card

   Copyright (c) 2018-2022, Seth J. Morabito

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.

   Except as contained in this notice, the name of the author shall
   not be used in advertising or otherwise to promote the sale, use or
   other dealings in this Software without prior written authorization
   from the author.
*/

/*
 * CTC is an intelligent feature card for the 3B2 that supports a
 * Cipher "FloppyTape(tm)" 525 drive that can read and write 23MB
 * DC600A cartridges.
 *
 * The CTC card is based on the Common I/O (CIO) platform.
 *
 * Notes:
 * ------
 *
 * The Cipher FloppyTape is an odd beast. Although it's a tape drive,
 * it is controlled by a floppy controller. It is divided into virtual
 * sectors that can be addressed by Cylinder / Track / Sector.
 * Stepping and head select pulses dictate where on the tape to read
 * from or write to. Moreover, System V maps a filesystem onto the
 * tape, and a properly formatted tape drive will have a VTOC on
 * partition 0.
 *
 */

#ifndef _3B2_CTC_H_
#define _3B2_CTC_H_

#include "3b2_defs.h"

#define CTC_ID        0x0005
#define CTC_IPL       12
#define CTC_VERSION   1

/* Request Opcodes */
#define CTC_CONFIG      30
#define CTC_CLOSE       31
#define CTC_FORMAT      32
#define CTC_OPEN        33
#define CTC_READ        34
#define CTC_WRITE       35
#define CTC_VWRITE      36

/* Completion Opcodes */
#define CTC_SUCCESS     0
#define CTC_HWERROR     32
#define CTC_RDONLY      33
#define CTC_NOTREADY    36
#define CTC_RWERROR     37
#define CTC_NOMEDIA     42

/* VTOC values */
#define VTOC_VERSION    1
#define VTOC_SECSZ      512
#define VTOC_PART       16          /* Number of "partitions" on tape */
#define VTOC_VALID      0x600DDEEE  /* Magic number for valid VTOC */

#define CTC_NUM_SD      2
#define CTC_SD_FT25     4
#define CTC_SD_FD5      1

/* Physical Device Info (pdinfo) values */
#define PD_VALID        0xCA5E600D  /* Magic number for valid PDINFO */
#define PD_DRIVEID      5
#define PD_VERSION      0
#define PD_CYLS         6
#define PD_TRACKS       245
#define PD_SECTORS      31
#define PD_BYTES        512
#define PD_LOGICALST    29

#define CTC_CAPACITY    (PD_CYLS * PD_TRACKS * PD_SECTORS) /* In blocks */

struct partition {
    uint16_t id;       /* Partition ID     */
    uint16_t flag;     /* Permission Flags */
    uint32_t sstart;   /* Starting Sector  */
    uint32_t ssize;    /* Size in Sectors  */
};

struct vtoc {
    uint32_t bootinfo[3];                /* n/a */
    uint32_t sanity;                     /* magic number */
    uint32_t version;                    /* layout version */
    uint8_t  volume[8];                  /* volume name */
    uint16_t sectorsz;                   /* sector size in bytes */
    uint16_t nparts;                     /* number of partitions */
    uint32_t reserved[10];               /* free space */
    struct partition part[VTOC_PART];  /* partition headers */
    uint32_t timestamp[VTOC_PART];       /* partition timestamp */
};

struct pdinfo {
    uint32_t driveid;     /* identifies the device type */
    uint32_t sanity;      /* verifies device sanity */
    uint32_t version;     /* version number */
    uint8_t  serial[12];  /* serial number of the device */
    uint32_t cyls;        /* number of cylinders per drive */
    uint32_t tracks;      /* number tracks per cylinder */
    uint32_t sectors;     /* number sectors per track */
    uint32_t bytes;       /* number of bytes per sector */
    uint32_t logicalst;   /* sector address of logical sector 0 */
    uint32_t errlogst;    /* sector address of error log area */
    uint32_t errlogsz;    /* size in bytes of error log area */
    uint32_t mfgst;       /* sector address of mfg. defect info */
    uint32_t mfgsz;       /* size in bytes of mfg. defect info */
    uint32_t defectst;    /* sector address of the defect map */
    uint32_t defectsz;    /* size in bytes of defect map */
    uint32_t relno;       /* number of relocation areas */
    uint32_t relst;       /* sector address of relocation area */
    uint32_t relsz;       /* size in sectors of relocation area */
    uint32_t relnext;     /* address of next avail reloc sector */
};

typedef struct {
    uint32_t time;        /* Time used during a tape session (in 25ms chunks) */
    uint32_t bytnum;      /* Byte number, for streaming mode */
} CTC_STATE;

t_stat ctc_reset(DEVICE *dptr);
t_stat ctc_svc(UNIT *uptr);
t_stat ctc_attach(UNIT *uptr, CONST char *cptr);
t_stat ctc_detach(UNIT *uptr);
void ctc_sysgen(uint8_t slot);
void ctc_express(uint8_t slot);
void ctc_full(uint8_t slot);

#endif /* _3B2_CTC_H_ */
