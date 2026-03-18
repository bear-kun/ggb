#include "object.h"
#include "tool.h"
#include <math.h>

static int bisector_eval(const float inputs[6], float outputs[6]) {
  const float nx1 = inputs[0];
  const float ny1 = inputs[1];
  const float dd1 = inputs[2];
  const float nx2 = inputs[3];
  const float ny2 = inputs[4];
  const float dd2 = inputs[5];
  const float cross = nx1 * ny2 - ny1 * nx2;
  if (fabsf(cross) < EPS) return 0;
  const float o_nx1 = nx1 + nx2;
  const float o_ny1 = ny1 + ny2;
  const float o_dd1 = dd1 + dd2;
  const float o_nx2 = nx1 - nx2;
  const float o_ny2 = ny1 - ny2;
  const float o_dd2 = dd1 - dd2;
  const float norm1 = sqrtf(o_nx1 * o_nx1 + o_ny1 * o_ny1);
  const float norm2 = sqrtf(o_nx2 * o_nx2 + o_ny2 * o_ny2);
  outputs[0] = o_nx1 / norm1;
  outputs[1] = o_ny1 / norm1;
  outputs[2] = o_dd1 / norm1;
  outputs[3] = o_nx2 / norm2;
  outputs[4] = o_ny2 / norm2;
  outputs[5] = o_dd2 / norm2;
  return 2;
}

static struct {
  int n;
  GeomId first;
  GeomId inputs[6];
} internal = {0, -1};

static void bisector_reset() {
  if (internal.first != -1) {
    board_deselect_object(internal.first);
    internal.n = 0;
    internal.first = -1;
  }
}

static void bisector_click(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id == -1) return;
  const GeomObject *obj = object_get(id);
  if (obj->type != LINE) return;

  if (id == internal.first) {
    bisector_reset();
    return;
  }

  copy_args(internal.inputs + internal.n * 3, obj->args, 3);
  if (++internal.n == 2) {
    GeomId args[10];
    init_line(args);
    init_line(args + 5);

    const GeomId outputs[6] = {args[0], args[1], args[2],
                               args[5], args[6], args[7]};

    const GeomId define = graph_add_constraint(6, internal.inputs, 6, outputs,
                                               bisector_eval);
    board_add_object(object_create(LINE, args, define, 0));
    board_add_object(object_create(LINE, args + 5, define, 1));
    bisector_reset();
  } else {
    internal.first = id;
    board_select_object(id);
  }
}

void tool_bisector(GeomTool *tool) {
  tool->usage = "angle bisector: select two lines";
  tool->reset = bisector_reset;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = bisector_click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}