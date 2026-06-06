#ifndef GGB_GEOMETRY_HPP
#define GGB_GEOMETRY_HPP

#include "geometry.h"
#include "raylib.hpp"
#include <string>

namespace app {
using Vec2 = rl::Vector2;
using Color = rl::Color;

class Transform {
public:
  Vec2 operator()(const float pt[2]) const;

  float operator()(const float v) const { return v * scale; }

  Vec2 inv(const Vec2 &pt) const;

private:
  float scale = 1.f;
  float translate[2] = {0.f, 0.f};
  float rotate[2][2] = {{1.f, 0.f}, {0.f, 1.f}};
};

class Geometry {
public:
  bool activated = false;
  bool valid = false;
  bool selected = false;
  GeomType type;
  std::string name;

  void init(GeomId cid, const Transform &xform);

  void update(GeomId cid, const Transform &xform);

  bool hovered(Vec2 pos) const;

  void draw() const;

  void draw_name() const;

  void activate() { activated = true; };

  void deactivate() { activated = false; };

  void select() { selected = true; }

  void deselect() { selected = false; }

  bool visible() const { return activated && valid; }

private:
  using Point = Vec2;

  struct Line {
    Point point1, point2;
  };

  struct Circle {
    Point center;
    float radius;
  };

  unsigned version = 0;
  Color color{};
  Vec2 name_pos{};

  union {
    Point pt;
    Line ln;
    Circle cr;
  } geom{};
};

}

#endif //GGB_GEOMETRY_HPP