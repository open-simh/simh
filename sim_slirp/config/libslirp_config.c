/* Feature test program for libslirp.
 *
 * Only used by the makefile to set the minimal glib features. Patterned
 * off the way CMake does testing for library functions.
 *
 * - If the source compiles, with the appropriate TEST_* symbol defined,
 *   then the resulting libslirp_config program is executed, which should
 *   exit with a success (0) status.
 *
 * - Any failure along the way and the makefile will not detect the feature
 *   and won't update the LIBSLIR_FEATURES variable.
 */

#if defined(TEST_VASPRINTF_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <stdio.h>

#if defined(TEST_CLOCK_GETTIME)
#include <time.h>
#endif
#if defined(TEST_GETTIMEOFDAY)
#include <sys/time.h>
#endif
#if defined(TEST_INET_PTON)
#include <arpa/inet.h>
#endif

int main(void)
{
#if defined(TEST_VASPRINTF) || defined(TEST_VASPRINTF_GNU_SOURCE)
  (void (*)()) (vasprintf);
#endif
#if defined(TEST_CLOCK_GETTIME)
  (void (*)()) (clock_gettime);
#endif
#if defined(TEST_GETTIMEOFDAY)
  (void (*)()) (gettimeofday);
#endif
#if defined(TEST_INET_PTON)
  (void (*)()) (inet_pton);
#endif

  return 0;
}
