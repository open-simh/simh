/* sim_slirp.c:
  ------------------------------------------------------------------------------
   Copyright (c) 2015, Mark Pizzolato
   Copyright (c) 2024, B. Scott Michel

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

  ------------------------------------------------------------------------------

   This module provides the interface needed between sim_ether and libslirp to
   provide NAT network functionality.

*/
#define SIMH_IP_NETWORK    0x0a000200  /* aka 10.0.2.0 */
#define SIMH_IP_NETMASK    0xffffff00  /* aka 255.255.255.0 */
#define SIMH_GATEWAY_ADDR  0x0a000202  /* aka 10.0.2.2 (SIMH's address) */
#define SIMH_RESOLVER_ADDR 0x0a000203  /* aka 10.0.2.3 (DNS resolver) */

/* IPv6 private address range (ULA) blessed by IANA */
#define SIMH_IP6_NETWORK   "fd00:cafe:dead:beef::"
#define SIMH_IP6_PREFIX_LEN 64
#define SIMH_GW_ADDR6      "fd00:cafe:dead:beef::2"   /* SIMH's IPv6 address */

#include <glib.h>
#include <errno.h>                  /* Paranoia for Win32/64 */

#include "libslirp.h"
#include "sim_defs.h"
#include "scp.h"
#include "sim_sock.h"
#include "sim_slirp_network.h"
#include "sim_printf_fmts.h"

#if !defined(_WIN32)
#  define simh_inet_aton inet_aton
#else
#  define simh_inet_aton slirp_inet_aton
#  if WINVER < 0x0601
#    undef inet_pton
#    define inet_pton sim_inet_pton
#  endif
#endif

#define IS_TCP 0
#define IS_UDP 1

static const char *tcpudp[] = {"TCP", "UDP"};

/* Additional debugging flags added to the device's debug table. */
DEBTAB slirp_dbgtable[] = {
    { "POLL", 0, "Show libslirp polling callback activity" },
    { "SOCKET", 0, "Show libslirp socket registration activity" },
    { NULL }
};

/*~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 * Port redirection management:
 *~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/

static int parse_redirect_port(struct redir_tcp_udp **head, const char *buff, int is_udp)
{
    char gbuf[4 * CBUFSIZE];
    uint32 inaddr = INADDR_ANY;
    int port = 0;
    int lport = 0;
    char *ipaddrstr = NULL;
    char *portstr = NULL;
    struct redir_tcp_udp *newp;

    gbuf[sizeof(gbuf) - 1] = '\0';
    strncpy(gbuf, buff, sizeof(gbuf) - 1);
    if (((ipaddrstr = strchr(gbuf, ':')) == NULL) || (*(ipaddrstr + 1) == 0)) {
        sim_printf("redir %s syntax error\n", tcpudp[is_udp]);
        return -1;
    }
    *ipaddrstr++ = 0;

    if ((ipaddrstr) && (((portstr = strchr(ipaddrstr, ':')) == NULL) || (*(portstr + 1) == 0))) {
        sim_printf("redir %s syntax error\n", tcpudp[is_udp]);
        return -1;
    }
    *portstr++ = 0;

    sscanf(gbuf, "%d", &lport);
    sscanf(portstr, "%d", &port);
    if (ipaddrstr != NULL) {
        struct in_addr addr;

        if (simh_inet_aton(ipaddrstr, &addr)) {
            inaddr = addr.s_addr;
        } 
    }

    if (inaddr == INADDR_ANY) {
        sim_printf("%s redirection error: an IP address must be specified\n", tcpudp[is_udp]);
        return -1;
    }

    if ((newp = (struct redir_tcp_udp *) calloc(1, sizeof(struct redir_tcp_udp))) == NULL)
        return -1;

    newp->is_udp = is_udp;
    newp->simh_host_port = port;
    simh_inet_aton(ipaddrstr, &newp->sim_local_inaddr);
    newp->sim_local_port = lport;
    newp->next = *head;
    *head = newp;
    return 0;
}

/* do_redirects: Adds the proxied (forwarded) ports from the guest network to the
 * outside network. */
