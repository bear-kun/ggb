#include "board.hpp"
#include "command.hpp"
#include "helper.hpp"
#include "toolbar.hpp"

namespace app::toolbar {
class Midpoint final : public GeomTool {
public:
  Midpoint() {
    usage = "midpoint: select two points";
  }

  void reset() override {
    if (board::object_valid(first)) {
      board::deselect_object(first);
    }
    first = -1;
  }

  void click(const Vec2 pos) override {
    const GeomId hovered = board::get_hovered_object();
    const GeomId pt = find_or_push_point(hovered, pos);

    if (first != -1) {
      if (!board::object_valid(first)) {
        first = -1;
      } else if (pt == first) {
        board::deselect_object(first);
        first = -1;
        return;
      }
    }

    if (first == -1) {
      first = pt;
      board::select_object(pt);
    } else {
      const GeomId out = geom_midpoint(first, pt);
      command::push(std::make_unique<command::Add>(1, &out));
      reset();
    }
  }

private:
  GeomId first = -1;
};

ToolPtr midpoint() {
  return std::make_unique<Midpoint>();
}
}