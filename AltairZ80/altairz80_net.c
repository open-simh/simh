/*  altairz80_net.c: networking capability

    Copyright (c) 2002-2014, Peter Schorn

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
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    PETER SCHORN BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    Except as contained in this notice, the name of Peter Schorn shall not
    be used in advertising or otherwise to promote the sale, use or other dealings
    in this Software without prior written authorization from Peter Schorn.
*/

#include "altairz80_defs.h"
#include "sim_tmxr.h"

/* Debug flags */
#define ACCEPT_MSG  (1 << 0)
#define DROP_MSG    (1 << 1)
#define IN_MSG      (1 << 2)
#define OUT_MSG     (1 << 3)

extern uint32 PCX;

#define UNIT_V_SERVER   (UNIT_V_UF + 0) /* define machine as a server   */
#define UNIT_SERVER     (1 << UNIT_V_SERVER)
#define NET_INIT_POLL_SERVER  16000
#define NET_INIT_POLL_CLIENT  15000

static t_stat net_attach    (UNIT *uptr, CONST char *cptr);
static t_stat net_detach    (UNIT *uptr);
static t_stat net_reset     (DEVICE *dptr);
static t_stat net_svc       (UNIT *uptr);
static t_stat set_net       (UNIT *uptr, int32 value, CONST char *cptr, void *desc);
int32 netStatus             (const int32 port, const int32 io, const int32 data);
int32 netData               (const int32 port, const int32 io, const int32 data);
static const char* net_description(DEVICE *dptr);

extern uint32 sim_map_resource(uint32 baseaddr, uint32 size, uint32 resource_type,
                               int32 (*routine)(const int32, const int32, const int32), const char* name, uint8 unmap);

#define MAX_CONNECTIONS 2   /* maximal number of server connections */

TMLN net_ldsc[MAX_CONNECTIONS];
TMXR net_desc = {MAX_CONNECTIONS, 0, 0, net_ldsc};

static struct {
    int32   Z80StatusPort;              /* Z80 status port associated with this ioSocket, read only             */
    int32   Z80DataPort;                /* Z80 data port associated with this ioSocket, read only               */
    int32   line;                       /* tmxr line */
    char    lastChar;                   /* last character received */
} serviceDescriptor[MAX_CONNECTIONS + 1] = {    /* serviceDescriptor[0] holds the information for a client      */
/*  stat    dat ms  ios in      inPR    inPW    inS out     outPR   outPW   outS */
    {0x32,  0x33}, /* client Z80 port 50 and 51  */
    {0x28,  0x29}, /* server Z80 port 40 and 41  */
    {0x2a,  0x2b}  /* server Z80 port 42 and 43  */
};

static UNIT net_unit = {
    UDATA (&net_svc, UNIT_ATTABLE, 0),
    0,  /* wait, set in attach  */
    0,  /* u3, unused           */
    0,  /* u4, unused           */
    0,  /* u5, unused           */
    0,  /* u6, unused           */
};

static REG net_reg[] = {
    { DRDATAD (POLL, net_unit.wait,  32, "Polling interval") },
    { NULL }
};

static const char* net_description(DEVICE *dptr) {
    return "Network";
}

static MTAB net_mod[] = {
    { UNIT_SERVER, 0,           "CLIENT", "CLIENT", &set_net, NULL, NULL,
        "Sets machine to client mode"}, /* machine is a client   */
    { UNIT_SERVER, UNIT_SERVER, "SERVER", "SERVER", &set_net, NULL, NULL,
        "Sets machine to server mode"}, /* machine is a server   */
    { 0 }
};

/* Debug Flags */
static DEBTAB net_dt[] = {
    { "ACCEPT", ACCEPT_MSG, "Accept messages"   },
    { "DROP",   DROP_MSG,   "Drop messages"     },
    { "IN",     IN_MSG,     "Incoming messages" },
    { "OUT",    OUT_MSG,    "Outgoing messages" },
    { NULL,     0                               }
};

DEVICE net_dev = {
    "NET", &net_unit, net_reg, net_mod,
    1, 10, 31, 1, 8, 8,
    NULL, NULL, &net_reset,
    NULL, &net_attach, &net_detach,
    NULL, (DEV_DISABLE | DEV_DEBUG), 0,
    net_dt, NULL, NULL, NULL, NULL, NULL, &net_description
};

static t_stat set_net(UNIT *uptr, int32 value, CONST char *cptr, void *desc) {
    char temp[CBUFSIZE];
    if ((net_unit.flags & UNIT_ATT) && ((net_unit.flags & UNIT_SERVER) != (uint32)value)) {
        strncpy(temp, net_unit.filename, CBUFSIZE - 1); /* save name for later attach */
        net_detach(&net_unit);
        net_unit.flags ^= UNIT_SERVER; /* now switch from client to server and vice versa */
        net_attach(uptr, temp);
        return SCPE_OK;
    }
    return SCPE_OK;
}

