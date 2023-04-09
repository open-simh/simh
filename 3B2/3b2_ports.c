/* 3b2_ports.c: CM195B 4-Port Serial CIO Card

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
 * PORTS is an intelligent feature card for the 3B2 that supports four
 * serial lines and one Centronics parallel port.
 *
 * The PORTS card is based on the Common I/O (CIO) platform. It uses
 * two SCN2681A DUARTs to supply the four serial lines, and uses the
 * SCN2681A parallel I/O pins for the Centronics parallel port.
 *
 * We make no attempt to emulate a PORTS card's internal workings
 * precisely. Instead, we treat it as a black box as seen from the 3B2
 * system board's point of view.
 *
 */

#include "3b2_ports.h"

#include "sim_tmxr.h"

#include "3b2_cpu.h"
#include "3b2_io.h"
#include "3b2_mem.h"
#include "3b2_stddev.h"

/* Device and units for PORTS cards
 * --------------------------------
 *
 * A 3B2/400 system can have up to 12 PORTS cards installed.  Each
 * card, in turn, has 5 TTY devices - four serial TTYs and one
 * parallel printer port (the printer port is not supported at this
 * time, and is a no-op).
 *
 * The PORTS emulator is backed by a terminal multiplexer with up to
 * 48 (12 * 4) serial lines. Lines can be specified with:
 *
 *   SET PORTS LINES=n
 *
 * Lines must be specified in multiples of 4.
 *
 * Implementation8
 * --------------
 *
 * Each set of 4 lines is mapped to a CIO_STATE struct in the "cio"
 * CIO_STATE structure.
 *
 */

#define MAX_LINES     32

#define PPQESIZE      12
#define DELAY_ASYNC   25
#define DELAY_DLM     100
#define DELAY_ULM     100
#define DELAY_FCF     100
#define DELAY_DOS     100
#define DELAY_DSD     100
#define DELAY_OPTIONS 100
#define DELAY_VERS    100
#define DELAY_CONN    100
#define DELAY_XMIT    50
#define DELAY_RECV    25
#define DELAY_DEVICE  25
#define DELAY_STD     100

#define PORTS_DIAG_CRC1 0x7ceec900
#define PORTS_DIAG_CRC2 0x77a1ea56
#define PORTS_DIAG_CRC3 0x84cf938b
#define PORTS_DIAG_CRC4 0x31b32383  /* Used by SVR 2.0.5 */
#define PORTS_DIAG_CRC5 0x4be7bccc  /* Used by SVR 2.0.5 */
#define PORTS_DIAG_CRC6 0x3197f6dd  /* Used by SVR 2.0.5 */

#define LN(slot,port)   (ports_slot_ln[(slot)] + (port))
#define LSLOT(ln)       (ports_ln_slot[ln])
#define LPORT(ln)       ((ln) % PORTS_LINES)

int8    ports_base_slot;          /* First slot in our contiguous block */
uint8   ports_int_slot;           /* Interrupting card ID   */
uint8   ports_int_subdev;         /* Interrupting subdevice */
t_bool  ports_conf = FALSE;       /* Have PORTS cards been configured? */
uint32  ports_crc;                /* CRC32 of downloaded memory */

/* Mapping of line number to CIO card slot. Up to 32 lines spread over 8
   slots are supported. */
uint8 ports_ln_slot[MAX_LINES];

/* Mapping of slot number to base line number belonging to the card in
   that slot. I.e., if there are two PORTS cards, one in slot 3 and one in
   slot 5, index 3 will have starting line 0, index 5 will have starting
   line 4. */
uint32 ports_slot_ln[CIO_SLOTS];

/* PORTS-specific state for each slot */
PORTS_LINE_STATE *ports_state = NULL;

/* Line-printer state (only one is supported) */
PORTS_LINE_STATE lpt_state;

/* Baud rates determined by the low nybble of the PORT_OPTIONS cflag */
CONST char *ports_baud[16] = {
    "75",    "110",  "134",  "150",
    "300",   "600",  "1200", "2000",
    "2400",  "4800", "1800", "9600",
    "19200", "9600", "9600", "9600"
};

TMLN *ports_ldsc = NULL;
TMXR ports_desc = { 0, 0, 0, NULL };

/* Three units service the Receive, Transmit, and CIO */
UNIT ports_unit[3] = {
    { UDATA(&ports_rcv_svc, UNIT_IDLE|UNIT_ATTABLE|TT_MODE_8B, 0) },
    { UDATA(&ports_xmt_svc, UNIT_DIS, 0), SERIAL_OUT_WAIT },
    { UDATA(&ports_cio_svc, UNIT_DIS, 0) }
};

