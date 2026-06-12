#include "toolbar.hpp"
#include "command.hpp"

namespace app::toolbar {
class Delete final : public GeomTool {
public:
  Delete() {
    usage = "delete: select object to delete";
  }

  void click(Vec2) override {
    const geom::Handle handle = board::get_hovered_object();
    if (!handle.valid()) return;
    command::push(std::make_unique<command::Delete>(1, &handle));
  }

  void drag(const Vec2 pos) override {
    click(pos);
  }
};

ToolPtr tool_delete() {
  return std::make_unique<Delete>();
}
}