/* Minimal definitions and functionality provided via the glib-2.0 utility library */

#if !defined(GLIB_H_MINIMAL)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

#if !defined(_WIN32)
#include <signal.h>
#endif

#if defined(_WIN32)
/* Code intentionally duplicated to keep the API version consistent. */

/* as defined in sdkddkver.h */
#if !defined(WINVER)
#define WINVER  0x0601 /* Windows 7 */
#endif
#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT WINVER
#endif

/* reduces the number of implicitly included headers */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>
#else
/* ntohl and friends... */
#include <arpa/inet.h>
#endif

#include <glib-endian.h>

/* Squelch ununsed formal paramter warnings... */
#define GLIB_UNUSED_PARAM(thing) (void)(thing)


/* Platform defines: */
#if defined(__unix__) || !(defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64))
#define G_OS_UNIX 1
#else
#define G_OS_WIN32 1
#endif

#if G_OS_UNIX
typedef ssize_t gssize;
#else
#include <basetsd.h>
typedef SSIZE_T gssize;
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE (!FALSE)
#endif

/* GLIB_CHECK_VERSION: Always greater than glib 2.72.0 */
#define GLIB_CHECK_VERSION(major, minor, micro) \
    (major > 2 || \
        (major == 2 && minor > 72) || \
        (major == 2 && minor == 72 && micro >= 0))

/* Stubs for inlining (note: The libslirp code itself just uses 'inline', no
 * preprocessor specials.) */

#if defined(_MSC_VER)
#define GLIB_INLINE_DECL _inline
#define GLIB_NOINLINE_DECL _declspec (noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define GLIB_INLINE_DECL inline
#define GLIB_NOINLINE_DECL  __attribute__ ((noinline))
#else
#define GLIB_INLINE_DECL
#define GLIB_NOINLINE_DECL
#endif

/* GCC: */
#if defined(__has_attribute)
#  if __has_attribute (format)
#    define G_GNUC_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#  endif
/* Clang, maybe older GCC-s */
#elif defined(__has_feature)
#  if __has_feature (format)
#    define G_GNUC_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#  endif
#endif

/* Everyone else... */
#if !defined(G_GNUC_PRINTF)
#define G_GNUC_PRINTF(fmt, args)
#endif

/* Static assertions appeared in C11 */
#if __STDC_VERSION__ >= 201101L
#define G_STATIC_ASSERT(expr) _Static_assert(expr, #expr)
#else
#define G_STATIC_ASSERT(expr)
#endif

#define G_N_ELEMENTS(arr) (sizeof(arr) / sizeof(arr[0]))

/* glib types: */
typedef void *gpointer;
typedef char gchar;
typedef int gboolean;
typedef int gint;

/* g_assert_not_reached(): Tell the compiler's flow analysis that the statement cannot
   be reached. */
#if defined(__has_builtin)
#  if __has_builtin(__builtin_unreachable)
#    define g_assert_not_reached() { (void) 0; __builtin_unreachable(); }
#  endif
#elif _MSC_VER
#  define g_assert_not_reached() { (void) 0; __assume(0); }
#endif

#if !defined(g_assert_not_reached)
#  define g_assert_not_reached() { (void) 0; }
#endif

/* G_LIKELY, G_UNLIKELY: Branch prediction hints. */
#define G_LIKELY
#define G_UNLIKELY

/* Pretty function names (G_STRFUNC replacement): */
#if defined (__GNUC__) && defined (__STDC_VERSION__)
#define GLIB_PRETTY_FUNCNAME     ((const char*) (__PRETTY_FUNCTION__))
#elif __STDC_VERSION__ >= 199901
#define GLIB_PRETTY_FUNCNAME     ((const char*) (__func__))
#elif defined (__GNUC__) || (defined(_MSC_VER) && (_MSC_VER > 1300))
#define GLIB_PRETTY_FUNCNAME     ((const char*) (__FUNCTION__))
#else
#define GLIB_PRETTY_FUNCNAME     ((const char*) ("???"))
#endif