MTAB ports_mod[] = {
    { TT_MODE, TT_MODE_7B, "7b", "7B", NULL, NULL, NULL, "7 bit mode" },
    { TT_MODE, TT_MODE_8B, "8b", "8B", NULL, NULL, NULL, "8 bit mode" },
    { TT_MODE, TT_MODE_7P, "7p", "7P", NULL, NULL, NULL, "7 bit mode - non printing suppressed" },
    { MTAB_XTD|MTAB_VDV|MTAB_VALR, 0, "LINES", "LINES=n",
      &ports_setnl, &tmxr_show_lines, (void *) &ports_desc, "Show or set number of lines" },
    { 0 }
};

static DEBTAB ports_debug[] = {
    { "IO",    IO_DBG,        "I/O Character Trace"          },
    { "TRACE", TRACE_DBG,     "Call Trace"                   },
    { "XMT",   TMXR_DBG_XMT,  "TMXR Transmit Data"           },
    { "RCV",   TMXR_DBG_RCV,  "TMXR Received Data"           },
    { "RET",   TMXR_DBG_RET,  "TMXR Returned Received Data"  },
    { "MDM",   TMXR_DBG_XMT,  "TMXR Modem Signals"           },
    { "CON",   TMXR_DBG_XMT,  "TMXR Connection Activity"     },
    { "ASY",   TMXR_DBG_ASY,  "TMXR Async Activity"          },
    { "PXMT",  TMXR_DBG_PXMT, "TMXR Transmit Packets"        },
    { "PRCV",  TMXR_DBG_PRCV, "TMXR Received Packets"        },
    { NULL }
};

DEVICE ports_dev = {
    "PORTS",                       /* name */
    ports_unit,                    /* units */
    NULL,                          /* registers */
    ports_mod,                     /* modifiers */
    3,                             /* #units */
    16,                            /* address radix */
    32,                            /* address width */
    1,                             /* address incr. */
    16,                            /* data radix */
    8,                             /* data width */
    NULL,                          /* examine routine */
    NULL,                          /* deposit routine */
    &ports_reset,                  /* reset routine */
    NULL,                          /* boot routine */
    &ports_attach,                 /* attach routine */
    &ports_detach,                 /* detach routine */
    NULL,                          /* context */
    DEV_DISABLE|DEV_DIS|DEV_DEBUG|DEV_MUX, /* flags */
    0,                             /* debug control flags */
    ports_debug,                   /* debug flag names */
    NULL,                          /* memory size change */
    NULL,                          /* logical name */
    NULL,                          /* help routine */
    NULL,                          /* attach help routine */
    (void *)&ports_desc,           /* help context */
    NULL,                          /* device description */
};

UNIT lpt_unit = {
    UDATA(NULL, UNIT_SEQ|UNIT_ATTABLE|UNIT_TEXT, 0), SERIAL_OUT_WAIT
};

DEVICE lpt_dev = {
    "LPT",                  /* name */
    &lpt_unit,              /* units */
    NULL,                   /* registers */
    NULL,                   /* modifiers */
    1,                      /* # units */
    16,                     /* address radix */
    32,                     /* address width */
    1,                      /* address incr. */
    16,                     /* data radix */
    8,                      /* data width */
    NULL,                   /* examine routine */
    NULL,                   /* deposit routine */
    &lpt_reset,             /* reset routine */
    NULL,                   /* boot routine */
    &lpt_attach,            /* attach routine */
    &lpt_detach,            /* detach routine */
    NULL,                   /* context */
    DEV_DISABLE|DEV_DIS,    /* flags */
    0,                      /* debug control flags */
    NULL,                   /* debug flag names */
    NULL,                   /* memory size change */
    NULL,                   /* logical name */
    &lpt_help,              /* help routine */
    NULL,                   /* attach help routine */
    NULL,                   /* help context */
    &lpt_description        /* device description */
};

static void cio_irq(uint8 slot, uint8 dev, int32 delay)
{
    ports_int_slot = slot;
    ports_int_subdev = dev & 0xf;
    sim_activate(&ports_unit[2], delay);
}

static void lpt_out(uint8 c)
{
    if (!lpt_state.conn) {
        return;
    }

    fputc(c, lpt_unit.fileref);

    if (ferror(lpt_unit.fileref)) {
        sim_perror("LPT I/O error");
        clearerr(lpt_unit.fileref);
        return;
    }

    lpt_unit.pos++;
}

/*
 * Set the number of lines for the PORTS mux. This will add or remove
 * cards as necessary. The number of lines must be a multiple of 4.
 */
