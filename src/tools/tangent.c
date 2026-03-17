#include "math.h"
#include "object.h"
#include "tool.h"

static int tangent(const float *inputs, const float r1, const float r2,
                   float *outputs[6]) {
  const float x1 = inputs[0];
  const float y1 = inputs[1];
  const float x2 = inputs[3];
  const float y2 = inputs[4];
  const float dx = x2 - x1;
  const float dy = y2 - y1;
  const float sr = r1 + r2;
  const float d2 = dx * dx + dy * dy;
  if (d2 < sr * sr * (1 - EPS)) return 0;
  if (d2 < sr * sr * (1 + EPS)) {
    const float nx1 = dx * sr / d2;
    const float nx2 = dx * sr / d2;
    const float ny1 = dy * sr / d2;
    const float ny2 = dy * sr / d2;
    *outputs[0] = nx1;
    *outputs[1] = ny1;
    *outputs[2] = nx1 * x2 + ny1 * y2 - r2;
    *outputs[3] = nx2;
    *outputs[4] = ny2;
    *outputs[5] = nx2 * x2 + ny2 * y2 - r2;
    return 1;
  }

  const float h = sqrtf(d2 - sr * sr);
  const float nx1 = (dx * sr + dy * h) / d2;
  const float nx2 = (dx * sr - dy * h) / d2;
  const float ny1 = (dy * sr - dx * h) / d2;
  const float ny2 = (dy * sr + dx * h) / d2;
  *outputs[0] = nx1;
  *outputs[1] = ny1;
  *outputs[2] = nx1 * x2 + ny1 * y2 - r2;
  *outputs[3] = nx2;
  *outputs[4] = ny2;
  *outputs[5] = nx2 * x2 + ny2 * y2 - r2;
  return 2;
}

static int tangent_circle_point(const float inputs[5], float *outputs[6]) {
  const float r = inputs[2];
  return tangent(inputs, r, 0, outputs);
}

static int tangent_circles_inner(const float inputs[6], float *outputs[6]) {
  const float r1 = inputs[2];
  const float r2 = inputs[5];
  return tangent(inputs, r1, -r2, outputs);
}

static int tangent_circles_outer(const float inputs[6], float *outputs[6]) {
  const float r1 = inputs[2];
  const float r2 = inputs[5];
  return tangent(inputs, r1, r2, outputs);
}

static struct {
  ObjectType first_t;
  GeomId first_id;
  GeomId inputs[6];
} internal = {UNKNOWN, -1};


static void create_tangents_cp() {
  GeomId args[10];
  init_line(args);
  init_line(args + 5);

  const GeomId outputs[6] = {args[0], args[1], args[2],
                             args[5], args[6], args[7]};

  const GeomId define = graph_add_constraint(5, internal.inputs, 6, outputs,
                                             tangent_circle_point);
  const GeomId one = object_create(LINE, args);
  const GeomId two = object_create(LINE, args + 5);
  object_set_coincident(two, define);
  board_add_object(one);
  board_add_object(two);
}

static void create_tangents_cc() {
  GeomId args[20];
  init_line(args);
  init_line(args + 5);
  init_line(args + 10);
  init_line(args + 15);

  const GeomId outputs[12] = {args[0], args[1], args[2], args[5],
                              args[6], args[7], args[10], args[11],
                              args[12], args[15], args[16], args[17]};

  const GeomId inner = graph_add_constraint(6, internal.inputs, 6, outputs,
                                            tangent_circles_inner);
  const GeomId outer = graph_add_constraint(6, internal.inputs, 6, outputs + 6,
                                            tangent_circles_outer);

  const GeomId inner_one = object_create(LINE, args);
  const GeomId inner_two = object_create(LINE, args + 5);
  const GeomId outer_one = object_create(LINE, args + 10);
  const GeomId outer_two = object_create(LINE, args + 15);
  object_set_coincident(inner_two, inner);
  object_set_coincident(outer_two, outer);
  board_add_object(inner_one);
  board_add_object(inner_two);
  board_add_object(outer_one);
  board_add_object(outer_two);
}

static void tangent_reset() {
  if (internal.first_id != -1) {
    board_deselect_object(internal.first_id);
    internal.first_t = UNKNOWN;
    internal.first_id = -1;
  }
}

static void tangent_click(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id == -1) return;
  const GeomObject *obj = object_get(id);
  if (!(obj->type & (POINT | CIRCLE))) return;

  if (id == internal.first_id) {
    tangent_reset();
    return;
  }

  if (internal.first_id == -1) {
    if (obj->type == POINT) {
      internal.first_t = POINT;
      copy_args(internal.inputs + 3, obj->args, 2);
    } else {
      internal.first_t = CIRCLE;
      copy_args(internal.inputs, obj->args, 3);
    }
    internal.first_id = id;
    board_select_object(id);
    return;
  }

  if (internal.first_t == POINT) {
    if (obj->type != CIRCLE) return;

    copy_args(internal.inputs, obj->args, 3);
    create_tangents_cp();
  } else {
    if (obj->type == POINT) {
      copy_args(internal.inputs + 3, obj->args, 2);
      create_tangents_cp();
    } else {
      copy_args(internal.inputs + 3, obj->args, 3);
      create_tangents_cc();
    }
  }

  tangent_reset();
}

void tool_tangent(GeomTool *tool) {
  tool->usage = "tangents: select point or circle";
  tool->reset = tangent_reset;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = tangent_click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}