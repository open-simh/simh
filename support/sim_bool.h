/*~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 * sim_bool.h
 *
 * t_bool typedef: If using a C11+ standard-compliant compiler or MS compiler
 * beyond version 1800, use the builtin boolean type, otherwise, default to int.
 *~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/

#if !defined(SIM_BOOL_H)

/* Boolean flag type: */
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || (defined(_MSC_VER) && _MSV_VER >= 1800)
#  if __STDC_VERSION__ < 202311L || defined(_MSC_VER)
     /* bool, true, false are keywords in C23. */
#    include <stdbool.h>
#  endif
#  if defined(TRUE)
#    undef TRUE
#  endif
#  if defined(FALSE)
#    undef FALSE
#  endif
#  define HAVE_STDBOOL_TYPE
#  define TRUE true
#  define FALSE false
   typedef bool            t_bool;
#else
   /* Not a standard-compliant compiler, default t_bool to int. */
#  if !defined(TRUE)
#    define TRUE            1
#  endif
#  if !defined(FALSE)
#    define FALSE           0
#  endif
#  undef HAVE_STDBOOL_TYPE
   typedef int             t_bool;
#endif

#define SIM_BOOL_H
#endif