static void serviceDescriptor_reset(const uint32 i) {
    tmxr_reset_ln (&net_ldsc[serviceDescriptor[i].line]);
    serviceDescriptor[i].lastChar = 0;
}

static t_stat net_reset(DEVICE *dptr) {
    uint32 i;
    if (net_unit.flags & UNIT_ATT)
        sim_activate(&net_unit, net_unit.wait); /* start poll */
    for (i = 0; i <= MAX_CONNECTIONS; i++) {
        serviceDescriptor_reset(i);
        sim_map_resource(serviceDescriptor[i].Z80StatusPort, 1,
                         RESOURCE_TYPE_IO, &netStatus, "netStatus", dptr->flags & DEV_DIS);
        sim_map_resource(serviceDescriptor[i].Z80DataPort, 1,
                         RESOURCE_TYPE_IO, &netData, "netData", dptr->flags & DEV_DIS);
    }
    return SCPE_OK;
}

static t_stat net_attach(UNIT *uptr, CONST char *cptr) {
    net_detach (uptr);
    net_reset(&net_dev);
    net_desc.notelnet = TRUE;
    if (net_unit.flags & UNIT_SERVER) {
        int32 i;
        for (i = 0; i < MAX_CONNECTIONS; i++)
            serviceDescriptor[i].line = i;
        net_unit.wait = NET_INIT_POLL_SERVER;
    } else {
        char connection[CBUFSIZE];

        net_unit.wait = NET_INIT_POLL_CLIENT;
        snprintf (connection, sizeof (connection), "Line=0, Connect=%s", cptr);
        cptr = connection;
    }
    return tmxr_attach (&net_desc, uptr, cptr);
}

static t_stat net_detach(UNIT *uptr) {
    if (!(net_unit.flags & UNIT_ATT))
        return SCPE_OK;       /* if not attached simply return */
    return tmxr_detach (&net_desc, uptr);
}

static t_stat net_svc(UNIT *uptr) {
    int32 newln;
    if (net_unit.flags & UNIT_ATT) {
        newln = tmxr_poll_conn (&net_desc);         /* check for new connection */
        if (newln >= 0) {
            net_desc.ldsc[newln].rcve = 1;
            sim_debug(ACCEPT_MSG, &net_dev, "NET: " ADDRESS_FORMAT "  connection established with %s %i\n", PCX, net_ldsc[newln].ipad, newln);
        }
        tmxr_poll_rx (&net_desc);                   /* get recieved data */
        tmxr_poll_tx (&net_desc);                   /* send any pending output data */
        sim_activate(&net_unit, net_unit.wait);     /* continue poll */\
    }
    return SCPE_OK;
}

int32 netStatus(const int32 port, const int32 io, const int32 data) {
    uint32 i;
    if ((net_unit.flags & UNIT_ATT) == 0)
        return 0;
    net_svc(&net_unit);
    if (io == 0)    /* IN   */
        for (i = 0; i <= MAX_CONNECTIONS; i++)
            if (serviceDescriptor[i].Z80StatusPort == port) {
                int32 line = serviceDescriptor[i].line;
                return ((tmxr_rqln (&net_ldsc[line]) > 0) ? 1 : 0) |
                       ((tmxr_tqln (&net_ldsc[line]) < net_ldsc[line].txbsz) ? 2 : 0);
            }
    return 0;
}

int32 netData(const int32 port, const int32 io, const int32 data) {
    uint32 i;
    int32 result;
    if ((net_unit.flags & UNIT_ATT) == 0)
        return 0;
    net_svc(&net_unit);
    for (i = 0; i <= MAX_CONNECTIONS; i++)
        if (serviceDescriptor[i].Z80DataPort == port) {
            if (io == 0) {  /* IN   */
                result = tmxr_getc_ln (&net_ldsc[serviceDescriptor[i].line]);
                if (result == 0) {
                    sim_printf("re-read from %i\n", port);
                    result = serviceDescriptor[i].lastChar;
                } else {
                    result &= ~TMXR_VALID;
                    serviceDescriptor[i].lastChar = result;
                }
                sim_debug(IN_MSG, &net_dev, "NET: " ADDRESS_FORMAT "  IN(%i)=%03xh (%c)\n", PCX, port, (result & 0xff), (32 <= (result & 0xff)) && ((result & 0xff) <= 127) ? (result & 0xff) : '?');
                return result;
            } else {          /* OUT  */
                result = tmxr_putc_ln (&net_ldsc[serviceDescriptor[i].line], data);
                if (result == SCPE_STALL) {
                    sim_printf("buffer full %i to %i ignored\n", data, port);
                } else {
                    if (result == SCPE_LOST)
                        sim_printf("data dropped (no connection) %i to %i\n", data, port);
                }
                sim_debug(OUT_MSG, &net_dev, "NET: " ADDRESS_FORMAT " OUT(%i)=%03xh (%c)\n", PCX, port, data, (32 <= data) && (data <= 127) ? data : '?');
                tmxr_send_buffered_data (&net_ldsc[serviceDescriptor[i].line]);
                return 0;
            }
        }
    return 0;
}
