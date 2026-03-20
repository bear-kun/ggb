#include "command.h"
#include <stdlib.h>

#define STACK_CAPACITY 16

static struct {
  GeomCommand *stack[STACK_CAPACITY];
  int size, top;
} manager;

void command_module_init() {
  manager.size = manager.top = 0;
}

void command_module_cleanup() {
  for (int i = 0; i < manager.size; i++) {
    free(manager.stack[i]);
  }
}

GeomCommand *command_create(const CommandFn exec, const CommandFn undo,
                            const unsigned size) {
  GeomCommand *cmd = malloc(sizeof(GeomCommand) + size);
  cmd->exec = exec;
  cmd->undo = undo;
  return cmd;
}

void command_push(GeomCommand *cmd) {
  if (manager.top == STACK_CAPACITY) {
    free(manager.stack[0]);
    for (int i = 1; i < STACK_CAPACITY; i++) {
      manager.stack[i - 1] = manager.stack[i];
    }
    manager.stack[STACK_CAPACITY - 1] = cmd;
  } else {
    manager.stack[manager.top++] = cmd;
    manager.size = manager.top;
  }
}

void command_undo() {
  if (manager.top == 0) return;
  GeomCommand *cmd = manager.stack[--manager.top];
  cmd->undo(cmd->ctx);
}

void command_redo() {
  if (manager.top == manager.size) return;
  GeomCommand *cmd = manager.stack[manager.top++];
  cmd->exec(cmd->ctx);
}