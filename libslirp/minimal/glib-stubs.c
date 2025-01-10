/* SPDX-License-Identifier: BSD-3-Clause */

/* Minimalist glib support.
 *
 * This is the minimalist approach to providing glib support for libslirp.
 * There are many reasons why you might want minimalism, one of which is
 * the overhead of yet-another-dependency. */

#include <stdint.h>
#include <ctype.h>
#include <glib.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(G_OS_UNIX)
#include <unistd.h>

#if defined(HAVE_TIME_H)
#include <time.h>
#endif

#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#endif

/* getenv() wrapper: */
const char *g_getenv(const char *envar)
{
#if !defined(HAVE_DUPENV_S)
    return getenv(envar);
#else
    static char *env_store = NULL;

    /* Don't leak previous calls. */
    free(env_store);
    _dupenv_s(&env_store, NULL, envar);
    return env_store;
#endif
}

/* Logging support: */
static void output_stderr_noexit(const char *fmt, va_list args);
static void output_stderr_exit(const char *fmt, va_list args);

GLibLoggingFlags  glib_logging_level = G_LOG_LEVEL_ERROR|G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_WARNING|G_LOG_LEVEL_DEBUG;

static GLibLogger basic_logger = {
    .logging_enabled = 1,
    .output_warning = output_stderr_noexit,
    .output_debug = output_stderr_noexit,
    .output_error = output_stderr_exit,
    .output_critical = output_stderr_exit
};

GLibLogger      *glib_logger = &basic_logger;

/* Compatibility with glib logging: */
void g_log_set_debug_enabled(int flag)
{
    glib_logger->logging_enabled = flag;
}

void glib_set_logging_hooks(const GLibLogger *hooks)
{
    glib_logger->output_warning = hooks->output_warning;
    glib_logger->output_debug = hooks->output_debug;
    glib_logger->output_error = hooks->output_error;
    glib_logger->output_critical = hooks->output_critical;
}

static void output_stderr_noexit(const char *fmt, va_list args)
{
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    fflush(stderr);
}

