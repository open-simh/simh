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

/* implementation note: gfetch is always invoked before TEST_EXIT */
t_stat gfetch(t_addr offset, t_value *data)
{
    // return bit=0: Pushes the contents of I into TOP (with no pop of the Return Stack)
    // return bit=1: Pushes the contents of I into TOP then performs a subroutine return
    if (I == offset)
    {
        *data = asic_file[I];
        sim_debug(DBG_ASB_R, &cpu_dev, sim_is_running ? "I=0x%X\n" : "I=0x%X", asic_file[I]);
        return SCPE_OK;
    }

    // return bit=0: Pushes the contents of I into TOP, popping the Return Stack
    // return bit=1: Pushes the contents of I into TOP without popping the Return Stack, then executes the Subroutine Return
    if (I_ASIC == offset)
    {
        *data = asic_file[I];
        sim_debug(DBG_ASB_R, &cpu_dev, sim_is_running ? "I=0x%X\n" : " ", asic_file[I]);
        if (!EXIT)
        {
            rs_pop(data);
        }
        return SCPE_OK;
    }
    // Pushes the contents of I shifted left by one bit, into TOP (the Return Stack is not popped)
    if (I_ALU == offset)
    {
        *data = (asic_file[I] << 1) & D16_UMAX;
        sim_debug(DBG_ASB_R, &cpu_dev, sim_is_running ? "I=0x%X\n" : " ", *data);
        return SCPE_OK;
    }

    if (_CR == offset)
    {
        *data = cr.pr & 0xFFEF; // Force SID to zero

        if (sim_is_running)
            sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "CR", cr_bits, cr.pr, *data, 1);
        return SCPE_OK;
    }

    if (MD == offset)
    {
        *data = asic_file[MD] & D16_UMAX;
        if (sim_is_running)
            sim_debug(DBG_ASB_R, &cpu_dev, "MD=0x%X\n", asic_file[MD]);
        return SCPE_OK;
    }

    if (SQ == offset)
    {
        RTX_WORD sq = asic_file[MD] << 1;
        *data = sq | asic_file[SR];
        sim_debug(DBG_ASB_R, &cpu_dev, "SQ=0x%X SR=0x%X\n", sq, asic_file[SR]);
        return SCPE_OK;
    }

    if (SR == offset)
    {
        *data = asic_file[SR];
        sim_debug(DBG_ASB_R, &cpu_dev, "SR=0x%X\n", asic_file[SR]);
        return SCPE_OK;
    }

    if (PC == offset)
    {
        *data = asic_file[PC];
        sim_debug(DBG_ASB_R, &cpu_dev, "PC=0x%X\n", asic_file[PC]);
        return SCPE_OK;
    }

    if (_IMR == offset)
    {
        *data = imr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "IMR", imr_bits, imr.pr, *data, 1);
        return SCPE_OK;
    }

    if (_SPR == offset)
    {
        *data = (((spr.fields.rsp) << 8) & 0xFF00) | (((spr.fields.psp) + 1) & 0x00FF);
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "SPR", spr_bits, spr.pr, *data, 1);
        return SCPE_OK;
    }

    if (_SUR == offset)
    {
        *data = sur.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "SUR", sur_bits, sur.pr, *data, 1);
        return SCPE_OK;
    }

    if (_IVR == offset)
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

    if (_IPR == offset)
    {
        *data = ipr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "IPR", ipr_bits, ipr.pr, *data, 1);
        return SCPE_OK;
    }

    if (_DPR == offset)
    {
        *data = dpr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "DPR", pr_bits, dpr.pr, *data, 1);
        return SCPE_OK;
    }

    if (_UPR == offset)
    {
        *data = upr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "UPR", pr_bits, upr.pr, *data, 1);
        return SCPE_OK;
    }

    if (_CPR == offset)
    {
        *data = cpr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "CPR", pr_bits, cpr.pr, *data, 1);
        return SCPE_OK;
    }

    if (_IBC == offset)
    {
        *data = ibc.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "IBC", ibc_bits, ibc.pr, *data, 1);
        return SCPE_OK;
    }

    if (_UBR == offset)
    {
        ubr.fields.page = ipr.fields.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "IPR", ipr_bits, ipr.pr, ipr.pr, 1);

        *data = ubr.pr;
        sim_debug_bits_hdr(DBG_ASB_R, &cpu_dev, "UBR", ubr_bits, ubr.pr, ubr.pr, 1);
        return SCPE_OK;
    }

    if (TC0 == offset)
    {
        *data = asic_file[TC0];
        sim_debug(DBG_ASB_R, &cpu_dev, "TC0=0x%X\n", *data);
        return SCPE_OK;
    }

    if (TC1 == offset)
    {
        *data = asic_file[TC1];
        sim_debug(DBG_ASB_R, &cpu_dev, "TC1=0x%X\n", *data);
        return SCPE_OK;
    }

    if (TC2 == offset)
    {
        *data = asic_file[TC2];
        sim_debug(DBG_ASB_R, &cpu_dev, "TC2=0x%X\n", *data);
        return SCPE_OK;
    }

    if (RX == offset)
    {
        *data = asic_file[RX];
        sim_debug(DBG_ASB_R, &cpu_dev, "RX=0x%X\n", *data);
        return SCPE_OK;
    }

    if (RH == offset)
    {
        *data = asic_file[RH];
        sim_debug(DBG_ASB_R, &cpu_dev, "RH=0x%X\n", *data);
        return SCPE_OK;
    }

    if (offset == 0x19)
    {
#if 0
    /* this gets I/O to work for now */
    if (offset == 0x19)
    {
        if (hostmode) /* invoke fsm for serving host mode */
        {
            return (host_read());
        }
        temp = getch() _MASKED_;
        /* printf("\n(%d) = '%c'", temp, temp); */
        return (temp);
    }
    if (offset == 0x1A)
        return (kbhit() _MASKED_);
#endif
        return SCPE_NXM;
    }

    if (offset >= 0x18 && offset <= 0x1F)
    {
        *data = gdata[offset];
        sim_debug(DBG_ASB_R, &cpu_dev, "GA=0x%X GD=0x%X\n", offset, *data);
        return SCPE_OK;
    }

    return SCPE_NXM;
}

