#ifndef GGB_COMMAND_H
#define GGB_COMMAND_H

#include <stdbool.h>

typedef void (*CommandFn)(void *);

typedef struct {
  CommandFn redo;
  CommandFn undo;
  CommandFn del;
  char ctx[0];
} GeomCommand;

void command_module_init();
void command_module_cleanup();

GeomCommand *command_create(CommandFn redo, CommandFn undo, CommandFn del, unsigned ctx_size);
void command_push(GeomCommand *cmd, bool first_do);
void command_undo();
void command_redo();

#endif //GGB_COMMAND_H