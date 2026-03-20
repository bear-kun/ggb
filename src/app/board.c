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
  bool selected;
  unsigned version;
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
  GeomId hovered_object;

  float xform_scale;
  Vec2 xform_translate;
  float xform_rotate[2][2];

  GeomSize objects_size;
  BoardGeomObject *objects;
  BoardGeomVector points, lines, circles;
} board;

static const Color selected_color = {230, 41, 55, 255};

static void board_vector_init(BoardGeomVector *v, GeomSize init_size);
static BoardGeomObject *board_vector_alloc(BoardGeomVector *v, GeomId id);
static void board_vector_remove(BoardGeomVector *v, GeomId id);

static void board_update_object(BoardGeomObject *b, const GeomObject *g);
static bool board_is_visible(const BoardGeomObject *obj);

static GeomId board_find_object(Vec2 pos);

void board_init(const int x, const int y, const int w, const int h) {
  board.window = (Rectangle){(float)x, (float)y, (float)w, (float)h};
  board.font = rl_get_font_default();
  board.control = (BoardControl){0};

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
  static Vec2 down_pos = {0, 0};

  const Vec2 pos = rl_get_mouse_position();
  if (!rl_check_collision_point_rec(pos, board.window)) return;

  board.hovered_object = board_find_object(pos);
  if (board.hovered_object != -1) {
    rl_set_mouse_cursor(MOUSE_CURSOR_POINTING_HAND);
  } else {
    rl_set_mouse_cursor(MOUSE_CURSOR_DEFAULT);
  }

  if (rl_is_mouse_button_pressed(MOUSE_BUTTON_LEFT)) {
    down_pos = pos;
    if (board.control.mouse_down) board.control.mouse_down(pos);
    return;
  }
  if (rl_is_mouse_button_released(MOUSE_BUTTON_LEFT)) {
    if (rl_check_collision_point_circle(pos, down_pos, 4)) {
      if (board.control.mouse_click) board.control.mouse_click(down_pos);
    }
    if (board.control.mouse_up) board.control.mouse_up(pos);
    return;
  }

  if (rl_check_collision_point_circle(pos, down_pos, 4)) return;

  if (rl_is_mouse_button_down(MOUSE_BUTTON_LEFT)) {
    if (board.control.mouse_drag) board.control.mouse_drag(pos);
    return;
  }
  if (board.control.mouse_move) board.control.mouse_move(pos);
}

void board_set_control(const BoardControl ctrl) { board.control = ctrl; }

