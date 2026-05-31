#include "geometry.h"
#include "graph.h"
#include <math.h>

static void copy_args(GeomId *dst, const GeomId *src, const int n) {
  for (int i = 0; i < n; i++) dst[i] = src[i];
}

static void init_line(GeomId args[5]) {
  args[0] = graph_add_value(0);
  args[1] = graph_add_value(0);
  args[2] = graph_add_value(0);
  args[3] = graph_add_value(-HUGE_VALUE);
  args[4] = graph_add_value(HUGE_VALUE);
}

static GeomId create_point(const GeomId xy[2], const GeomId define, const GeomId soln_id) {
  GeomId args[4];
  copy_args(args, xy, 2);
  copy_args(args + 2, args, 2);
  return geom_new_object(POINT, args, define, soln_id);
}

// --- basis ---
bool geom_get_point(const CGeometry *pt, float xy[2]) {
  if (pt->define != -1 && graph_is_degenerate(pt->define, pt->soln_id)) return false;
  return graph_get_values(2, pt->args, xy);
}

bool geom_get_line(const CGeometry *ln, float pt1[2], float pt2[2]) {
  if (ln->define != -1 && graph_is_degenerate(ln->define, ln->soln_id)) return false;

  float args[5];
  if (!graph_get_values(5, ln->args, args)) return false;

  const float nx = args[0], ny = args[1], dd = args[2];
  const float t1 = args[3], t2 = args[4];
  pt1[0] = nx * dd + ny * t1;
  pt1[1] = ny * dd - nx * t1;
  pt2[0] = nx * dd + ny * t2;
  pt2[1] = ny * dd - nx * t2;
  return true;
}

bool geom_get_circle(const CGeometry *cr, float center[2], float *radius) {
  if (cr->define != -1 && graph_is_degenerate(cr->define, cr->soln_id)) return false;

  float args[3];
  if (!graph_get_values(3, cr->args, args)) return false;

  center[0] = args[0];
  center[1] = args[1];
  *radius = args[2];
  return true;
}

// --- point ---
static int pt_eval_ln(const float inputs[5], float outputs[2]) {
  const float nx = inputs[0], ny = inputs[1], dd = inputs[2];
  const float px = inputs[3], py = inputs[4];
  const float t = ny * px - nx * py;
  outputs[0] = nx * dd + ny * t;
  outputs[1] = ny * dd - nx * t;
  return 1;
}

static int pt_eval_cr(const float inputs[5], float outputs[2]) {
  const float cx = inputs[0], cy = inputs[1], r = inputs[2];
  const float px = inputs[3], py = inputs[4];
  const float vx = px - cx, vy = py - cy;
  const float d = sqrtf(vx * vx + vy * vy);
  if (d < EPS) return 0;

  outputs[0] = cx + vx * r / d;
  outputs[1] = cy + vy * r / d;
  return 1;
}

GeomId geom_new_point(const float x, const float y, const GeomId on) {
  if (on == -1) {
    GeomId args[4];
    args[0] = graph_add_value(x);
    args[1] = graph_add_value(y);
    copy_args(args + 2, args, 2);
    return geom_new_object(POINT, args, -1, 0);
  }

  const CGeometry *obj = geom_get_object(on);

  GeomId inputs[5];
  copy_args(inputs, obj->args, 3);
  inputs[3] = graph_add_value(x);
  inputs[4] = graph_add_value(y);

  GeomId args[4];
  args[0] = graph_add_value(0);
  args[1] = graph_add_value(0);
  copy_args(args + 2, inputs + 3, 2);

  const ValueEval eval = obj->type == LINE ? pt_eval_ln : pt_eval_cr;
  const GeomId define = graph_add_constraint(5, inputs, 2, args, eval);
  return geom_new_object(POINT, args, define, 0);
}