static int do_redirects(SimSlirpNetwork *slirp, struct redir_tcp_udp *head)
{
    struct in_addr host_addr;
    int ret = 0;

    host_addr.s_addr = htonl(INADDR_ANY);
    if (head != NULL) {
        ret = do_redirects(slirp, head->next);
        if (slirp_add_hostfwd(slirp->slirp_cxn, head->is_udp, host_addr, head->sim_local_port,
                              head->sim_local_inaddr, head->simh_host_port) < 0) {
            sim_printf("Can't establish redirector for: redir %s   =%d:%s:%d\n", tcpudp[head->is_udp],
                       head->sim_local_port, sim_inet_ntoa4(&head->sim_local_inaddr),
                       head->simh_host_port);
            ++ret;
        }
    }
    return ret;
}

static unsigned int collect_slirp_debug(const char *dbg_tokens, int *err)
{
    unsigned int slirp_dbg = 0;

    while (dbg_tokens != NULL && *dbg_tokens != '\0' && *err == 0) {
        if (!strncasecmp(dbg_tokens, "CALL", 4)) {
            slirp_dbg |= SLIRP_DBG_CALL;
        } else if (!strncasecmp(dbg_tokens, "MISC", 4)) {
            slirp_dbg |= SLIRP_DBG_MISC;
        } else if (!strncasecmp(dbg_tokens, "ERROR", 5)) {
            slirp_dbg |= SLIRP_DBG_ERROR;
        } else if (!strncasecmp(dbg_tokens, "VERBOSE_CALL", 13)) {
            slirp_dbg |= SLIRP_DBG_VERBOSE_CALL;
        } else if (!strncasecmp(dbg_tokens, "ALL", 3)) {
            slirp_dbg |= (SLIRP_DBG_CALL | SLIRP_DBG_MISC | SLIRP_DBG_ERROR | SLIRP_DBG_VERBOSE_CALL);
        } else
            *err = 1;

        if (*err == 0) {
            dbg_tokens = strchr(dbg_tokens, ';');
            if (dbg_tokens != NULL)
                ++dbg_tokens;
        }
    }

    return slirp_dbg;
}

#if defined(GLIB_H_MINIMAL)
/* SIMH uses a custom logger to output libslirp messages. */

static void output_stderr_noexit(const char *fmt, va_list args)
{
    char stackbuf[256];
    char *buf = stackbuf;
    size_t bufsize = sizeof(stackbuf);
    int vlen;

    do {
        vlen = vsnprintf(buf, bufsize, fmt, args);
        if (vlen < 0 || ((size_t) vlen) >= bufsize - 2) {
            /* Reallocate a larger buffer... */
            if (buf != stackbuf)
                free(buf);
            bufsize *= 2;
            if (bufsize < (size_t) (vlen + 2))
                bufsize = (size_t) (vlen + 2);
            buf = (char *) calloc(bufsize, sizeof(char));
            if (buf == NULL)
                return;
        }
    } while (vlen < 0 || (size_t) vlen >= bufsize - 1);

    buf[vlen++] = '\n';
    buf[vlen] = '\0';

    sim_misc_debug("libslirp", "trace", buf);

    if (buf != stackbuf)
        free(buf);
}

static void output_stderr_exit(const char *fmt, va_list args)
{
    output_stderr_noexit(fmt, args);
    exit(1);
    g_assert_not_reached();
}

static GLibLogger simh_logger = {
    .logging_enabled = 1,
    .output_warning = output_stderr_noexit,
    .output_debug = output_stderr_noexit,
    .output_error = output_stderr_exit,
    .output_critical = output_stderr_exit
};
#endif

static void libslirp_guest_error(const char *msg, void *opaque)
{
    /* SimSlirpNetwork *slirp = (SimSlirpNetwork *) opaque; */
    /* Avoid unused parameter warning */
    SIM_UNUSED_ARG(opaque);

    if (sim_deb != NULL) {
        fprintf(sim_deb, "libslirp guest error: %s\n", msg);
        fflush(sim_deb);
    }

    fprintf(stderr, "libslirp guest error: %s\n", msg);
    fflush(stderr);
}

/* Forward decl's... */
static int initialize_poll_fds(SimSlirpNetwork *slirp);
static slirp_ssize_t invoke_sim_packet_callback(const void *buf, size_t len, void *opaque);
static void notify_callback(void *opaque);

