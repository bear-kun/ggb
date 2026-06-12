#include "geometry.hpp"
#include "raylib.hpp"
#include "menu_bar.hpp"
#include <array>

namespace app::menu_bar {
using ItemInit = MenuItemPtr (*)();

static constexpr int MENU_ITEM_COUNT = 2;
static constexpr int MENU_ITEM_WIDTH = 80;
static constexpr std::array<ItemInit, MENU_ITEM_COUNT> item_init = {
    item_load, item_save
};

static struct {
  rl::Rectangle window{};

  std::array<MenuItemPtr, MENU_ITEM_COUNT> items;
} menu_bar;

void init(const int x, const int y, const int w, const int h) {
  menu_bar.window = {(float)x, (float)y, (float)w, (float)h};
  for (int i = 0; i < MENU_ITEM_COUNT; i++) {
    menu_bar.items[i] = item_init[i]();
  }
}

void cleanup() {
  for (int i = 0; i < MENU_ITEM_COUNT; i++) {
    menu_bar.items[i].reset();
  }
}

void listen() {
  const Vec2 mouse = rl::get_mouse_position();
  if (!rl::check_collision_point_rec(mouse, menu_bar.window)) return;
  if (!rl::is_mouse_button_pressed(rl::MOUSE_BUTTON_LEFT)) return;

  const int i = static_cast<int>(mouse.x - menu_bar.window.x) / MENU_ITEM_WIDTH;
  if (i >= MENU_ITEM_COUNT) return;

  menu_bar.items[i]->execute();
}

void draw() {
  static constexpr rl::Color bkg_color = rl::GRAY;
  static constexpr rl::Color text_color = rl::BLACK;
  static const rl::Font &font = rl::get_font_default();

  rl::draw_rectangle_rec(menu_bar.window, bkg_color);

  const float font_size = menu_bar.window.height * 0.75f;
  int middle_x = static_cast<int>(menu_bar.window.x) + MENU_ITEM_WIDTH / 2;
  const int middle_y = static_cast<int>(menu_bar.window.y + menu_bar.window.height / 2);
  for (const auto &item : menu_bar.items) {
    const Vec2 size = rl::measure_text_ex(font, item->title.c_str(), font_size, 1);
    const Vec2 top_left = {(float)middle_x - size.x / 2, (float)middle_y - size.y / 2};
    rl::draw_text_ex(font, item->title.c_str(), top_left, font_size, 1, text_color);
    middle_x += MENU_ITEM_WIDTH;
  }
}
}