static void output_stderr_exit(const char *fmt, va_list args)
{
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    fflush(stderr);

    exit(1);
    g_assert_not_reached();
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/
/* Random number generation: Uses the xoshiro128** PRNG. */

/* This is xoshiro128** 1.1, one of our 32-bit all-purpose, rock-solid
   generators. It has excellent speed, a state size (128 bits) that is
   large enough for mild parallelism, and it passes all tests we are aware
   of.

   Note that version 1.0 had mistakenly s[0] instead of s[1] as state
   word passed to the scrambler.

   For generating just single-precision (i.e., 32-bit) floating-point
   numbers, xoshiro128+ is even faster.

   The state must be seeded so that it is not everywhere zero. */

/*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

static inline uint32_t rotl(const uint32_t x, int k)
{
	return (x << k) | (x >> (32 - k));
}

uint32_t next(GRand *seed)
{
	const uint32_t result = rotl(seed->rand_state[1] * 5, 7) * 9;

	const uint32_t t = seed->rand_state[1] << 9;

	seed->rand_state[2] ^= seed->rand_state[0];
	seed->rand_state[3] ^= seed->rand_state[1];
	seed->rand_state[1] ^= seed->rand_state[2];
	seed->rand_state[0] ^= seed->rand_state[3];

	seed->rand_state[2] ^= t;

	seed->rand_state[3] = rotl(seed->rand_state[3], 11);

	return result;
}

GRand *g_rand_new()
{
    GRand *retval = calloc(1, sizeof(GRand));

    if (NULL == retval)
        return retval;

#if !defined(_WIN32)
#  if defined(HAVE_CLOCK_GETTIME)
   struct timespec now;

   clock_gettime(CLOCK_REALTIME, &now);

    retval->rand_state[0] = now.tv_sec;
    retval->rand_state[1] = now.tv_nsec;
#  elif defined(HAVE_GETTIMEOFDAY)
    struct timeval now;

    gettimeofday (&now, NULL);

    retval->rand_state[0] = now.tv_sec;
    retval->rand_state[1] = now.tv_usec;
#  endif

    retval->rand_state[2] = getpid ();
#else
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );

    time =  ((uint64_t) file_time.dwLowDateTime );
    time += ((uint64_t) file_time.dwHighDateTime) << 32;

    retval->rand_state[0] = (uint32_t) ((time - EPOCH) / 10000000L);
    retval->rand_state[1] = (uint32_t) (system_time.wMilliseconds * 1000);
    retval->rand_state[2] = (uint32_t) GetCurrentProcessId();
#endif

    retval->rand_state[3] = 0;

    return retval;
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/
void g_rand_free(GRand *rand)
{
    free(rand);
}

uint32_t g_rand_int_range (GRand *rand, int begin, int end)
{
    return ((int) (next(rand) % (end - begin) + begin));
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/
/* String operations: */

GString *g_string_new(const char *initial)
{
    GString *retval = calloc(1, sizeof(GString));

    if (NULL == retval)
        return retval;

    retval->len = (initial == NULL) ? 0 : strlen(initial);
    retval->allocated = MAX(G_STRING_MIN_ALLOCATION, retval->len);

    retval->str = calloc(retval->allocated, sizeof(char));
    if (NULL != initial) {
#if !defined(HAVE_STRCPY_S)
        strcpy(retval->str, initial);
#else
        strcpy_s(retval->str, retval->allocated, initial);
#endif
    }

    return retval;
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/
char *g_string_free(GString *gstr, int free_string)
{
    char *inner_str = NULL;

    if (NULL != gstr) {
        if (free_string)
            free(inner_str);
        else
            inner_str = gstr->str;


        free(gstr);
    }

    return inner_str;
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/
void g_strfreev(char **strs)
{
    char **iter = strs;

    if (NULL == strs)
        return;

    /* free content */
    while (*iter != NULL) {
        free(*iter);
        ++iter;
    }

    /* Then free the vector. */
    free(strs);
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/
int g_ascii_strcasecmp(const char *s1, const char *s2)
{
    while (*s1 && *s2 && isascii(*s1) && isascii(*s2)) {
        if ((isalpha(*s1) && isalpha(*s2) && tolower(*s1) == tolower(*s2)) || *s1 == *s2) {
            ++s1;
            ++s2;
        } else {
            /* Branchless: booleans are 0/1 since C99, so if *s1 < *s2, return -1 (note the
             * negation) or 1 if *s1 > *s2. */
            return -(*s1 < *s2) + (*s1 > *s2);
        }
    }

    /* Another branchless if/then: s1 shorter than s2, return -1, s2 shorter than s1, return 1,
     * otherwise they were the same length and the entire expression is 0. */
    return (*s1 != '\0') * -1 + (*s2 != '\0') * 1;
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/
/* GError: */
void g_error_free(GError *err)
{
    if (NULL != err) {
        free(err->message);
        free(err);
    }
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/
/* OpenBSD implementation:
 *
 * Copy string src to buffer dst of size dsize.  At most dsize-1
 * chars will be copied.  Always NUL terminates (unless dsize == 0).
 * Returns strlen(src); if retval >= dsize, truncation occurred.
 */
size_t
g_strlcpy(char *dst, const char *src, size_t dsize)
{
	const char *osrc = src;
	size_t nleft = dsize;

	/* Copy as many bytes as will fit. */
	if (nleft != 0) {
		while (--nleft != 0) {
			if ((*dst++ = *src++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src. */
	if (nleft == 0) {
		if (dsize != 0)
			*dst = '\0';		/* NUL-terminate dst */
		while (*src++)
			;
	}

	return(src - osrc - 1);	/* count does not include NUL */
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/
/* g_strerror(): */
const char *g_strerror(int the_error)
{
#if !defined(HAVE_STRERROR_S)
    const char *errstr = strerror(the_error);
#else
    static char errstr[256];

    strerror_s(errstr, sizeof(errstr)/sizeof(errstr[0]), the_error);
#endif

    return errstr;
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/
/* Simple Rabin-Karp g_strstr_len implementation. */

/* Larger numbers usually result in fewer hash collisions. */
static const int hash_base = 512;
static const int hash_prime = 241;

/* Pre-seeded hash corrector values, up to 128 character search strings. N.B. that
 * the correctors are dynamically generated instead of statically initialized.
 * This allows hash_base and hash_prime to change without having to regenerate
 * the initializers. */
static int correctors[128] = { 0, };
static int have_correctors = FALSE;
static const size_t n_correctors = sizeof(correctors) / sizeof(correctors[0]);

static inline void seed_correctors()
{
    size_t i;
    int hval = 1;

    for (i = 0; i < n_correctors; ++i) {
        correctors[i] = hval;
        hval = (hval * hash_base) % hash_prime;
    }
}

/* Would have preferred a tail-recursive hash function implementation here.
 * Debug builds would potentially choke on long strings, so use iterative
 * loop to construct the hash. */
static inline int full_hash(const char *str, size_t len)
{
    int hash = 0;
    size_t offs;

    for (offs = 0; offs < len; ++offs) {
        hash = (hash * hash_base + str[offs]) % hash_prime;
    }

    return hash;
}

char *g_strstr_len(char *src, gssize src_len, const char *srch)
{
    size_t srch_len = strlen(srch);
    size_t src_limit = (src_len >= 0) ? (size_t) src_len : strlen(src);
    char *retval = NULL;

    if (srch_len == 0)
        return src;

    if (src_limit > 0 && srch_len <= src_limit) {
        int hash_correction = 1;
        size_t i;
        size_t offset = 0;
        int srch_hash = full_hash(srch, srch_len);
        int hval = full_hash(src, srch_len);

        /* Correction factor used when removing the leftmost character from the
         * rolling hash value, hash_base^(srch_len-1) % hash_prime. */
        if (srch_len < n_correctors) {
            if (!have_correctors) {
                seed_correctors();
                have_correctors = TRUE;
            }

            hash_correction = correctors[srch_len - 1];
        } else {
            /* Brute force... but take advantage of the last pre-seeded corrector value. */
            hash_correction = correctors[n_correctors - 1];
            for (i = n_correctors; i < srch_len - 1; ++i)
                hash_correction = (hash_correction * hash_base) % hash_prime;
        }

        while (NULL == retval && offset + srch_len <= src_limit) {
            if (hval == srch_hash) {
                size_t j;

                for (i = offset, j = 0; j < srch_len && src[i] == srch[j]; ++i, ++j)
                    /* NOP */;

                if (j == srch_len)
                    retval = src + offset;
            }

            /* If the strings don't match when hval == srch_hash, fall through into
             * computing the rolling hash anyway. */
            hval = hash_base * (hval - src[offset] * hash_correction) + src[offset + srch_len];
            hval %= hash_prime;
            /* Branchless adjustment: (hval < 0) == 0 or 1. */
            hval += (hval < 0) * hash_prime;
            ++offset;
        }
    }

    return retval;
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/
/* g_parse_debug_string: */

static inline int arg_delim(char c)
{
    return (c == ' ' || c == ',' || c == ';' || c == ':' || c == '\t');
}

/* Note: Unrecognized debug tokens are sliently ignored. */
uint32_t g_parse_debug_string(const char *str, const GDebugKey *keys, size_t n_keys)
{
    uint32_t retval = 0;
    uint32_t all_mask = 0;
    int      all_flag = 0;
    const char *p = str;
    const char *q;
    const char *s_limit = str + strlen(str);
    size_t i;

    for (i = 0; i < n_keys; ++i)
        all_mask |= keys[i].bitvalue;

    for (q = p; p <= s_limit; ++p) {
        if ((arg_delim(*p) || *p == '\0')) {
            size_t k_len = p - q;
            if (k_len > 0) {
                int is_all = !strncmp("all", q, k_len);
                int is_help = !strncmp("help", q, k_len);

                if (!is_all && !is_help) {
                    for (i = 0; i < n_keys && (strncmp(q, keys[i].key, k_len) || strlen(keys[i].key) != k_len); ++i)
                        /* NOP */;
                    retval |= (i < n_keys) * keys[i].bitvalue;
                } else if (is_all) {
                    all_flag = 1;
                } else if (is_help) {
                    fputs("Debug keys: \n", stderr);
                    for (i = 0; i < n_keys; ++i)
                        fprintf(stderr, "  %s (%08x)\n", keys[i].key, keys[i].bitvalue);
                }
            }
            
            /* Bump start of next token. */
            q = p + 1;
        }
    }

    return (!all_flag ? retval : (all_mask & ~retval));
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/
void g_string_append_printf(GString *gstr, const char *fmt, ...)
{
    char *buf = NULL;
    va_list args;
    int ret;

#if defined(HAVE_VASPRINTF)
    va_start(args, fmt);
    ret = vasprintf(&buf, fmt, args);
    va_end(args);
#else
    /* Have to call vnsprintf() twice to figure out how large buf needs to
     * be. */
    char c;

    va_start(args, fmt);
    ret = vsnprintf(&c, 1, fmt, args);
    va_end(args);
    if (ret >= 0) {
        ++ret;
        buf = (char *) calloc((size_t) ret + 1, sizeof(char));
        va_start(args, fmt);
        ret = vsnprintf(buf, ret, fmt, args);
        va_end(args);
    }
#endif

    if (ret >= 0) {
        if (ret + gstr->len + 1 >= gstr->allocated) {
            /* Resize in 64-byte increments. */
            const unsigned int shift = 6;
            size_t needed = ret + gstr->len + 1;
            size_t new_alloc = ((needed >> shift) + ((needed & ((1 << shift) - 1)) != 0)) << shift;
            char *newstr = realloc(gstr->str, new_alloc);

            if (NULL != newstr) {
                gstr->str = newstr;
                gstr->allocated = new_alloc;
            } else {
                g_error("glib-minimal: Unable to expand string.");
            }
        }

        memcpy(gstr->str + gstr->len, buf, (size_t) (ret + 1));
        gstr->len += ret;
        free(buf);
    } else {
        g_error("glib-minimal: Unable to vasprintf or vsnprintf"); 
    }
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/

int g_shell_parse_argv (const char *cmd, int *argcp, char ***argvp,
                        GError** error)
{
    /* FIXME: */
    GLIB_UNUSED_PARAM(cmd);
    GLIB_UNUSED_PARAM(argcp);
    GLIB_UNUSED_PARAM(argvp);
    GLIB_UNUSED_PARAM(error);
    return 0;
}

/*~=~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=*~=**/

int g_spawn_async (const char *wd, char **argv, char **envp,
                   GSpawnFlags flags,
                   GSpawnChildSetupFunc child_setup,
                   void *user_data, GPid *child_pid, GError **error)
{
    /* FIXME: */
    GLIB_UNUSED_PARAM(wd);
    GLIB_UNUSED_PARAM(argv);
    GLIB_UNUSED_PARAM(envp);
    GLIB_UNUSED_PARAM(flags);
    GLIB_UNUSED_PARAM(child_setup);
    GLIB_UNUSED_PARAM(user_data);
    GLIB_UNUSED_PARAM(child_pid);
    GLIB_UNUSED_PARAM(error);
    return 0;
}