t_stat ports_setnl(UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
    int32 newln, i, t;
    t_stat r = SCPE_OK;

    if (cptr == NULL) {
        return SCPE_ARG;
    }

    newln = (int32) get_uint(cptr, 10, (MAX_CARDS * PORTS_LINES), &r);

    if ((r != SCPE_OK) || (newln == ports_desc.lines)) {
        return r;
    }

    if ((newln == 0) || LPORT(newln) != 0 || newln > MAX_LINES) {
        return SCPE_ARG;
    }

    sim_debug(TRACE_DBG, &ports_dev,
              "[ports_setnl] Setting line count to %d\n",
              newln);

    if (newln < ports_desc.lines) {
        for (i = newln, t = 0; i < ports_desc.lines; i++) {
            t = t | ports_ldsc[i].conn;
        }

        if (t && !get_yn("This will disconnect users; proceed [N]?", FALSE)) {
            return SCPE_OK;
        }

        for (i = newln; i < ports_desc.lines; i++) {
            if (ports_ldsc[i].conn) {
                tmxr_linemsg(&ports_ldsc[i], "\r\nOperator disconnected line\r\n");
                tmxr_send_buffered_data(&ports_ldsc[i]);
            }
            /* completely reset line */
            tmxr_detach_ln(&ports_ldsc[i]);
        }
    }

    ports_desc.ldsc = ports_ldsc = (TMLN *)realloc(ports_ldsc, newln*sizeof(*ports_ldsc));
    ports_state = (PORTS_LINE_STATE *)realloc(ports_state, newln*sizeof(*ports_state));

    if (ports_desc.lines < newln) {
        memset(ports_ldsc + ports_desc.lines, 0, sizeof(*ports_ldsc)*(newln-ports_desc.lines));
        memset(ports_state + ports_desc.lines, 0, sizeof(*ports_state)*(newln-ports_desc.lines));
    }

    ports_desc.lines = newln;

    /* setup lines and auto config */
    ports_conf = FALSE;
    return ports_reset(&ports_dev);
}


