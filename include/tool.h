#ifndef GGB_COMMAND_H
#define GGB_COMMAND_H

#include "board.h"

typedef struct {
  const char *usage;
  void (*reset)(void);
  BoardControl ctrl;
} GeomTool;

void copy_args(GeomId *dst, const GeomId *src, int n);
void init_line(GeomId args[5]);

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

#endif // GGB_COMMAND_H
