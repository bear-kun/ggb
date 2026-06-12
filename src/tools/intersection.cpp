#include "board.hpp"
#include "command.hpp"
#include "toolbar.hpp"

namespace app::toolbar {
class Intersection final : public GeomTool {
public:
  Intersection() {
    usage = "intersection point: select two lines, circles or both";
  }

  void reset() override {
    if (first.valid()) first->deselect();
    first.reset();
  }

  void click(Vec2 pos) override {
    const geom::Handle handle = board::get_hovered_object();
    if (!handle.valid() || handle->type == POINT) return;

    if (first.valid()) {
      if (!first->visible()) {
        first.reset();
      } else if (handle == first) {
        first->deselect();
        first.reset();
        return;
      }
    }

    if (!first.valid()) {
      first = handle;
      handle->select();
    } else {
      const auto out = geom::intersection(first, handle);
      command::push(std::make_unique<command::Add>(out[1].valid() ? 2 : 1, out.data()));
      reset();
    }
  }

private:
  geom::Handle first;
};

ToolPtr tool_intersection() {
  return std::make_unique<Intersection>();
}
}