static void ports_cmd(uint8 slot, cio_entry *rentry, uint8 *rapp_data)
{
    cio_entry centry = {0};
    uint32 ln, i;
    PORTS_OPTIONS opts;
    char line_config[16];
    uint8 app_data[4] = {0};
    PORTS_LINE_STATE *state;

    centry.address = rentry->address;
    cio[slot].op = rentry->opcode;

    if ((rentry->subdevice & 7) == PORTS_CENTRONICS) {
        ln = 0;
        state = &lpt_state;
    } else {
        ln = LN(slot, rentry->subdevice & 7);
        state = &ports_state[ln];
    }

    sim_debug(TRACE_DBG, &ports_dev,
              "[ports_cmd] Command: slot %d, subdev %d, opcode 0x%02x\n",
              slot, rentry->subdevice, rentry->opcode);

    switch(rentry->opcode) {
    case CIO_DLM:
        for (i = 0; i < rentry->byte_count; i++) {
            ports_crc = cio_crc32_shift(ports_crc, pread_b(rentry->address + i, BUS_PER));
        }
        centry.address = rentry->address + rentry->byte_count;
        sim_debug(TRACE_DBG, &ports_dev,
                  "[ports_cmd] CIO Download Memory: bytecnt=%04x "
                  "addr=%08x return_addr=%08x subdev=%02x (CRC=%08x)\n",
                  rentry->byte_count, rentry->address,
                  centry.address, centry.subdevice, ports_crc);
        /* We intentionally do not set the subdevice in
         * the completion entry */
        cio_cexpress(slot, PPQESIZE, &centry, app_data);
        cio_irq(slot, rentry->subdevice, DELAY_DLM);
        break;
    case CIO_ULM:
        sim_debug(TRACE_DBG, &ports_dev,
                  "[ports_cmd] CIO Upload Memory\n");
        cio_cexpress(slot, PPQESIZE, &centry, app_data);
        cio_irq(slot, rentry->subdevice, DELAY_ULM);
        break;
    case CIO_FCF:
        sim_debug(TRACE_DBG, &ports_dev,
                  "[ports_cmd] CIO Force Function Call (CRC=%08x)\n",
                  ports_crc);

        /* If the currently running program is a diagnostics program,
         * we are expected to write results into memory at address
         * 0x200f000 */
        if (ports_crc == PORTS_DIAG_CRC1 ||
            ports_crc == PORTS_DIAG_CRC2 ||
            ports_crc == PORTS_DIAG_CRC3 ||
            ports_crc == PORTS_DIAG_CRC4 ||
            ports_crc == PORTS_DIAG_CRC5 ||
            ports_crc == PORTS_DIAG_CRC6) {
            pwrite_h(0x200f000, 0x1, BUS_PER);   /* Test success */
            pwrite_h(0x200f002, 0x0, BUS_PER);   /* Test Number */
            pwrite_h(0x200f004, 0x0, BUS_PER);   /* Actual */
            pwrite_h(0x200f006, 0x0, BUS_PER);   /* Expected */
            pwrite_b(0x200f008, 0x1, BUS_PER);   /* Success flag again */
        }

        /* An interesting (?) side-effect of FORCE FUNCTION CALL is
         * that it resets the card state such that a new SYSGEN is
         * required in order for new commands to work. In fact, an
         * INT0/INT1 combo _without_ a RESET can sysgen the board. So,
         * we reset the command bits here. */
        cio[slot].sysgen_s = 0;
        cio_cexpress(slot, PPQESIZE, &centry, app_data);
        cio_irq(slot, rentry->subdevice, DELAY_FCF);
        break;
    case CIO_DOS:
        sim_debug(TRACE_DBG, &ports_dev,
                  "[ports_cmd] CIO Determine Op Status\n");
        cio_cexpress(slot, PPQESIZE, &centry, app_data);
        cio_irq(slot, rentry->subdevice, DELAY_DOS);
        break;
    case CIO_DSD:
        /* Determine Sub-Devices. We have none. */
        sim_debug(TRACE_DBG, &ports_dev,
                  "[ports_cmd] Determine Sub-Devices.\n");

        /* The system wants us to write sub-device structures
         * at the supplied address */

        pwrite_h(rentry->address, 0x0, BUS_PER);
        cio_cexpress(slot, PPQESIZE, &centry, app_data);
        cio_irq(slot, rentry->subdevice, DELAY_DSD);
        break;
    case PPC_OPTIONS:
        sim_debug(TRACE_DBG, &ports_dev,
                  "[ports_cmd] PPC Options Operation\n");

        opts.line   = pread_h(rentry->address, BUS_PER);
        opts.iflag  = pread_h(rentry->address + 4, BUS_PER);
        opts.oflag  = pread_h(rentry->address + 6, BUS_PER);
        opts.cflag  = pread_h(rentry->address + 8, BUS_PER);
        opts.lflag  = pread_h(rentry->address + 10, BUS_PER);
        opts.cerase = pread_b(rentry->address + 11, BUS_PER);
        opts.ckill  = pread_b(rentry->address + 12, BUS_PER);
        opts.cinter = pread_b(rentry->address + 13, BUS_PER);
        opts.cquit  = pread_b(rentry->address + 14, BUS_PER);
        opts.ceof   = pread_b(rentry->address + 15, BUS_PER);
        opts.ceol   = pread_b(rentry->address + 16, BUS_PER);
        opts.itime  = pread_b(rentry->address + 17, BUS_PER);
        opts.vtime  = pread_b(rentry->address + 18, BUS_PER);
        opts.vcount = pread_b(rentry->address + 19, BUS_PER);

        sim_debug(TRACE_DBG, &ports_dev,  "    PPC Options: iflag=%04x\n", opts.iflag);
        sim_debug(TRACE_DBG, &ports_dev,  "    PPC Options: oflag=%04x\n", opts.oflag);
        sim_debug(TRACE_DBG, &ports_dev,  "    PPC Options: cflag=%04x\n", opts.cflag);
        sim_debug(TRACE_DBG, &ports_dev,  "    PPC Options: lflag=%04x\n", opts.lflag);
        sim_debug(TRACE_DBG, &ports_dev,  "    PPC Options: itime=%02x\n", opts.itime);
        sim_debug(TRACE_DBG, &ports_dev,  "    PPC Options: vtime=%02x\n", opts.vtime);
        sim_debug(TRACE_DBG, &ports_dev,  "    PPC Options: vcount=%02x\n", opts.vcount);

        state->iflag = opts.iflag;
        state->oflag = opts.oflag;

        if ((rentry->subdevice & 7) < PORTS_LINES) {
            /* Adjust baud rate */
            sprintf(line_config, "%s-8N1",
                    ports_baud[opts.cflag&0xf]);

            sim_debug(TRACE_DBG, &ports_dev,
                      "Setting PORTS line %d to %s\n",
                      ln, line_config);

            tmxr_set_config_line(&ports_ldsc[ln], line_config);
        }

        centry.byte_count = 20;
        centry.opcode = PPC_OPTIONS;
        centry.subdevice = rentry->subdevice;
        centry.address = rentry->address;
        cio_cqueue(slot, CIO_STAT, PPQESIZE, &centry, app_data);
        cio_irq(slot, rentry->subdevice, DELAY_OPTIONS);
        break;
    case PPC_VERS:
        sim_debug(TRACE_DBG, &ports_dev,
                  "[ports_cmd] PPC Version\n");

        /* Write the version number at the supplied address */
        pwrite_b(rentry->address, PORTS_VERSION, BUS_PER);

        centry.opcode = CIO_ULM;

        /* 0x50 (80 decimal) is the expected PROM version */
        app_data[0] = 0x50;
        cio_cqueue(slot, CIO_STAT, PPQESIZE, &centry, app_data);
        cio_irq(slot, rentry->subdevice, DELAY_VERS);
        break;
    case PPC_CONN:
        /* CONNECT - Full request and completion queues */
        sim_debug(TRACE_DBG, &ports_dev,
                  "[ports_cmd] PPC CONNECT - subdevice = %02x\n",
                  rentry->subdevice);

        if (rentry->subdevice < PORTS_LINES) {
            ports_state[ln].conn = TRUE;
        }

        centry.opcode = PPC_CONN;
        centry.subdevice = rentry->subdevice;
        centry.address = rentry->address;
        cio_cqueue(slot, CIO_STAT, PPQESIZE, &centry, app_data);
        cio_irq(slot, rentry->subdevice, DELAY_CONN);
        break;
    case PPC_XMIT:
        /* XMIT - Full request and completion queues */

        /* The port being referred to is in the subdevice. */
        sim_debug(TRACE_DBG, &ports_dev,
                  "[ports_cmd] PPC XMIT - subdevice = %02x, address=%08x, byte_count=%d\n",
                  rentry->subdevice, rentry->address, rentry->byte_count);

        state->tx_addr = rentry->address;
        state->tx_req_addr = state->tx_addr;
        state->tx_chars = rentry->byte_count + 1;
        state->tx_req_chars = state->tx_chars;

        sim_activate_after(&ports_unit[1], ports_unit[1].wait);

        break;
    case PPC_DEVICE:
        /* DEVICE Control - Express request and completion queues */
        /* The port being referred to is in the subdevice. */
        sim_debug(TRACE_DBG, &ports_dev,
                  "[ports_cmd] PPC DEVICE - subdevice = %02x\n",
                  rentry->subdevice);
        centry.subdevice = rentry->subdevice;
        centry.opcode = PPC_DEVICE;
        cio_cexpress(slot, PPQESIZE, &centry, app_data);
        cio_irq(slot, rentry->subdevice, DELAY_DEVICE);
        break;
    case PPC_RECV:
        /* RECV - Full request and completion queues */

        /* The port being referred to is in the subdevice. */
        sim_debug(TRACE_DBG, &ports_dev,
                  "[ports_cmd] PPC RECV - subdevice = %02x addr=%08x\n",
                  rentry->subdevice, rentry->address);

        break;
    case PPC_DISC:
        /* Disconnect */
        centry.subdevice = rentry->subdevice;
        centry.opcode = PPC_DISC;
        ports_ldsc[ln].rcve = 0;
        cio_cqueue(slot, CIO_STAT, PPQESIZE, &centry, app_data);
        cio_irq(slot, rentry->subdevice, DELAY_STD);
        break;
    case PPC_BRK:
    case PPC_CLR:
    default:
        sim_debug(TRACE_DBG, &ports_dev,
                  ">>> Op %d Not Handled Yet\n",
                  rentry->opcode);

        cio_cexpress(slot, PPQESIZE, &centry, app_data);
        cio_irq(slot, rentry->subdevice, DELAY_STD);
        break;
    }
}

