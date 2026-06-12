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

geom::Handle get_hovered_object();

void set_control(Control &ctrl);
Vec2 xform_to_world(Vec2 pos);
}

#endif // GGB_BOARD_H