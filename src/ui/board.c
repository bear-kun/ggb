#include "board.h"
#include "object.h"
#include "raylib_.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  Vec2 pt1, pt2;
} BoardLine;

typedef struct {
  Vec2 center;
  float radius;
} BoardCircle;

typedef union {
  Vec2 pt;
  BoardLine ln;
  BoardCircle cr;
} BoardGeomData;

typedef struct {
  bool show;
  bool valid;
  bool coincident;
  bool selected;
  Color color;
  char name[8];
  Vec2 name_pos;
  BoardGeomData geom;
} BoardGeomObject;

typedef struct {
  GeomSize cap, size;
  GeomId *elems;
} BoardGeomVector;

static struct {
  Rectangle window;
  Font font;
  BoardControl control;

  float xform_scale;
  Vec2 xform_translate;
  float xform_rotate[2][2];

  GeomSize objects_size;
  BoardGeomObject *objects;
  BoardGeomVector points, lines, circles;
} board;

static const Color selected_color = {230, 41, 55, 255};

static void board_vector_init(BoardGeomVector *v, GeomSize init_size);
static void board_vector_clear(BoardGeomVector *v) { v->size = 0; }
static BoardGeomObject *board_vector_insert(BoardGeomVector *v, GeomId id);
static void board_vector_remove(BoardGeomVector *v, GeomId id);
static void get_board_buffer(GeomId id, const GeomObject *obj);
static float vec2_distance(Vec2 v1, Vec2 v2);
static bool board_is_visible(const BoardGeomObject *obj);

void board_init(const int x, const int y, const int w, const int h) {
  board.window = (Rectangle){(float)x, (float)y, (float)w, (float)h};
  board.font = rl_get_font_default();
  board.control = NULL;

  board.xform_scale = 1.f;
  board.xform_translate.y = 0.f;
  board.xform_rotate[0][0] = 1.f;
  board.xform_rotate[1][1] = 1.f;

  board.objects_size = 128;
  board.objects = malloc(sizeof(BoardGeomObject) * board.objects_size);
  board_vector_init(&board.points, 64);
  board_vector_init(&board.lines, 32);
  board_vector_init(&board.circles, 16);
  object_module_init();
}

void board_cleanup() {
  free(board.points.elems);
  free(board.lines.elems);
  free(board.circles.elems);
  free(board.objects);
  object_module_cleanup();
}

void board_listen() {
  const Vec2 mouse_pos = rl_get_mouse_position();
  if (!rl_check_collision_point_rec(mouse_pos, board.window)) return;

  MouseEvent event = MOUSE_NULL;
  if (rl_is_mouse_button_pressed(MOUSE_BUTTON_LEFT)) {
    event = MOUSE_PRESS;
  } else if (rl_is_mouse_button_released(MOUSE_BUTTON_LEFT)) {
    event = MOUSE_RELEASE;
  }

  if (board.control) {
    board.control(mouse_pos, event);
  }
}

void board_draw() {
  const BoardGeomVector *vector = &board.circles;
  for (GeomSize i = 0; i < vector->size; i++) {
    const BoardGeomObject *obj = board.objects + vector->elems[i];
    if (!board_is_visible(obj)) continue;
    const BoardCircle cr = obj->geom.cr;
    rl_draw_ring(cr.center, cr.radius - 2, cr.radius + 2, 0, 360, 36,
                 obj->color);
    if (obj->selected) {
      rl_draw_circle_lines_v(cr.center, cr.radius - 2.4f, selected_color);
      rl_draw_circle_lines_v(cr.center, cr.radius + 2.4f, selected_color);
    }
  }

  vector = &board.lines;
  for (GeomSize i = 0; i < vector->size; i++) {
    const BoardGeomObject *obj = board.objects + vector->elems[i];
    if (!board_is_visible(obj)) continue;
    const BoardLine ln = obj->geom.ln;
    rl_draw_line_ex(ln.pt1, ln.pt2, 4, obj->color);
    if (obj->selected) {
      const Vec2 v = {ln.pt1.x - ln.pt2.x, ln.pt1.y - ln.pt2.y};
      const float norm = sqrtf(v.x * v.x + v.y * v.y);
      const Vec2 vp = {v.y / norm * 2.3f, -v.x / norm * 2.3f};
      rl_draw_line_v((Vec2){ln.pt1.x + vp.x, ln.pt1.y + vp.y},
                     (Vec2){ln.pt2.x + vp.x, ln.pt2.y + vp.y}, selected_color);
      rl_draw_line_v((Vec2){ln.pt1.x - vp.x, ln.pt1.y - vp.y},
                     (Vec2){ln.pt2.x - vp.x, ln.pt2.y - vp.y}, selected_color);
    }
  }

  vector = &board.points;
  for (GeomSize i = 0; i < vector->size; i++) {
    const BoardGeomObject *obj = board.objects + vector->elems[i];
    if (!board_is_visible(obj)) continue;
    rl_draw_circle_v(obj->geom.pt, 5, obj->color);
    if (obj->selected) {
      rl_draw_circle_lines_v(obj->geom.pt, 5.4f, selected_color);
    }
  }

  for (GeomSize i = 0; i < vector->size; i++) {
    const BoardGeomObject *obj = board.objects + vector->elems[i];
    if (!board_is_visible(obj)) continue;
    const Color color = obj->selected ? selected_color : obj->color;
    rl_draw_text_ex(board.font, obj->name, obj->name_pos, 20, 1, color);
  }
}

