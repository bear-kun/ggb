#ifndef GGB_BOARD_H
#define GGB_BOARD_H

#include "geometry.hpp"

namespace app::board {
class Control {
public:
  virtual ~Control() = default;

  virtual void down(Vec2) {
  }

  virtual void up(Vec2) {
  }

  virtual void click(Vec2) {
  }

  virtual void move(Vec2) {
  }

  virtual void drag(Vec2) {
  }
};

void init(int x, int y, int w, int h);
void listen();
void draw();
void cleanup();

GeomId get_hovered_object();
bool object_valid(GeomId id);
void add_object(GeomId id);
void activate_object(GeomId id);
void deactivate_object(GeomId id);
void select_object(GeomId id);
void deselect_object(GeomId id);
void update_objects();

void set_control(Control &ctrl);
Vec2 xform_to_world(Vec2 pos);
}

#endif // GGB_BOARD_H