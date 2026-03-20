#include "object.h"
#include "tool.h"

static struct {
  GeomId selected;
  Vec2 last_pos;
} intl = {-1};

static void reset() {
  if (intl.selected != -1) {
    board_deselect_object(intl.selected);
    intl.selected = -1;
  }
}

static void down(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id == -1 || object_get(id)->type != POINT) {
    reset();
  } else {
    intl.selected = id;
    board_select_object(id);
  }
}

static void up(Vec2 pos) {
  reset();
}

static void drag(const Vec2 pos) {
  if (intl.selected == -1) return;

  const Vec2 world_pos = xform_to_world(pos);
  const GeomObject *obj = object_get(intl.selected);
  graph_change_value(2, obj->args, (float *)&world_pos);
  board_update_objects();
}

void tool_move(GeomTool *tool) {
  tool->usage = "move: drag or select object";
  tool->reset = reset;
  tool->ctrl.mouse_down = down;
  tool->ctrl.mouse_up = up;
  tool->ctrl.mouse_click = NULL;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = drag;
}