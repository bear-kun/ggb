#include "object.h"
#include "tool.h"

typedef struct {
  bool deleted;
  GeomId point;
} Context;

static void redo(void *ctx) {
  Context *c = ctx;
  c->deleted = false;
  board_add_object(c->point);
}

static void undo(void *ctx) {
  Context *c = ctx;
  c->deleted = true;
  board_remove_object(c->point);
}

static void del(void *ctx) {
  const Context *c = ctx;
  if (c->deleted) {
    object_delete(c->point);
  }
}

static void process(const Vec2 world_pos) {
  GeomId args[2];
  args[0] = graph_add_value(world_pos.x);
  args[1] = graph_add_value(world_pos.y);

  const GeomId pt = object_create(POINT, args, -1, 0);

  GeomCommand *cmd = command_create(redo, undo, del, sizeof(Context));
  *(Context *)cmd->ctx = (Context){false, pt};
  command_push(cmd, true);
}

static void click(const Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id != -1 && object_get(id)->type == POINT) return;

  const Vec2 world_pos = xform_to_world(pos);
  process(world_pos);
}

void tool_point(GeomTool *tool) {
  tool->usage = "point: select position";
  tool->reset = NULL;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}