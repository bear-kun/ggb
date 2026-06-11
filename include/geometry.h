#ifndef GGB_OBJECT_H
#define GGB_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

typedef struct {
  GeomType type;
  GeomId define;
  GeomId soln_id;
  GeomId args[5];
} CGeometry;

void geometry_core_init();
void geometry_core_cleanup();

GeomType geom_get_type(GeomId id);
unsigned geom_get_version(GeomId id);
GeomId geom_new_object(GeomType type, const GeomId *args, GeomId define, GeomId soln_id);
void geom_delete_object(GeomId id);
void geom_delete_all_object();

bool geom_get_point(GeomId id, float xy[2]);
bool geom_get_line(GeomId id, float pt1[2], float pt2[2]);
bool geom_get_circle(GeomId id, float center[2], float *radius);

GeomId geom_new_point(float x, float y, GeomId on);
GeomId geom_new_line(GeomId pt1, GeomId pt2);
GeomId geom_new_circle(GeomId center, GeomId pt);
GeomId geom_midpoint(GeomId pt1, GeomId pt2);
GeomId geom_parallel(GeomId ln, GeomId pt);
GeomId geom_perpendicular(GeomId ln, GeomId pt);
void geom_angle_bisector(GeomId ln1, GeomId ln2, GeomId out[2]);
void geom_tangent(GeomId cr, GeomId cr_or_pt, GeomId out[4]);
GeomId geom_circumcircle(GeomId pt1, GeomId pt2, GeomId pt3);
void geom_intersection(GeomId g1, GeomId g2, GeomId out[2]);
void geom_move(GeomId pt, const float to[2]);

#ifdef __cplusplus
}
#endif

#endif // GGB_OBJECT_H