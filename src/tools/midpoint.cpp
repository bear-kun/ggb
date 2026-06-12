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
    if (first.valid()) first->deselect();
    first.reset();
  }

  void click(const Vec2 pos) override {
    const geom::Handle hovered = board::get_hovered_object();
    const geom::Handle handle = find_or_push_point(hovered, pos);

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
      const geom::Handle out = geom::midpoint(first, handle);
      command::push(std::make_unique<command::Add>(1, &out));
      reset();
    }
  }

private:
  geom::Handle first;
};

ToolPtr tool_midpoint() {
  return std::make_unique<Midpoint>();
}
}