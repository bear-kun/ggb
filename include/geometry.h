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

void geom_core_init();
void geom_core_cleanup();

GeomId object_find(const char *name);
CGeometry *geom_get_object(GeomId id);
unsigned geom_get_version(const CGeometry *obj);
GeomId geom_new_object(GeomType type, const GeomId *args, GeomId define, GeomId soln_id);
void geom_delete_object(GeomId id);
void geom_delete_all_object();
void geom_traverse_objects(void (*callback)(GeomId id, const CGeometry *));

bool geom_get_point(const CGeometry *pt, float xy[2]);
bool geom_get_line(const CGeometry *ln, float pt1[2], float pt2[2]);
bool geom_get_circle(const CGeometry *cr, float center[2], float *radius);

#ifdef __cplusplus
}
#endif

#endif // GGB_OBJECT_H