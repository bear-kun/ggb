#include "object.h"
#include "str_hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  GeomSize cap, size;
  uint64_t *bitmap;
  GeomObject *data;
} GeomSparseArray;

static const GeomSize type_argc[] = {0, 2, 5, 0, 3};
static const Color type_color[] = {
    {0}, {0, 82, 172, 255}, {130, 130, 130, 255}, {0}, {130, 130, 130, 255}};

static struct {
  StringHashTable hash;
  GeomSparseArray objects;
} internal;

static uint64_t ctz(uint64_t value);
static GeomId object_alloc(ObjectType type);
static void object_remove(GeomId id);
static void object_module_resize();

void object_module_init() {
  const GeomSize init_size = 128;

  internal.objects.cap = init_size;
  internal.objects.size = 0;
  internal.objects.bitmap = calloc(init_size / 8, 1);
  internal.objects.data = malloc(init_size * sizeof(GeomObject));

  string_hash_init(&internal.hash, init_size);
  computation_graph_init(init_size * 4);
}

void object_module_cleanup() {
  free(internal.objects.bitmap);
  free(internal.objects.data);
  string_hash_release(&internal.hash);
  computation_graph_cleanup();
}

GeomObject *object_get(const GeomId id) { return internal.objects.data + id; }

bool object_is_valid(const GeomId id) {
  const GeomObject *obj = internal.objects.data + id;
  return graph_is_valid(type_argc[obj->type], obj->args);
}

bool object_check_coincident(const GeomId id) {
  const GeomObject *obj = internal.objects.data + id;
  return obj->define != -1 && graph_is_degenerate(obj->define);
}

GeomId object_create(const ObjectType type, const GeomId *args) {
  if (internal.objects.size == internal.objects.cap) object_module_resize();

  const GeomId id = object_alloc(type);
  GeomObject *obj = internal.objects.data + id;
  obj->type = type;
  obj->define = -1;

  obj->color = type_color[type];
  for (int i = 0; i < type_argc[type]; i++) {
    obj->args[i] = args[i];
    graph_ref_value(args[i]);
  }
  return id;
}

void object_set_coincident(const GeomId obj_id, const GeomId define) {
  internal.objects.data[obj_id].define = define;
}

void object_delete(const GeomId id) {
  const GeomObject *obj = internal.objects.data + id;
  graph_unref_value(type_argc[obj->type], obj->args);
  object_remove(id);
}

void object_delete_all() {
  internal.objects.size = 0;
  memset(internal.objects.bitmap, 0, internal.objects.cap / 8);
  string_hash_clear(&internal.hash);
  computation_graph_clear();
}

void object_traverse(void (*callback)(GeomId id, const GeomObject *)) {
  const GeomSparseArray *array = &internal.objects;
  for (GeomSize i = 0; i < array->cap; i += 64) {
    uint64_t bitmap = array->bitmap[i >> 6];
    while (bitmap) {
      const uint64_t j = ctz(bitmap);
      const GeomId id = (GeomId)(i | j);
      callback(id, array->data + (i | j));
      bitmap &= bitmap - 1;
    }
  }
}

#if defined(__GNUC__) || defined(__clang__)
static uint64_t ctz(const uint64_t value) {
  return __builtin_ctzll(value);
}
#elif defined(_MSC_VER)
#include <intrin0.h>

static uint64_t ctz(const uint64_t value) {
  unsigned long res;
  _BitScanForward64(&res, value);
  return res;
}
#endif

static void get_default_name(char *name, const ObjectType type) {
  static unsigned point = 0, line = 0, circle = 0;
  switch (type) {
  case POINT:
    if (point < 26) {
      sprintf(name, "%c", 'A' + point);
    } else {
      sprintf(name, "%c%u", 'A' + point % 26, point / 26 + 1);
    }
    point++;
    return;
  case LINE:
    if (line < 26) {
      sprintf(name, "%c", 'a' + line);
    } else {
      sprintf(name, "%c%u", 'a' + line % 26, line / 26 + 1);
    }
    line++;
    return;
  case CIRCLE:
    sprintf(name, "c%u", ++circle);
  default:
    break;
  }
}

static GeomId object_alloc(const ObjectType type) {
  const GeomId id = string_hash_alloc_id(&internal.hash);
  GeomObject *obj = internal.objects.data + id;
  get_default_name(obj->name, type);
  string_hash_insert(&internal.hash, obj->name, id);
  internal.objects.bitmap[id >> 6] |= 1llu << (id & 63);
  internal.objects.size++;
  return id;
}

static void object_remove(const GeomId id) {
  const GeomObject *obj = internal.objects.data + id;
  string_hash_remove(&internal.hash, obj->name);
  internal.objects.bitmap[id >> 6] ^= 1llu << (id & 63);
  internal.objects.size--;
}

static void object_module_resize() {
  GeomSparseArray *objects = &internal.objects;
  const GeomSize half_cap = objects->cap;

  objects->cap *= 2;
  string_hash_resize(&internal.hash, objects->cap);

  void *mem = realloc(objects->bitmap, objects->cap / 8);
  if (!mem) abort();
  objects->bitmap = mem;
  memset((char *)objects->bitmap + half_cap / 8, 0, half_cap / 8);

  mem = realloc(objects->data, objects->cap * sizeof(GeomObject));
  if (!mem) abort();
  objects->data = mem;
}