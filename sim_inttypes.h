#if !defined(SIM_INTTYPES_H)
/* Length specific integer declarations */

/* Handle the special/unusual cases first with everything else leveraging stdints.h */
#if defined (VMS)
#include <ints.h>
#elif defined(_MSC_VER) && (_MSC_VER < 1600)
typedef __int8           int8;
typedef __int16          int16;
typedef __int32          int32;
typedef unsigned __int8  uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
#else                                                   
/* All modern/standard compiler environments */
/* any other environment needa a special case above */
#include <stdint.h>
typedef int8_t          int8;
typedef int16_t         int16;
typedef int32_t         int32;
typedef uint8_t         uint8;
typedef uint16_t        uint16;
typedef uint32_t        uint32;
#endif                                                  /* end standard integers */

#define SIM_INTTYPES_H
#endif