/* g_assert: */
#if !defined(G_DISABLE_ASSERT)
#define g_assert(expr) g_do_assert(__FILE__, __LINE__, (expr), #expr)

static void GLIB_INLINE_DECL g_do_assert(const char *where, int line, int bool_expr, const char *msg)
{
    if (!bool_expr) {
        fprintf(stderr, "%s:%d: failed assertion: %s\n", where, line, msg);
        fflush(stderr);
        exit(1);
    }
}
#else
#define g_assert(expr)
#endif

/* Emulated glib logging facility: */
#if !defined(G_LOG_DOMAIN)
#define G_LOG_DOMAIN "glib"
#endif

typedef enum
{
  /* GLib log levels */
  G_LOG_LEVEL_ERROR             = 1 << 2,       /* always fatal */
  G_LOG_LEVEL_CRITICAL          = 1 << 3,
  G_LOG_LEVEL_WARNING           = 1 << 4,
  G_LOG_LEVEL_MESSAGE           = 1 << 5,
  G_LOG_LEVEL_INFO              = 1 << 6,
  G_LOG_LEVEL_DEBUG             = 1 << 7
} GLibLoggingFlags;

typedef struct GLibLogger
{
    int  logging_enabled;
    void (*output_warning)(const char *fmt, va_list args) /* G_GNUC_PRINTF(1, 2) */;
    void (*output_debug)(const char *fmt, va_list args) /* G_GNUC_PRINTF(1, 2) */;
    void (*output_error)(const char *fmt, va_list args) /* G_GNUC_PRINTF(1, 2) */;
    void (*output_critical)(const char *fmt, va_list args) /* G_GNUC_PRINTF(1, 2) */;
} GLibLogger;


/* Override default logging output.*/
extern GLibLoggingFlags glib_logging_level;
extern GLibLogger      *glib_logger;

/* Compatibility with glib logging: */
void g_log_set_debug_enabled(int);
void glib_set_logging_hooks(const GLibLogger *hooks);

static GLIB_INLINE_DECL G_GNUC_PRINTF(1, 2) void g_warning(const char *fmt, ...)
{
    if (glib_logger->logging_enabled && glib_logging_level & G_LOG_LEVEL_WARNING) {
        va_list args;

        va_start(args, fmt);
        glib_logger->output_warning(fmt, args);
        va_end(args);
    }
}

static GLIB_INLINE_DECL G_GNUC_PRINTF(1, 2) void g_debug(const char *fmt, ...)
{
    if (glib_logger->logging_enabled && glib_logging_level & G_LOG_LEVEL_DEBUG) {
        va_list args;
        
        va_start(args, fmt);
        glib_logger->output_debug(fmt, args);
        va_end(args);
    }
}


static GLIB_INLINE_DECL G_GNUC_PRINTF(1, 2)
void g_error(const char *fmt, ...)
{
    if (glib_logger->logging_enabled && glib_logging_level & G_LOG_LEVEL_ERROR) {
        va_list args;
        
        va_start(args, fmt);
        glib_logger->output_error(fmt, args);
        va_end(args);
    }
}

static GLIB_INLINE_DECL G_GNUC_PRINTF(1, 2)
void g_critical(const char *fmt, ...)
{
    va_list args;
        
    va_start(args, fmt);
    glib_logger->output_critical(fmt, args);
    va_end(args);
}

