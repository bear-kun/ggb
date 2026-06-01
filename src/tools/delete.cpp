#include "toolbar.hpp"
#include "command.hpp"

namespace app::toolbar {
class Delete final : public GeomTool {
public:
  Delete() {
    usage = "delete: select object to delete";
  }

  void click(Vec2) override {
    const GeomId id = board::get_hovered_object();
    if (id != -1) return;
    command::push(std::make_unique<command::Delete>(1, &id));
  }

  void drag(const Vec2 pos) override {
    click(pos);
  }
};

ToolPtr delete_() {
  return std::make_unique<Delete>();
}
}