/*
 * Update the connection status of the given port.
 */
static void ports_update_conn(uint8 slot, uint8 subdev)
{
    cio_entry centry = {0};
    uint8 app_data[4] = {0};
    uint32 ln = LN(slot, subdev);

    /* If the card hasn't sysgened, there's no way to write a completion
     * queue entry */
    if (cio[slot].sysgen_s != CIO_SYSGEN) {
        return;
    }

    sim_debug(TRACE_DBG, &ports_dev, "[ports_update_conn] slot=%d, subdev=%d\n", slot, subdev);

    if (subdev == PORTS_CENTRONICS) {
        if (!lpt_state.conn) {
            return;
        }
        app_data[0] = AC_CON;
    } else {
        if (ports_ldsc[ln].conn) {
            app_data[0] = AC_CON;
            ports_state[ln].conn = TRUE;
        } else {
            if (ports_state[ln].conn) {
                app_data[0] = AC_DIS;
                ports_state[ln].conn = FALSE;
            } else {
                app_data[0] = 0;
            }
        }
    }

    centry.opcode = PPC_ASYNC;
    centry.subdevice = subdev;
    cio_cqueue(slot, CIO_CMD, PPQESIZE, &centry, app_data);

    /* Interrupt */
    CIO_SET_INT(slot);
}