SimSlirpNetwork *sim_slirp_open(const char *args, void *pkt_opaque, packet_callback pkt_callback, DEVICE *dptr, uint32 dbit,
                                char *errbuf, size_t errbuf_size)
{
    SimSlirpNetwork  *slirp = (SimSlirpNetwork *) calloc(1, sizeof(*slirp));
    SlirpConfig     *cfg = &slirp->slirp_config;
    SlirpCb         *cbs = &slirp->slirp_callbacks;

    char            *targs = strdup(args);
    const char      *tptr = targs;
    const char      *cptr;
    char             tbuf[CBUFSIZE], gbuf[CBUFSIZE], abuf[CBUFSIZE];
    int              err;
    struct in6_addr  default_ipv6_prefix, default_ipv6_gw;

    /* Default IPv6 address -- FIXME */
    inet_pton(AF_INET6, SIMH_IP6_NETWORK, &default_ipv6_prefix);
    inet_pton(AF_INET6, SIMH_GW_ADDR6, &default_ipv6_gw);

    /* Version 1 config */
    cfg->version = SLIRP_CONFIG_VERSION_MAX;
    cfg->restricted = 0;
    cfg->in_enabled = 1;
    cfg->vnetwork.s_addr = htonl(SIMH_IP_NETWORK);
    cfg->vnetmask.s_addr = htonl(SIMH_IP_NETMASK);
    cfg->vhost.s_addr = htonl(SIMH_GATEWAY_ADDR);
    cfg->in6_enabled = 0;
    cfg->vprefix_addr6 = default_ipv6_prefix;
    cfg->vprefix_len = SIMH_IP6_PREFIX_LEN;
    cfg->vhost6 = default_ipv6_gw;
    cfg->vnameserver.s_addr = htonl(SIMH_RESOLVER_ADDR);

    /* Version 2 config: nothing used */
    /* Version 3 config: */
    cfg->disable_dns = 0;

    /* Version 4 config */
    /* DHCP enabled by default. */
    cfg->disable_dhcp = 0;

    /* Version 5 config: nothing used */

    /* SIMH state/config */
    slirp->args = (char *) calloc(1 + strlen(args), sizeof(char));
    strcpy(slirp->args, args);
#if defined(USE_READER_THREAD)
    pthread_mutex_init(&slirp->libslirp_lock, NULL);
    pthread_cond_init(&slirp->no_sockets_cv, NULL);
    pthread_mutex_init(&slirp->no_sockets_lock, NULL);
#endif
    sim_atomic_init(&slirp->n_sockets);

    slirp->original_debflags = dptr->debflags;
    dptr->debflags = sim_combine_debtabs(dptr->debflags, slirp_dbgtable);
    sim_fill_debtab_flags(dptr->debflags);
    slirp->flag_offset = sim_debtab_nelems(slirp->original_debflags);

    /* Parse through arguments... */
    err = 0;
    while (*tptr && !err) {
        tptr = get_glyph_nc(tptr, tbuf, ',');
        if (!tbuf[0])
            break;
        cptr = tbuf;
        cptr = get_glyph(cptr, gbuf, '=');
        if (0 == MATCH_CMD(gbuf, "DHCP")) {
            cfg->disable_dhcp = 0;
            if (cptr != NULL && *cptr != '\0')
                simh_inet_aton(cptr, &cfg->vdhcp_start);
            continue;
        }
        if (0 == MATCH_CMD(gbuf, "TFTP")) {
            if (cptr != NULL && *cptr != '\0')
                slirp->the_tftp_path = strdup(cptr);
            else {
                strlcpy(errbuf, "Missing TFTP Path", errbuf_size);
                err = 1;
            }
            continue;
        }
        if (0 == MATCH_CMD(gbuf, "BOOTFILE")) {
            if (cptr && *cptr)
                slirp->the_bootfile = strdup(cptr);
            else {
                strlcpy(errbuf, "Missing DHCP Boot file name", errbuf_size);
                err = 1;
            }
            continue;
        }
        if ((0 == MATCH_CMD(gbuf, "NAMESERVER")) || (0 == MATCH_CMD(gbuf, "DNS"))) {
            if (cptr && *cptr)
                simh_inet_aton(cptr, &cfg->vnameserver);
            else {
                strlcpy(errbuf, "Missing nameserver", errbuf_size);
                err = 1;
            }
            continue;
        }
        if (0 == MATCH_CMD(gbuf, "DNSSEARCH")) {
            if (cptr != NULL && *cptr) {
                int count = 0;
                char *name;

                slirp->dns_search = strdup(cptr);
                name = slirp->dns_search;
                do {
                    ++count;
                    slirp->dns_search_domains = (char **)realloc(cfg->vdnssearch, (count + 1) * sizeof(char *));
                    slirp->dns_search_domains[count] = NULL;
                    slirp->dns_search_domains[count - 1] = name;
                    if (NULL != (name = strchr(name, ','))) {
                        *name = '\0';
                        ++name;
                    }
                } while (NULL != name && *name);
            } else {
                strlcpy(errbuf, "Missing DNS search list", errbuf_size);
                err = 1;
            }
            continue;
        }
        if (0 == MATCH_CMD(gbuf, "GATEWAY") || 0 == MATCH_CMD(gbuf, "GW")) {
            if (cptr && *cptr) {
                cptr = get_glyph(cptr, abuf, '/');
                if (cptr && *cptr)
                    cfg->vnetmask.s_addr = htonl(~((1 << (32 - atoi(cptr))) - 1));
                simh_inet_aton(abuf, &cfg->vhost);
            } else {
                strlcpy(errbuf, "Missing host", errbuf_size);
                err = 1;
            }
            continue;
        }
        if (0 == MATCH_CMD(gbuf, "NETWORK")) {
            if (cptr && *cptr) {
                cptr = get_glyph(cptr, abuf, '/');
                if (cptr && *cptr)
                    cfg->vnetmask.s_addr = htonl(~((1 << (32 - atoi(cptr))) - 1));
                simh_inet_aton(abuf, &cfg->vnetwork);
            } else {
                strlcpy(errbuf, "Missing network", errbuf_size);
                err = 1;
            }
            continue;
        }
        if (0 == MATCH_CMD(gbuf, "NODHCP")) {
            cfg->disable_dhcp = 1;
            continue;
        }
        if (0 == MATCH_CMD(gbuf, "UDP")) {
            if (cptr && *cptr)
                err = parse_redirect_port(&slirp->rtcp, cptr, IS_UDP);
            else {
                strlcpy(errbuf, "Missing UDP port mapping", errbuf_size);
                err = 1;
            }
            continue;
        }
        if (0 == MATCH_CMD(gbuf, "TCP")) {
            if (cptr && *cptr)
                err = parse_redirect_port(&slirp->rtcp, cptr, IS_TCP);
            else {
                strlcpy(errbuf, "Missing TCP port mapping", errbuf_size);
                err = 1;
            }
            continue;
        }
        if (0 == MATCH_CMD(gbuf, "IPV6")) {
            cfg->in6_enabled = 1;
            continue;
        }
        if (0 == MATCH_CMD(gbuf, "SLIRP")) {
            unsigned int slirp_dbg = collect_slirp_debug(cptr, &err);

            if (!err) {
                slirp_set_debug(slirp_dbg);
                continue;
            }
        }
        if (0 == MATCH_CMD(gbuf, "NOSLIRP")) {
            unsigned int slirp_dbg = collect_slirp_debug(cptr, &err);

            if (!err) {
                slirp_reset_debug(slirp_dbg);
                continue;
            }
        }
        snprintf(errbuf, errbuf_size - 1, "Unexpected NAT argument: %s", gbuf);
        err = 1;
    }

    if (err) {
        sim_slirp_close(slirp);
        free(targs);
        return NULL;
    }

    /* Adjust the network prefix, update the guest's configuration. */
    cfg->vnetwork.s_addr = cfg->vhost.s_addr & cfg->vnetmask.s_addr;
    if ((cfg->vhost.s_addr & ~cfg->vnetmask.s_addr) == 0)
        cfg->vhost.s_addr = htonl(ntohl(cfg->vnetwork.s_addr) | 2);
    if (cfg->vdhcp_start.s_addr == 0)
        cfg->vdhcp_start.s_addr = htonl(ntohl(cfg->vnetwork.s_addr) | 15);
    if (cfg->vnameserver.s_addr == 0)
        cfg->vnameserver.s_addr = htonl(ntohl(cfg->vnetwork.s_addr) | 3);

    /* Set the DNS search domains */
    cfg->vdnssearch = (const char **) slirp->dns_search_domains;

    /* Set the BOOTP file and TFTP path in the Slirp config: */
    cfg->bootfile = slirp->the_bootfile;
    cfg->tftp_path = slirp->the_tftp_path;

    /* Initialize the callbacks: */
    slirp->pkt_callback = pkt_callback;
    slirp->pkt_opaque = pkt_opaque;
#if defined(GLIB_H_MINIMAL)
    glib_set_logging_hooks(&simh_logger);
#endif

    cbs->send_packet = invoke_sim_packet_callback;
    cbs->guest_error = libslirp_guest_error;
    cbs->clock_get_ns = sim_clock_get_ns;
    cbs->register_poll_socket = register_poll_socket;
    cbs->unregister_poll_socket = unregister_poll_socket;
    cbs->notify = notify_callback;
    cbs->timer_mod = simh_timer_mod;
    cbs->timer_free = simh_timer_free;
    cbs->timer_new_opaque = simh_timer_new_opaque;

    slirp->slirp_cxn = slirp_new(cfg, cbs, (void *) slirp);

    /* Capture the debugging info. */
    slirp->dbit = dptr->dctrl = dbit;
    slirp->dptr = dptr;

    initialize_poll_fds(slirp);

    if (do_redirects(slirp, slirp->rtcp)) {
        sim_slirp_close(slirp);
        slirp = NULL;
    } else {
        sim_slirp_show(slirp, stdout);
        if (sim_log != NULL && sim_log != stdout)
            sim_slirp_show(slirp, sim_log);
        if (sim_deb != NULL && sim_deb != stdout && sim_deb != sim_log)
            sim_slirp_show(slirp, sim_deb);
    }

    free(targs);
    return slirp;
}

