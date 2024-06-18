/* vax_2681.h: 2681 DUART Simulator

   Copyright (c) 2011-2013, Matt Burke

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
   THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of the author shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from the author.

   11-Jun-2013  MB      First version
*/

#include "sim_defs.h"

typedef t_stat (*put_char_t)(uint8_t);
typedef t_stat (*get_char_t)(uint8_t*);
typedef void   (*set_int_t)(uint32_t);

struct uart2681_port_t {
    put_char_t  put_char;
    get_char_t  get_char;
    uint32_t      sts;
    uint32_t      cmd;
    uint32_t      mode[2];
    uint32_t      mode_ptr;
    uint32_t      buf;
    };

struct uart2681_t {
    set_int_t   set_int;
    set_int_t   output_port;
    struct uart2681_port_t port[2];
    uint32_t      ists;
    uint32_t      imask;
    uint8_t       iport;
    uint8_t       ipcr;
    uint8_t       oport;
    uint8_t       opcr;
    uint8_t       acr;
    };

typedef struct uart2681_t UART2681;
    
void ua2681_wr (UART2681 *ctx, uint32_t rg, uint32_t data);
uint32_t ua2681_rd (UART2681 *ctx, uint32_t rg);
void ua2681_ip0_wr (UART2681 *ctx, uint32_t set);
void ua2681_ip1_wr (UART2681 *ctx, uint32_t set);
void ua2681_ip2_wr (UART2681 *ctx, uint32_t set);
void ua2681_ip3_wr (UART2681 *ctx, uint32_t set);
t_stat ua2681_svc (UART2681 *ctx);
t_stat ua2681_reset (UART2681 *ctx);