void ports_sysgen(uint8 slot)
{
    cio_entry cqe = {0};
    uint8 app_data[4] = {0};

    ports_crc = 0;

    cqe.opcode = 3; /* Sysgen success! */

    /* It's not clear why we put a response in both the express
     * and the full queue. */
    cio_cexpress(slot, PPQESIZE, &cqe, app_data);
    cio_cqueue(slot, CIO_STAT, PPQESIZE, &cqe, app_data);

    ports_int_slot = slot;
    sim_activate(&ports_unit[2], DELAY_STD);
}

void ports_express(uint8 slot)
{
    cio_entry rqe = {0};
    uint8 app_data[4] = {0};
    cio_rexpress(slot, PPQESIZE, &rqe, app_data);
    ports_cmd(slot, &rqe, app_data);
}

void ports_full(uint8 slot)
{
    uint32 i;
    cio_entry rqe = {0};
    uint8 app_data[4] = {0};

    for (i = 0; i < PORTS_RCV_QUEUE; i++) {
        if (cio_rqueue(slot, i, PPQESIZE, &rqe, app_data) == SCPE_OK) {
            ports_cmd(slot, &rqe, app_data);
        }
    }
}

t_stat ports_reset(DEVICE *dptr)
{
    int32 i, j;
    uint8 slot;
    t_stat r;

    ports_crc = 0;

    if (ports_ldsc == NULL) {
        sim_set_uname(&ports_unit[0], "PORTS-RCV");
        sim_set_uname(&ports_unit[1], "PORTS-XMT");
        sim_set_uname(&ports_unit[2], "PORTS-CIO");

        ports_desc.lines = PORTS_LINES;
        ports_desc.ldsc = ports_ldsc =
            (TMLN *)calloc(ports_desc.lines, sizeof(*ports_ldsc));
    }

    if (ports_state == NULL) {
        ports_state = (PORTS_LINE_STATE *)calloc(ports_desc.lines, sizeof(*ports_state));
        memset(ports_state, 0, ports_desc.lines*sizeof(*ports_state));
    }

    tmxr_set_port_speed_control(&ports_desc);

    if ((dptr->flags & DEV_DIS)) {
        cio_remove_all(PORTS_ID);
        ports_conf = FALSE;
        return SCPE_OK;
    }

    if (!ports_conf) {
        /* Clear out any old cards, we're starting fresh */
        cio_remove_all(PORTS_ID);

        memset(ports_slot_ln, 0, sizeof(ports_slot_ln));
        memset(ports_ln_slot, 0, sizeof(ports_ln_slot));

        /* Insert necessary cards into the backplane */
        j = 0;
        for (i = 0; i < ports_desc.lines/PORTS_LINES; i++) {
            r = cio_install(PORTS_ID, "PORTS", PORTS_IPL,
                            &ports_express, &ports_full, &ports_sysgen, NULL,
                            &slot);
            if (r != SCPE_OK) {
                return r;
            }
            /* Remember the port assignments */
            ports_slot_ln[slot] = i * PORTS_LINES;
            for (; j < (i * PORTS_LINES) + PORTS_LINES; j++) {
                ports_ln_slot[j] = slot;
            }
        }

        ports_conf = TRUE;
    }

    /* If attached, start polling for connections */
    if (ports_unit[0].flags & UNIT_ATT) {
        sim_activate_after_abs(&ports_unit[0], ports_unit[0].wait);
    } else {
        sim_cancel(&ports_unit[0]);
    }

    return SCPE_OK;
}

t_stat ports_cio_svc(UNIT *uptr)
{
    sim_debug(TRACE_DBG, &ports_dev,
              "[ports_cio_svc] IRQ for board %d device %d\n",
              ports_int_slot, ports_int_subdev);

    CIO_SET_INT(ports_int_slot);

    switch (cio[ports_int_slot].op) {
    case PPC_CONN:
        cio[ports_int_slot].op = PPC_ASYNC;
        if (ports_int_subdev < PORTS_LINES) {
            ports_ldsc[LN(ports_int_slot, ports_int_subdev)].rcve = 1;
        }
        sim_activate(&ports_unit[2], DELAY_ASYNC);
        break;
    case PPC_ASYNC:
        ports_update_conn(ports_int_slot, ports_int_subdev);
        break;
    default:
        break;
    }

    return SCPE_OK;
}

