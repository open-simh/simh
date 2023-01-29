/* rtx2001a_ab.c: RTX2001A CPU simulator

   Copyright (c) 2022, Systasis Computer Systems, Inc.

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
   SYSTASIS COMPUTER SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Systasis Computer Systems shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Systasis Computer Systems.

    9-Sep-22   SYS      ASIC bus routines
*/

#include "rtx2001a_ab.h"
#include "rtx2001a_execute.h"

RTX_WORD hostmode = FALSE; /* when true, in process of talking to the host */

// https://embedjournal.com/implementing-circular-buffer-embedded-c/
typedef struct
{
    t_value *const buffer;
    int head;
    int tail;
    const int maxlen;
} circ_bbuf_t;

t_value _kbbuf[255] = {'\0'};
circ_bbuf_t kbbuf = {_kbbuf, 0, 0, 254};

int circ_bbuf_push(circ_bbuf_t *c, t_value data)
{
    int next;

    next = c->head + 1; // next is where head will point to after this write.
    if (next >= c->maxlen)
        next = 0;

    if (next == c->tail) // if the head + 1 == tail, circular buffer is full
        return -1;

    c->buffer[c->head] = data; // Load data and then move
    c->head = next;            // head to next data offset.
    return 0;                  // return success to indicate successful push.
}

int circ_bbuf_pop(circ_bbuf_t *c, t_value *data)
{
    int next;

    if (c->head == c->tail) // if the head == tail, we don't have any data
        return -1;

    next = c->tail + 1; // next is where tail will point to after this read.
    if (next >= c->maxlen)
        next = 0;

    *data = c->buffer[c->tail]; // Read data and then move
    c->tail = next;             // tail to next offset.
    return 0;                   // return success to indicate successful push.
}

// t_stat poll_kbd(t_value *data)
// {
//     int status;
//     unsigned char buf[1];

//     status = read(0, buf, 1);
//     *data = '\0';
//     if (status != 1)
//         return SCPE_OK;
//     if (sim_brk_char && (buf[0] == sim_brk_char))
//         return SCPE_BREAK;
//     if (sim_int_char && (buf[0] == sim_int_char))
//         return SCPE_STOP;
//     *data = buf[0];
//     return SCPE_OK;
// }

