#ifndef GGB_BOARD_H
#define GGB_BOARD_H

#include "types.h"

typedef struct {
  void (*mouse_down)(Vec2 pos);
  void (*mouse_up)(Vec2 pos);
  void (*mouse_click)(Vec2 pos);
  void (*mouse_move)(Vec2 pos);
  void (*mouse_drag)(Vec2 pos);
} BoardControl;

void board_init(int x, int y, int w, int h);
void board_listen();
void board_draw();
void board_cleanup();

void board_add_object(GeomId id);
void board_remove_object(GeomId id);
void board_set_control(BoardControl ctrl);
void board_select_object(GeomId id);
void board_deselect_object(GeomId id);
void board_update_objects();

Vec2 xform_to_world(Vec2 pos);
GeomId board_hovered_object();

#endif // GGB_BOARD_H