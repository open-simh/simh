/*************************************************************************
 *                                                                       *
 * Copyright (c) 2025 Patrick A. Linstruth.                              *
 * https://github.com/deltecent                                          *
 *                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining *
 * a copy of this software and associated documentation files (the       *
 * "Software"), to deal in the Software without restriction, including   *
 * without limitation the rights to use, copy, modify, merge, publish,   *
 * distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to *
 * the following conditions:                                             *
 *                                                                       *
 * The above copyright notice and this permission notice shall be        *
 * included in all copies or substantial portions of the Software.       *
 *                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-            *
 * INFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE   *
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN       *
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN     *
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE      *
 * SOFTWARE.                                                             *
 *                                                                       *
 * Except as contained in this notice, the names of The Authors shall    *
 * not be used in advertising or otherwise to promote the sale, use or   *
 * other dealings in this Software without prior written authorization   *
 * from the Authors.                                                     *
 *                                                                       *
 * Based on s100_tdd.c by Howard M. Harte.                               *
 *                                                                       *
 * Module Description:                                                   *
 *     SD Systems VersaFloppy II Controller module for SIMH.             *
 * This module is a wrapper around the WD179X FDC module.                *
 *                                                                       *
 * Reference:                                                            *
 * http://www.s100computers.com/Hardware%20Manuals/SD%20Systems/VersaFloppyII%20Manual%20(JM).pdf
 *                                                                       *
 *************************************************************************/

#include "altairz80_defs.h"
#include "wd179x.h"

/* Debug flags */
#define STATUS_MSG  (1 << 0)
#define DRIVE_MSG   (1 << 1)
#define VERBOSE_MSG (1 << 2)
#define IRQ_MSG     (1 << 3)

#define VFII_MAX_DRIVES  4

#define VFII_IO_BASE     0x63
#define VFII_IO_SIZE     0x01

typedef struct {
    PNP_INFO    pnp;    /* Plug and Play */
    uint8       creg;   /* Control Register */
} VFII_INFO;

extern WD179X_INFO_PUB *wd179x_infop;

static VFII_INFO vfii_info_data = { { 0x0000, 0, VFII_IO_BASE, VFII_IO_SIZE } };

extern t_stat set_iobase(UNIT *uptr, int32 val, CONST char *cptr, void *desc);
extern t_stat show_iobase(FILE *st, UNIT *uptr, int32 val, CONST void *desc);
extern uint32 sim_map_resource(uint32 baseaddr, uint32 size, uint32 resource_type,
                               int32 (*routine)(const int32, const int32, const int32), const char* name, uint8 unmap);
extern uint8 floorlog2(unsigned int n);

extern uint32 PCX;      /* external view of PC  */

#define VFII_CAPACITY        (77*26*128)   /* Default SSSD 8" (IBM 3740) Disk Capacity */

#define VFII_DSEL_MASK          0x0f
#define VFII_SIDE_MASK          0x10
#define VFII_SIZE_MASK          0x20
#define VFII_DDEN_MASK          0x40
#define VFII_WAIT_MASK          0x80

static t_stat vfii_reset(DEVICE *vfii_dev);

static int32 vfii_control(const int32 port, const int32 io, const int32 data);
static const char* vfii_description(DEVICE *dptr);

static UNIT vfii_unit[] = {
    { UDATA (NULL, UNIT_FIX + UNIT_ATTABLE + UNIT_DISABLE + UNIT_ROABLE, VFII_CAPACITY) },
    { UDATA (NULL, UNIT_FIX + UNIT_ATTABLE + UNIT_DISABLE + UNIT_ROABLE, VFII_CAPACITY) },
    { UDATA (NULL, UNIT_FIX + UNIT_ATTABLE + UNIT_DISABLE + UNIT_ROABLE, VFII_CAPACITY) },
    { UDATA (NULL, UNIT_FIX + UNIT_ATTABLE + UNIT_DISABLE + UNIT_ROABLE, VFII_CAPACITY) }
};

static REG vfii_reg[] = {
    { NULL }
};

#define VFII_NAME    "SD Systems VersaFloppy II"
#define VFII_SNAME   "VFII"


