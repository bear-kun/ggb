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
  Add(GeomSize count, const GeomId indices[]);
  ~Add() override;
  void redo() override;
  void undo() override;

private:
  bool remove = false;
  GeomSize count;
  std::unique_ptr<GeomId[]> indices;
};

class Delete final : public Command {
public:
  Delete(GeomSize count, const GeomId indices[]);
  ~Delete() override;
  void redo() override;
  void undo() override;

private:
  bool remove = true;
  GeomSize count;
  std::unique_ptr<GeomId[]> indices;
};

class Move final : public Command {
public:
  Move(GeomId point, Vec2 from, Vec2 to);
  void redo() override;
  void undo() override;

private:
  GeomId point;
  Vec2 from, to;
};

void init();
void cleanup();

void push(std::unique_ptr<Command> &&cmd);
void undo();
void redo();
}

#endif //GGB_COMMAND_H