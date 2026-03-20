#include "object.h"
#include "tool.h"
#include <math.h>

static int eval(const float xyxy[4], float line[3]) {
  const float x1 = xyxy[0], y1 = xyxy[1];
  const float x2 = xyxy[2], y2 = xyxy[3];
  const float dx = x2 - x1, dy = y2 - y1;

  const float dist = sqrtf(dx * dx + dy * dy);
  if (dist < EPS) return 0;

  const float nx = -dy / dist;
  const float ny = dx / dist;
  line[0] = nx;
  line[1] = ny;
  line[2] = nx * x1 + ny * y1; // dd = n · (x, y)
  return 1;
}

static int clip_end_point(const float inputs[4], float *t[1]) {
  const float nx = inputs[0];
  const float ny = inputs[1];
  const float px = inputs[2];
  const float py = inputs[3];
  *t[0] = ny * px - nx * py; // (ny, -nx) · (px, py)
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
    GeomId args[5];
    init_line(args);
    const GeomId define = graph_add_constraint(4, intl.inputs, 3, args,
                                               eval);
    board_add_object(object_create(LINE, args, define, 0));
    reset();
  } else {
    intl.first = id;
    board_select_object(id);
  }
}

void tool_line(GeomTool *tool) {
  tool->usage = "line: select two points or positions";
  tool->reset = reset;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}