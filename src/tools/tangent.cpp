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
    if (board::object_exist(first_id)) {
      board::deselect_object(first_id);
    }
    first_id = -1;
  }

  static GeomId get_required(const Vec2 pos) {
    const GeomId hovered = board::get_hovered_object();
    if (hovered == -1) return find_or_push_point(hovered, pos);
    const CGeometry *obj = geom_get_object(hovered);
    if (obj->type & (POINT | CIRCLE)) return hovered;
    return find_or_push_point(hovered, pos);
  }

  void click(const Vec2 pos) override {
    const GeomId id = get_required(pos);
    const CGeometry *obj = geom_get_object(id);
    const GeomType type = obj->type;

    if (first_id != -1) {
      if (!board::object_exist(first_id)) {
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

    GeomId out[4];
    if (first_t == POINT) {
      if (type != CIRCLE) return;
      geom_tangent(id, first_id, out);
    } else {
      // first_t == CIRCLE
      geom_tangent(first_id, id, out);
    }

    command::push(std::make_unique<command::Add>(out[2] == -1 ? 2 : 4, out));
    reset();
  }

private:
  GeomId first_id = -1;
  GeomType first_t = UNKNOWN;
};

ToolPtr tangent() {
  return std::make_unique<Tangent>();
}
}