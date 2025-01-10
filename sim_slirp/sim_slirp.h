#if !defined(SIM_SLIRP_H)

#if defined(HAVE_SLIRP_NETWORK)

#include "sim_defs.h"
#include "libslirp.h"


typedef struct sim_slirp SimSlirpNetwork;

typedef void (*packet_callback)(void *opaque, const unsigned char *buf, int len);

SimSlirpNetwork *sim_slirp_open (const char *args, void *pkt_opaque, packet_callback pkt_callback, DEVICE *dptr, uint32 dbit, char *errbuf, size_t errbuf_size);
void sim_slirp_shutdown(void *opaque);
void sim_slirp_close (SimSlirpNetwork *slirp);
int sim_slirp_send (SimSlirpNetwork *slirp, const char *msg, size_t len, int flags);
int sim_slirp_select (SimSlirpNetwork *slirp, int ms_timeout);
t_stat sim_slirp_attach_help(FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr);
void sim_slirp_show (SimSlirpNetwork *slirp, FILE *st);

/* Internal support functions: */
int64_t sim_clock_get_ns(void *opaque);

#endif /* HAVE_SLIRP_NETWORK */


#define SIM_SLIRP_H
#endif
