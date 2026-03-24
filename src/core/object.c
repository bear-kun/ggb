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

static const GeomSize type_argc_in[] = {0, 4, 5, 0, 3};
static const GeomSize type_argc_out[] = {0, 2, 5, 0, 3};
static const Color type_color[] = {
    {0}, {0, 82, 172, 255}, {130, 130, 130, 255}, {0}, {130, 130, 130, 255}};

static struct {
  StringHashTable hash;
  GeomSparseArray objects;
} intl;

static uint64_t ctz(uint64_t value);
static GeomId object_alloc(ObjectType type);
static void object_remove(GeomId id);
static void object_module_resize();

void object_module_init() {
  const GeomSize init_size = 128;

  intl.objects.cap = init_size;
  intl.objects.size = 0;
  intl.objects.bitmap = calloc(init_size / 8, 1);
  intl.objects.data = malloc(init_size * sizeof(GeomObject));

  string_hash_init(&intl.hash, init_size);
  computation_graph_init(init_size * 4);
}

void object_module_cleanup() {
  free(intl.objects.bitmap);
  free(intl.objects.data);
  string_hash_release(&intl.hash);
  computation_graph_cleanup();
}

GeomId object_find(const char *name) {
  return string_hash_find(&intl.hash, name);
}

GeomObject *object_get(const GeomId id) { return intl.objects.data + id; }

unsigned object_get_version(const GeomObject *obj) {
  return graph_get_version(type_argc_out[obj->type], obj->args);
}

bool object_get_values(const GeomObject *obj, float values[]) {
  return graph_get_values(type_argc_out[obj->type], obj->args, values);
}

GeomId object_create(const ObjectType type, const GeomId *args,
                     const GeomId define, const GeomId soln_id) {
  if (intl.objects.size == intl.objects.cap) object_module_resize();

  const GeomId id = object_alloc(type);
  GeomObject *obj = intl.objects.data + id;
  obj->type = type;
  obj->define = define;
  obj->soln_id = soln_id;
  obj->color = type_color[type];

  if (define != -1) graph_ref(define);
  for (int i = 0; i < type_argc_in[type]; i++) {
    obj->args[i] = args[i];
    graph_ref(args[i]);
  }

  return id;
}

void object_delete(const GeomId id) {
  const GeomObject *obj = intl.objects.data + id;
  if (obj->define != -1) graph_unref(obj->define);
  for (int i = 0; i < type_argc_in[obj->type]; i++) graph_unref(obj->args[i]);
  object_remove(id);
}

void object_delete_all() {
  intl.objects.size = 0;
  memset(intl.objects.bitmap, 0, intl.objects.cap / 8);
  string_hash_clear(&intl.hash);
  computation_graph_clear();
}

void object_traverse(void (*callback)(GeomId id, const GeomObject *)) {
  const GeomSparseArray *array = &intl.objects;
  for (GeomSize i = 0; i < array->cap; i += 64) {
    uint64_t bitmap = array->bitmap[i >> 6];
    while (bitmap) {
      const uint64_t j = ctz(bitmap);
      const GeomId id = (GeomId)(i | j);
      callback(id, array->data + id);
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

static GeomId object_alloc(const ObjectType type) {
  const GeomId id = string_hash_alloc_id(&intl.hash);
  GeomObject *obj = intl.objects.data + id;
  get_default_name(obj->name, type);
  string_hash_insert(&intl.hash, obj->name, id);
  intl.objects.bitmap[id >> 6] |= 1llu << (id & 63);
  intl.objects.size++;
  return id;
}

static void object_remove(const GeomId id) {
  const GeomObject *obj = intl.objects.data + id;
  string_hash_remove(&intl.hash, obj->name);
  intl.objects.bitmap[id >> 6] ^= 1llu << (id & 63);
  intl.objects.size--;
}

static void object_module_resize() {
  GeomSparseArray *objects = &intl.objects;
  const GeomSize half_cap = objects->cap;

  objects->cap *= 2;
  string_hash_resize(&intl.hash, objects->cap);

  void *mem = realloc(objects->bitmap, objects->cap / 8);
  if (!mem) abort();
  objects->bitmap = mem;
  memset((char *)objects->bitmap + half_cap / 8, 0, half_cap / 8);

  mem = realloc(objects->data, objects->cap * sizeof(GeomObject));
  if (!mem) abort();
  objects->data = mem;
}