t_stat ports_rcv_svc(UNIT *uptr)
{
    uint8 slot;
    int32 temp, ln;
    char c;
    cio_entry rentry = {0};
    cio_entry centry = {0};
    uint8 rapp_data[4] = {0};
    uint8 capp_data[4] = {0};

    if ((uptr->flags & UNIT_ATT) == 0) {
        return SCPE_OK;
    }

    ln = tmxr_poll_conn(&ports_desc);
    if (ln >= 0) {
        /* Possibly connect a newly opened port */
        ports_update_conn(LSLOT(ln), LPORT(ln));
    }

    for (ln = 0; ln < ports_desc.lines; ln++) {
        slot = LSLOT(ln);

        if (!ports_ldsc[ln].conn && ports_state[ln].conn) {
            /* Disconnect a connected line, it has been dropped */
            ports_update_conn(LSLOT(ln), LPORT(ln));
        } else if (ports_ldsc[ln].conn && ports_state[ln].conn) {
            temp = tmxr_getc_ln(&ports_ldsc[ln]);

            if (temp && !(temp & SCPE_BREAK)) {

                c = (char) (temp & 0xff);

                sim_debug(IO_DBG, &ports_dev,
                          "[LINE %d RECEIVE] char = %02x (%c)\n",
                          ln, c, c);

                if (c == 0xd && (ports_state[ln].iflag & ICRNL)) {
                    c = 0xa;
                }

                if (cio[slot].ivec > 0 &&
                    cio_rqueue(slot, PORTS_RCV_QUEUE,
                               PPQESIZE, &rentry, rapp_data) == SCPE_OK) {
                    CIO_SET_INT(slot);

                    /* Write the character to the memory address */
                    pwrite_b(rentry.address, c, BUS_PER);
                    centry.subdevice = LPORT(ln);
                    centry.opcode = PPC_RECV;
                    centry.address = rentry.address;
                    capp_data[3] = RC_TMR;

                    cio_cqueue(slot, CIO_STAT, PPQESIZE, &centry, capp_data);
                }
            }
        }
    }

    tmxr_poll_rx(&ports_desc);
    tmxr_poll_tx(&ports_desc);

    tmxr_clock_coschedule(uptr, tmxr_poll);

    return SCPE_OK;
}

t_stat ports_xmt_svc(UNIT *uptr)
{
    uint8 slot, ln;
    char c;
    t_bool tx = FALSE; /* Did a tx ever occur? */
    cio_entry centry = {0};
    uint8 app_data[4] = {0};
    uint32 wait = 0x7fffffff;

    /* Scan all MUX lines for output */
    for (ln = 0; ln < ports_desc.lines; ln++) {
        slot = LSLOT(ln);
        if (ports_ldsc[ln].conn && ports_state[ln].tx_chars > 0) {
            tx = TRUE; /* Even an attempt at TX counts for rescheduling */
            c = sim_tt_outcvt(pread_b(ports_state[ln].tx_addr, BUS_PER),
                              TT_GET_MODE(ports_unit[0].flags));

            /* The PORTS card optionally handles NL->CRLF */
            if (c == 0xa &&
                (ports_state[ln].oflag & ONLCR) &&
                !(ports_state[ln].crlf)) {
                if (tmxr_putc_ln(&ports_ldsc[ln], 0xd) == SCPE_OK) {
                    wait = MIN(wait, ports_ldsc[ln].txdeltausecs);
                    sim_debug(IO_DBG, &ports_dev,
                              "[ports_xmt_svc] [LINE %d] XMIT (crlf):  %02x (%c)\n",
                              ln, 0xd, 0xd);
                    /* Indicate that we're in a CRLF translation */
                    ports_state[ln].crlf = TRUE;
                }
            } else {
                ports_state[ln].crlf = FALSE;

                if (tmxr_putc_ln(&ports_ldsc[ln], c) == SCPE_OK) {
                    wait = MIN(wait, ports_ldsc[ln].txdeltausecs);
                    ports_state[ln].tx_chars--;
                    ports_state[ln].tx_addr++;
                }

                if (ports_state[ln].tx_chars == 0) {
                    centry.byte_count = ports_state[ln].tx_req_chars;
                    centry.subdevice = LPORT(ln);
                    centry.opcode = PPC_XMIT;
                    centry.address = ports_state[ln].tx_req_addr;
                    app_data[0] = GC_FLU;
                    cio_cqueue(slot, CIO_STAT, PPQESIZE, &centry, app_data);
                    CIO_SET_INT(slot);
                }
            }
        }
    }

    /* Scan LPT line for output */
    if (lpt_state.conn && lpt_state.tx_chars > 0) {
        tx = TRUE;
        /* This is a hack -- we just want the slot of the first installed
           PORTS board */
        slot = LSLOT(0);
        wait = MIN(wait, SERIAL_OUT_WAIT);
        c = pread_b(lpt_state.tx_addr, BUS_PER);

        /* The PORTS card optionally handles NL->CRLF */
        if (c == 0xa && (lpt_state.oflag & ONLCR) && !(lpt_state.crlf)) {
            /* Indicate that we're in a CRLF translation */
            lpt_state.crlf = TRUE;
            lpt_out(0xd);
        } else {
            lpt_state.crlf = FALSE;
            lpt_state.tx_chars--;
            lpt_state.tx_addr++;
            lpt_out(c);
            if (lpt_state.tx_chars == 0) {
                centry.byte_count = lpt_state.tx_req_chars;
                centry.subdevice = PORTS_CENTRONICS;
                centry.opcode = PPC_XMIT;
                centry.address = lpt_state.tx_req_addr;
                app_data[0] = GC_FLU;
                cio_cqueue(slot, CIO_STAT, PPQESIZE, &centry, app_data);
                CIO_SET_INT(slot);
            }
        }
    }

    tmxr_poll_tx(&ports_desc);

    if (tx) {
        tmxr_activate_after(uptr, wait);
    }

    return SCPE_OK;
}

