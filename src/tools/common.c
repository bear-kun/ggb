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

GeomId create_point(const Vec2 pos, GeomId xy[2]) {
  const Vec2 world = xform_to_world(pos);
  xy[0] = graph_add_value(world.x);
  xy[1] = graph_add_value(world.y);
  const GeomId id = object_create(POINT, xy);
  board_add_object(id);
  return id;
}

GeomId find_point(const Vec2 pos, GeomId xy[2]) {
  const GeomId id = board_find_object(POINT, pos);
  if (id == -1) return id;
  const GeomObject *obj = object_get(id);
  xy[0] = obj->args[0];
  xy[1] = obj->args[1];
  return id;
}