void sim_slirp_shutdown(void *opaque)
{
#if defined(USE_READER_THREAD)
    SimSlirpNetwork *slirp = (SimSlirpNetwork *) opaque;
    volatile sim_atomic_type_t n_sockets = sim_atomic_get(&slirp->n_sockets);

    /* Set the reader thread's exit condition. If the reader thread is waiting
     * on the condvar, signal the condition. */
    sim_atomic_put(&slirp->n_sockets, -1);
    if (n_sockets == 0)
        pthread_cond_broadcast(&slirp->no_sockets_cv);
#else
    SIM_UNUSED_ARG(opaque);
#endif
}

void sim_slirp_close(SimSlirpNetwork *slirp)
{
    if (slirp == NULL)
        return;

    free(slirp->args);
    free(slirp->the_tftp_path);
    free(slirp->the_bootfile);
    free(slirp->dns_search);
    free(slirp->dns_search_domains);

    if (slirp->slirp_cxn != NULL) {
        struct redir_tcp_udp *rtmp, *rnext;

        for (rtmp = rnext = slirp->rtcp; rtmp != NULL; rtmp = rnext) {
            slirp_remove_hostfwd(slirp->slirp_cxn, rtmp->is_udp, rtmp->sim_local_inaddr, rtmp->sim_local_port);
            rnext = rtmp->next;
            free(rtmp);
        }

        slirp->rtcp = NULL;
        slirp_cleanup(slirp->slirp_cxn);
        slirp->slirp_cxn = NULL;
    }

    if (slirp->dptr != NULL) {
        free(slirp->dptr->debflags);
        slirp->dptr->debflags = slirp->original_debflags;
    }

#if SIM_USE_SELECT
    free(slirp->lut);
    slirp->lut = NULL;
#elif SIM_USE_POLL
    free(slirp->fds);
    slirp->fds = NULL;
#endif

#if defined(USE_READER_THREAD)
    pthread_mutex_destroy(&slirp->libslirp_lock);
    sim_atomic_destroy(&slirp->n_sockets);
    pthread_cond_destroy(&slirp->no_sockets_cv);
    pthread_mutex_destroy(&slirp->no_sockets_lock);
    sim_atomic_destroy(&slirp->n_sockets);
#endif

    free(slirp);
}