// --- line ---
static int ln_eval(const float xyxy[4], float line[3]) {
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

GeomId geom_new_line(const GeomId pt1, const GeomId pt2) {
  const CGeometry *p = geom_get_object(pt1);
  const CGeometry *q = geom_get_object(pt2);

  GeomId inputs[4];
  copy_args(inputs, p->args, 2);
  copy_args(inputs + 2, q->args, 2);

  GeomId args[5];
  init_line(args);

  const GeomId define = graph_add_constraint(4, inputs, 3, args, ln_eval);
  return geom_new_object(LINE, args, define, 0);
}

// --- circle ---
static int cr_eval(const float xyxy[4], float radius[1]) {
  const float dx = xyxy[2] - xyxy[0];
  const float dy = xyxy[3] - xyxy[1];
  radius[0] = sqrtf(dx * dx + dy * dy);
  return 1;
}

GeomId geom_new_circle(const GeomId center, const GeomId pt) {
  const CGeometry *c = geom_get_object(center);
  const CGeometry *p = geom_get_object(pt);

  GeomId inputs[4];
  copy_args(inputs, c->args, 2);
  copy_args(inputs + 2, p->args, 2);

  GeomId args[3];
  copy_args(args, c->args, 2);
  args[2] = graph_add_value(0);

  const GeomId define = graph_add_constraint(4, inputs, 1, args + 2, cr_eval);
  return geom_new_object(CIRCLE, args, define, 0);
}

// --- midpoint ---
static int mp_eval(const float inputs[4], float outputs[2]) {
  const float x1 = inputs[0], y1 = inputs[1];
  const float x2 = inputs[2], y2 = inputs[3];
  outputs[0] = (x1 + x2) / 2.f;
  outputs[1] = (y1 + y2) / 2.f;
  return 1;
}

GeomId geom_midpoint(const GeomId pt1, const GeomId pt2) {
  const CGeometry *p = geom_get_object(pt1);
  const CGeometry *q = geom_get_object(pt2);

  GeomId inputs[4];
  copy_args(inputs, p->args, 2);
  copy_args(inputs + 2, q->args, 2);

  GeomId args[2];
  args[0] = graph_add_value(0);
  args[1] = graph_add_value(0);

  const GeomId define = graph_add_constraint(4, inputs, 2, args, mp_eval);
  return create_point(args, define, 0);
}

// --- parallel ---
static int pl_eval(const float inputs[4], float outputs[1]) {
  const float nx = inputs[0], ny = inputs[1];
  const float px = inputs[2], py = inputs[3];
  outputs[0] = nx * px + ny * py; // parallel line dd
  return 1;
}

GeomId geom_parallel(const GeomId ln, const GeomId pt) {
  const CGeometry *l = geom_get_object(ln);
  const CGeometry *p = geom_get_object(pt);

  GeomId inputs[4];
  copy_args(inputs, l->args, 2);
  copy_args(inputs + 2, p->args, 2);

  GeomId args[5];
  copy_args(args, l->args, 2);
  args[2] = graph_add_value(0);
  args[3] = graph_add_value(-HUGE_VALUE);
  args[4] = graph_add_value(HUGE_VALUE);

  const GeomId define = graph_add_constraint(4, inputs, 1, args + 2, pl_eval);
  return geom_new_object(LINE, args, define, 0);
}

// --- perp ---
static int pp_eval(const float inputs[4], float output[3]) {
  const float nx = inputs[0], ny = inputs[1];
  const float px = inputs[2], py = inputs[3];
  output[0] = -ny;
  output[1] = nx;
  output[2] = -ny * px + nx * py; // np · (px, py)
  return 1;
}

GeomId geom_perp(const GeomId ln, const GeomId pt) {
  const CGeometry *l = geom_get_object(ln);
  const CGeometry *p = geom_get_object(pt);

  GeomId inputs[4];
  copy_args(inputs, l->args, 2);
  copy_args(inputs + 2, p->args, 2);

  GeomId args[5];
  init_line(args);

  const GeomId define = graph_add_constraint(4, inputs, 3, args, pp_eval);
  return geom_new_object(LINE, args, define, 0);
}

// --- bisector ---
static int bs_eval(const float inputs[6], float outputs[6]) {
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

void geom_bisector(const GeomId ln1, const GeomId ln2, GeomId out[2]) {
  const CGeometry *l = geom_get_object(ln1);
  const CGeometry *m = geom_get_object(ln2);

  GeomId inputs[6];
  copy_args(inputs, l->args, 3);
  copy_args(inputs + 3, m->args, 3);

  GeomId args[10];
  init_line(args);
  init_line(args + 5);

  GeomId outputs[6];
  copy_args(outputs, args, 3);
  copy_args(outputs + 3, args + 5, 3);

  const GeomId define = graph_add_constraint(6, inputs, 6, outputs, bs_eval);
  out[0] = geom_new_object(LINE, args, define, 0);
  out[1] = geom_new_object(LINE, args + 5, define, 1);
}

// --- tangent ---
static int tg_eval(const float *inputs, const float r1, const float r2,
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

static int tg_eval_pt(const float inputs[5], float outputs[6]) {
  const float r = inputs[2];
  return tg_eval(inputs, r, 0, outputs);
}

static int tg_eval_in(const float inputs[6], float outputs[6]) {
  const float r1 = inputs[2];
  const float r2 = inputs[5];
  return tg_eval(inputs, r1, -r2, outputs);
}

static int tg_eval_out(const float inputs[6], float outputs[6]) {
  const float r1 = inputs[2];
  const float r2 = inputs[5];
  return tg_eval(inputs, r1, r2, outputs);
}

void geom_tangent(const GeomId cr, const GeomId cr_or_pt, GeomId out[4]) {
  const CGeometry *c = geom_get_object(cr);
  const CGeometry *cp = geom_get_object(cr_or_pt);

  if (cp->type == POINT) {
    GeomId inputs[5];
    copy_args(inputs, c->args, 3);
    copy_args(inputs + 3, cp->args, 2);

    GeomId args[10];
    init_line(args);
    init_line(args + 5);

    GeomId outputs[6];
    copy_args(outputs, args, 3);
    copy_args(outputs + 3, args + 5, 3);

    const GeomId define = graph_add_constraint(5, inputs, 6, outputs, tg_eval_pt);
    out[0] = geom_new_object(LINE, args, define, 0);
    out[1] = geom_new_object(LINE, args + 5, define, 1);
    out[2] = out[3] = -1;
    return;
  }

  GeomId inputs[6];
  copy_args(inputs, c->args, 3);
  copy_args(inputs + 3, cp->args, 3);

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

  const GeomId def_in = graph_add_constraint(6, inputs, 6, outputs, tg_eval_in);
  const GeomId def_out = graph_add_constraint(6, inputs, 6, outputs + 6, tg_eval_out);

  out[0] = geom_new_object(LINE, args, def_in, 0);
  out[1] = geom_new_object(LINE, args + 5, def_in, 1);
  out[2] = geom_new_object(LINE, args + 10, def_out, 0);
  out[3] = geom_new_object(LINE, args + 15, def_out, 1);
}

// --- circum ---
static int cc_eval(const float inputs[6], float outputs[3]) {
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

GeomId geom_circum(const GeomId pt1, const GeomId pt2, const GeomId pt3) {
  const CGeometry *p = geom_get_object(pt1);
  const CGeometry *q = geom_get_object(pt2);
  const CGeometry *r = geom_get_object(pt3);

  GeomId inputs[6];
  copy_args(inputs, p->args, 2);
  copy_args(inputs + 2, q->args, 2);
  copy_args(inputs + 4, r->args, 2);

  GeomId args[3];
  args[0] = graph_add_value(0);
  args[1] = graph_add_value(0);
  args[2] = graph_add_value(0);

  const GeomId define = graph_add_constraint(6, inputs, 3, args, cc_eval);
  return geom_new_object(CIRCLE, args, define, 0);
}

// --- isect ---
static int is_eval_ll(const float inputs[6], float outputs[2]) {
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

static int is_eval_lc(const float inputs[6], float outputs[4]) {
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

static int is_eval_cc(const float inputs[6], float outputs[4]) {
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

void geom_isect(const GeomId g1, const GeomId g2, GeomId out[2]) {
  const CGeometry *g = geom_get_object(g1);
  const CGeometry *h = geom_get_object(g2);

  if (g->type == LINE && h->type == LINE) {
    GeomId inputs[6];
    copy_args(inputs, g->args, 3);
    copy_args(inputs + 3, h->args, 3);

    GeomId args[2];
    args[0] = graph_add_value(0);
    args[1] = graph_add_value(0);

    const GeomId define = graph_add_constraint(6, inputs, 2, args, is_eval_ll);
    out[0] = create_point(args, define, 0);
    out[1] = -1;
    return;
  }

  GeomId args[4];
  args[0] = graph_add_value(0);
  args[1] = graph_add_value(0);
  args[2] = graph_add_value(0);
  args[3] = graph_add_value(0);

  if (g->type == CIRCLE && h->type == CIRCLE) {
    GeomId inputs[6];
    copy_args(inputs, g->args, 3);
    copy_args(inputs + 3, h->args, 3);

    const GeomId define = graph_add_constraint(6, inputs, 4, args, is_eval_cc);
    out[0] = create_point(args, define, 0);
    out[1] = create_point(args + 2, define, 1);
    return;
  }

  GeomId inputs[6];
  if (g->type == LINE) {
    copy_args(inputs, g->args, 3);
    copy_args(inputs + 3, h->args, 3);
  } else {
    copy_args(inputs, h->args, 3);
    copy_args(inputs + 3, g->args, 3);
  }

  const GeomId define = graph_add_constraint(6, inputs, 4, args, is_eval_lc);
  out[0] = create_point(args, define, 0);
  out[1] = create_point(args + 2, define, 1);
}

// --- move ---
void geom_move(const GeomId pt, const float to[2]) {
  const CGeometry *p = geom_get_object(pt);
  graph_change_value(2, p->args + 2, (float *)&to);
}