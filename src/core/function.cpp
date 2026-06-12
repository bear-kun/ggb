#include "internal.h"
#include <cmath>

namespace geom {
static constexpr float HUGE_VALUE = 4096.f;
static constexpr float EPS = 1e-5f;

static GeomId new_arg(const float value = 0) {
  return graph::add_value(value);
}

static void init_line(GeomId *args) {
  args[0] = new_arg();
  args[1] = new_arg();
  args[2] = new_arg();
  args[3] = new_arg(-HUGE_VALUE);
  args[4] = new_arg(HUGE_VALUE);
}

static void copy_args(GeomId *dst, const GeomId *src, const int n) {
  for (int i = 0; i < n; i++) dst[i] = src[i];
}

// --- point ---
static int eval_point_on_line(const float inputs[5], float outputs[2]) {
  const float nx = inputs[0], ny = inputs[1], dd = inputs[2];
  const float px = inputs[3], py = inputs[4];
  const float t = ny * px - nx * py;
  outputs[0] = nx * dd + ny * t;
  outputs[1] = ny * dd - nx * t;
  return 1;
}

static int eval_point_on_circle(const float inputs[5], float outputs[2]) {
  const float cx = inputs[0], cy = inputs[1], r = inputs[2];
  const float px = inputs[3], py = inputs[4];
  const float vx = px - cx, vy = py - cy;
  const float d = sqrtf(vx * vx + vy * vy);
  if (d < EPS) return 0;

  outputs[0] = cx + vx * r / d;
  outputs[1] = cy + vy * r / d;
  return 1;
}

Handle new_point(const float x, const float y, const Handle on) {
  if (!on.valid()) {
    GeomId args[4] = {new_arg(x), new_arg(y)};
    copy_args(args + 2, args, 2);
    return new_object(POINT, args);
  }

  GeomId inputs[5];
  copy_args(inputs, on->data.args, 3);
  inputs[3] = graph::add_value(x);
  inputs[4] = graph::add_value(y);

  GeomId args[4] = {new_arg(x), new_arg(y)};
  copy_args(args + 2, inputs + 3, 2);

  const EvalType eval = on->type == LINE ? EVAL_POINT_ON_LINE : EVAL_POINT_ON_CIRCLE;
  const GeomId define = graph::add_constraint(5, inputs, 2, args, eval);
  return new_object(POINT, args, define);
}

// --- line ---
static int eval_line(const float xyxy[4], float line[3]) {
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

Handle new_line(const Handle pt1, const Handle pt2) {
  GeomId inputs[4];
  copy_args(inputs, pt1->data.args, 2);
  copy_args(inputs + 2, pt2->data.args, 2);

  GeomId args[5];
  init_line(args);

  const GeomId define = graph::add_constraint(4, inputs, 3, args, EVAL_LINE);
  return new_object(LINE, args, define);
}

// --- circle ---
static int eval_circle(const float xyxy[4], float radius[1]) {
  const float dx = xyxy[2] - xyxy[0];
  const float dy = xyxy[3] - xyxy[1];
  radius[0] = sqrtf(dx * dx + dy * dy);
  return 1;
}

Handle new_circle(const Handle center, const Handle pt) {
  GeomId inputs[4];
  copy_args(inputs, center->data.args, 2);
  copy_args(inputs + 2, pt->data.args, 2);

  GeomId args[3];
  copy_args(args, center->data.args, 2);
  args[2] = new_arg();

  const GeomId define = graph::add_constraint(4, inputs, 1, args + 2, EVAL_CIRCLE);
  return new_object(CIRCLE, args, define);
}

// --- midpoint ---
static int eval_midpoint(const float inputs[4], float outputs[2]) {
  const float x1 = inputs[0], y1 = inputs[1];
  const float x2 = inputs[2], y2 = inputs[3];
  outputs[0] = (x1 + x2) / 2.f;
  outputs[1] = (y1 + y2) / 2.f;
  return 1;
}

Handle midpoint(const Handle pt1, const Handle pt2) {
  GeomId inputs[4];
  copy_args(inputs, pt1->data.args, 2);
  copy_args(inputs + 2, pt2->data.args, 2);

  GeomId args[4] = {new_arg(), new_arg()};
  copy_args(args + 2, args, 2);

  const GeomId define = graph::add_constraint(4, inputs, 2, args, EVAL_MIDPOINT);
  return new_object(POINT, args, define);
}

// --- parallel ---
static int eval_parallel(const float inputs[4], float outputs[1]) {
  const float nx = inputs[0], ny = inputs[1];
  const float px = inputs[2], py = inputs[3];
  outputs[0] = nx * px + ny * py; // parallel line dd
  return 1;
}

Handle parallel(const Handle ln, const Handle pt) {
  GeomId inputs[4];
  copy_args(inputs, ln->data.args, 2);
  copy_args(inputs + 2, pt->data.args, 2);

  GeomId args[5];
  copy_args(args, ln->data.args, 2);
  args[2] = new_arg();
  args[3] = new_arg(-HUGE_VALUE);
  args[4] = new_arg(HUGE_VALUE);

  const GeomId define = graph::add_constraint(4, inputs, 1, args + 2, EVAL_PARALLEL);
  return new_object(LINE, args, define);
}

// --- perpendicular ---
static int eval_perpendicular(const float inputs[4], float output[3]) {
  const float nx = inputs[0], ny = inputs[1];
  const float px = inputs[2], py = inputs[3];
  output[0] = -ny;
  output[1] = nx;
  output[2] = -ny * px + nx * py; // np · (px, py)
  return 1;
}

Handle perpendicular(const Handle ln, const Handle pt) {
  GeomId inputs[4];
  copy_args(inputs, ln->data.args, 2);
  copy_args(inputs + 2, pt->data.args, 2);

  GeomId args[5];
  init_line(args);

  const GeomId define = graph::add_constraint(4, inputs, 3, args, EVAL_PERPENDICULAR);
  return new_object(LINE, args, define);
}

// --- angle bisector ---
static int eval_angle_bisector(const float inputs[6], float outputs[6]) {
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

std::array<Handle, 2> angle_bisector(const Handle ln1, const Handle ln2) {
  GeomId inputs[6];
  copy_args(inputs, ln1->data.args, 3);
  copy_args(inputs + 3, ln2->data.args, 3);

  GeomId args[10];
  init_line(args);
  init_line(args + 5);

  GeomId outputs[6];
  copy_args(outputs, args, 3);
  copy_args(outputs + 3, args + 5, 3);

  const GeomId define = graph::add_constraint(6, inputs, 6, outputs, EVAL_ANGLE_BISECTOR);
  return {
      new_object(LINE, args, define, 0),
      new_object(LINE, args + 5, define, 1)
  };
}

// --- tangent ---
static int eval_tangent(const float *inputs, const float r1, const float r2,
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

static int eval_tangent_point(const float inputs[5], float outputs[6]) {
  const float r = inputs[2];
  return eval_tangent(inputs, r, 0, outputs);
}

static int eval_tangent_inner(const float inputs[6], float outputs[6]) {
  const float r1 = inputs[2];
  const float r2 = inputs[5];
  return eval_tangent(inputs, r1, -r2, outputs);
}

static int eval_tangent_outer(const float inputs[6], float outputs[6]) {
  const float r1 = inputs[2];
  const float r2 = inputs[5];
  return eval_tangent(inputs, r1, r2, outputs);
}

std::array<Handle, 4> tangent(const Handle cr, const Handle cr_or_pt) {
  if (cr_or_pt->type == POINT) {
    GeomId inputs[5];
    copy_args(inputs, cr->data.args, 3);
    copy_args(inputs + 3, cr_or_pt->data.args, 2);

    GeomId args[10];
    init_line(args);
    init_line(args + 5);

    GeomId outputs[6];
    copy_args(outputs, args, 3);
    copy_args(outputs + 3, args + 5, 3);

    const GeomId define = graph::add_constraint(5, inputs, 6, outputs, EVAL_TANGENT_POINT);
    return {
        new_object(LINE, args, define, 0),
        new_object(LINE, args + 5, define, 1),
        Handle(), Handle()
    };
  }

  GeomId inputs[6];
  copy_args(inputs, cr->data.args, 3);
  copy_args(inputs + 3, cr_or_pt->data.args, 3);

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

  const GeomId def_in = graph::add_constraint(6, inputs, 6, outputs, EVAL_TANGENT_INNER);
  const GeomId def_out = graph::add_constraint(6, inputs, 6, outputs + 6, EVAL_TANGENT_OUTER);

  return {
      new_object(LINE, args, def_in, 0),
      new_object(LINE, args + 5, def_in, 1),
      new_object(LINE, args + 10, def_out, 0),
      new_object(LINE, args + 15, def_out, 1)
  };
}

// --- circumcircle ---
static int eval_circumcircle(const float inputs[6], float outputs[3]) {
  const float x1 = inputs[0], y1 = inputs[1];
  const float x2 = inputs[2], y2 = inputs[3];
  const float x3 = inputs[4], y3 = inputs[5];
  const float D = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
  if (fabsf(D) < EPS) return 0;

  const float sq_xy1 = x1 * x1 + y1 * y1;
  const float sq_xy2 = x2 * x2 + y2 * y2;
  const float sq_xy3 = x3 * x3 + y3 * y3;
  const float cx = (sq_xy1 * (y2 - y3) + sq_xy2 * (y3 - y1) + sq_xy3 * (y1 - y2)) / D;
  const float cy = (sq_xy1 * (x3 - x2) + sq_xy2 * (x1 - x3) + sq_xy3 * (x2 - x1)) / D;
  const float rx = x1 - cx;
  const float ry = y1 - cy;
  outputs[0] = cx;
  outputs[1] = cy;
  outputs[2] = sqrtf(rx * rx + ry * ry);
  return 1;
}

Handle circumcircle(const Handle pt1, const Handle pt2, const Handle pt3) {
  GeomId inputs[6];
  copy_args(inputs, pt1->data.args, 2);
  copy_args(inputs + 2, pt2->data.args, 2);
  copy_args(inputs + 4, pt3->data.args, 2);

  const GeomId args[3] = {new_arg(), new_arg(), new_arg()};

  const GeomId define = graph::add_constraint(6, inputs, 3, args, EVAL_CIRCUMCIRCLE);
  return new_object(CIRCLE, args, define);
}

// --- intersection point ---
static int eval_intersection_line_line(const float inputs[6], float outputs[2]) {
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

static int eval_intersection_line_circle(const float inputs[6], float outputs[4]) {
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

static int eval_intersection_circle_circle(const float inputs[6], float outputs[4]) {
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

std::array<Handle, 2> intersection(const Handle ln_or_cr1, const Handle ln_or_cr2) {
  if (ln_or_cr1->type == LINE && ln_or_cr2->type == LINE) {
    GeomId inputs[6];
    copy_args(inputs, ln_or_cr1->data.args, 3);
    copy_args(inputs + 3, ln_or_cr2->data.args, 3);

    GeomId args[4] = {new_arg(), new_arg()};
    copy_args(args + 2, args, 2);

    const GeomId define = graph::add_constraint(6, inputs, 2, args, EVAL_INTERSECTION_LINE_LINE);
    return {new_object(POINT, args, define), Handle()};
  }

  GeomId args[8] = {new_arg(), new_arg(), 0, 0, new_arg(), new_arg()};
  copy_args(args + 2, args, 2);
  copy_args(args + 6, args + 4, 2);

  if (ln_or_cr1->type == CIRCLE && ln_or_cr2->type == CIRCLE) {
    GeomId inputs[6];
    copy_args(inputs, ln_or_cr1->data.args, 3);
    copy_args(inputs + 3, ln_or_cr2->data.args, 3);

    const GeomId define = graph::add_constraint(6, inputs, 4, args + 2,
                                                EVAL_INTERSECTION_CIRCLE_CIRCLE);
    return {
        new_object(POINT, args, define, 0),
        new_object(POINT, args + 4, define, 1)
    };
  }

  GeomId inputs[6];
  if (ln_or_cr1->type == LINE) {
    copy_args(inputs, ln_or_cr1->data.args, 3);
    copy_args(inputs + 3, ln_or_cr2->data.args, 3);
  } else {
    copy_args(inputs, ln_or_cr2->data.args, 3);
    copy_args(inputs + 3, ln_or_cr1->data.args, 3);
  }

  const GeomId define =
      graph::add_constraint(6, inputs, 4, args + 2, EVAL_INTERSECTION_LINE_CIRCLE);
  return {new_object(POINT, args, define, 0),
          new_object(POINT, args + 4, define, 1)};
}

// --- move ---
void move(const Handle pt, const Vec2 to) {
  graph::change_value(2, pt->data.args + 2, reinterpret_cast<const float *>(&to));
}

const ValueEval eval_map[] = {
    nullptr, // NULL
    eval_point_on_line, // POINT_ON_LINE
    eval_point_on_circle, // POINT_ON_CIRCLE
    eval_line, // LINE
    eval_circle, // CIRCLE
    eval_midpoint, // MIDPOINT
    eval_parallel, // PARALLEL
    eval_perpendicular, // PERPENDICULAR
    eval_angle_bisector, // ANGLE_BISECTOR
    eval_tangent_point, // TANGENT_POINT
    eval_tangent_inner, // TANGENT_INNER
    eval_tangent_outer, // TANGENT_OUTER
    eval_circumcircle, // CIRCUMCIRCLE
    eval_intersection_line_line, // INTERSECTION_LINE_LINE
    eval_intersection_line_circle, //INTERSECTION_LINE_CIRCLE
    eval_intersection_circle_circle //INTERSECTION_CIRCLE_CIRCLE
};
}