t_stat sim_slirp_attach_help(FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
    SIM_UNUSED_ARG(dptr);
    SIM_UNUSED_ARG(uptr);
    SIM_UNUSED_ARG(flag);
    SIM_UNUSED_ARG(cptr);

    fprintf(st, "%s",
            "NAT options:\n"
            "    DHCP{=dhcp_start_address}           Enables DHCP server and specifies\n"
            "                                        guest LAN DHCP start IP address\n"
            "    BOOTFILE=bootfilename               specifies DHCP returned Boot Filename\n"
            "    TFTP=tftp-base-path                 Enables TFTP server and specifies\n"
            "                                        base file path\n"
            "    NAMESERVER=nameserver_ipaddres      specifies DHCP nameserver IP address\n"
            "    DNS=nameserver_ipaddres             specifies DHCP nameserver IP address\n"
            "    DNSSEARCH=domain{:domain{:domain}}  specifies DNS Domains search suffixes\n"
            "    GATEWAY=host_ipaddress{/masklen}    specifies LAN gateway IP address\n"
            "    NETWORK=network_ipaddress{/masklen} specifies LAN network address\n"
            "    UDP=port:address:address's-port     maps host UDP port to guest port\n"
            "    TCP=port:address:address's-port     maps host TCP port to guest port\n"
            "    NODHCP                              disables DHCP server\n\n"
            "Default NAT Options: GATEWAY=10.0.2.2, masklen=24(netmask is 255.255.255.0)\n"
            "                     DHCP=10.0.2.15, NAMESERVER=10.0.2.3\n"
            "    Nameserver defaults to proxy traffic to host system's active nameserver\n\n"
            "The 'address' field in the UDP and TCP port mappings are the simulated\n"
            "(guest) system's IP address which, if DHCP allocated would default to\n"
            "10.0.2.15 or could be statically configured to any address including\n"
            "10.0.2.4 thru 10.0.2.14.\n\n"
            "NAT limitations\n\n"
            "There are four limitations of NAT mode which users should be aware of:\n\n"
            " 1) ICMP protocol limitations:\n"
            "    Some frequently used network debugging tools (e.g. ping or tracerouting)\n"
            "    rely on the ICMP protocol for sending/receiving messages. While some\n"
            "    ICMP support is available on some hosts (ping may or may not work),\n"
            "    some other tools may not work reliably.\n\n"
            " 2) Receiving of UDP broadcasts is not reliable:\n"
            "    The guest does not reliably receive broadcasts, since, in order to save\n"
            "    resources, it only listens for a certain amount of time after the guest\n"
            "    has sent UDP data on a particular port.\n\n"
            " 3) Protocols such as GRE, DECnet, LAT and Clustering are unsupported:\n"
            "    Protocols other than TCP and UDP are not supported.\n\n"
            " 4) Forwarding host ports < 1024 impossible:\n"
            "    On Unix-based hosts (e.g. Linux, Solaris, Mac OS X) it is not possible\n"
            "    to bind to ports below 1024 from applications that are not run by root.\n"
            "    As a result, if you try to configure such a port forwarding, the attach\n"
            "    will fail.\n\n"
            "These limitations normally don't affect standard network use. But the\n"
            "presence of NAT has also subtle effects that may interfere with protocols\n"
            "that are normally working. One example is NFS, where the server is often\n"
            "configured to refuse connections from non-privileged ports (i.e. ports not\n"
            " below 1024).\n");
    return SCPE_OK;
}

