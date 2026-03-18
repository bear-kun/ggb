#include "object.h"
#include "tool.h"
#include <math.h>

static int circle_point_eval(const float xyxy[4], float radius[1]) {
  const float dx = xyxy[2] - xyxy[0];
  const float dy = xyxy[3] - xyxy[1];
  radius[0] = sqrtf(dx * dx + dy * dy);
  return 1;
}

static struct {
  int n;
  GeomId center;
  GeomId inputs[4];
} internal = {0, -1};

static void circle_reset() {
  if (internal.center != -1) {
    board_deselect_object(internal.center);
    internal.n = 0;
    internal.center = -1;
  }
}

static void circle_click(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id == -1) return;
  const GeomObject *obj = object_get(id);
  if (obj->type != POINT) return;

  if (id == internal.center) {
    circle_reset();
    return;
  }

  copy_args(internal.inputs + internal.n * 2, obj->args, 2);
  if (++internal.n == 2) {
    GeomId args[3];
    args[0] = internal.inputs[0];
    args[1] = internal.inputs[1];
    args[2] = graph_add_value(0);
    const GeomId define = graph_add_constraint(4, internal.inputs, 1, args + 2,
                                               circle_point_eval);
    board_add_object(object_create(CIRCLE, args, define, 0));
    circle_reset();
  } else {
    internal.center = id;
    board_select_object(id);
  }
}

void tool_circle(GeomTool *tool) {
  tool->usage = "circle: select center point, then point on circle";
  tool->reset = circle_reset;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = circle_click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}