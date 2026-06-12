#include "board.hpp"
#include "command.hpp"
#include "toolbar.hpp"

namespace app::toolbar {
class Move final : public GeomTool {
public:
  Move() {
    usage = "move: drag or select object";
  }

  void reset() override {
    if (object.valid()) object->deselect();
    object.reset();
  }

  void down(const Vec2 pos) override {
    const geom::Handle handle = board::get_hovered_object();
    if (!handle.valid() || handle->type != POINT) {
      reset();
      return;
    }
    object = handle;
    from = board::xform_to_world(pos);
    handle->select();
  }

  void up(const Vec2 pos) override {
    if (!object.valid()) return;

    const Vec2 to = board::xform_to_world(pos);
    command::push(std::make_unique<command::Move>(object, from, to));
    reset();
  }

  void drag(const Vec2 pos) override {
    if (!object.valid()) return;

    const Vec2 to = board::xform_to_world(pos);
    geom::move(object, to);
    geom::update_all();
  }

private:
  geom::Handle object;
  Vec2 from{};
};

ToolPtr tool_move() {
  return std::make_unique<Move>();
}
}