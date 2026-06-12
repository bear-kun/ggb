#include "board.hpp"
#include "command.hpp"
#include "toolbar.hpp"

namespace app::toolbar {
class AngleBisector final : public GeomTool {
public:
  AngleBisector() {
    usage = "angle bisector: select two lines";
  }

  void reset() override {
    if (first.valid()) first->deselect();
    first.reset();
  }

  void click(Vec2) override {
    const geom::Handle handle = board::get_hovered_object();
    if (!handle.valid() || handle->type != LINE) return;

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
      const auto out = geom::angle_bisector(first, handle);
      command::push(std::make_unique<command::Add>(2, out.data()));
      reset();
    }
  }

private:
  geom::Handle first;
};

ToolPtr tool_angle_bisector() {
  return std::make_unique<AngleBisector>();
}
}