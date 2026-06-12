#ifndef GGB_COMMAND_H
#define GGB_COMMAND_H

#include "geometry.hpp"
#include <memory>

namespace app::command {
class Command {
public:
  virtual ~Command() = default;
  virtual void redo() = 0;
  virtual void undo() = 0;
};

class Add final : public Command {
public:
  Add(int count, const geom::Handle handles[]);
  ~Add() override;
  void redo() override;
  void undo() override;

private:
  bool remove = false;
  int count;
  std::unique_ptr<geom::Handle[]> handles;
};

class Delete final : public Command {
public:
  Delete(int count, const geom::Handle handles[]);
  ~Delete() override;
  void redo() override;
  void undo() override;

private:
  bool remove = true;
  int count;
  std::unique_ptr<geom::Handle[]> handles;
};

class Move final : public Command {
public:
  Move(geom::Handle point, Vec2 from, Vec2 to);
  void redo() override;
  void undo() override;

private:
  geom::Handle point;
  Vec2 from, to;
};

void init();
void cleanup();

void push(std::unique_ptr<Command> &&cmd);
void undo();
void redo();
}

#endif //GGB_COMMAND_H