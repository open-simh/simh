/* Automagically generated endianness include.
 *
 * Also defines GLIB_SIZE_VOID_P using the CMake CMAKE_SIZEOF_VOID_P
 * substitution. Probably not where this manifest constant really
 * belongs, but there's not really a better place for it. */

#if !defined(_GLIB_ENDIAN_H)
#define _GLIB_ENDIAN_H

#define G_BIG_ENDIAN 0x1234
#define G_LITTLE_ENDIAN 0x4321

#define G_BYTE_ORDER @G_BYTE_ORDER@
#define GLIB_SIZEOF_VOID_P @CMAKE_SIZEOF_VOID_P@
#endif
