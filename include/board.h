#ifndef GGB_BOARD_H
#define GGB_BOARD_H

#include "types.h"

typedef enum {
  MOUSE_NULL = 0,
  MOUSE_PRESS = 1,
  MOUSE_RELEASE = 2
} MouseEvent;

typedef void (*BoardControl)(Vec2 pos, MouseEvent event);

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
GeomId board_find_object(ObjectType types, Vec2 pos);

#endif // GGB_BOARD_H