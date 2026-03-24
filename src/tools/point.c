#include "object.h"
#include "tool.h"

#include <math.h>

static int eval_line(const float inputs[5], float outputs[2]) {
  const float nx = inputs[0], ny = inputs[1], dd = inputs[2];
  const float px = inputs[3], py = inputs[4];
  const float t = ny * px - nx * py;
  outputs[0] = nx * dd + ny * t;
  outputs[1] = ny * dd - nx * t;
  return 1;
}

static int eval_circle(const float inputs[5], float outputs[2]) {
  const float cx = inputs[0], cy = inputs[1], r = inputs[2];
  const float px = inputs[3], py = inputs[4];
  const float vx = px - cx, vy = py - cy;
  const float d = sqrtf(vx * vx + vy * vy);
  if (d < EPS) return 0;

  outputs[0] = cx + vx * r / d;
  outputs[1] = cy + vy * r / d;
  return 1;
}

typedef struct {
  bool deleted;
  GeomId point;
} Context;

static void redo(void *ctx) {
  Context *c = ctx;
  c->deleted = false;
  board_add_object(c->point);
}

static void undo(void *ctx) {
  Context *c = ctx;
  c->deleted = true;
  board_remove_object(c->point);
}

static void del(void *ctx) {
  const Context *c = ctx;
  if (c->deleted) {
    object_delete(c->point);
  }
}

static GeomId point_free(const Vec2 world_pos) {
  GeomId args[4];
  args[0] = graph_add_value(world_pos.x);
  args[1] = graph_add_value(world_pos.y);
  copy_args(args + 2, args, 2);
  return object_create(POINT, args, -1, 0);
}

static GeomId point_on_line(const GeomObject *ln, const Vec2 world_pos) {
  GeomId args[4];
  args[0] = graph_add_value(0);
  args[1] = graph_add_value(0);
  args[2] = graph_add_value(world_pos.x);
  args[3] = graph_add_value(world_pos.y);

  GeomId inputs[5];
  copy_args(inputs, ln->args, 3);
  copy_args(inputs + 3, args + 2, 2);

  const GeomId define = graph_add_constraint(5, inputs, 2, args, eval_line);
  return object_create(POINT, args, define, 0);
}

static GeomId point_on_circle(const GeomObject *cr, const Vec2 world_pos) {
  GeomId args[4];
  args[0] = graph_add_value(0);
  args[1] = graph_add_value(0);
  args[2] = graph_add_value(world_pos.x);
  args[3] = graph_add_value(world_pos.y);

  GeomId inputs[5];
  copy_args(inputs, cr->args, 3);
  copy_args(inputs + 3, args + 2, 2);

  const GeomId define = graph_add_constraint(5, inputs, 2, args, eval_circle);
  return object_create(POINT, args, define, 0);
}

static void process(const GeomId id, const Vec2 pos) {
  GeomId pt;
  if (id == -1) {
    pt = point_free(xform_to_world(pos));
  } else {
    const GeomObject *obj = object_get(id);
    if (obj->type == POINT) return;

    const Vec2 world_pos = xform_to_world(pos);
    if (obj->type == LINE) {
      pt = point_on_line(obj, world_pos);
    } else {
      pt = point_on_circle(obj, world_pos);
    }
  }

  GeomCommand *cmd = command_create(redo, undo, del, sizeof(Context));
  *(Context *)cmd->ctx = (Context){false, pt};
  command_push(cmd, true);
}

static void click(const Vec2 pos) {
  const GeomId id = board_hovered_object();
  process(id, pos);
}

void tool_point(GeomTool *tool) {
  tool->usage = "point: select position";
  tool->reset = NULL;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}