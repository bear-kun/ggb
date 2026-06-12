#ifndef GGB_CORE_INTERNAL_H
#define GGB_CORE_INTERNAL_H

#include "geometry.hpp"

namespace geom {
using ValueEval = int (*)(const float inputs[], float outputs[]);

enum EvalType {
  EVAL_NULL,
  EVAL_POINT_ON_LINE,
  EVAL_POINT_ON_CIRCLE,
  EVAL_LINE,
  EVAL_CIRCLE,
  EVAL_MIDPOINT,
  EVAL_PARALLEL,
  EVAL_PERPENDICULAR,
  EVAL_ANGLE_BISECTOR,
  EVAL_TANGENT_POINT,
  EVAL_TANGENT_INNER,
  EVAL_TANGENT_OUTER,
  EVAL_CIRCUMCIRCLE,
  EVAL_INTERSECTION_LINE_LINE,
  EVAL_INTERSECTION_LINE_CIRCLE,
  EVAL_INTERSECTION_CIRCLE_CIRCLE,
};

extern const ValueEval eval_map[];

namespace graph {
void init(GeomSize init_size);
void cleanup();
void clear();
GeomId add_value(float value = 0);
GeomId add_constraint(GeomSize input_size, const GeomId *inputs,
                      GeomSize output_size, const GeomId *outputs, EvalType eval);

unsigned get_version(GeomSize count, const GeomId *ids);
bool get_values(GeomSize count, const GeomId *ids, float *values);
bool is_degenerate(GeomId constr, GeomId soln_id);
void ref_node(GeomId id);
void unref_node(GeomId id);
void change_value(GeomSize count, const GeomId *ids, const float *values);
}
}

#endif //GGB_CORE_INTERNAL_H