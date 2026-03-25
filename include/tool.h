#ifndef GGB_TOOL_H
#define GGB_TOOL_H

#include "object.h"
#include "board.h"
#include "command.h"

typedef struct {
  const char *usage;
  void (*reset)(void);
  BoardControl ctrl;
} GeomTool;

static void copy_args(GeomId *dst, const GeomId *src, const int n) {
  for (int i = 0; i < n; i++) dst[i] = src[i];
}

static void init_line(GeomId args[5]) {
  args[0] = graph_add_value(0);
  args[1] = graph_add_value(0);
  args[2] = graph_add_value(0);
  args[3] = graph_add_value(-HUGE_VALUE);
  args[4] = graph_add_value(HUGE_VALUE);
}

GeomId find_or_push_point(GeomId hovered, Vec2 pos);
GeomId create_point(const GeomId xy[2], GeomId define, GeomId soln_id);

void tool_move(GeomTool *);
void tool_point(GeomTool *);
void tool_line(GeomTool *);
void tool_circle(GeomTool *);
void tool_midpoint(GeomTool *);
void tool_perp(GeomTool *);
void tool_parallel(GeomTool *);
void tool_bisector(GeomTool *);
void tool_tangent(GeomTool *);
void tool_circum(GeomTool *);
void tool_isect(GeomTool *);
void tool_delete(GeomTool *);

#endif // GGB_TOOL_H