GeomId board_find_object(const ObjectType types, const Vec2 pos) {
  if (types & POINT) {
    const BoardGeomVector *vector = &board.points;
    for (GeomSize j = 0; j < vector->size; j++) {
      const GeomId id = vector->elems[j];
      const BoardGeomObject *obj = board.objects + id;
      if (!board_is_visible(obj)) continue;
      if (rl_check_collision_point_circle(pos, obj->geom.pt, 6)) {
        return id;
      }
    }
  }
  if (types & LINE) {
    const BoardGeomVector *vector = &board.lines;
    for (GeomSize j = 0; j < vector->size; j++) {
      const GeomId id = vector->elems[j];
      const BoardGeomObject *obj = board.objects + id;
      if (!board_is_visible(obj)) continue;
      if (rl_check_collision_point_line(pos, obj->geom.ln.pt1, obj->geom.ln.pt2,
                                        3)) {
        return id;
      }
    }
  }
  if (types & CIRCLE) {
    const BoardGeomVector *vector = &board.circles;
    for (GeomSize j = 0; j < vector->size; j++) {
      const GeomId id = vector->elems[j];
      const BoardGeomObject *obj = board.objects + id;
      if (!board_is_visible(obj)) continue;
      const float dist = vec2_distance(pos, obj->geom.cr.center);
      if (fabsf(dist - obj->geom.cr.radius) <= 3) {
        return id;
      }
    }
  }
  return -1;
}

void board_set_control(const BoardControl ctrl) { board.control = ctrl; }

void board_add_object(const GeomId id) {
  const GeomObject *obj = object_get(id);
  get_board_buffer(id, obj);
}

void board_remove_object(const GeomId id) {
  const GeomObject *obj = object_get(id);
  switch (obj->type) {
  case POINT:
    return board_vector_remove(&board.points, id);
  case LINE:
    return board_vector_remove(&board.lines, id);
  default:
    return board_vector_remove(&board.circles, id);
  }
}

void board_select_object(const GeomId id) { board.objects[id].selected = true; }

void board_deselect_object(const GeomId id) {
  board.objects[id].selected = false;
}

void board_update_objects() {
  board_vector_clear(&board.points);
  board_vector_clear(&board.lines);
  board_vector_clear(&board.circles);
  object_traverse(get_board_buffer);
}

Vec2 xform_to_world(const Vec2 pos) {
  const Vec2 translated = {pos.x - board.xform_translate.x,
                           pos.y - board.xform_translate.y};
  const float (*rotate)[2] = board.xform_rotate;
  const float cross = rotate[0][0] * rotate[1][1] - rotate[0][1] * rotate[1][0];
  const Vec2 rotated = {
      (rotate[1][1] * translated.x - rotate[0][1] * translated.y) / cross,
      (-rotate[1][0] * translated.x + rotate[0][0] * translated.y) / cross};
  const Vec2 scaled = {rotated.x / board.xform_scale,
                       rotated.y / board.xform_scale};
  return scaled;
}

static Vec2 xform_to_board(const float x, const float y) {
  const Vec2 scaled = {x * board.xform_scale, y * board.xform_scale};
  const Vec2 rotated = {
      board.xform_rotate[0][0] * scaled.x + board.xform_rotate[0][1] * scaled.y,
      board.xform_rotate[1][0] * scaled.x + board.xform_rotate[1][1] * scaled.y,
  };
  const Vec2 translated = {rotated.x + board.xform_translate.x,
                           rotated.y + board.xform_translate.y};
  return translated;
}

