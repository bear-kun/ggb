#include "board.hpp"
#include "toolbar.hpp"
#include "command.hpp"
#include "helper.hpp"

namespace app::toolbar {
class Perpendicular final : public GeomTool {
public:
  Perpendicular() {
    usage = "perpendicular line: select line and point";
  }

  void reset() override {
    if (first.valid()) first->deselect();
    first.reset();
  }

  static geom::Handle get_required(const Vec2 pos) {
    const geom::Handle hovered = board::get_hovered_object();
    if (!hovered.valid()) return find_or_push_point(hovered, pos);
    if (hovered->type & (POINT | LINE)) return hovered;
    return find_or_push_point(hovered, pos);
  }

  void click(const Vec2 pos) override {
    const geom::Handle handle = get_required(pos);
    const GeomType type = handle->type;

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
      return;
    }

    if (type == first->type) return;

    geom::Handle out;
    if (type == LINE) {
      out = geom::perpendicular(handle, first);
    } else {
      out = geom::perpendicular(first, handle);
    }

    command::push(std::make_unique<command::Add>(1, &out));
    reset();
  }

private:
  geom::Handle first;
};

ToolPtr tool_perpendicular() {
  return std::make_unique<Perpendicular>();
}
}