#include "toolbar.hpp"
#include <array>

namespace app::toolbar {
using ToolInit = ToolPtr (*)();

static constexpr int TOOL_COUNT = 12;
static constexpr int TOOL_ICON_SIZE = 32;
static constexpr int TOOL_ICON_BORDER_X = 4;
static constexpr int TOOL_ICON_BORDER_Y = 4;
static constexpr std::array<ToolInit, TOOL_COUNT> tool_init = {
    tool_move, tool_point, tool_line, tool_circle,
    tool_midpoint, tool_perpendicular, tool_parallel, tool_angle_bisector,
    tool_tangent, tool_circumcircle, tool_intersection, tool_delete
};

static struct {
  rl::Rectangle window{};

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
  const Vec2 mouse = rl::get_mouse_position();
  if (!rl::check_collision_point_rec(mouse, toolbar.window)) return;
  if (!rl::is_mouse_button_pressed(rl::MOUSE_BUTTON_LEFT)) return;

  const int x = static_cast<int>(mouse.x - toolbar.window.x);
  const int y = static_cast<int>(mouse.y - toolbar.window.y);
  if (y < TOOL_ICON_BORDER_Y || y >= TOOL_ICON_BORDER_Y + TOOL_ICON_SIZE) {
    return;
  }

  const int i = x / (TOOL_ICON_BORDER_X + TOOL_ICON_SIZE);
  const int border = x % (TOOL_ICON_BORDER_X + TOOL_ICON_SIZE);
  if (i >= TOOL_COUNT || border < TOOL_ICON_BORDER_X) return;

  if (toolbar.selected) toolbar.selected->reset();
  toolbar.selected = toolbar.tools[i].get();
  board::set_control(*toolbar.selected);
}

void draw() {
  static constexpr rl::Color bkg_color = rl::DARKGRAY;

  rl::draw_rectangle_rec(toolbar.window, bkg_color);

  int x = TOOL_ICON_BORDER_X + static_cast<int>(toolbar.window.x);
  const int y = TOOL_ICON_BORDER_Y + static_cast<int>(toolbar.window.y);
  for (int i = 0; i < TOOL_COUNT; i++) {
    rl::draw_rectangle(x, y, TOOL_ICON_SIZE, TOOL_ICON_SIZE, rl::WHITE);
    x += TOOL_ICON_SIZE + TOOL_ICON_BORDER_X;
  }

  if (toolbar.selected) {
    x = TOOL_ICON_BORDER_X + static_cast<int>(toolbar.window.x);
    rl::draw_text(toolbar.selected->usage.c_str(), x, y + TOOL_ICON_BORDER_Y * 2 + TOOL_ICON_SIZE,
                  30, rl::GREEN);
  }
}
}