static bool board_is_visible(const BoardGeomObject *obj) {
  return obj->show && obj->valid && !obj->coincident;
}

static void board_vector_init(BoardGeomVector *v, const GeomSize init_size) {
  v->cap = init_size;
  v->size = 0;
  v->elems = malloc(init_size * sizeof(BoardGeomObject));
}

static BoardGeomObject *board_vector_insert(BoardGeomVector *v,
                                            const GeomId id) {
  if (v->size == v->cap) {
    v->cap *= 2;
    void *mem = realloc(v->elems, v->cap * sizeof(GeomId));
    if (!mem) abort();
    v->elems = mem;
  }
  if (id >= board.objects_size) {
    board.objects_size *= 2;
    void *mem =
        realloc(board.objects, sizeof(BoardGeomObject) * board.objects_size);
    if (!mem) abort();
    board.objects = mem;
  }
  v->elems[v->size++] = id;

  BoardGeomObject *obj = board.objects + id;
  obj->show = true;
  obj->valid = true;
  obj->coincident = false;
  obj->selected = false;
  return obj;
}

static void board_vector_remove(BoardGeomVector *v, const GeomId id) {
  GeomSize i = 0;
  while (v->elems[i] != id) i++;

  v->size--;
  for (; i < v->size; i++) v->elems[i] = v->elems[i + 1];
}

static float vec2_distance(const Vec2 v1, const Vec2 v2) {
  const float dx = v1.x - v2.x;
  const float dy = v1.y - v2.y;
  return sqrtf(dx * dx + dy * dy);
}

static void get_board_buffer(const GeomId id, const GeomObject *obj) {
  const GeomId *args = obj->args;
  switch (obj->type) {
  case POINT: {
    BoardGeomObject *b_obj = board_vector_insert(&board.points, id);
    b_obj->valid = object_is_valid(id);
    b_obj->coincident = object_check_coincident(id);
    if (!b_obj->valid || b_obj->coincident) return;

    const float x = graph_get_value(args[0]);
    const float y = graph_get_value(args[1]);
    const Vec2 pt = xform_to_board(x, y);
    const Vec2 name_pos = {pt.x + 4, pt.y + 4};

    b_obj->geom.pt = pt;
    b_obj->name_pos = name_pos;
    b_obj->color = obj->color;
    memcpy(b_obj->name, obj->name, sizeof(b_obj->name));
    break;
  }
  case CIRCLE: {
    BoardGeomObject *b_obj = board_vector_insert(&board.circles, id);
    b_obj->valid = object_is_valid(id);
    b_obj->coincident = object_check_coincident(id);
    if (!b_obj->valid || b_obj->coincident) return;

    const float cx = graph_get_value(args[0]);
    const float cy = graph_get_value(args[1]);
    const float r = graph_get_value(args[2]);
    const Vec2 center = xform_to_board(cx, cy);
    const float radius = r * board.xform_scale;
    const Vec2 name_pos = {center.x + radius / 1.414f + 2,
                           center.y + radius / 1.414f + 2};

    b_obj->geom.cr.center = center;
    b_obj->geom.cr.radius = radius;
    b_obj->name_pos = name_pos;
    b_obj->color = obj->color;
    memcpy(b_obj->name, obj->name, sizeof(b_obj->name));
    break;
  }
  default: {
    BoardGeomObject *b_obj = board_vector_insert(&board.lines, id);
    b_obj->valid = object_is_valid(id);
    b_obj->coincident = object_check_coincident(id);
    if (!b_obj->valid || b_obj->coincident) return;

    const float nx = graph_get_value(args[0]);
    const float ny = graph_get_value(args[1]);
    const float dd = graph_get_value(args[2]);
    const float t1 = graph_get_value(args[3]);
    const float t2 = graph_get_value(args[4]);
    const Vec2 pt1 = xform_to_board(nx * dd + ny * t1, ny * dd - nx * t1);
    const Vec2 pt2 = xform_to_board(nx * dd + ny * t2, ny * dd - nx * t2);
    const Vec2 name_pos = {(pt1.x + pt2.x) / 2.f + 2,
                           (pt1.y + pt2.y) / 2.f + 2};

    b_obj->geom.ln.pt1 = pt1;
    b_obj->geom.ln.pt2 = pt2;
    b_obj->name_pos = name_pos;
    b_obj->color = obj->color;
    memcpy(b_obj->name, obj->name, sizeof(b_obj->name));
  }
  }
}