/* Macros where we want to see file and line where the error occurred. */
#define g_warn_if_fail(expr) \
  do { \
    if (!(expr)) \
        g_warn_message (G_LOG_DOMAIN, __FILE__, __LINE__, GLIB_PRETTY_FUNCNAME, \
                        "Warning: Assertion failed - " #expr); \
  } while (0)

static GLIB_INLINE_DECL void g_warn_message(const char *domain, const char *where_file, int where_line,
                                            const char *fnname, const char *msg)
{
    if (glib_logger->logging_enabled) {
        fprintf(stderr, "[%s/%s] %s:%d: %s\n", domain, fnname, where_file, where_line, msg);
    }
}

#define g_return_val_if_fail(expr, retval) \
    if (!(expr)) return retval

#define g_return_if_fail(expr) \
    if (!(expr)) return

#define g_warn_if_reached() \
    g_warn_message(G_LOG_DOMAIN, __FILE__, __LINE__, GLIB_PRETTY_FUNCNAME, \
                   "Reached a statement that shouldn't be reached.")

/* g_vnsprintf, g_snprintf: */
static GLIB_INLINE_DECL
int g_vsnprintf(char *dest, size_t n_dest, const char *fmt, va_list args)
{
    if (fmt == NULL)
        return -1;

    return vsnprintf(dest, n_dest, fmt, args);
}

static GLIB_INLINE_DECL G_GNUC_PRINTF(3, 4)
int g_snprintf(char *dest, size_t n_dest, const char *fmt, ...)
{
    va_list args;
    int retval;
    
    va_start(args, fmt);
    retval = g_vsnprintf(dest, n_dest, fmt, args);
    va_end(args);

    return retval;
}

/* g_str_has_prefix: */
static GLIB_INLINE_DECL int g_str_has_prefix(const char *str, const char *prefix)
{
    if (NULL == str || NULL == prefix)
        return 0;
    
    size_t l_str = strlen(str), l_prefix = strlen(prefix);
    if (l_str < l_prefix)
        return 0;

    return memcmp(str, prefix, l_prefix);
}

/* g_strstr: */
char *g_strstr_len(char *src, gssize src_len, const char *srch);

/* g_malloc, g_malloc0, g_free, g_realloc, g_new0 */
static GLIB_INLINE_DECL void *g_malloc(size_t size)
{
    return calloc(1, size);
}
static GLIB_INLINE_DECL void *g_malloc0(size_t size)
{
    return calloc(1, size);
}

static GLIB_INLINE_DECL void g_free(void *ptr)
{
    if (NULL != ptr)
        free(ptr);
}

static GLIB_INLINE_DECL void *g_realloc(void *ptr, size_t new_size)
{
    return realloc(ptr, new_size);
}

/* g_new: A wrapper around calloc() returning a pointer cast to the required 
 * structure type. If n == 0, returns NULL. */
#define g_new(struct_type, n) ((struct_type *) (n > 0 ? calloc(n, sizeof(struct_type)) : NULL))

/* g_new0: A wrapper around calloc() returning a pointer cast to the required
 * structure type. */
#define g_new0(struct_type, n) ((struct_type *) (n > 0 ? calloc(n, sizeof(struct_type)) : NULL))

/* GStrv: array of character pointers. */

typedef char **GStrv;

static unsigned int GLIB_INLINE_DECL g_strv_length(GStrv strs)
{
    if (NULL == strs)
        return 0;

    size_t i;
    unsigned int n_strs;

    for (i = 0, n_strs = 0; strs[i] != NULL; ++i, ++n_strs);

    return n_strs;
}

/* g_rand_int_range() and GRand -- glib returns gint32 (uint32_t), 32-bit random
 * numbers. */

/* GRand (insert "Tubular Bells" joke here...) */
typedef struct GRand
{
    uint32_t rand_state[4];
} GRand;

GRand *g_rand_new();
void g_rand_free(GRand *rand);
uint32_t g_rand_int_range (GRand  *rand, int begin, int end);

/* glib string functions: */

typedef struct GString {
  char *str;
  size_t len;
  size_t allocated;
} GString;

#define G_STRING_MIN_ALLOCATION 64

GString *g_string_new(const char *initial);
char *g_string_free(GString *, int);
void g_strfreev(char **);
void g_string_append_printf(GString *, const char *, ...);
int g_ascii_strcasecmp(const char *s1, const char *s2);
size_t g_strlcpy(char *dest, const char *src, size_t n_dest);

static GLIB_INLINE_DECL char *g_strdup(const char *s)
{
  if (NULL == s)
    return NULL;

#if !defined(HAVE__STRDUP_S)
    return strdup(s);
#else
    return _strdup(s);
#endif
}

/* g_getenv: */
const char *g_getenv(const char *envar);

/* Error support: */
typedef struct GError {
  int domain;
  int code;
  char *message;
} GError;

/* GDebugKey: */
typedef struct GDebugKey {
  const char *key;
  uint32_t bitvalue;
} GDebugKey;

void g_error_free(GError *);
uint32_t g_parse_debug_string(const char *str, const GDebugKey *keys, size_t n_keys);

/* Spawn/exec support: */

#define G_SPAWN_SEARCH_PATH (1 << 0)

typedef uint32_t GSpawnFlags;
typedef void (*GSpawnChildSetupFunc)(void *user_data);

#if !defined(_WIN32)
typedef int GPid;
#else
/* This could be a DWORD for the process identifier, but really want
 * the HANDLE for TerminateProcess() or WaitSingleObject(). */
typedef HANDLE GPid;
#endif

int g_spawn_async (const char *wd, char **argv, char **envp,
                   GSpawnFlags flags,
                   GSpawnChildSetupFunc child_setup,
                   void *user_data, GPid *child_pid, GError **error);

/* Logging domain for shell functionality */
#define G_SHELL_ERROR 0x00001000
/* Specific shell functionality error codes */
enum GShellError
{
    G_SHELL_ERROR_BAD_QUOTING,
    G_SHELL_ERROR_EMPTY_STRING,
    G_SHELL_ERROR_FAILED
};

int g_shell_parse_argv (const char *cmd, int *argcp, char ***argvp,
                        GError** error);

/* "Safe" version of strerror() - when HAVE_GETENV_S is defined (notably, for
 * Windows), return an internal buffer via strerror_s(). Otherwise, just
 * returns whatever strerror() returns. */
const char *g_strerror(int the_error);

/* MIN and MAX: */
#if !defined(MAX)
#  if defined(__GNUC__) || defined(__clang__)
#    define MAX(a, b) ({__typeof__(a) _a = a; __typeof__(b) _b = b; (_a > _b ? _a : _b); })
#  elif defined(_MSC_VER)
#    define MAX __max
#  else
#    define MAX(a, b) ((a) > (b) ? (a) : (b))
#  endif
#endif

#if !defined(MIN)
#  if defined(__GNUC__) || defined(__clang__)
#    define MIN(a, b) ({__typeof__(a) _a = a; __typeof__(b) _b = b; (_a < _b ? _a : _b); })
#  elif defined(_MSC_VER)
#    define MIN __min
#  else
#    define MIN(a, b) ((a) < (b) ? (a) : (b))
#  endif
#endif

/* And the network byte ordering and flipping. These are inlines to provide type
 * safety. */
static inline uint16_t GUINT16_FROM_BE(uint16_t val)
{
    return ntohs(val);
}

static inline uint16_t GUINT16_TO_BE(uint16_t val)
{
    return htons(val);
}

static inline uint32_t GUINT32_FROM_BE(uint32_t val)
{
    return ntohl(val);
}

static inline uint32_t GUINT32_TO_BE(uint32_t val)
{
    return htonl(val);
}

static inline int16_t GINT16_FROM_BE(int16_t val)
{
    return ((int16_t) ntohs((uint16_t) val));
}

static inline int16_t GINT16_TO_BE(int16_t val)
{
    return ((int16_t) htons((uint16_t) val));
}

static inline int32_t GINT32_FROM_BE(int32_t val)
{
    return ((int32_t) ntohl((uint32_t) val));
}

static inline int32_t GINT32_TO_BE(int32_t val)
{
    return ((int32_t) htonl((uint32_t) val));
}

#define GLIB_H_MINIMAL
#endif
