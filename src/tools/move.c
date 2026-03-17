#include "object.h"
#include "tool.h"
#include <math.h>

static struct {
  GeomId selected;
  Vec2 last_pos;
} internal = {-1};

static void move_reset() {
  if (internal.selected != -1) {
    board_deselect_object(internal.selected);
    internal.selected = -1;
  }
}

static void move_ctrl(const Vec2 pos, const MouseEvent event) {
  if (event == MOUSE_PRESS) {
    internal.selected = board_find_object(POINT, pos);
    if (internal.selected == -1) return;
    board_select_object(internal.selected);
    internal.last_pos = xform_to_world(pos);
  } else if (event == MOUSE_RELEASE) {
    move_reset();
  } else if (internal.selected != -1) {
    const Vec2 world_pos = xform_to_world(pos);
    if (fabsf(world_pos.x - internal.last_pos.x) + fabsf(
            world_pos.y - internal.last_pos.y) > 0.001f) {
      const GeomObject *obj = object_get(internal.selected);
      graph_change_value(2, obj->args, (float *)&world_pos);
      board_update_objects();
      internal.last_pos = world_pos;
    }
  }
}

void tool_move(GeomTool *tool) {
  tool->usage = "move: drag or select object";
  tool->ctrl = move_ctrl;
  tool->reset = move_reset;
}