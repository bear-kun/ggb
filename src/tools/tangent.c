#include "math.h"
#include "object.h"
#include "tool.h"

static int tangent(const float *inputs, const float r1, const float r2,
                   float outputs[6]) {
  const float x1 = inputs[0], y1 = inputs[1];
  const float x2 = inputs[3], y2 = inputs[4];

  const float dx = x2 - x1, dy = y2 - y1;
  const float sr = r1 + r2;
  const float d2 = dx * dx + dy * dy;
  if (d2 < sr * sr * (1 - EPS)) return 0;

  if (d2 < sr * sr * (1 + EPS)) {
    const float nx1 = dx * sr / d2;
    const float nx2 = dx * sr / d2;
    const float ny1 = dy * sr / d2;
    const float ny2 = dy * sr / d2;
    outputs[0] = nx1;
    outputs[1] = ny1;
    outputs[2] = nx1 * x2 + ny1 * y2 - r2;
    outputs[3] = nx2;
    outputs[4] = ny2;
    outputs[5] = nx2 * x2 + ny2 * y2 - r2;
    return 1;
  }

  const float h = sqrtf(d2 - sr * sr);
  const float nx1 = (dx * sr + dy * h) / d2;
  const float nx2 = (dx * sr - dy * h) / d2;
  const float ny1 = (dy * sr - dx * h) / d2;
  const float ny2 = (dy * sr + dx * h) / d2;
  outputs[0] = nx1;
  outputs[1] = ny1;
  outputs[2] = nx1 * x2 + ny1 * y2 - r2;
  outputs[3] = nx2;
  outputs[4] = ny2;
  outputs[5] = nx2 * x2 + ny2 * y2 - r2;
  return 2;
}

static int eval_cr_pt(const float inputs[5], float outputs[6]) {
  const float r = inputs[2];
  return tangent(inputs, r, 0, outputs);
}

static int eval_2cr_inner(const float inputs[6], float outputs[6]) {
  const float r1 = inputs[2];
  const float r2 = inputs[5];
  return tangent(inputs, r1, -r2, outputs);
}

static int eval_2cr_outer(const float inputs[6], float outputs[6]) {
  const float r1 = inputs[2];
  const float r2 = inputs[5];
  return tangent(inputs, r1, r2, outputs);
}

typedef struct {
  bool deleted;
  bool has4ln;
  GeomId inner[2];
  GeomId outer[2];
} Context;

static void redo(void *ctx) {
  Context *c = ctx;
  c->deleted = false;
  board_add_object(c->inner[0]);
  board_add_object(c->inner[1]);
  if (c->has4ln) {
    board_add_object(c->outer[0]);
    board_add_object(c->outer[1]);
  }
}

static void undo(void *ctx) {
  Context *c = ctx;
  c->deleted = true;
  board_remove_object(c->inner[0]);
  board_remove_object(c->inner[1]);
  if (c->has4ln) {
    board_remove_object(c->outer[0]);
    board_remove_object(c->outer[1]);
  }
}

static void del(void *ctx) {
  const Context *c = ctx;
  if (c->deleted) {
    object_delete(c->inner[0]);
    object_delete(c->inner[1]);
    if (c->has4ln) {
      object_delete(c->outer[0]);
      object_delete(c->outer[1]);
    }
  }
}

static void process_cr_pt(const GeomId inputs[5]) {
  GeomId args[10];
  init_line(args);
  init_line(args + 5);

  const GeomId outputs[6] = {args[0], args[1], args[2],
                             args[5], args[6], args[7]};

  const GeomId define =
      graph_add_constraint(5, inputs, 6, outputs, eval_cr_pt);
  const GeomId one = object_create(LINE, args, define, 0);
  const GeomId two = object_create(LINE, args + 5, define, 1);

  GeomCommand *cmd = command_create(redo, undo, del, sizeof(Context));
  *(Context *)cmd->ctx = (Context){false, false, one, two};
  command_push(cmd, true);
}

static void process_2cr(const GeomId inputs[6]) {
  GeomId args[20];
  init_line(args);
  init_line(args + 5);
  init_line(args + 10);
  init_line(args + 15);

  GeomId outputs[12];
  copy_args(outputs, args, 3);
  copy_args(outputs + 3, args + 5, 3);
  copy_args(outputs + 6, args + 10, 3);
  copy_args(outputs + 9, args + 15, 3);

  const GeomId def_inner =
      graph_add_constraint(6, inputs, 6, outputs, eval_2cr_inner);
  const GeomId def_outer =
      graph_add_constraint(6, inputs, 6, outputs + 6, eval_2cr_outer);

  const GeomId inner1 = object_create(LINE, args, def_inner, 0);
  const GeomId inner2 = object_create(LINE, args + 5, def_inner, 1);
  const GeomId outer1 = object_create(LINE, args + 10, def_outer, 0);
  const GeomId outer2 = object_create(LINE, args + 15, def_outer, 1);

  GeomCommand *cmd = command_create(redo, undo, del, sizeof(Context));
  *(Context *)cmd->ctx = (Context)
      {false, false, inner1, inner2, outer1, outer2};
  command_push(cmd, true);
}

static struct {
  ObjectType first_t;
  GeomId first_id;
  GeomId inputs[6];
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
  if (!(obj->type & (POINT | CIRCLE))) return;

  if (id == intl.first_id) {
    reset();
    return;
  }

  if (intl.first_id == -1) {
    if (obj->type == POINT) {
      intl.first_t = POINT;
      copy_args(intl.inputs + 3, obj->args, 2);
    } else {
      intl.first_t = CIRCLE;
      copy_args(intl.inputs, obj->args, 3);
    }
    intl.first_id = id;
    board_select_object(id);
    return;
  }

  if (intl.first_t == POINT) {
    if (obj->type != CIRCLE) return;

    copy_args(intl.inputs, obj->args, 3);
    process_cr_pt(intl.inputs);
  } else {
    if (obj->type == POINT) {
      copy_args(intl.inputs + 3, obj->args, 2);
      process_cr_pt(intl.inputs);
    } else {
      copy_args(intl.inputs + 3, obj->args, 3);
      process_2cr(intl.inputs);
    }
  }

  reset();
}

void tool_tangent(GeomTool *tool) {
  tool->usage = "tangents: select point or circle";
  tool->reset = reset;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}