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
    if (object != -1) {
      board::deselect_object(object);
      object = -1;
    }
  }

  void down(const Vec2 pos) override {
    const GeomId id = board::get_hovered_object();
    if (id == -1 || geom_get_object(id)->type != POINT) {
      reset();
      return;
    }
    object = id;
    from = board::xform_to_world(pos);
    board::select_object(id);

  }

  void up(const Vec2 pos) override {
    const Vec2 to = board::xform_to_world(pos);
    command::push(std::make_unique<command::Move>(object, from, to));
    reset();
  }

  void drag(const Vec2 pos) override {
    if (object == -1) return;

    const Vec2 to = board::xform_to_world(pos);
    geom_move(object, (float *)&to);
    board::update_objects();
  }

private:
  GeomId object = -1;
  Vec2 from{};
};

ToolPtr move() {
  return std::make_unique<Move>();
}
}