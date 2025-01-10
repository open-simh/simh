#include <stdio.h>
#include <stdint.h>
#include "sim_printf_fmts.h"

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif

/* Bridge until printf format patch accepted. */
#if defined(__MINGW64__) || defined(_WIN64) || defined(__WIN64)
#  if defined(PRIu64)
#    define SIM_PRIsize_t   PRIu64
#  else
#    define SIM_PRIsize_t   "llu"
#  endif
#elif defined(__MINGW32__) || defined(_WIN32) || defined(__WIN32)
#  define SIM_PRIsize_t   "zu"
#elif defined(__GNU_LIBRARY__) || defined(__GLIBC__) || defined(__GLIBC_MINOR__) || \
      defined(__APPLE__)
/* GNU libc (Linux) and macOS */
#  define SIM_PRIsize_t   "zu"
#else
#  define SIM_PRIsize_t  "u"
#endif

const char *preamble[] = {
  "/* Automagically generated endianness include.",
  " *",
  " * Also defines GLIB_SIZE_VOID_P using the CMake CMAKE_SIZEOF_VOID_P",
  " * substitution. Probably not where this manifest constant really",
  " * belongs, but there's not really a better place for it. */",
  "",
  "#if !defined(_GLIB_ENDIAN_H)",
  "#define _GLIB_ENDIAN_H",
  ""
  "#define G_BIG_ENDIAN 0x1234",
  "#define G_LITTLE_ENDIAN 0x4321",
  ""
};

const char *epilog[] = {
  "",
  "#endif /* _GLIB_ENDIAN_H */"
};

const int endian_test = 0x1234;

int main(void) {
  size_t i;

  for (i = 0; i < sizeof(preamble) / sizeof(preamble[0]); ++i) {
    fputs(preamble[i], stdout);
    fputc('\n', stdout);
  }

  printf("#define G_BYTE_ORDER %s\n", (*((const char *) &endian_test) == 1) ? "G_BIG_ENDIAN" : "G_LITTLE_ENDIAN");
  printf("#define GLIB_SIZEOF_VOID_P %" SIM_PRIsize_t "\n", sizeof(void *));

  for (i = 0; i < sizeof(epilog) / sizeof(epilog[0]); ++i) {
    fputs(epilog[i], stdout);
    fputc('\n', stdout);
  }

  return 0;
}
