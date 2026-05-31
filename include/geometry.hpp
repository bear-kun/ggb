#ifndef GGB_GEOMETRY_HPP
#define GGB_GEOMETRY_HPP

#include "geometry.h"
#include "raylib_.h"
#include <string>

namespace app {
using Vec2 = Vector2;

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
  GeomType type;
  Color color{};
  std::string name;

  void update(GeomId cid, const Transform &xform);

  bool select_try(Vec2 pos) const;

  void draw() const;

  void draw_name() const;

  void select() { selected = true; }

  void deselect() { selected = false; }

private:
  bool visible() const { return exist && valid; }

  using Point = Vec2;

  struct Line {
    Point point1, point2;
  };

  struct Circle {
    Point center;
    float radius;
  };

  bool exist = false;
  bool valid = false;
  bool selected = false;
  unsigned version = 0;
  Vec2 name_pos{};

  union {
    Point pt;
    Line ln;
    Circle cr;
  } geom{};
};

}

#endif //GGB_GEOMETRY_HPP