void board_add_object(const GeomId id) {
  const GeomObject *g_obj = object_get(id);
  BoardGeomObject *b_obj;
  switch (g_obj->type) {
  case POINT:
    b_obj = board_vector_alloc(&board.points, id);
    break;
  case LINE:
    b_obj = board_vector_alloc(&board.lines, id);
    break;
  default:
    b_obj = board_vector_alloc(&board.circles, id);
  }
  board_update_object(b_obj, g_obj);
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

static void board_update_objects_h(const GeomId id, const GeomObject *obj) {
  board_update_object(board.objects + id, obj);
}

void board_update_objects() {
  object_traverse(board_update_objects_h);
}

GeomId board_hovered_object() { return board.hovered_object; }

void board_select_object(const GeomId id) { board.objects[id].selected = true; }

void board_deselect_object(const GeomId id) {
  board.objects[id].selected = false;
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

static bool board_is_visible(const BoardGeomObject *obj) {
  return obj->show && obj->valid;
}

static void board_vector_init(BoardGeomVector *v, const GeomSize init_size) {
  v->cap = init_size;
  v->size = 0;
  v->elems = malloc(init_size * sizeof(BoardGeomObject));
}

static BoardGeomObject *board_vector_alloc(BoardGeomVector *v,
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
  obj->selected = false;
  obj->version = -1;
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

static void board_draw_name(const GeomId id) {
  const BoardGeomObject *obj = board.objects + id;
  if (!board_is_visible(obj)) return;
  const Color color = obj->selected ? selected_color : obj->color;
  rl_draw_text_ex(board.font, obj->name, obj->name_pos, 20, 1, color);
}

void board_draw() {
  const BoardGeomVector *circles = &board.circles;
  for (GeomSize i = 0; i < circles->size; i++) {
    const BoardGeomObject *obj = board.objects + circles->elems[i];
    if (!board_is_visible(obj)) continue;
    const BoardCircle cr = obj->geom.cr;
    rl_draw_ring(cr.center, cr.radius - 2, cr.radius + 2, 0, 360, 36,
                 obj->color);
    if (obj->selected) {
      rl_draw_circle_lines_v(cr.center, cr.radius - 2.4f, selected_color);
      rl_draw_circle_lines_v(cr.center, cr.radius + 2.4f, selected_color);
    }
  }

  const BoardGeomVector *lines = &board.lines;
  for (GeomSize i = 0; i < lines->size; i++) {
    const BoardGeomObject *obj = board.objects + lines->elems[i];
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

  const BoardGeomVector *points = &board.points;
  for (GeomSize i = 0; i < points->size; i++) {
    const BoardGeomObject *obj = board.objects + points->elems[i];
    if (!board_is_visible(obj)) continue;
    rl_draw_circle_v(obj->geom.pt, 5, obj->color);
    if (obj->selected) {
      rl_draw_circle_lines_v(obj->geom.pt, 5.4f, selected_color);
    }
  }

  for (GeomSize i = 0; i < circles->size; i++) {
    board_draw_name(circles->elems[i]);
  }

  for (GeomSize i = 0; i < lines->size; i++) {
    board_draw_name(lines->elems[i]);
  }

  for (GeomSize i = 0; i < points->size; i++) {
    board_draw_name(points->elems[i]);
  }
}

static GeomId board_find_object(const Vec2 pos) {
  const BoardGeomVector *points = &board.points;
  for (GeomSize j = 0; j < points->size; j++) {
    const GeomId id = points->elems[j];
    const BoardGeomObject *obj = board.objects + id;
    if (!board_is_visible(obj)) continue;
    if (rl_check_collision_point_circle(pos, obj->geom.pt, 6)) {
      return id;
    }
  }

  const BoardGeomVector *lines = &board.lines;
  for (GeomSize j = 0; j < lines->size; j++) {
    const GeomId id = lines->elems[j];
    const BoardGeomObject *obj = board.objects + id;
    if (!board_is_visible(obj)) continue;
    if (rl_check_collision_point_line(pos, obj->geom.ln.pt1, obj->geom.ln.pt2,
                                      3)) {
      return id;
    }
  }

  const BoardGeomVector *circles = &board.circles;
  for (GeomSize j = 0; j < circles->size; j++) {
    const GeomId id = circles->elems[j];
    const BoardGeomObject *obj = board.objects + id;
    if (!board_is_visible(obj)) continue;
    const float dist = vec2_distance(pos, obj->geom.cr.center);
    if (fabsf(dist - obj->geom.cr.radius) <= 3) {
      return id;
    }
  }
  return -1;
}

static void board_update_object(BoardGeomObject *b, const GeomObject *g) {
  const unsigned version = object_get_version(g);
  if (b->version == version) return;
  b->version = version;

  float args[5];
  b->valid = object_get_values(g, args);
  if (!b->valid) return;

  switch (g->type) {
  case POINT: {
    const float x = args[0], y = args[1];
    const Vec2 pt = xform_to_board(x, y);
    const Vec2 name_pos = {pt.x + 4, pt.y + 4};

    b->geom.pt = pt;
    b->name_pos = name_pos;
    break;
  }
  case LINE: {
    const float nx = args[0], ny = args[1], dd = args[2];
    const float t1 = args[3], t2 = args[4];
    const Vec2 pt1 = xform_to_board(nx * dd + ny * t1, ny * dd - nx * t1);
    const Vec2 pt2 = xform_to_board(nx * dd + ny * t2, ny * dd - nx * t2);
    const Vec2 name_pos = {(pt1.x + pt2.x) / 2.f + 2,
                           (pt1.y + pt2.y) / 2.f + 2};

    b->geom.ln.pt1 = pt1;
    b->geom.ln.pt2 = pt2;
    b->name_pos = name_pos;
    break;
  }
  default: {
    const float cx = args[0], cy = args[1], r = args[2];
    const Vec2 center = xform_to_board(cx, cy);
    const float radius = r * board.xform_scale;
    const Vec2 name_pos = {center.x + radius / 1.414f + 2,
                           center.y + radius / 1.414f + 2};

    b->geom.cr.center = center;
    b->geom.cr.radius = radius;
    b->name_pos = name_pos;
  }
  }
  b->color = g->color;
  memcpy(b->name, g->name, sizeof(b->name));
}