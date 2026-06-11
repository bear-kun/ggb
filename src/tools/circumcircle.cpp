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
    for (int & point : points) {
      if (board::object_valid(point)) {
        board::deselect_object(point);
      }
      point = -1;
    }
  }

  void click(const Vec2 pos) override {
    const GeomId hovered = board::get_hovered_object();
    const GeomId id = find_or_push_point(hovered, pos);

    for (int & point : points) {
      if (point != -1) {
        if (!board::object_valid(point)) {
          point = -1;
        } else if (id == point) {
          board::deselect_object(point);
          point = -1;
          return;
        }
      }
    }

    for (int & point : points) {
      if (point == -1) {
        point = id;
        board::select_object(id);
        break;
      }
    }

    if (points[0] != -1 && points[1] != -1 && points[2] != -1) {
      const GeomId out = geom_circumcircle(points[0], points[1], points[2]);
      command::push(std::make_unique<command::Add>(1, &out));
      reset();
    }
  }

private:
  GeomId points[3] = {-1, -1, -1};
};

ToolPtr circumcircle() {
  return std::make_unique<Circumcircle>();
}
}