#include "object.h"
#include "tool.h"
#include <string.h>

void copy_args(GeomId *dst, const GeomId *src, const int n) {
  memcpy(dst, src, n * sizeof(GeomId));
}

void init_line(GeomId *args) {
  args[0] = graph_add_value(0);
  args[1] = graph_add_value(0);
  args[2] = graph_add_value(0);
  args[3] = graph_add_value(-HUGE_VALUE);
  args[4] = graph_add_value(HUGE_VALUE);
}
