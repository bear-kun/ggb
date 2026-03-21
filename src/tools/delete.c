#include "object.h"
#include "tool.h"

typedef struct {
  bool deleted;
  GeomId id;
} Context;

static void redo(void *ctx) {
  Context *c = ctx;
  c->deleted = true;
  board_remove_object(c->id);
}

static void undo(void *ctx) {
  Context *c = ctx;
  c->deleted = false;
  board_add_object(c->id);
}

static void del(void *ctx) {
  const Context *c = ctx;
  if (c->deleted) {
    object_delete(c->id);
  }
}

static void process(const GeomId id) {
  GeomCommand *cmd = command_create(redo, undo, del, sizeof(Context));
  *(Context *)cmd->ctx = (Context){true, id};
  command_push(cmd, true);
}

static void click(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id != -1) process(id);
}

static void drag(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id != -1) process(id);
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