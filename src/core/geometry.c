#include "geometry.h"
#include "graph.h"
#include <stdlib.h>

static const int type_argc_in[] = {0, 4, 5, 0, 3};
static const int type_argc_out[] = {0, 2, 5, 0, 3};

static struct {
  GeomSize capacity, size, range;
  GeomId *indices;
  CGeometry *array;
} intl;

static GeomId alloc_object();
static void remove_object(GeomId id);

void geometry_core_init() {
  const GeomSize init_size = 128;

  intl.capacity = init_size;
  intl.size = intl.range = 0;
  intl.indices = malloc(init_size * sizeof(GeomId));
  intl.array = malloc(init_size * sizeof(CGeometry));

  computation_graph_init(init_size * 4);
}

void geometry_core_cleanup() {
  free(intl.indices);
  free(intl.array);
  computation_graph_cleanup();
}

CGeometry *geom_get_object(const GeomId id) { return intl.array + id; }

GeomType geom_get_type(const GeomId id) { return intl.array[id].type; }

unsigned geom_get_version(const GeomId id) {
  const CGeometry *obj = geom_get_object(id);
  return graph_get_version(type_argc_out[obj->type], obj->args);
}

GeomId geom_new_object(const GeomType type, const GeomId *args,
                       const GeomId define, const GeomId soln_id) {
  const GeomId id = alloc_object();
  CGeometry *obj = intl.array + id;
  obj->type = type;
  obj->define = define;
  obj->soln_id = soln_id;

  if (define != -1) graph_ref(define);
  for (int i = 0; i < type_argc_in[type]; i++) {
    obj->args[i] = args[i];
    graph_ref(args[i]);
  }

  return id;
}

void geom_delete_object(const GeomId id) {
  const CGeometry *obj = intl.array + id;
  if (obj->define != -1) graph_unref(obj->define);
  for (int i = 0; i < type_argc_in[obj->type]; i++) graph_unref(obj->args[i]);
  remove_object(id);
}

void geom_delete_all_object() {
  intl.size = intl.range = 0;
  computation_graph_clear();
}

static void resize_objects() {
  intl.capacity *= 2;
  void *mem = realloc(intl.array, intl.capacity * sizeof(CGeometry));
  if (!mem) abort();
  intl.array = mem;
}

static GeomId alloc_object() {
  if (intl.size == intl.capacity) {
    resize_objects();
  }
  if (intl.size == intl.range) {
    intl.indices[intl.range] = (GeomId)intl.range++;
  }

  return intl.indices[intl.size++];
}

static void remove_object(const GeomId id) {
  for (GeomSize i = 0; i < intl.size; i++) {
    if (intl.indices[i] == id) {
      intl.indices[i] = intl.indices[intl.size - 1];
      intl.indices[intl.size - 1] = id;
      break;
    }
  }
  intl.size--;
}