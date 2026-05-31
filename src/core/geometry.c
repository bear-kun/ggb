#include "geometry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const GeomSize type_argc_in[] = {0, 4, 5, 0, 3};
static const GeomSize type_argc_out[] = {0, 2, 5, 0, 3};

static struct {
  GeomSize capacity, size, range;
  GeomId *indices;
  uint64_t *bitmap;
  CGeometry *array;
} intl;

static uint64_t ctz(uint64_t value);
static GeomId alloc_object();
static void remove_object(GeomId id);

void geom_core_init() {
  const GeomSize init_size = 128;

  intl.capacity = init_size;
  intl.size = intl.range = 0;
  intl.indices = malloc(init_size * sizeof(GeomId));
  intl.bitmap = calloc(init_size / 8, 1);
  intl.array = malloc(init_size * sizeof(CGeometry));

  computation_graph_init(init_size * 4);
}

void geom_core_cleanup() {
  free(intl.indices);
  free(intl.bitmap);
  free(intl.array);
  computation_graph_cleanup();
}

CGeometry *geom_get_object(const GeomId id) { return intl.array + id; }

unsigned object_get_version(const CGeometry *obj) {
  return graph_get_version(type_argc_out[obj->type], obj->args);
}

bool object_get_values(const CGeometry *obj, float values[]) {
  if (obj->define != -1 && graph_is_degenerate(obj->define, obj->soln_id)) return false;
  return graph_get_values(type_argc_out[obj->type], obj->args, values);
}

GeomId geom_new_object(const ObjectType type, const GeomId *args,
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
  memset(intl.bitmap, 0, intl.capacity / 8);
  computation_graph_clear();
}

void geom_traverse_objects(void (*callback)(GeomId id, const CGeometry *)) {
  for (GeomSize i = 0; i < intl.capacity; i += 64) {
    uint64_t bitmap = intl.bitmap[i >> 6];
    while (bitmap) {
      const uint64_t j = ctz(bitmap);
      const GeomId id = (GeomId)(i | j);
      callback(id, intl.array + id);
      bitmap &= bitmap - 1;
    }
  }
}

#if defined(__GNUC__) || defined(__clang__)
static uint64_t ctz(const uint64_t value) {
  return __builtin_ctzll(value);
}
#elif defined(_MSC_VER)
#include <intrin.h>

static uint64_t ctz(const uint64_t value) {
  unsigned long res;
  _BitScanForward64(&res, value);
  return res;
}
#endif

/*
static void u2str(char *str, unsigned x) {
  str += x / 10 ;
  while (x) {
    *str++ = (char)('0' + x % 10);
    x /= 10;
  }
}

static void get_default_name(char *name, const ObjectType type) {
  static unsigned point = 0, line = 0, circle = 0;
  memset(name, 0, sizeof(((GeomObject *)0)->name));
  switch (type) {
  case POINT:
    name[0] = (char)('A' + point % 26);
    u2str(name + 1, point / 26);
    point++;
    return;
  case LINE:
    name[0] = (char)('a' + line % 26);
    u2str(name + 1, line / 26);
    if (line++ % 26 == 'c' - 'a' - 1) line++;
    return;
  default:
    name[0] = 'c';
    u2str(name + 1, circle++);
  }
}
*/

static void resize_objects() {
  const GeomSize half_cap = intl.capacity;
  intl.capacity *= 2;

  void *mem = realloc(intl.bitmap, intl.capacity / 8);
  if (!mem) abort();
  intl.bitmap = mem;
  memset((char *)intl.bitmap + half_cap / 8, 0, half_cap / 8);

  mem = realloc(intl.array, intl.capacity * sizeof(CGeometry));
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

  const GeomId id = intl.indices[intl.size];
  intl.bitmap[id >> 6] |= 1llu << (id & 63);
  intl.size++;
  return id;
}

static void remove_object(const GeomId id) {
  for (GeomId i = 0; i < intl.size; i++) {
    if (intl.indices[i] == id) {
      intl.indices[i] = intl.indices[intl.size - 1];
      intl.indices[intl.size - 1] = id;
      break;
    }
  }
  intl.bitmap[id >> 6] ^= 1llu << (id & 63);
  intl.size--;
}