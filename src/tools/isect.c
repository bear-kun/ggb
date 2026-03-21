#include "object.h"
#include "tool.h"
#include <math.h>

static int eval_2ln(const float inputs[6], float outputs[2]) {
  const float nx1 = inputs[0];
  const float ny1 = inputs[1];
  const float dd1 = inputs[2];
  const float nx2 = inputs[3];
  const float ny2 = inputs[4];
  const float dd2 = inputs[5];
  const float D = nx1 * ny2 - nx2 * ny1;
  if (fabsf(D) < EPS) return 0;
  outputs[0] = (ny2 * dd1 - ny1 * dd2) / D;
  outputs[1] = (nx1 * dd2 - nx2 * dd1) / D;
  return 1;
}

static int eval_ln_cr(const float inputs[6], float outputs[4]) {
  const float nx = inputs[0];
  const float ny = inputs[1];
  const float dd = inputs[2];
  const float cx = inputs[3];
  const float cy = inputs[4];
  const float r = inputs[5];
  const float A = dd - nx * cx - ny * cy;
  if (fabsf(A) > r * (1 + EPS)) return 0;
  if (fabsf(A) > r * (1 - EPS)) {
    // tangent
    const float tx = A * nx + cx;
    const float ty = A * ny + cy;
    outputs[0] = tx;
    outputs[1] = ty;
    outputs[2] = tx;
    outputs[3] = ty;
    return 1;
  }
  const float B = sqrtf(r * r - A * A);
  outputs[0] = A * nx - B * ny + cx;
  outputs[1] = A * ny + B * nx + cy;
  outputs[2] = A * nx + B * ny + cx;
  outputs[3] = A * ny - B * nx + cy;
  return 2;
}

static int eval_2cr(const float inputs[6], float outputs[4]) {
  const float x1 = inputs[0];
  const float y1 = inputs[1];
  const float r1 = inputs[2];
  const float x2 = inputs[3];
  const float y2 = inputs[4];
  const float r2 = inputs[5];
  const float dx = x2 - x1;
  const float dy = y2 - y1;
  const float d = sqrtf(dx * dx + dy * dy);
  if (d < EPS || d > (r1 + r2) * (1 + EPS)) return 0;
  const float a = (d * d + r1 * r1 - r2 * r2) / (2 * d);
  const float ux = dx / d;
  const float uy = dy / d;
  const float px = x1 + a * ux;
  const float py = y1 + a * uy;
  if (d > (r1 + r2) * (1 - EPS)) {
    outputs[0] = px;
    outputs[1] = py;
    outputs[2] = px;
    outputs[3] = py;
    return 1;
  }
  const float h = sqrtf(r1 * r1 - a * a);
  outputs[0] = px + h * uy;
  outputs[1] = py - h * ux;
  outputs[2] = px - h * uy;
  outputs[3] = py + h * ux;
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
  if (c->two != -1) board_add_object(c->two);
}

static void undo(void *ctx) {
  Context *c = ctx;
  c->deleted = true;
  board_remove_object(c->one);
  if (c->two != -1) board_remove_object(c->two);
}

static void del(void *ctx) {
  const Context *c = ctx;
  if (c->deleted) {
    object_delete(c->one);
    if (c->two != -1) object_delete(c->two);
  }
}

static void process_2ln(const GeomId inputs[6]) {
  GeomId args[2];
  args[0] = graph_add_value(0);
  args[1] = graph_add_value(0);

  const GeomId define = graph_add_constraint(6, inputs, 2, args, eval_2ln);
  const GeomId pt = object_create(POINT, args, define, 0);

  GeomCommand *cmd = command_create(redo, undo, del, sizeof(Context));
  *(Context *)cmd->ctx = (Context){false, pt, -1};
  command_push(cmd, true);
}

static void process_2pt(const GeomId inputs[6], const ValueEval eval) {
  GeomId args[4];
  args[0] = graph_add_value(0);
  args[1] = graph_add_value(0);
  args[2] = graph_add_value(0);
  args[3] = graph_add_value(0);

  const GeomId define = graph_add_constraint(6, inputs, 4, args, eval);
  const GeomId one = object_create(POINT, args, define, 0);
  const GeomId two = object_create(POINT, args + 2, define, 1);

  GeomCommand *cmd = command_create(redo, undo, del, sizeof(Context));
  *(Context *)cmd->ctx = (Context){false, one, two};
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
  if (!(obj->type & (LINE | CIRCLE))) return;

  if (id == intl.first_id) {
    reset();
    return;
  }

  if (intl.first_id == -1) {
    if (obj->type == LINE) {
      copy_args(intl.inputs, obj->args, 3);
      intl.first_t = LINE;
    } else {
      copy_args(intl.inputs + 3, obj->args, 3);
      intl.first_t = CIRCLE;
    }
    intl.first_id = id;
    board_select_object(id);
    return;
  }

  if (intl.first_t == LINE) {
    copy_args(intl.inputs + 3, obj->args, 3);
    if (obj->type == LINE) {
      process_2ln(intl.inputs);
    } else {
      process_2pt(intl.inputs, eval_ln_cr);
    }
  } else {
    copy_args(intl.inputs, obj->args, 3);
    process_2pt(intl.inputs, obj->type == LINE
                               ? eval_ln_cr
                               : eval_2cr);
  }

  reset();
}

void tool_isect(GeomTool *tool) {
  tool->usage = "intersection point: select two lines, circles or both";
  tool->reset = reset;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}