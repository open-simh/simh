/*~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * sim_printf_fmts.h
 *
 * Cross-platform printf() formats for simh data types. Refactored out to
 * this header so that these formats are available to more than SCP.
 *
 * Author: B. Scott Michel
 *
 * "scooter me fecit"
 *~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*/

#pragma once
#if !defined(SIM_PRINTF_H)

/* cross-platform printf() format specifiers:
 *
 * Note: MS apparently does recognize "ll" as "l" in its printf() routines, but "I64" is
 * preferred for 64-bit types.
 *
 * MinGW note: __MINGW64__ and __MINGW32__ are both defined by 64-bit gcc. Check
 * for __MINGW64__ before __MINGW32__.
 *
 *
 * LL_FMT: long long format modifier, e.g. "%016" LL_FMT "x"
 * SIZE_T: size_t format modifier, e.g., "%" SIZE_T_FMT "u" (can use "d", but you will
 *         probably get a warning.)
 * T_UINT64_FMT: t_uint64 format modifier, e.g. "%016" T_UINT64_FMT "x"
 * T_INT64_FMT: t_int64 format modifier, e.g., "%" T_INT64_FMT "d"
 * POINTER_FMT: Format modifier for pointers, e.g. "%08" POINTER_FMT "X"
*/

#if defined (_WIN32) || defined(_WIN64) || defined(__MINGW64__) || defined(__MINGW32__)

#  if defined(__USE_MINGW_ANSI_STDIO) && __USE_MINGW_ANSI_STDIO
#    define LL_FMT     "ll"
#  else
#    define LL_FMT     "I64"
#  endif

#  if defined(__MINGW64__) || defined(_WIN64)
#    define SIZE_T_FMT "I64"
#  elif defined(__MINGW32__) || defined(_WIN32)
#    define SIZE_T_FMT "I32"
#  endif

#  define T_UINT64_FMT   "I64"
#  define T_INT64_FMT    "I64"
#  define POINTER_FMT    "p"

#elif defined(__GNU_LIBRARY__) || defined(__GLIBC__) || defined(__GLIBC_MINOR__) || \
      defined(__APPLE__)

/* GNU libc (Linux) and macOS */
#  define LL_FMT         "ll"
#  define SIZE_T_FMT     "z"
#  define T_UINT64_FMT   "ll"
#  define T_INT64_FMT    "ll"
#  define POINTER_FMT    "p"

#elif defined(__VAX)

/* No 64 bit ints on VAX, nothing special about size_t */
#  define LL_FMT         "l"
#  define SIZE_T_FMT     ""
#  define T_UINT64_FMT   ""
#  define T_INT64_FMT    ""
#  define POINTER_FMT    ""

#else
/* Defaults. */
#  define LL_FMT         "ll"
#  define SIZE_T_FMT     ""
#  define T_UINT64_FMT   ""
#  define T_INT64_FMT    ""
#  define POINTER_FMT    ""
#endif

#if defined (USE_INT64) && defined (USE_ADDR64)
#  define T_ADDR_FMT      T_UINT64_FMT
#else
#  define T_ADDR_FMT      ""
#endif

#if defined (USE_INT64)
#  define T_VALUE_FMT     T_UINT64_FMT
#  define T_SVALUE_FMT    T_INT64_FMT
#else
#  define T_VALUE_FMT     ""
#  define T_SVALUE_FMT    ""
#endif

#define SIM_PRINTF_H
#endif
