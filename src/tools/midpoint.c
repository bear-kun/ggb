#include "object.h"
#include "tool.h"

static int eval(const float inputs[4], float outputs[2]) {
  const float x1 = inputs[0], y1 = inputs[1];
  const float x2 = inputs[2], y2 = inputs[3];
  outputs[0] = (x1 + x2) / 2.f;
  outputs[1] = (y1 + y2) / 2.f;
  return 1;
}

static struct {
  int n;
  GeomId first;
  GeomId inputs[4];
} intl = {0, -1};

static void reset() {
  if (intl.first != -1) {
    board_deselect_object(intl.first);
    intl.n = 0;
    intl.first = -1;
  }
}

static void click(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id == -1) return;
  const GeomObject *obj = object_get(id);
  if (obj->type != POINT) return;

  if (id == intl.first) {
    reset();
    return;
  }

  copy_args(intl.inputs + intl.n * 2, obj->args, 2);
  if (++intl.n == 2) {
    GeomId args[2];
    args[0] = graph_add_value(0);
    args[1] = graph_add_value(0);
    const GeomId define = graph_add_constraint(4, intl.inputs, 2, args, eval);
    board_add_object(object_create(POINT, args, define, 0));
    reset();
  } else {
    intl.first = id;
    board_select_object(id);
  }
}

void tool_midpoint(GeomTool *tool) {
  tool->usage = "midpoint: select two points";
  tool->reset = reset;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}