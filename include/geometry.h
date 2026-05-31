#ifndef GGB_OBJECT_H
#define GGB_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "graph.h"

typedef struct {
  ObjectType type;
  GeomId define;
  GeomId soln_id;
  GeomId args[5];
} CGeometry;

void geom_core_init();
void geom_core_cleanup();

GeomId object_find(const char *name);
CGeometry *geom_get_object(GeomId id);
unsigned object_get_version(const CGeometry *obj);
bool object_get_values(const CGeometry *obj, float values[]);
GeomId geom_new_object(ObjectType type, const GeomId *args, GeomId define, GeomId soln_id);
void geom_delete_object(GeomId id);
void geom_delete_all_object();
void geom_traverse_objects(void (*callback)(GeomId id, const CGeometry *));

#ifdef __cplusplus
}
#endif

#endif // GGB_OBJECT_H