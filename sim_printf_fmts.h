/*~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
 * sim_printf_fmts.h
 *
 * Cross-platform printf() formats for simh data types. Refactored out to
 * this header so that these formats are avaiable to more than SCP.
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
 */

#if defined (_WIN32) || defined(_WIN64)
#  if defined(__MINGW64__)
#    define LL_FMT "I64"
#  elif defined(_MSC_VER) || defined(__MINGW32__)
#    define LL_FMT "ll"
#  else
#    define LL_FMT "ll"
#  endif
#elif defined (__VAX) /* No 64 bit ints on VAX */
#  define LL_FMT "l"
#else
#  define LL_FMT "ll"
#endif

#if defined(_WIN32) || defined(_WIN64)

#  if defined(__MINGW64__)
#    define SIZE_T_FMT   "I64"
#  elif defined(_MSC_VER) || defined(__MINGW32__)
#    define SIZE_T_FMT   "z"
#  endif

#  if defined(_WIN64)
#    define SOCKET_FMT   "I64"
#  else
#    define SOCKET_FMT   "I32"
#  endif

#  define T_UINT64_FMT   "I64"
#  define T_INT64_FMT    "I64"
#  define NTOHL_FMT      "l"
#  define IP_SADDR_FMT   "l"
#  define POINTER_FMT    "p"
#elif defined(__GNU_LIBRARY__) || defined(__GLIBC__) || defined(__GLIBC_MINOR__)
/* glibc (basically, most Linuxen) */
#  define SIZE_T_FMT     "z"
#  define T_UINT64_FMT   "ll"
#  define T_INT64_FMT    "ll"
#  define NTOHL_FMT      ""
#  define IP_SADDR_FMT   ""
#  define SOCKET_FMT     ""
#  define POINTER_FMT    "p"
#else
/* 32-bit platform, no 64 bit quantities */
#  define SIZE_T_FMT     ""
#  define T_UINT64_FMT   ""
#  define T_INT64_FMT    ""
#  define NTOHL_FMT      "l"
#  define IP_SADDR_FMT   "l"
#  define SOCKET_FMT     "l"
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