t_stat ports_attach(UNIT *uptr, CONST char *cptr)
{
    TMLN *lp;
    t_stat r;
    int i;

    if ((sim_switches & SWMASK('M'))) {
        tmxr_set_modem_control_passthru(&ports_desc);
    }

    for (i = 0; i < ports_desc.lines; i++) {
        sim_debug(TRACE_DBG, &ports_dev,
                  "[ports_reset] Setting up line %d...\n", i);
        tmxr_set_line_output_unit(&ports_desc, i, &ports_unit[1]);
        if (!ports_ldsc[i].conn) {
            ports_ldsc[i].xmte = 1;
        }
        ports_ldsc[i].rcve = 0;
        tmxr_set_config_line(&ports_ldsc[i], "9600-8N1");
    }

    r = tmxr_attach(&ports_desc, uptr, cptr);
    if (r != SCPE_OK) {
        tmxr_clear_modem_control_passthru(&ports_desc);
        return r;
    }

    for (i = 0; i < ports_desc.lines; i++) {
        lp = &ports_ldsc[i];
        tmxr_set_get_modem_bits(lp, TMXR_MDM_DTR|TMXR_MDM_RTS, 0, NULL);
    }

    return SCPE_OK;
}

t_stat ports_detach(UNIT *uptr)
{
    t_stat r;

    r = tmxr_detach(&ports_desc, uptr);

    if (r != SCPE_OK) {
        return r;
    }

    tmxr_clear_modem_control_passthru(&ports_desc);

    return SCPE_OK;
}

t_stat lpt_reset(DEVICE *dptr)
{
    /* No-op */
    return SCPE_OK;
}

t_stat lpt_attach(UNIT *uptr, const char *cptr)
{
    t_stat reason;

    if (ports_dev.flags & DEV_DIS) {
        return SCPE_NOFNC;
    }

    if ((reason = attach_unit(uptr, cptr)) == SCPE_OK) {
        lpt_state.conn = TRUE;
    }

    return reason;
}

t_stat lpt_detach(UNIT *uptr)
{
    lpt_state.conn = FALSE;
    return detach_unit(uptr);
}

t_stat lpt_help(FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
    fprintf(st, "Line Printer (LPT)\n\n");
    fprintf(st, "The line printer (LPT) simulates an AT&T 470 120cps line printer\n");
    fprintf(st, "connected to the Centronics port of the first installed PORTS card. It\n");
    fprintf(st, "writes text output to an attached file on disk. To use the LPT device,\n");
    fprintf(st, "first ENABLE it, then ATTACH it to a disk file to be used for text \n");
    fprintf(st, "output, for example:\n\n");
    fprintf(st, "  SET LPT ENABLE\n");
    fprintf(st, "  ATTACH LPT my_output.txt\n\n");
    fprintf(st, "To use this device under System V UNIX, set up a printer attached to\n");
    fprintf(st, "TTY number 5 of the first installed PORTS card.\n\n");
    fprintf(st, "For example, if the first installed PORTS card has created TTY devices\n");
    fprintf(st, "/dev/tty21 through /dev/tty25 (where /dev/tty2* indicates that the PORTS\n");
    fprintf(st, "card is in backplane slot 2), the printer will be on TTY device /dev/tty25.\n\n");
    fprintf(st, "Please note that the LPT requires at least one PORTS card be configured. It\n");
    fprintf(st, "will not allow attaching to an output file unless a PORTS card is enabled.\n");
    fprintf(st, "If you see the output\n\n");
    fprintf(st, "  Command not allowed\n\n");
    fprintf(st, "when trying to attach the LPT device, it means you have not enabled at\n");
    fprintf(st, "least one PORTS card.\n");
    return SCPE_OK;
}

const char *lpt_description(DEVICE *dptr)
{
    return "AT&T 470 120cps Dot Matrix Printer";
}
