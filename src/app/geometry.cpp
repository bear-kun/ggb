#include "geometry.hpp"
#include <cmath>

namespace app {

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

Vec2 Transform::operator()(const float pt[2]) const {
  const Vec2 scaled = {pt[0] * scale, pt[1] * scale};
  const Vec2 rotated = {
      rotate[0][0] * scaled.x + rotate[0][1] * scaled.y,
      rotate[1][0] * scaled.x + rotate[1][1] * scaled.y,
  };
  const Vec2 translated = {rotated.x + translate[0],
                           rotated.y + translate[1]};
  return translated;
}

Vec2 Transform::inv(const Vec2 &pt) const {
  const Vec2 translated = {pt.x - translate[0],
                           pt.y - translate[1]};
  const float cross = rotate[0][0] * rotate[1][1] - rotate[0][1] * rotate[1][0];
  const Vec2 rotated = {
      (rotate[1][1] * translated.x - rotate[0][1] * translated.y) / cross,
      (-rotate[1][0] * translated.x + rotate[0][0] * translated.y) / cross};
  const Vec2 scaled = {rotated.x / scale,
                       rotated.y / scale};
  return scaled;
}


static void u2str(char *str, unsigned x) {
  str += x / 10;
  while (x) {
    *str++ = static_cast<char>('0' + x % 10);
    x /= 10;
  }
}

static void get_default_name(std::string &name, const GeomType type) {
  static unsigned point = 0, line = 0, circle = 0;
  name.assign(8, 0);
  switch (type) {
  case POINT:
    name[0] = static_cast<char>('A' + point % 26);
    u2str(name.data() + 1, point / 26);
    point++;
    return;
  case LINE:
    name[0] = static_cast<char>('a' + line % 26);
    u2str(name.data() + 1, line / 26);
    if (line++ % 26 == 'c' - 'a' - 1) line++;
    return;
  default:
    name[0] = 'c';
    u2str(name.data() + 1, circle++);
  }
}

void Geometry::init(const GeomId cid, const Transform &xform) {
  version = 0;
  type = geom_get_type(cid);
  if (type == POINT) {
    color = {0, 82, 172, 255};
  } else {
    color = {130, 130, 130, 255};
  }

  get_default_name(name, type);
  update(cid, xform);
}

void Geometry::update(const GeomId cid, const Transform &xform) {
  const unsigned g_version = geom_get_version(cid);
  if (version == g_version) return;
  version = g_version;

  switch (geom_get_type(cid)) {
  case POINT: {
    float xy[2];
    valid = geom_get_point(cid, xy);
    if (!valid) return;

    geom.pt = xform(xy);
    name_pos = geom.pt + 4.f;
    break;
  }
  case LINE: {
    float pt1[2], pt2[2];
    valid = geom_get_line(cid, pt1, pt2);
    if (!valid) return;

    geom.ln.point1 = xform(pt1);
    geom.ln.point2 = xform(pt2);
    name_pos = (geom.ln.point1 + geom.ln.point2) / 2.f + 2.f;
    break;
  }
  default: {
    float center[2], radius;
    valid = geom_get_circle(cid, center, &radius);
    if (!valid) return;

    geom.cr.center = xform(center);
    geom.cr.radius = xform(radius);
    name_pos = geom.cr.center + (geom.cr.radius / 1.414f + 2.f);
  }
  }
}

static float dist(const Vec2 &p, const Vec2 &q) {
  const float dx = p.x - q.x;
  const float dy = p.y - q.y;
  return sqrtf(dx * dx + dy * dy);
}

bool Geometry::hovered(const Vec2 pos) const {
  if (!visible()) return false;

  switch (type) {
  case POINT:
    return rl::check_collision_point_circle(pos, geom.pt, 10);
  case LINE:
    return rl::check_collision_point_line(pos, geom.ln.point1, geom.ln.point2, 6);
  default:
    return fabsf(dist(pos, geom.cr.center) - geom.cr.radius) <= 6;
  }
}

static constexpr Color selected_color = {230, 41, 55, 255};

void Geometry::draw() const {
  if (!visible()) return;

  switch (type) {
  case POINT: {
    rl::draw_circle_v(geom.pt, 5, color);
    if (selected) {
      rl::draw_circle_lines_v(geom.pt, 5.4f, selected_color);
    }
    break;
  }
  case LINE: {
    const auto &[point1, point2] = geom.ln;
    rl::draw_line_ex(point1, point2, 4, color);
    if (selected) {
      const auto [vx, vy] = point1 - point2;
      const float norm = sqrtf(vx * vx + vy * vy);
      const Vec2 vp = {vy / norm * 2.3f, -vx / norm * 2.3f};
      rl::draw_line_v(point1 + vp, point2 + vp, selected_color);
      rl::draw_line_v(point1 - vp, point2 - vp, selected_color);
    }
    break;
  }
  default: {
    const auto &[center, radius] = geom.cr;
    rl::draw_ring(center, radius - 2, radius + 2, 0, 360, 36, color);
    if (selected) {
      rl::draw_circle_lines_v(center, radius - 2.4f, selected_color);
      rl::draw_circle_lines_v(center, radius + 2.4f, selected_color);
    }
    break;
  }
  }
}

void Geometry::draw_name() const {
  static const rl::Font font = rl::get_font_default();
  if (!visible()) return;
  rl::draw_text_ex(font, name.c_str(), name_pos, 20, 1, selected ? selected_color : color);
}

}