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
    if (board::object_valid(first_id)) {
      board::deselect_object(first_id);
    }
    first_id = -1;
  }

  static GeomId get_required(const Vec2 pos) {
    const GeomId hovered = board::get_hovered_object();
    if (hovered == -1) return find_or_push_point(hovered, pos);
    if (geom_get_type(hovered) & (POINT | LINE)) return hovered;
    return find_or_push_point(hovered, pos);
  }

  void click(const Vec2 pos) override {
    const GeomId id = get_required(pos);
    const GeomType type = geom_get_type(id);

    if (first_id != -1) {
      if (!board::object_valid(first_id)) {
        first_id = -1;
      } else if (id == first_id) {
        board::deselect_object(first_id);
        first_id = -1;
        return;
      }
    }

    if (first_id == -1) {
      first_id = id;
      first_t = type;
      board::select_object(id);
      return;
    }

    if (type == first_t) return;

    GeomId out;
    if (type == LINE) {
      out = geom_perpendicular(id, first_id);
    } else {
      out = geom_perpendicular(first_id, id);
    }

    command::push(std::make_unique<command::Add>(1, &out));
    reset();
  }

private:
  GeomId first_id = -1;
  GeomType first_t = UNKNOWN;
};

ToolPtr perpendicular() {
  return std::make_unique<Perpendicular>();
}
}