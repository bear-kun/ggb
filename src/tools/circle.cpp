#include "board.hpp"
#include "command.hpp"
#include "helper.hpp"
#include "toolbar.hpp"

namespace app::toolbar {
class Circle final : public GeomTool {
public:
  Circle() {
    usage = "circle: select center point, then point on circle";
  }

  void reset() override {
    if (board::object_exist(center)) {
      board::deselect_object(center);
    }
    center = -1;
  }

  void click(const Vec2 pos) override {
    const GeomId hovered = board::get_hovered_object();
    const GeomId pt = find_or_push_point(hovered, pos);

    if (center != -1) {
      if (!board::object_exist(center)) {
        center = -1;
      } else if (pt == center) {
        board::deselect_object(center);
        center = -1;
        return;
      }
    }

    if (center == -1) {
      center = pt;
      board::select_object(pt);
    } else {
      const GeomId out = geom_new_circle(center, pt);
      command::push(std::make_unique<command::Add>(1, &out));
      reset();
    }
  }

private:
  GeomId center = -1;
};

ToolPtr circle() {
  return std::make_unique<Circle>();
}
}