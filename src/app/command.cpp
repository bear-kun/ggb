#include "command.hpp"
#include "board.hpp"
#include <vector>

namespace app::command {
static constexpr int STACK_CAPACITY = 32;

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

void push(std::unique_ptr<Command> &&cmd) {
  if (manager.top == STACK_CAPACITY) {
    manager.stack[0].reset();

    for (int i = 1; i < STACK_CAPACITY; i++) {
      manager.stack[i - 1] = std::move(manager.stack[i]);
    }

    manager.stack[STACK_CAPACITY - 1] = std::move(cmd);
    return;
  }

  for (int i = manager.top; i < manager.size; i++) manager.stack[i].reset();

  manager.stack[manager.top] = std::move(cmd);
  manager.size = ++manager.top;
}

void undo() {
  if (manager.top == 0) return;
  manager.stack[--manager.top]->undo();
}

void redo() {
  if (manager.top == manager.size) return;
  manager.stack[manager.top++]->redo();
}

Add::Add(const int count, const geom::Handle handles[]) {
  this->count = count;
  this->handles = std::make_unique<geom::Handle[]>(count);
  for (int i = 0; i < count; i++) this->handles[i] = handles[i];
  redo();
}

void Add::redo() {
  for (int i = 0; i < count; i++) handles[i]->activate();
  remove = false;
}

void Add::undo() {
  for (int i = 0; i < count; i++) handles[i]->deactivate();
  remove = true;
}

Add::~Add() {
  if (remove) {
    for (int i = 0; i < count; i++) handles[i]->remove();
  }
}

Delete::Delete(const int count, const geom::Handle handles[]) {
  this->count = count;
  this->handles = std::make_unique<geom::Handle[]>(count);
  for (int i = 0; i < count; i++) this->handles[i] = handles[i];
  redo();
}

void Delete::redo() {
  for (int i = 0; i < count; i++) handles[i]->deactivate();
  remove = true;
}

void Delete::undo() {
  for (int i = 0; i < count; i++) handles[i]->activate();
  remove = false;
}

Delete::~Delete() {
  if (remove) {
    for (int i = 0; i < count; i++) handles[i]->remove();
  }
}

Move::Move(const geom::Handle point, const Vec2 from, const Vec2 to) : point(point), from(from),
  to(to) {
  redo();
}

void Move::redo() {
  geom::move(point, to);
  geom::update_all();
}

void Move::undo() {
  geom::move(point, from);
  geom::update_all();
}

}