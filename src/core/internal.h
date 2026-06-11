#ifndef GGB_CORE_INTERNAL_H
#define GGB_CORE_INTERNAL_H

#include "geometry.h"

typedef int (*ValueEval)(const float *inputs, float *outputs);

typedef enum {
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
} EvalType;

extern const ValueEval eval_map[];

CGeometry *geom_get_object(GeomId id);

void computation_graph_init(GeomSize init_size);
void computation_graph_cleanup();
void computation_graph_clear();
GeomId graph_add_value(float value);
GeomId graph_add_constraint(GeomSize input_size, const GeomId *inputs,
                            GeomSize output_size, const GeomId *outputs,
                            EvalType eval);
unsigned graph_get_version(GeomSize count, const GeomId *ids);
bool graph_get_values(GeomSize count, const GeomId *ids, float *values);
bool graph_is_degenerate(GeomId constr, GeomId soln_id);
void graph_ref(GeomId id);
void graph_unref(GeomId id);
void graph_change_value(GeomSize count, const GeomId *ids, const float *values);

#endif //GGB_CORE_INTERNAL_H