
#ifndef GGB_TYPES_H
#define GGB_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef int32_t GeomInt;
typedef int32_t GeomId;
typedef uint32_t GeomSize;

typedef enum {
  UNKNOWN = 0,
  POINT = 1,
  LINE = 2,
  CIRCLE = 4,
  ANY = 7
} GeomType;

#endif //GGB_TYPES_H