#ifndef GGB_GEOMETRY_HPP
#define GGB_GEOMETRY_HPP

#include "types.h"
#include "raylib.hpp"

#include <array>
#include <string>
#include <vector>

using Vec2 = rl::Vector2;

namespace geom {
static Vec2 operator+(const Vec2 &lhs, const Vec2 &rhs) {
  return {lhs.x + rhs.x, lhs.y + rhs.y};
}

static Vec2 operator+(const Vec2 &lhs, const float rhs) {
  return {lhs.x + rhs, lhs.y + rhs};
}

static Vec2 operator-(const Vec2 &lhs, const Vec2 &rhs) {
  return {lhs.x - rhs.x, lhs.y - rhs.y};
}

static Vec2 operator-(const Vec2 &lhs, const float rhs) {
  return {lhs.x - rhs, lhs.y - rhs};
}

static Vec2 operator*(const Vec2 &lhs, const float rhs) {
  return {lhs.x * rhs, lhs.y * rhs};
}

static Vec2 operator/(const Vec2 &lhs, const float rhs) {
  return {lhs.x / rhs, lhs.y / rhs};
}

class Transform {
public:
  Vec2 forth(const Vec2 &v) const;
  float forth(const float v) const { return v * scale; }
  Vec2 inv(const Vec2 &v) const;

  Vec2 operator()(const Vec2 &v) const { return forth(v); }
  float operator()(const float v) const { return forth(v); }

private:
  float scale = 1.f;
  float rotate[2][2] = {{1.f, 0.f}, {0.f, 1.f}};
  Vec2 translate = {0.f, 0.f};
};

class Geometry;

class Handle {
public:
  Handle() = default;

  explicit Handle(const GeomId id) : id(id) {
  }

  GeomId get_id() const { return id; }
  bool valid() const { return id != -1; }
  void reset() { id = -1; }

  Geometry *operator->() const { return &objects[id]; }
  bool operator==(const Handle &rhs) const { return id == rhs.id; }

private:
  static std::vector<Geometry> &objects;
  GeomId id = -1;
};

class Geometry {
public:
  struct Data {
    GeomId define;
    GeomId soln_id;
    GeomId args[5];
  };

  bool active = false;
  bool valid = false;
  bool selected = false;
  Handle handle;
  GeomType type;
  std::string name;
  rl::Color color{};
  Data data{};

  void init(GeomId id_, GeomType type_, const GeomId *args, GeomId define, GeomId soln_id);

  void remove();

  void update();

  bool hovered(Vec2 pos) const;

  void draw() const;

  void draw_name() const;

  bool visible() const { return active && valid; }

  void activate();

  void deactivate();

  void select() { selected = true; }

  void deselect() { selected = false; }

private:
  using Point = Vec2;

  union Render {
    Point pt;

    struct Line {
      Point point1, point2;
    } ln;

    struct Circle {
      Point center;
      float radius;
    } cr;
  };

  unsigned version = 0;
  Render render{};
  Point name_pos{};
};

void init();
void cleanup();
void draw_all();
void remove_all();
void update_all();
void set_xform(const Transform &xform);
Handle get_hovered_object(Vec2 pos);
Handle new_object(GeomType type, const GeomId *args, GeomId define = -1, GeomId soln_id = 0);

Handle new_point(float x, float y, Handle on = Handle());
Handle new_line(Handle pt1, Handle pt2);
Handle new_circle(Handle center, Handle pt);
Handle midpoint(Handle pt1, Handle pt2);
Handle parallel(Handle ln, Handle pt);
Handle perpendicular(Handle ln, Handle pt);
std::array<Handle, 2> angle_bisector(Handle ln1, Handle ln2);
std::array<Handle, 4> tangent(Handle cr, Handle cr_or_pt);
Handle circumcircle(Handle pt1, Handle pt2, Handle pt3);
std::array<Handle, 2> intersection(Handle ln_or_cr1, Handle ln_or_cr2);

void move(Handle pt, Vec2 to);
}

#endif //GGB_GEOMETRY_HPP