/* Initialize the select/poll file descriptor arrays. */
static int initialize_poll_fds(SimSlirpNetwork *slirp)
{
    size_t i;

#if SIM_USE_SELECT
    FD_ZERO(&slirp->readfds);
    FD_ZERO(&slirp->writefds);
    FD_ZERO(&slirp->exceptfds);

    /* Start out with a generous number of LUT slots */
    slirp->lut_alloc = FDS_ALLOC_INIT;
    slirp->lut = (SOCKET *) malloc(slirp->lut_alloc * sizeof(SOCKET));

    for (i = 0; i < slirp->lut_alloc; ++i)
        slirp->lut[i] = INVALID_SOCKET;
#elif SIM_USE_POLL
    /* poll()-based file descriptor polling. */
    static const sim_pollfd_t poll_initializer = { INVALID_SOCKET, 0, 0};

    slirp->n_fds = FDS_ALLOC_INIT;
    slirp->fd_idx = 0;
    slirp->fds = (sim_pollfd_t *) malloc(slirp->n_fds * sizeof(sim_pollfd_t));
    for (i = 0; i < slirp->n_fds; ++i) {
        slirp->fds[i] = poll_initializer;
    }
#endif

    return 0;
}

/* Show NAT network statistics. */
void sim_slirp_show(SimSlirpNetwork *slirp, FILE *st)
{
    struct redir_tcp_udp *rtmp;
    const SlirpConfig *cfg = &slirp->slirp_config;
    char *conn_info;

    if (slirp == NULL || slirp->slirp_cxn == NULL)
        return;

    fprintf(st, "NAT args: %s\n", (slirp->args != NULL ? slirp->args : "(none given)"));
    fprintf(st, "NAT network setup:\n");
    fprintf(st, "        gateway       = %s", sim_inet_ntoa4(&cfg->vhost));
    fprintf(st, " (%s)\n", sim_inet_ntoa4(&cfg->vnetmask));
#if defined(AF_INET6)
    fprintf(st, "        IPv6          = %sabled.\n", slirp->slirp_config.in6_enabled ? "en" : "dis");
    if (slirp->slirp_config.in6_enabled) {
        fprintf(st, "          V6 Prefix   = %s/%d\n", sim_inet_ntoa6(&slirp->slirp_config.vprefix_addr6),
                slirp->slirp_config.vprefix_len);
        fprintf(st, "          V6 Gateway  = %s\n", sim_inet_ntoa6(&slirp->slirp_config.vhost6));
    }
#endif
    fprintf(st, "        DNS           = %s\n", sim_inet_ntoa4(&cfg->vnameserver));
    if (cfg->vdhcp_start.s_addr != 0)
        fprintf(st, "        dhcp_start    = %s\n", sim_inet_ntoa4(&cfg->vdhcp_start));
    if (cfg->bootfile != NULL)
        fprintf(st, "        dhcp bootfile = %s\n", cfg->bootfile);
    if (cfg->vdnssearch) {
        const char **domains = cfg->vdnssearch;

        fprintf(st, "        DNS domains   = ");
        while (*domains != NULL) {
            fprintf(st, "%s%s", (domains != cfg->vdnssearch) ? ", " : "", *domains);
            ++domains;
        }
        fprintf(st, "\n");
    }
    if (cfg->tftp_path != NULL)
        fprintf(st, "        tftp prefix   = %s\n", cfg->tftp_path);
    rtmp = slirp->rtcp;
    while (rtmp != NULL) {
        fprintf(st, "        redir %3s     = %d:%s:%d\n", tcpudp[rtmp->is_udp], rtmp->sim_local_port,
                sim_inet_ntoa4(&rtmp->sim_local_inaddr), rtmp->simh_host_port);
        rtmp = rtmp->next;
    }

    if ((conn_info = slirp_connection_info(slirp->slirp_cxn)) != NULL) {
        fputs(conn_info, st);
        free(conn_info);
    }
}

