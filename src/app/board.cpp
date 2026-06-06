#include "board.hpp"
#include "command.hpp"
#include "geometry.hpp"
#include "raylib.hpp"
#include <vector>

namespace app::board {
static struct {
  rl::Rectangle window{};
  rl::Font font{};

  Control *control = nullptr;
  GeomId hovered_object = -1;

  Transform xform;
  std::vector<Geometry> objects;
  std::vector<GeomId> points, lines, circles;
} board;

static GeomId board_hover_object(Vec2 pos);

void init(const int x, const int y, const int w, const int h) {
  board.window = {(float)x, (float)y, (float)w, (float)h};
  board.font = rl::get_font_default();

  board.objects.resize(128);
  board.points.reserve(128);
  board.lines.reserve(64);
  board.circles.reserve(32);

  geometry_core_init();
  command::init();
}

void cleanup() {
  command::cleanup();
  geometry_core_cleanup();
}

void listen() {
  static Vec2 down_pos = {0, 0};

  if (board.control == nullptr) return;

  // redo & undo
  if (rl::is_key_down(rl::KEY_LEFT_CONTROL) || rl::is_key_down(rl::KEY_RIGHT_CONTROL)) {
    if (rl::is_key_pressed(rl::KEY_Z)) {
      if (rl::is_key_down(rl::KEY_LEFT_SHIFT) || rl::is_key_down(rl::KEY_RIGHT_SHIFT)) {
        command::redo();
      } else {
        command::undo();
      }
    }
  }

  const Vec2 pos = rl::get_mouse_position();
  if (!rl::check_collision_point_rec(pos, board.window)) return;

  // set mouse cursor when mouse hovering
  board.hovered_object = board_hover_object(pos);
  if (board.hovered_object != -1) {
    rl::set_mouse_cursor(rl::MOUSE_CURSOR_POINTING_HAND);
  } else {
    rl::set_mouse_cursor(rl::MOUSE_CURSOR_DEFAULT);
  }

  // mouse event
  if (rl::is_mouse_button_pressed(rl::MOUSE_BUTTON_LEFT)) {
    down_pos = pos;
    board.control->down(pos);
    return;
  }
  if (rl::is_mouse_button_released(rl::MOUSE_BUTTON_LEFT)) {
    if (rl::check_collision_point_circle(pos, down_pos, 8)) {
      board.control->click(down_pos);
    }
    board.control->up(pos);
    return;
  }

  if (rl::check_collision_point_circle(pos, down_pos, 8)) return;

  if (rl::is_mouse_button_down(rl::MOUSE_BUTTON_LEFT)) {
    board.control->drag(pos);
    return;
  }
  board.control->move(pos);
}

void draw() {
  for (const GeomId id : board.circles) board.objects[id].draw();
  for (const GeomId id : board.lines) board.objects[id].draw();
  for (const GeomId id : board.points) board.objects[id].draw();

  for (const GeomId id : board.circles) board.objects[id].draw_name();
  for (const GeomId id : board.lines) board.objects[id].draw_name();
  for (const GeomId id : board.points) board.objects[id].draw_name();
}

void set_control(Control &ctrl) {
  board.control = &ctrl;
}

Vec2 xform_to_world(const Vec2 pos) {
  return board.xform.inv(pos);
}

GeomId get_hovered_object() {
  return board.hovered_object;
}

bool object_valid(const GeomId id) {
  if (id < 0) return false;
  return board.objects[id].visible();
}

void add_object(const GeomId id) {
  if (id >= board.objects.size()) board.objects.resize(id * 2);
  board.objects[id].init(id, board.xform);
  activate_object(id);
}

void update_objects() {
  for (GeomId i = 0; i < board.objects.size(); i++) {
    Geometry &obj = board.objects[i];
    if (obj.activated) obj.update(i, board.xform);
  }
}

void select_object(const GeomId id) {
  board.objects[id].select();
}

void deselect_object(const GeomId id) {
  board.objects[id].deselect();
}

void activate_object(const GeomId id) {
  Geometry &obj = board.objects[id];
  obj.activate();

  switch (obj.type) {
  case POINT:
    board.points.push_back(id);
    break;
  case LINE:
    board.lines.push_back(id);
    break;
  default:
    board.circles.push_back(id);
  }
}

static void indices_delete(std::vector<GeomId> &indices, const GeomId id) {
  for (size_t i = 0; i < indices.size(); i++) {
    if (indices[i] == id) {
      indices[i] = indices[indices.size() - 1];
      indices.pop_back();
      return;
    }
  }
}

void deactivate_object(const GeomId id) {
  Geometry &obj = board.objects[id];
  obj.deactivate();

  switch (obj.type) {
  case POINT:
    indices_delete(board.points, id);
    break;
  case LINE:
    indices_delete(board.lines, id);
    break;
  default:
    indices_delete(board.circles, id);
  }
}

static GeomId board_hover_object(const Vec2 pos) {
  for (const GeomId id : board.points) {
    if (board.objects[id].hovered(pos)) return id;
  }
  for (const GeomId id : board.lines) {
    if (board.objects[id].hovered(pos)) return id;
  }
  for (const GeomId id : board.circles) {
    if (board.objects[id].hovered(pos)) return id;
  }
  return -1;
}
}