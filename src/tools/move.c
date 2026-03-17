#include "object.h"
#include "tool.h"

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

static void move_down(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id != -1 && object_get(id)->type == POINT) {
    internal.selected = id;
    board_deselect_object(id);
  }
}

static void move_up(Vec2 pos) {
  move_reset();
}

static void move_drag(const Vec2 pos) {
  if (internal.selected == -1) return;

  const Vec2 world_pos = xform_to_world(pos);
  const GeomObject *obj = object_get(internal.selected);
  graph_change_value(2, obj->args, (float *)&world_pos);
  board_update_objects();
}

void tool_move(GeomTool *tool) {
  tool->usage = "move: drag or select object";
  tool->reset = move_reset;
  tool->ctrl.mouse_down = move_down;
  tool->ctrl.mouse_up = move_up;
  tool->ctrl.mouse_click = NULL;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = move_drag;
}