static const char* vfii_description(DEVICE *dptr) {
    if (dptr == NULL) {
        return NULL;
    }
    return VFII_NAME;
}

static MTAB vfii_mod[] = {
    { MTAB_XTD|MTAB_VDV,    0,                      "IOBASE",   "IOBASE",
        &set_iobase, &show_iobase, NULL, "Sets disk controller I/O base address"        },
    { 0 }
};

/* Debug Flags */
static DEBTAB vfii_dt[] = {
    { "STATUS",     STATUS_MSG,     "Status messages"   },
    { "DRIVE",      DRIVE_MSG,      "Drive messages"    },
    { "VERBOSE",    VERBOSE_MSG,    "Verbose messages"  },
    { "IRQ",        IRQ_MSG,        "IRQ messages"      },
    { NULL,         0                                   }
};

DEVICE vfii_dev = {
    VFII_SNAME, vfii_unit, vfii_reg, vfii_mod,
    VFII_MAX_DRIVES, 10, 31, 1, VFII_MAX_DRIVES, VFII_MAX_DRIVES,
    NULL, NULL, &vfii_reset,
    NULL, &wd179x_attach, &wd179x_detach,
    &vfii_info_data, (DEV_DISABLE | DEV_DIS | DEV_DEBUG), 0,
    vfii_dt, NULL, NULL, NULL, NULL, NULL, &vfii_description
};

/* Reset routine */
static t_stat vfii_reset(DEVICE *dptr)
{
    DEVICE *wd179x = NULL;
    PNP_INFO *pnp = (PNP_INFO *) dptr->ctxt;
    char ioaddr[5];

    if(dptr->flags & DEV_DIS) { /* Disconnect I/O Ports */
        sim_map_resource(pnp->io_base, 1, RESOURCE_TYPE_IO, &vfii_control, "vfii_control", TRUE);
    } else {
        /* Connect VFII Control Register */
        if(sim_map_resource(pnp->io_base, pnp->io_size, RESOURCE_TYPE_IO, &vfii_control, "vfii_control", FALSE) != 0) {
            sim_printf("%s: error mapping I/O resource at 0x%04x\n", __FUNCTION__, pnp->io_base);
            return SCPE_ARG;
        }

        /* Enable WD179x device and set I/O port address for VersaFloppy II */
        if ((wd179x = find_dev("WD179X")) != NULL) {
            set_dev_enbdis(wd179x, NULL, 1, NULL);

            sprintf(ioaddr, "%X", pnp->io_base + 1);
            set_iobase(wd179x->units, 0, ioaddr, "");
        }
    }

    return SCPE_OK;
}

/* VersaFloppy II Control/Status
 *
 * BIT 0-3 Drive Select
 * BIT 4   Side Select (1 = Side 0)
 * BIT 5   5"/8" Drive (1 = 8")
 * BIT 6   Double/Single Density (1 = SD)
 * BIT 7   Wait Enable (Not used in simulator)
 *
 * All bits are inverted on the VFII
 *
 */

static int32 vfii_control(const int32 port, const int32 io, const int32 data)
{
    int32 result = 0;
    if (io) { /* I/O Write */
        if (port == vfii_info_data.pnp.io_base) {
            wd179x_infop->sel_drive = floorlog2((~data) & VFII_DSEL_MASK);
            wd179x_infop->fdc_head = (data & VFII_SIDE_MASK) ? 0 : 1;
            wd179x_infop->ddens = (data & VFII_DDEN_MASK) ? FALSE : TRUE;
            wd179x_infop->drivetype = (data & VFII_SIZE_MASK) ? 8 : 5;

            vfii_info_data.creg = data & 0xff;

            sim_debug(DRIVE_MSG, &vfii_dev, VFII_SNAME ": " ADDRESS_FORMAT " WR CTRL(0x%02x) = 0x%02x: Drive:%d Head:%d Size:%d %s-Density.\n",
                PCX, port,
                data & 0xFF,
                wd179x_infop->sel_drive,
                wd179x_infop->fdc_head,
                wd179x_infop->drivetype,
                wd179x_infop->ddens ? "Double" : "Single");
        }
    } else { /* I/O Read */
        result = (vfii_info_data.creg);
    }

    return result;
}
