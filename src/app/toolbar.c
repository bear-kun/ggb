#include "toolbar.h"
#include "raylib_.h"
#include "tool.h"

typedef void (*ToolInit)(GeomTool *);

ToolInit tool_init[] = {tool_move, tool_point, tool_line, tool_circle,
                        tool_midpoint, tool_perp, tool_parallel,
                        tool_bisector, tool_tangent, tool_circum,
                        tool_isect, tool_delete};

#define TOOL_COUNT (sizeof(tool_init) / sizeof(ToolInit))
#define TOOL_ICON_SIZE 32
#define TOOL_ICON_BORDER_X 4
#define TOOL_ICON_BORDER_Y 4

static struct {
  Rectangle window;
  Color bkg_color;

  GeomTool *selected;
  GeomTool tools[TOOL_COUNT];
} toolbar;

void toolbar_init(const int x, const int y, const int w, const int h) {
  toolbar.window = (Rectangle){(float)x, (float)y, (float)w, (float)h};
  toolbar.bkg_color = DARKGRAY;

  toolbar.selected = NULL;
  for (int i = 0; i < TOOL_COUNT; i++) {
    tool_init[i](toolbar.tools + i);
  }
}

void toolbar_cleanup() {
}

void toolbar_listen() {
  const Vec2 pos = rl_get_mouse_position();
  if (!rl_check_collision_point_rec(pos, toolbar.window)) return;
  if (!rl_is_mouse_button_pressed(MOUSE_BUTTON_LEFT)) return;

  const int x = (int)pos.x;
  const int y = (int)pos.y;
  if (y < TOOL_ICON_BORDER_Y || y >= TOOL_ICON_BORDER_Y + TOOL_ICON_SIZE) {
    return;
  }

  const int count = x / (TOOL_ICON_BORDER_X + TOOL_ICON_SIZE);
  const int border = x % (TOOL_ICON_BORDER_X + TOOL_ICON_SIZE);
  if (count >= TOOL_COUNT || border < TOOL_ICON_BORDER_X) return;

  if (toolbar.selected && toolbar.selected->reset) toolbar.selected->reset();
  toolbar.selected = toolbar.tools + count;
  board_set_control(toolbar.selected->ctrl);
}

void toolbar_draw() {
  rl_draw_rectangle_rec(toolbar.window, toolbar.bkg_color);

  int x = TOOL_ICON_BORDER_X;
  for (int i = 0; i < TOOL_COUNT; i++) {
    rl_draw_rectangle(x, TOOL_ICON_BORDER_Y, TOOL_ICON_SIZE, TOOL_ICON_SIZE,
                      WHITE);
    x += TOOL_ICON_SIZE + TOOL_ICON_BORDER_X;
  }

  if (toolbar.selected) {
    rl_draw_text(toolbar.selected->usage, TOOL_ICON_BORDER_X,
                 TOOL_ICON_BORDER_Y * 3 + TOOL_ICON_SIZE, 30, GREEN);
  }
}