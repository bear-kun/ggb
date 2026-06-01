#include "command.hpp"
#include "board.hpp"
#include <vector>

namespace app::command {
static constexpr int STACK_CAPACITY = 16;

static struct {
  int top = 0, size = 0;
  std::vector<std::unique_ptr<Command>> stack;
} manager;

void init() {
  manager.stack.resize(STACK_CAPACITY);
}

void cleanup() {
  manager.stack.clear();
}

void push(std::unique_ptr<Command> &&cmd, const bool first_do) {
  if (manager.top == STACK_CAPACITY) {
    manager.stack[0].reset();

    for (int i = 1; i < STACK_CAPACITY; i++) {
      manager.stack[i - 1] = std::move(manager.stack[i]);
    }

    manager.stack[STACK_CAPACITY - 1] = std::move(cmd);
  } else {
    for (int i = manager.top; i < manager.size; i++) manager.stack[i].reset();

    manager.stack[manager.top] = std::move(cmd);
    manager.size = ++manager.top;
  }

  if (first_do) manager.stack[manager.top - 1]->redo();
}

void undo() {
  if (manager.top == 0) return;
  manager.stack[--manager.top]->undo();
}

void redo() {
  if (manager.top == manager.size) return;
  manager.stack[manager.top++]->redo();
}

Add::Add(const GeomSize count, const GeomId indices[]) {
  this->count = count;
  this->indices = std::make_unique<GeomId[]>(count);
  for (GeomSize i = 0; i < count; i++) this->indices[i] = indices[i];
}

void Add::redo() {
  for (GeomSize i = 0; i < count; i++) board::add_object(indices[i]);
  remove = false;
}

void Add::undo() {
  for (GeomSize i = 0; i < count; i++) board::remove_object(indices[i]);
  remove = true;
}

Add::~Add() {
  if (remove) {
    for (GeomSize i = 0; i < count; i++) geom_delete_object(indices[i]);
  }
}

Delete::Delete(const GeomSize count, const GeomId indices[]) {
  this->count = count;
  this->indices = std::make_unique<GeomId[]>(count);
  for (GeomSize i = 0; i < count; i++) this->indices[i] = indices[i];
}

void Delete::redo() {
  for (GeomSize i = 0; i < count; i++) board::remove_object(indices[i]);
  remove = true;
}

void Delete::undo() {
  for (GeomSize i = 0; i < count; i++) board::add_object(indices[i]);
  remove = false;
}

Delete::~Delete() {
  if (remove) {
    for (GeomSize i = 0; i < count; i++) geom_delete_object(indices[i]);
  }
}

void Move::redo() {
  geom_move(point, reinterpret_cast<float *>(&to));
  board::update_objects();
}

void Move::undo() {
  geom_move(point, reinterpret_cast<float *>(&from));
  board::update_objects();
}

}