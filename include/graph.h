#ifndef GGB_GRAPH_H
#define GGB_GRAPH_H

#include "types.h"

static const float HUGE_VALUE = 4096.f;
static const float EPS = 1e-5f;

typedef int (*ValueEval)(const float *inputs, float *outputs);

void computation_graph_init(GeomSize init_size);
void computation_graph_cleanup();
void computation_graph_clear();
GeomId graph_add_value(float value);
GeomId graph_add_constraint(GeomSize input_size, const GeomId *inputs,
                            GeomSize output_size, const GeomId *outputs,
                            ValueEval eval);
float graph_get_value(GeomId id);
bool graph_is_valid(GeomSize count, const GeomId *ids);
bool graph_is_degenerate(GeomId constr, GeomId soln_id);
void graph_ref(GeomId id);
void graph_unref(GeomId id);
void graph_change_value(GeomSize count, const GeomId *ids, const float *values);

#endif // GGB_GRAPH_H
