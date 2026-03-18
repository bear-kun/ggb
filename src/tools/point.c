#include "object.h"
#include "tool.h"

static void point_click(const Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id != -1 && object_get(id)->type == POINT) return;

  const Vec2 world_pos = xform_to_world(pos);
  GeomId args[2];
  args[0] = graph_add_value(world_pos.x);
  args[1] = graph_add_value(world_pos.y);
  board_add_object(object_create(POINT, args, -1, 0));
}

void tool_point(GeomTool *tool) {
  tool->usage = "point: select position";
  tool->reset = NULL;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = point_click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}