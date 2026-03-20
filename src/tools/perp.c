#include "object.h"
#include "tool.h"

static int eval(const float inputs[4], float output[3]) {
  const float nx = inputs[0], ny = inputs[1];
  const float px = inputs[2], py = inputs[3];
  output[0] = -ny;
  output[1] = nx;
  output[2] = -ny * px + nx * py; // np · (px, py)
  return 1;
}

static struct {
  ObjectType first_t;
  GeomId first_id;
  GeomId inputs[4];
} intl = {UNKNOWN, -1};

static void reset() {
  if (intl.first_id != -1) {
    board_deselect_object(intl.first_id);
    intl.first_t = UNKNOWN;
    intl.first_id = -1;
  }
}

static void click(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id == -1) return;
  const GeomObject *obj = object_get(id);
  if (!(obj->type & (POINT | LINE))) return;

  if (id == intl.first_id) {
    reset();
    return;
  }


  if (intl.first_id == -1) {
    if (obj->type == LINE) {
      intl.first_t = LINE;
      copy_args(intl.inputs, obj->args, 2);
    } else {
      intl.first_t = POINT;
      copy_args(intl.inputs + 2, obj->args, 2);
    }
    intl.first_id = id;
    board_select_object(id);
    return;
  }

  if (obj->type == intl.first_t) return;

  if (obj->type == LINE) {
    copy_args(intl.inputs, obj->args, 2);
  } else {
    copy_args(intl.inputs + 2, obj->args, 2);
  }

  GeomId args[5];
  init_line(args);
  const GeomId define = graph_add_constraint(4, intl.inputs, 3, args, eval);
  board_add_object(object_create(LINE, args, define, 0));
  reset();
}

void tool_perp(GeomTool *tool) {
  tool->usage = "perpendicular line: select line and point";
  tool->reset = reset;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}