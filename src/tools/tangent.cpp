#include "board.hpp"
#include "toolbar.hpp"
#include "command.hpp"
#include "helper.hpp"

namespace app::toolbar {
class Tangent final : public GeomTool {
public:
  Tangent() {
    usage = "tangents: select point or circle";
  }

  void reset() override {
    if (first.valid()) first->deselect();
    first.reset();
  }

  static geom::Handle get_required(const Vec2 pos) {
    const geom::Handle hovered = board::get_hovered_object();
    if (!hovered.valid()) return find_or_push_point(hovered, pos);
    if (hovered->type & (POINT | CIRCLE)) return hovered;
    return find_or_push_point(hovered, pos);
  }

  void click(const Vec2 pos) override {
    const geom::Handle handle = get_required(pos);

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

    std::array<geom::Handle, 4> out;
    if (first->type == POINT) {
      if (handle->type == POINT) return;
      out = geom::tangent(handle, first);
    } else {
      // first_t == CIRCLE
      out = geom::tangent(first, handle);
    }

    command::push(std::make_unique<command::Add>(out[2].valid() ? 4 : 2, out.data()));
    reset();
  }

private:
  geom::Handle first;
};

ToolPtr tool_tangent() {
  return std::make_unique<Tangent>();
}
}