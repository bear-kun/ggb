#include "board.hpp"
#include "toolbar.hpp"
#include "command.hpp"

namespace app::toolbar {
GeomId find_or_push_point(const GeomId hovered, const Vec2 pos) {
  GeomId pt;
  const Vec2 world_pos = board::xform_to_world(pos);

  if (hovered == -1) {
    pt = geom_new_point(world_pos.x, world_pos.y, -1);
  } else {
    const CGeometry *obj = geom_get_object(hovered);
    if (obj->type == POINT) return hovered;

    pt = geom_new_point(world_pos.x, world_pos.y, hovered);
  }

  command::push(std::make_unique<command::Add>(1, &pt));
  return pt;
}

class Point final : public GeomTool {
public:
  Point() {
    usage = "point: select position";
  }

  void click(const Vec2 pos) override {
    const GeomId id = board::get_hovered_object();
    find_or_push_point(id, pos);
  }
};

ToolPtr point() {
  return std::make_unique<Point>();
}
}