t_stat gstore(t_addr offset, t_value data)
{
    /* implementation note: gstore is always invoked after TEST_EXIT */

    if (I == offset)
    {
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
    }

    if (I_ASIC == offset)
    {
        rs_push(cpr.pr, data);
        return SCPE_OK;
    }

    if (I_ALU == offset)
    {
        rs_push(cpr.pr, data);
        STREAM = TRUE;
        /* @TODO: Replace with instruction test during stream instruction execution
        23-Sep All stream instructions are NYI
        Unsigned Mul    Unsigned div
         U*' A89C        U/'     A41A
        Signed Mul       /'      A45A
         *'  A89D        U/1''   A458
         *'' A49D       Sq. Root
                         S1'     A51A
                         S'      A55A
                         S''     A558
        */
        return SCPE_OK;
    }

    if (_CR == offset)
    {
        CR ocr;
        ocr.pr = cr.pr;

        cr.pr = data & 0xC01F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CR", cr_bits, ocr.pr, cr.pr, 1);
        return SCPE_OK;
    }

    // Pop the Paramter Stack item into the MD Register, replacing its previous contents
    if (MD == offset)
    {
        asic_file[MD] = data & 0xFFFF;
        sim_debug(DBG_ASB_W, &cpu_dev, "MD=0x%X\n", asic_file[MD]);
        return SCPE_OK;
    }

    if (SQ == offset)
    {
        asic_file[MD] = data << 8;
        sim_debug(DBG_ASB_W, &cpu_dev, "SQ=0x%X\n", asic_file[MD]);
        return SCPE_OK;
    }

    if (SR == offset)
    {
        asic_file[SR] = data;
        sim_debug(DBG_ASB_W, &cpu_dev, "SR=0x%X\n", asic_file[SR]);
        return SCPE_OK;
    }

    if (PC == offset)
    {
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
    }

    if (_IMR == offset)
    {
        IMR oimr;
        oimr.pr = imr.pr;

        imr.pr = data & 0x3FFE;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "IMR", imr_bits, oimr.pr, imr.pr, 1);
        return SCPE_OK;
    }

    if (_SPR == offset)
    {
        SPR ospr;
        ospr.pr = spr.pr;

        spr.fields.rsp = (data >> 8) & 0x7F;
        spr.fields.psp = data & 0x7F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "SPR", spr_bits, ospr.pr, spr.pr, 1);
        return SCPE_OK;
    }

    if (_SUR == offset)
    {
        SUR osur;
        osur.pr = sur.pr;

        sur.pr = data & 0xFBFB; // Bits 2 & 10 MBZ
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "SUR", sur_bits, osur.pr, sur.pr, 1);
        return SCPE_OK;
    }

    if (_SVR == offset)
    {
        SVR osvr;
        osvr.pr = svr.pr;

        svr.pr = 0;
        RVL = (data >> 8) & 0x3F;
        PVL = data & 0x3F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "SVR", svr_bits, osvr.pr, svr.pr, 1);
        return SCPE_OK;
    }

    if (_IPR == offset)
    {
        IPR oipr;
        oipr.pr = ipr.pr;

        ipr.pr = data & 0x1F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "IPR", ipr_bits, oipr.pr, ipr.pr, 1);
        return SCPE_OK;
    }

    if (_DPR == offset)
    {
        DPR odpr;
        odpr.pr = dpr.pr;

        dpr.pr = data & 0x0F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "DPR", pr_bits, odpr.pr, dpr.pr, 1);
        return SCPE_OK;
    }

    if (_UPR == offset)
    {
        UPR oupr;
        oupr.pr = upr.pr;

        upr.pr = data & 0x0F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "UPR", pr_bits, oupr.pr, upr.pr, 1);
        return SCPE_OK;
    }

    if (_CPR == offset)
    {
        CPR ocpr;
        ocpr.pr = cpr.pr;

        cpr.pr = data & 0x0F;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "CPR", pr_bits, ocpr.pr, cpr.pr, 1);
        return SCPE_OK;
    }

    if (_IBC == offset)
    {
        IBC oibcr;
        oibcr.pr = ibc.pr;

        ibc.pr = data & 0xFFBF;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "IBC", ibc_bits, oibcr.pr, ibc.pr, 1);
        return SCPE_OK;
    }

    if (_UBR == offset)
    {
        UBR oubr;
        oubr.pr = ubr.pr;

        ubr.fields.adr = data & 0x3FF;
        sim_debug_bits_hdr(DBG_ASB_W, &cpu_dev, "UBR", ubr_bits, oubr.pr, ubr.pr, 1);
        return SCPE_OK;
    }

    if (0x12 == offset)
    {
        return SCPE_NXM;
    }

    if (TP0 == offset)
    {
        asic_file[TP0] = data & D16_MASK;
        sim_debug(DBG_ASB_W, &cpu_dev, "TP0=0x%X\n", asic_file[TP0]);
        return SCPE_OK;
    } /* timer side effect? */

    if (TP1 == offset)
    {
        asic_file[TP1] = data & D16_MASK;
        sim_debug(DBG_ASB_W, &cpu_dev, "TP1=0x%X\n", asic_file[TP1]);
        return SCPE_OK;
    }

    if (TP2 == offset)
    {
        asic_file[TP2] = data & D16_MASK;
        sim_debug(DBG_ASB_W, &cpu_dev, "TP2=0x%X\n", asic_file[TP2]);
        return SCPE_OK;
    }

    if (RX == offset)
    {
        asic_file[RX] = data & D16_MASK;
        sim_debug(DBG_ASB_W, &cpu_dev, "RX=0x%X\n", asic_file[RX]);
        return SCPE_OK;
    }

    if (RH == offset)
    {
        asic_file[RH] = data & D16_MASK;
        sim_debug(DBG_ASB_W, &cpu_dev, "RH=0x%X\n", asic_file[RH]);
        return SCPE_OK;
    }

    /* off-chip gstores */
    if (0x19 == offset)
    {
        // if (hostmode) /* invoke fsm for serving host mode */
        // {
        // @TODO: SIMH virtual file write
        // host_write(data);
        // return;
        // }
        if (data)
        {
            // @TODO: SIMH virtual console write
            putchar(data);
            return SCPE_OK;
        }
        //         else
        //         {
        //             hostmode = -1;
        //         }
        // #endif
        return SCPE_NXM;
    }

    if (offset >= 0x18 && offset <= 0x1F)
    {
        gdata[offset] = data & D16_MASK;
        sim_debug(DBG_ASB_W, &cpu_dev, "GA=0x%X GD=0x%X\n", offset, gdata[offset]);
        return SCPE_OK;
    }

    return SCPE_NXM;
}
