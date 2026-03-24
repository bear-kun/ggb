#include "object.h"
#include "tool.h"

typedef struct {
  GeomId id;
  Vec2 from, to;
} Context;

static void redo(void *ctx) {
  const Context *c = ctx;
  const GeomObject *obj = object_get(c->id);
  graph_change_value(2, obj->args, (float *)&c->to);
  board_update_objects();
}

static void undo(void *ctx) {
  const Context *c = ctx;
  const GeomObject *obj = object_get(c->id);
  graph_change_value(2, obj->args, (float *)&c->from);
  board_update_objects();
}

static void process(const GeomId id, const Vec2 from, const Vec2 to) {
  GeomCommand *cmd = command_create(redo, undo, NULL, sizeof(Context));
  *(Context *)cmd->ctx = (Context){id, from, to};
  command_push(cmd, false);
}

static struct {
  GeomId id;
  Vec2 from; // world
} intl = {-1};

static void reset() {
  if (intl.id != -1) {
    board_deselect_object(intl.id);
    intl.id = -1;
  }
}

static void down(const Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id == -1 || object_get(id)->type != POINT) {
    reset();
  } else {
    intl.id = id;
    intl.from = xform_to_world(pos);
    board_select_object(id);
  }
}

static void up(const Vec2 pos) {
  process(intl.id, intl.from, xform_to_world(pos));
  reset();
}

static void drag(const Vec2 pos) {
  if (intl.id == -1) return;

  const Vec2 to = xform_to_world(pos);
  const GeomObject *obj = object_get(intl.id);
  graph_change_value(2, obj->args + 2, (float *)&to);
  board_update_objects();
}

void tool_move(GeomTool *tool) {
  tool->usage = "move: drag or select object";
  tool->reset = reset;
  tool->ctrl.mouse_down = down;
  tool->ctrl.mouse_up = up;
  tool->ctrl.mouse_click = NULL;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = drag;
}