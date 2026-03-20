#ifndef GGB_COMMAND_H
#define GGB_COMMAND_H

typedef void (*CommandFn)(void *);

typedef struct {
  CommandFn exec;
  CommandFn undo;
  char ctx[0];
} GeomCommand;

void command_module_init();
void command_module_cleanup();

GeomCommand *command_create(CommandFn exec, CommandFn undo, unsigned size);
void command_push(GeomCommand *cmd);
void command_undo();
void command_redo();

#endif //GGB_COMMAND_H