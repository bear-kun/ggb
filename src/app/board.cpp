#include "board.hpp"
#include "command.hpp"
#include "geometry.hpp"

namespace app::board {
static struct {
  rl::Rectangle window{};
  rl::Font font{};

  Control *control = nullptr;
  geom::Handle hovered_object{};

  geom::Transform xform;
} board;

void init(const int x, const int y, const int w, const int h) {
  board.window = {(float)x, (float)y, (float)w, (float)h};
  board.font = rl::get_font_default();

  geom::init();
  command::init();

  geom::set_xform(board.xform);
}

void cleanup() {
  command::cleanup();
  geom::cleanup();
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
  board.hovered_object = geom::get_hovered_object(pos);
  if (board.hovered_object.valid()) {
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
  geom::draw_all();
}

void set_control(Control &ctrl) {
  board.control = &ctrl;
}

Vec2 xform_to_world(const Vec2 pos) {
  return board.xform.inv(pos);
}

geom::Handle get_hovered_object() {
  return board.hovered_object;
}
}