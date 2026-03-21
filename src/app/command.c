#include "command.h"
#include <stdlib.h>

#define STACK_CAPACITY 16

static struct {
  GeomCommand *stack[STACK_CAPACITY];
  int size, top;
} manager;

static void command_delete(const int idx) {
  GeomCommand *cmd = manager.stack[idx];
  if (cmd->del) cmd->del(cmd->ctx);
  free(cmd);
}

void command_module_init() {
  manager.size = manager.top = 0;
}

void command_module_cleanup() {
  for (int i = 0; i < manager.size; i++) command_delete(i);
}

GeomCommand *command_create(CommandFn redo, CommandFn undo, CommandFn del,
                            const unsigned ctx_size) {
  GeomCommand *cmd = malloc(sizeof(GeomCommand) + ctx_size);
  cmd->redo = redo;
  cmd->undo = undo;
  cmd->del = del;
  return cmd;
}

void command_push(GeomCommand *cmd, const bool first_do) {
  if (manager.top == STACK_CAPACITY) {
    command_delete(0);

    for (int i = 1; i < STACK_CAPACITY; i++) {
      manager.stack[i - 1] = manager.stack[i];
    }

    manager.stack[STACK_CAPACITY - 1] = cmd;
  } else {
    for (int i = manager.top; i < manager.size; i++) command_delete(i);

    manager.stack[manager.top++] = cmd;
    manager.size = manager.top;
  }

  if (first_do) cmd->redo(cmd->ctx);
}

void command_undo() {
  if (manager.top == 0) return;
  GeomCommand *cmd = manager.stack[--manager.top];
  cmd->undo(cmd->ctx);
}

void command_redo() {
  if (manager.top == manager.size) return;
  GeomCommand *cmd = manager.stack[manager.top++];
  cmd->redo(cmd->ctx);
}