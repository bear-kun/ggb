#include "object.h"
#include "tool.h"
#include <math.h>

static int eval_2pt(const float xyxy[4], float radius[1]) {
  const float dx = xyxy[2] - xyxy[0];
  const float dy = xyxy[3] - xyxy[1];
  radius[0] = sqrtf(dx * dx + dy * dy);
  return 1;
}

typedef struct {
  bool deleted;
  GeomId circle;
} Context;

static void redo(void *ctx) {
  Context *c = ctx;
  c->deleted = false;
  board_add_object(c->circle);
}

static void undo(void *ctx) {
  Context *c = ctx;
  c->deleted = true;
  board_remove_object(c->circle);
}

static void del(void *ctx) {
  const Context *c = ctx;
  if (c->deleted) {
    object_delete(c->circle);
  }
}

static void process(const GeomId inputs[4]) {
  GeomId args[3];
  args[0] = inputs[0];
  args[1] = inputs[1];
  args[2] = graph_add_value(0);

  const GeomId define = graph_add_constraint(4, inputs, 1, args + 2, eval_2pt);
  const GeomId cr = object_create(CIRCLE, args, define, 0);

  GeomCommand *cmd = command_create(redo, undo, del, sizeof(Context));
  *(Context *)cmd->ctx = (Context){false, cr};
  command_push(cmd, true);
}

static struct {
  GeomId center;
  GeomId inputs[4];
} intl = {-1};

static void reset() {
  if (intl.center != -1) {
    board_deselect_object(intl.center);
    intl.center = -1;
  }
}

static void click(Vec2 pos) {
  const GeomId id = board_hovered_object();
  if (id == -1) return;
  const GeomObject *obj = object_get(id);
  if (obj->type != POINT) return;

  if (intl.center != -1) {
    if (!board_exist(intl.center)) {
      intl.center = -1;
    } else if (id == intl.center) {
      board_deselect_object(intl.center);
      intl.center = -1;
      return;
    }
  }

  if (intl.center == -1) {
    intl.center = id;
    board_select_object(id);
    copy_args(intl.inputs, obj->args, 2);
  } else {
    copy_args(intl.inputs + 2, obj->args, 2);
    process(intl.inputs);
    reset();
  }
}

void tool_circle(GeomTool *tool) {
  tool->usage = "circle: select center point, then point on circle";
  tool->reset = reset;
  tool->ctrl.mouse_down = NULL;
  tool->ctrl.mouse_up = NULL;
  tool->ctrl.mouse_click = click;
  tool->ctrl.mouse_move = NULL;
  tool->ctrl.mouse_drag = NULL;
}