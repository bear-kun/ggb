#include "board.hpp"
#include "command.hpp"
#include "toolbar.hpp"

namespace app::toolbar {
class Bisector final : public GeomTool {
public:
  Bisector() {
    usage = "angle bisector: select two lines";
  }

  void reset() override {
    if (board::object_exist(first)) {
      board::deselect_object(first);
    }
    first = -1;
  }

  void click(Vec2) override {
    const GeomId id = board::get_hovered_object();
    if (id == -1) return;
    const CGeometry *obj = geom_get_object(id);
    if (obj->type != LINE) return;

    if (first != -1) {
      if (!board::object_exist(first)) {
        first = -1;
      } else if (id == first) {
        board::deselect_object(first);
        first = -1;
        return;
      }
    }

    if (first == -1) {
      first = id;
      board::select_object(id);
    } else {
      GeomId out[2];
      geom_bisector(first, id, out);
      command::push(std::make_unique<command::Add>(2, out));
      reset();
    }
  }

private:
  GeomId first = -1;
};

ToolPtr bisector() {
  return std::make_unique<Bisector>();
}
}