#include "object.h"
#include "tool.h"
#include <math.h>

static int eval(const float inputs[6], float outputs[6]) {
  const float nx1 = inputs[0], ny1 = inputs[1], dd1 = inputs[2];
  const float nx2 = inputs[3], ny2 = inputs[4], dd2 = inputs[5];

  const float cross = nx1 * ny2 - ny1 * nx2;
  if (fabsf(cross) < EPS) return 0;

  const float o_nx1 = nx1 + nx2, o_ny1 = ny1 + ny2, o_dd1 = dd1 + dd2;
  const float o_nx2 = nx1 - nx2, o_ny2 = ny1 - ny2, o_dd2 = dd1 - dd2;
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

typedef struct {
  bool deleted;
  GeomId one, two;
} Context;

static void redo(void *ctx) {
  Context *c = ctx;
  c->deleted = false;
  board_add_object(c->one);
  board_add_object(c->two);
}

static void undo(void *ctx) {
  Context *c = ctx;
  c->deleted = true;
  board_remove_object(c->one);
  board_remove_object(c->two);
}

static void del(void *ctx) {
  const Context *c = ctx;
  if (c->deleted) {
    object_delete(c->one);
    object_delete(c->two);
  }
}

static void process(const GeomId inputs[6]) {
  GeomId args[10];
  init_line(args);
  init_line(args + 5);

  GeomId outputs[6];
  copy_args(outputs, args, 3);
  copy_args(outputs + 3, args + 5, 3);

  const GeomId define = graph_add_constraint(6, inputs, 6, outputs, eval);
  const GeomId one = object_create(LINE, args, define, 0);
  const GeomId two = object_create(LINE, args + 5, define, 1);

  GeomCommand *cmd = command_create(redo, undo, del, sizeof(Context));
  *(Context *)cmd->ctx = (Context){false, one, two};
  command_push(cmd, true);
}

static struct {
  int n;
  GeomId first;
  GeomId inputs[6];
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
  if (obj->type != LINE) return;

  if (id == intl.first) {
    reset();
    return;
  }

  copy_args(intl.inputs + intl.n * 3, obj->args, 3);
  if (++intl.n == 2) {
    process(intl.inputs);
    reset();
  } else {
    intl.first = id;
    board_select_object(id);
  }
}

void tool_bisector(GeomTool *tool) {
  tool->usage = "angle bisector: select two lines";
  tool->reset = reset;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}