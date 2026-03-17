#include "object.h"
#include "tool.h"

static int midpoint_eval(const float inputs[4], float *outputs[2]) {
  *outputs[0] = (inputs[0] + inputs[2]) / 2.f;
  *outputs[1] = (inputs[1] + inputs[3]) / 2.f;
  return 1;
}

static struct {
  int n;
  GeomId first;
  GeomId inputs[4];
} internal = {0, -1};

static void midpoint_reset() {
  if (internal.first != -1) {
    board_deselect_object(internal.first);
    internal.n = 0;
    internal.first = -1;
  }
}

static void midpoint_click(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id == -1) return;
  const GeomObject *obj = object_get(id);
  if (obj->type != POINT) return;

  if (id == internal.first) {
    midpoint_reset();
    return;
  }

  copy_args(internal.inputs + internal.n * 2, obj->args, 2);
  if (++internal.n == 2) {
    GeomId args[2];
    args[0] = graph_add_value(0);
    args[1] = graph_add_value(0);
    graph_add_constraint(4, internal.inputs, 2, args, midpoint_eval);
    board_add_object(object_create(POINT, args));
    midpoint_reset();
  } else {
    internal.first = id;
    board_select_object(id);
  }
}

void tool_midpoint(GeomTool *tool) {
  tool->usage = "midpoint: select two points";
  tool->reset = midpoint_reset;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = midpoint_click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}