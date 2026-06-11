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
    if (board::object_valid(first)) {
      board::deselect_object(first);
    }
    first = -1;
  }

  void click(Vec2) override {
    const GeomId id = board::get_hovered_object();
    if (id == -1 || geom_get_type(id) != LINE) return;

    if (first != -1) {
      if (!board::object_valid(first)) {
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
      geom_angle_bisector(first, id, out);
      command::push(std::make_unique<command::Add>(2, out));
      reset();
    }
  }

private:
  GeomId first = -1;
};

ToolPtr angle_bisector() {
  return std::make_unique<AngleBisector>();
}
}