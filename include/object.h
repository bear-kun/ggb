#ifndef GGB_OBJECT_H
#define GGB_OBJECT_H

#include "graph.h"

typedef struct {
  char name[8];
  ObjectType type;
  Color color;
  GeomId define;
  GeomId soln_id;
  GeomId args[5];
} GeomObject;

void object_module_init();
void object_module_cleanup();

GeomObject *object_get(GeomId id);
unsigned object_get_version(const GeomObject *obj);
bool object_get_values(const GeomObject *obj, float values[]);
GeomId object_create(ObjectType type, const GeomId *args, GeomId define, GeomId soln_id);
void object_delete(GeomId id);
void object_delete_all();
void object_traverse(void (*callback)(GeomId id, const GeomObject *));

#endif // GGB_OBJECT_H