#include "board.hpp"
#include "command.hpp"
#include "helper.hpp"
#include "toolbar.hpp"

namespace app::toolbar {
class Circumcircle final : public GeomTool {
public:
  Circumcircle() {
    usage = "circumcircle: select three points";
  }

  void reset() override {
    for (geom::Handle &point : points) {
      if (point.valid()) point->deselect();
      point.reset();
    }
  }

  void click(const Vec2 pos) override {
    const geom::Handle hovered = board::get_hovered_object();
    const geom::Handle handle = find_or_push_point(hovered, pos);

    for (geom::Handle &point : points) {
      if (point.valid()) {
        if (!point->visible()) {
          point.reset();
        } else if (handle == point) {
          point->deselect();
          point.reset();
          return;
        }
      }
    }

    for (geom::Handle &point : points) {
      if (!point.valid()) {
        point = handle;
        handle->select();
        break;
      }
    }

    if (points[0].valid() && points[1].valid() && points[2].valid()) {
      const geom::Handle out = geom::circumcircle(points[0], points[1], points[2]);
      command::push(std::make_unique<command::Add>(1, &out));
      reset();
    }
  }

private:
  geom::Handle points[3];
};

ToolPtr tool_circumcircle() {
  return std::make_unique<Circumcircle>();
}
}