/*~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 * The libslirp interface.
 *
 * libslirp has an inverted sense of input and output. "Input" means "input into libslirp", whereas "output" means
 * output to the guest (the simulator via sim_ether and friends.)
 *~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/

/* Invoke the SIMH packet callback. */
static slirp_ssize_t invoke_sim_packet_callback(const void *buf, size_t len, void *opaque)
{
    SimSlirpNetwork *slirp = (SimSlirpNetwork *) opaque;

    /* Note: Should really range check len for int bounds. */
    slirp->pkt_callback(slirp->pkt_opaque, buf, (int) len);
    /* FIXME: the packet callback should tell us how many octets were written.
     * For the time being, though, assume it was successful. */
    return len;
}

int sim_slirp_send(SimSlirpNetwork *slirp, const char *msg, size_t len, int flags)
{
    SIM_UNUSED_ARG(flags);

    /* Just send the packet up to libslirp. */
    pthread_mutex_lock(&slirp->libslirp_lock);
    slirp_input(slirp->slirp_cxn, (const uint8_t *) msg, (int) len);
    pthread_mutex_unlock(&slirp->libslirp_lock);

    return (int) len;
}

/* I/O thread notify callback from Slirp. Indicates that there's packet data waiting, I/O events
 * are pending. */
static void notify_callback(void *opaque)
{
    /* SimSlirpNetwork *slirp = (SimSlirpNetwork *) opaque; */
    SIM_UNUSED_ARG(opaque);
}

int64_t sim_clock_get_ns(void *opaque)
{
    SIM_UNUSED_ARG(opaque);

    /* Internally, libslirp cuts the nanoseconds down to milliseconds. */
    return ((uint64_t) sim_os_msec()) * 10000000ull;
}