/* implementation note: gfetch is always invoked before TEST_EXIT */
t_stat gfetch(t_addr offset, t_value *data)
{
    switch (offset)
    {

    case 0x19: // RCV
    {
        // if (hostmode) /* invoke fsm for serving host mode */
        // {
        //     return (host_read());
        // }
        // *data = getchar() & D16_MASK;
        /* printf("\n(%d) = '%c'", temp, temp); */
        // handle
        if (0 == circ_bbuf_pop(&kbbuf, data))
        {
            return SCPE_OK;
        }
        *data = 0;
        t_stat status = sim_poll_kbd();
        switch (status)
        {
        case SCPE_OK:   // no char
        case SCPE_LOST: // Error
            return status;
        default:
            *data = status & ~SCPE_KFLAG;
        }

        return SCPE_OK;
    }

    case 0x1A: // RCV?
    {
        t_stat status = sim_poll_kbd();
        *data = 0;
        switch (status)
        {
        case SCPE_OK:   // no char
        case SCPE_LOST: // Error
            return status;
        default:
            circ_bbuf_push(&kbbuf, status & ~SCPE_KFLAG);
        }
        *data = 0xFF;
        return SCPE_OK;
    }

    // return bit=0: Pushes the contents of I into TOP (with no pop of the Return Stack)
    // return bit=1: Pushes the contents of I into TOP then performs a subroutine return
    case I:
        *data = asic_file[I];
        sim_debug(DBG_ASB_R, &cpu_dev, sim_is_running ? "I=0x%X\n" : "I=0x%X", asic_file[I]);
        return SCPE_OK;

    // return bit=0: Pushes the contents of I into TOP, popping the Return Stack
    // return bit=1: Pushes the contents of I into TOP without popping the Return Stack, then executes the Subroutine Return
    case I_ASIC:
        *data = asic_file[I];
        sim_debug(DBG_ASB_R, &cpu_dev, "I=0x%X\n", asic_file[I]);
        if (!EXIT)
        {
            rs_pop();
        }
        return SCPE_OK;

    // Pushes the contents of I shifted left by one bit, into TOP (the Return Stack is not popped)
    case I_ALU:
        *data = (asic_file[I] << 1) & D16_UMAX;
        sim_debug(DBG_ASB_R, &cpu_dev, sim_is_running ? "I=0x%X\n" : " ", *data);
        return SCPE_OK;

    case _CR:
        *data = cr.pr & 0xFFEF; // Force SID to zero

        if (sim_is_running)
            sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "CR", cr_bits, cr.pr, *data, 1);
        return SCPE_OK;

    case MD:
        *data = asic_file[MD] & D16_UMAX;
        if (sim_is_running)
            sim_debug(DBG_ASB_R, &cpu_dev, "MD=0x%X\n", asic_file[MD]);
        return SCPE_OK;

    case SQ:
    {
        RTX_WORD sq = asic_file[MD] << 1;
        *data = sq | asic_file[SR];
        sim_debug(DBG_ASB_R, &cpu_dev, "SQ=0x%X SR=0x%X\n", sq, asic_file[SR]);
        return SCPE_OK;
    }

    case SR:
    {
        *data = asic_file[SR];
        sim_debug(DBG_ASB_R, &cpu_dev, "SR=0x%X\n", asic_file[SR]);
        return SCPE_OK;
    }

    case PC:
        *data = asic_file[PC];
        sim_debug(DBG_ASB_R, &cpu_dev, "PC=0x%X\n", asic_file[PC]);
        return SCPE_OK;

    case _IMR:
        *data = imr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "IMR", imr_bits, imr.pr, *data, 1);
        return SCPE_OK;

    case _SPR:
        *data = (((spr.fields.rsp) << 8) & 0xFF00) | (((spr.fields.psp) + 1) & 0x00FF);
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "SPR", spr_bits, spr.pr, *data, 1);
        return SCPE_OK;

    case _SUR:
        *data = sur.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "SUR", sur_bits, sur.pr, *data, 1);
        return SCPE_OK;

    case _IVR:
    {
        IMR _imr;
        IVR _ivr;

        _ivr.fields.ivb = ibc.fields.ivb; // Interrupt Vector Base, MA10 - MA15
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "IBCR", ibc_bits, ibc.pr, ibc.pr, 1);

        _imr.pr = imr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "IMR", imr_bits, imr.pr, imr.pr, 1);

        for (uint32 pri = 15; pri > 0; pri--)
        {
            if (_imr.pr & 1)
            {
                _ivr.fields.vec = pri;
                break;
            }
            _imr.pr = _imr.pr >> 1;
        }
        *data = _ivr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "IVR", ivr_bits, ivr.pr, *data, 1);
        return SCPE_OK;
    }

        // SVR is write-only

    case _IPR:
        *data = ipr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "IPR", ipr_bits, ipr.pr, *data, 1);
        return SCPE_OK;

    case _DPR:
        *data = dpr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "DPR", pr_bits, dpr.pr, *data, 1);
        return SCPE_OK;

    case _UPR:
        *data = upr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "UPR", pr_bits, upr.pr, *data, 1);
        return SCPE_OK;

    case _CPR:
        *data = cpr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "CPR", pr_bits, cpr.pr, *data, 1);
        return SCPE_OK;

    case _IBC:
        *data = ibc.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "IBC", ibc_bits, ibc.pr, *data, 1);
        return SCPE_OK;

    case _UBR:
        ubr.fields.page = ipr.fields.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "IPR", ipr_bits, ipr.pr, ipr.pr, 1);

        *data = ubr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "UBR", ubr_bits, ubr.pr, ubr.pr, 1);
        return SCPE_OK;

    case TC0:
        *data = asic_file[TC0];
        sim_debug(DBG_ASB_R, &cpu_dev, "TC0=0x%X\n", *data);
        return SCPE_OK;

    case TC1:
        *data = asic_file[TC1];
        sim_debug(DBG_ASB_R, &cpu_dev, "TC1=0x%X\n", *data);
        return SCPE_OK;

    case TC2:
        *data = asic_file[TC2];
        sim_debug(DBG_ASB_R, &cpu_dev, "TC2=0x%X\n", *data);
        return SCPE_OK;

    case RX:
        *data = asic_file[RX];
        sim_debug(DBG_ASB_R, &cpu_dev, "RX=0x%X\n", *data);
        return SCPE_OK;

    case RH:
        *data = asic_file[RH];
        sim_debug(DBG_ASB_R, &cpu_dev, "RH=0x%X\n", *data);
        return SCPE_OK;

    default:
        if (offset >= 0x18 && offset <= 0x1F)
        {
            *data = gdata[offset];
            sim_debug(DBG_ASB_R, &cpu_dev, "GA=0x%X GD=0x%X\n", offset, *data);
            return SCPE_OK;
        }
    }

    return SCPE_NXM;
}

