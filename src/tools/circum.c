#include "object.h"
#include "tool.h"
#include <math.h>

static int eval(const float inputs[6], float outputs[3]) {
  const float x1 = inputs[0], y1 = inputs[1];
  const float x2 = inputs[2], y2 = inputs[3];
  const float x3 = inputs[4], y3 = inputs[5];
  const float D = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
  if (fabsf(D) < EPS) return 0;

  const float sq_xy1 = x1 * x1 + y1 * y1;
  const float sq_xy2 = x2 * x2 + y2 * y2;
  const float sq_xy3 = x3 * x3 + y3 * y3;
  const float cx =
      (sq_xy1 * (y2 - y3) + sq_xy2 * (y3 - y1) + sq_xy3 * (y1 - y2)) / D;
  const float cy =
      (sq_xy1 * (x3 - x2) + sq_xy2 * (x1 - x3) + sq_xy3 * (x2 - x1)) / D;
  const float rx = x1 - cx;
  const float ry = y1 - cy;
  outputs[0] = cx;
  outputs[1] = cy;
  outputs[2] = sqrtf(rx * rx + ry * ry);
  return 1;
}

typedef struct {
  bool deleted;
  GeomId circle;
} Context;

static void redo(void *ctx) {
  Context *c = ctx;
  c->deleted = false;
  board_add_object(c->circle);
}

static void undo(void *ctx) {
  Context *c = ctx;
  c->deleted = true;
  board_remove_object(c->circle);
}

static void del(void *ctx) {
  const Context *c = ctx;
  if (c->deleted) {
    object_delete(c->circle);
  }
}

static void process(const GeomId inputs[6]) {
  GeomId args[3];
  args[0] = graph_add_value(0);
  args[1] = graph_add_value(0);
  args[2] = graph_add_value(0);

  const GeomId define = graph_add_constraint(6, inputs, 3, args, eval);
  const GeomId cr = object_create(CIRCLE, args, define, 0);

  GeomCommand *cmd = command_create(redo, undo, del, sizeof(Context));
  *(Context *)cmd->ctx = (Context){false, cr};
  command_push(cmd, true);
}

static struct {
  GeomId points[3];
  GeomId inputs[6];
} intl = {0, -1, -1, -1};

static void reset() {
  for (int i = 0; i < 3; i++) {
    if (intl.points[i] != -1) {
      board_deselect_object(intl.points[i]);
      intl.points[i] = -1;
    }
  }
}

static void click(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id == -1) return;
  const GeomObject *obj = object_get(id);
  if (obj->type != POINT) return;

  for (int i = 0; i < 3; i++) {
    if (intl.points[i] != -1) {
      if (!board_exist(intl.points[i])) {
        intl.points[i] = -1;
      } else if (id == intl.points[i]) {
        board_deselect_object(intl.points[i]);
        intl.points[i] = -1;
        return;
      }
    }
  }

  for (int i = 0; i < 3; i++) {
    if (intl.points[i] == -1) {
      intl.points[i] = id;
      board_select_object(id);
      copy_args(intl.inputs + i * 2, obj->args, 2);
      return;
    }
  }

  process(intl.inputs);
  reset();
}

void tool_circum(GeomTool *tool) {
  tool->usage = "circumcircle: select three points";
  tool->reset = reset;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}