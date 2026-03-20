#include "object.h"
#include "tool.h"

static void click(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id != -1) {
    board_remove_object(id);
    object_delete(id);
  }
}

static void drag(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id != -1) {
    board_remove_object(id);
    object_delete(id);
  }
}

void tool_delete(GeomTool *tool) {
  tool->usage = "delete: select object to delete";
  tool->reset = NULL;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = drag;
}