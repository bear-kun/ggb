#include "object.h"
#include "tool.h"
#include <math.h>

static int eval_2pt(const float xyxy[4], float radius[1]) {
  const float dx = xyxy[2] - xyxy[0];
  const float dy = xyxy[3] - xyxy[1];
  radius[0] = sqrtf(dx * dx + dy * dy);
  return 1;
}

static struct {
  int n;
  GeomId center;
  GeomId inputs[4];
} intl = {0, -1};

static void circle_reset() {
  if (intl.center != -1) {
    board_deselect_object(intl.center);
    intl.n = 0;
    intl.center = -1;
  }
}

static void circle_click(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id == -1) return;
  const GeomObject *obj = object_get(id);
  if (obj->type != POINT) return;

  if (id == intl.center) {
    circle_reset();
    return;
  }

  copy_args(intl.inputs + intl.n * 2, obj->args, 2);
  if (++intl.n == 2) {
    GeomId args[3];
    args[0] = intl.inputs[0];
    args[1] = intl.inputs[1];
    args[2] = graph_add_value(0);
    const GeomId define = graph_add_constraint(4, intl.inputs, 1, args + 2,
                                               eval_2pt);
    board_add_object(object_create(CIRCLE, args, define, 0));
    circle_reset();
  } else {
    intl.center = id;
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