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
    if (center.valid()) center->deselect();
    center.reset();
  }

  void click(const Vec2 pos) override {
    const geom::Handle hovered = board::get_hovered_object();
    const geom::Handle handle = find_or_push_point(hovered, pos);

    if (center.valid()) {
      if (!center->visible()) {
        center.reset();
      } else if (handle == center) {
        center->deselect();
        center.reset();
        return;
      }
    }

    if (!center.valid()) {
      center = handle;
      handle->select();
    } else {
      const geom::Handle out = geom::new_circle(center, handle);
      command::push(std::make_unique<command::Add>(1, &out));
      reset();
    }
  }

private:
  geom::Handle center;
};

ToolPtr tool_circle() {
  return std::make_unique<Circle>();
}
}