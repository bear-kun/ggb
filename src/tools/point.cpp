#include "board.hpp"
#include "toolbar.hpp"
#include "command.hpp"

namespace app::toolbar {
geom::Handle find_or_push_point(const geom::Handle hovered, const Vec2 pos) {
  geom::Handle pt;
  const auto [x, y] = board::xform_to_world(pos);

  if (!hovered.valid()) {
    pt = geom::new_point(x, y);
  } else {
    if (hovered->type == POINT) return hovered;
    pt = geom::new_point(x, y, hovered);
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
    const geom::Handle handle = board::get_hovered_object();
    find_or_push_point(handle, pos);
  }
};

ToolPtr tool_point() {
  return std::make_unique<Point>();
}
}