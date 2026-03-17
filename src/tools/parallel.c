#include "object.h"
#include "tool.h"

static int parallel_eval(const float inputs[4], float *outputs[3]) {
  const float nx = inputs[0];
  const float ny = inputs[1];
  const float px = inputs[2];
  const float py = inputs[3];
  *outputs[0] = nx;
  *outputs[1] = ny;
  *outputs[2] = nx * px + ny * py; // parallel line dd
  return 1;
}

static struct {
  ObjectType first_t;
  GeomId first_id;
  GeomId inputs[4];
} internal = {UNKNOWN, -1};

static void parallel_reset() {
  if (internal.first_id != -1) {
    board_deselect_object(internal.first_id);
    internal.first_t = UNKNOWN;
    internal.first_id = -1;
  }
}

static void parallel_click(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id == -1) return;
  const GeomObject *obj = object_get(id);
  if (!(obj->type & (POINT | LINE))) return;

  if (id == internal.first_id) {
    parallel_reset();
    return;
  }

  if (internal.first_id == -1) {
    if (obj->type == LINE) {
      internal.first_t = LINE;
      copy_args(internal.inputs, obj->args, 2);
    } else {
      internal.first_t = POINT;
      copy_args(internal.inputs + 2, obj->args, 2);
    }

    internal.first_id = id;
    board_select_object(id);
    return;
  }

  if (obj->type == internal.first_t) return;

  if (obj->type == LINE) {
    copy_args(internal.inputs, obj->args, 2);
  } else {
    copy_args(internal.inputs + 2, obj->args, 2);
  }

  GeomId args[5];
  init_line(args);
  graph_add_constraint(4, internal.inputs, 3, args, parallel_eval);
  board_add_object(object_create(LINE, args));
  parallel_reset();
}

void tool_parallel(GeomTool *tool) {
  tool->usage = "parallel line: select line and point";
  tool->reset = parallel_reset;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = parallel_click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}