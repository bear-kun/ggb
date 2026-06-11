#include "toolbar.hpp"
#include <array>

namespace app::toolbar {
static constexpr int TOOL_COUNT = 12;
static constexpr int TOOL_ICON_SIZE = 32;
static constexpr int TOOL_ICON_BORDER_X = 4;
static constexpr int TOOL_ICON_BORDER_Y = 4;

using ToolInit = ToolPtr (*)();

static constexpr std::array<ToolInit, TOOL_COUNT> tool_init = {
    move, point, line, circle,
    midpoint, perpendicular, parallel, angle_bisector,
    tangent, circumcircle, intersection, delete_
};

static struct {
  rl::Rectangle window{};
  Color bkg_color = rl::DARKGRAY;

  GeomTool *selected = nullptr;
  std::array<ToolPtr, TOOL_COUNT> tools;
} toolbar;

void init(const int x, const int y, const int w, const int h) {
  toolbar.window = {(float)x, (float)y, (float)w, (float)h};
  for (int i = 0; i < TOOL_COUNT; i++) toolbar.tools[i] = tool_init[i]();
}

void cleanup() {
  for (int i = 0; i < TOOL_COUNT; i++) toolbar.tools[i].reset();
}

void listen() {
  const Vec2 pos = rl::get_mouse_position();
  if (!rl::check_collision_point_rec(pos, toolbar.window)) return;
  if (!rl::is_mouse_button_pressed(rl::MOUSE_BUTTON_LEFT)) return;

  const int x = static_cast<int>(pos.x);
  const int y = static_cast<int>(pos.y);
  if (y < TOOL_ICON_BORDER_Y || y >= TOOL_ICON_BORDER_Y + TOOL_ICON_SIZE) {
    return;
  }

  const int count = x / (TOOL_ICON_BORDER_X + TOOL_ICON_SIZE);
  const int border = x % (TOOL_ICON_BORDER_X + TOOL_ICON_SIZE);
  if (count >= TOOL_COUNT || border < TOOL_ICON_BORDER_X) return;

  if (toolbar.selected) toolbar.selected->reset();
  toolbar.selected = toolbar.tools[count].get();
  board::set_control(*toolbar.selected);
}

void draw() {
  rl::draw_rectangle_rec(toolbar.window, toolbar.bkg_color);

  int x = TOOL_ICON_BORDER_X;
  for (int i = 0; i < TOOL_COUNT; i++) {
    rl::draw_rectangle(x, TOOL_ICON_BORDER_Y, TOOL_ICON_SIZE, TOOL_ICON_SIZE, rl::WHITE);
    x += TOOL_ICON_SIZE + TOOL_ICON_BORDER_X;
  }

  if (toolbar.selected) {
    rl::draw_text(toolbar.selected->usage.c_str(), TOOL_ICON_BORDER_X,
                  TOOL_ICON_BORDER_Y * 3 + TOOL_ICON_SIZE, 30, rl::GREEN);
  }
}
}