t_stat gstore(t_addr offset, t_value data)
{
    /* implementation note: gstore is always invoked after TEST_EXIT */

    switch (offset)
    {
    case I:
        if (EXIT)
        {
            // Performs a Subroutine Return, then pushes the contents of TOP into I
            rs_push(cpr.pr, data);
        }
        else
        {
            // Pops the contents of TOP into I (with no push of the Return Stack)
            set_IPR(cpr.pr);
            asic_file[I] = data & D16_UMAX;
            sim_debug(DBG_ASB_W, &cpu_dev, "I=0x%X\n", asic_file[I]);
        }
        return SCPE_OK;

    case I_ASIC:
        rs_push(cpr.pr, data);
        return SCPE_OK;

    case I_ALU:
        rs_push(cpr.pr, data);
        STREAM = TRUE;
        return SCPE_OK;

    case _CR:
    {
        CR ocr;
        ocr.pr = cr.pr;

        cr.pr = data & 0xC01F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 1);
        return SCPE_OK;
    }

    // Pop the Paramter Stack item into the MD Register, replacing its previous contents
    case MD:
        asic_file[MD] = data & 0xFFFF;
        sim_debug(DBG_ASB_W, &cpu_dev, "MD=0x%X\n", asic_file[MD]);
        return SCPE_OK;

    case SQ:
        asic_file[MD] = data << 8;
        sim_debug(DBG_ASB_W, &cpu_dev, "SQ=0x%X\n", asic_file[MD]);
        return SCPE_OK;

    case SR:
        asic_file[SR] = data;
        sim_debug(DBG_ASB_W, &cpu_dev, "SR=0x%X\n", asic_file[SR]);
        return SCPE_OK;

    case PC:
        // push value onto RS, then perform exit
        if (EXIT)
        {
            asic_file[PC] = data & D16_MASK;
            sim_debug(DBG_ASB_W, &cpu_dev, "PC=0x%X\n", asic_file[PC]);
        }
        // perform a subroutine call
        else
        {
            rs_push(cpr.pr, asic_file[PC]);
            asic_file[PC] = data & D16_MASK;
            sim_debug(DBG_ASB_W, &cpu_dev, "PC=0x%X\n", asic_file[PC]);
        }
        return SCPE_OK;

    case _IMR:
    {
        IMR oimr;
        oimr.pr = imr.pr;

        imr.pr = data & 0x3FFE;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "IMR", imr_bits, oimr.pr, imr.pr, 1);
        return SCPE_OK;
    }

    case _SPR:
    {
        SPR ospr;
        ospr.pr = spr.pr;

        spr.fields.rsp = (data >> 8) & 0x7F;
        spr.fields.psp = data & 0x7F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "SPR", spr_bits, ospr.pr, spr.pr, 1);
        return SCPE_OK;
    }

    case _SUR:
    {
        SUR osur;
        osur.pr = sur.pr;

        sur.pr = data & 0xFBFB; // Bits 2 & 10 MBZ
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "SUR", sur_bits, osur.pr, sur.pr, 1);
        return SCPE_OK;
    }

    case _SVR:
    {
        SVR osvr;
        osvr.pr = svr.pr;

        svr.pr = 0;
        RVL = (data >> 8) & 0x3F;
        PVL = data & 0x3F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "SVR", svr_bits, osvr.pr, svr.pr, 1);
        return SCPE_OK;
    }

    case _IPR:
    {
        IPR oipr;
        oipr.pr = ipr.pr;

        ipr.pr = data & 0x1F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "IPR", ipr_bits, oipr.pr, ipr.pr, 1);
        return SCPE_OK;
    }

    case _DPR:
    {
        DPR odpr;
        odpr.pr = dpr.pr;

        dpr.pr = data & 0x0F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "DPR", pr_bits, odpr.pr, dpr.pr, 1);
        return SCPE_OK;
    }

    case _UPR:
    {
        UPR oupr;
        oupr.pr = upr.pr;

        upr.pr = data & 0x0F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "UPR", pr_bits, oupr.pr, upr.pr, 1);
        return SCPE_OK;
    }

    case _CPR:
    {
        CPR ocpr;
        ocpr.pr = cpr.pr;

        cpr.pr = data & 0x0F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CPR", pr_bits, ocpr.pr, cpr.pr, 1);
        return SCPE_OK;
    }

    case _IBC:
    {
        IBC oibcr;
        oibcr.pr = ibc.pr;

        ibc.pr = data & 0xFFBF;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "IBC", ibc_bits, oibcr.pr, ibc.pr, 1);
        return SCPE_OK;
    }

    case _UBR:
    {
        UBR oubr;
        oubr.pr = ubr.pr;

        ubr.fields.adr = data & 0x3FF;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "UBR", ubr_bits, oubr.pr, ubr.pr, 1);
        return SCPE_OK;
    }

    case TP0:
        asic_file[TP0] = data & D16_MASK;
        sim_debug(DBG_ASB_W, &cpu_dev, "TP0=0x%X\n", asic_file[TP0]);
        return SCPE_OK;
        /* timer side effect? */

    case TP1:
        asic_file[TP1] = data & D16_MASK;
        sim_debug(DBG_ASB_W, &cpu_dev, "TP1=0x%X\n", asic_file[TP1]);
        return SCPE_OK;

    case TP2:
        asic_file[TP2] = data & D16_MASK;
        sim_debug(DBG_ASB_W, &cpu_dev, "TP2=0x%X\n", asic_file[TP2]);
        return SCPE_OK;

    case RX:
        asic_file[RX] = data & D16_MASK;
        sim_debug(DBG_ASB_W, &cpu_dev, "RX=0x%X\n", asic_file[RX]);
        return SCPE_OK;

    case RH:
        asic_file[RH] = data & D16_MASK;
        sim_debug(DBG_ASB_W, &cpu_dev, "RH=0x%X\n", asic_file[RH]);
        return SCPE_OK;

    case 0x19: // XMT
    {
        if (hostmode)
        { /* invoke fsm for serving host mode */
            return host_write(data);
        }
        if (data)
        {
            putchar(data);
            // @TODO: SIMH virtual console write
            return SCPE_OK;
        }
        else
        {
            hostmode = -1;
        }
    }

    default:
        /* off-chip gstores */
        if (offset >= 0x18 && offset <= 0x1F)
        {
            gdata[offset] = data & D16_MASK;
            sim_debug(DBG_ASB_W, &cpu_dev, "GA=0x%X GD=0x%X\n", offset, gdata[offset]);
            return SCPE_OK;
        }
    }

    return SCPE_NXM;
}

t_stat host_write(t_value data)
{
    if (data == 0xFF)
    {
        return SCPE_EXIT;
    }
    hostmode = FALSE;

    return SCPE_OK;
}