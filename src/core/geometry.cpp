#include "geometry.hpp"
#include "internal.hpp"
#include <cmath>

namespace geom {
static constexpr rl::Color selected_color = rl::RED;
static constexpr int object_arg_count[] = {0, 4, 5, 0, 3};

class IDManager {
public:
  void init(const int size_) {
    indices.resize(size_);
  }

  GeomId alloc() {
    if (size == indices.size()) indices.resize(size * 2);
    if (size == range) indices[size] = range++;
    return indices[size++];
  }

  void free(const GeomId &id) {
    for (int i = 0; i < size; i++) {
      if (indices[i] == id) {
        size--;
        indices[i] = indices[size];
        indices[size] = id;
        break;
      }
    }
  }

  void clear() {
    range = size = 0;
  }

private:
  int range = 0, size = 0;
  std::vector<GeomId> indices;
};

static struct {
  const Transform *xform = nullptr;

  IDManager manager;
  std::vector<Geometry> objects;

  struct {
    std::vector<Handle> points;
    std::vector<Handle> lines;
    std::vector<Handle> circles;
  } active;
} intl;

std::vector<Geometry> &Handle::objects = intl.objects;

void init() {
  static constexpr int init_size = 64;

  intl.manager.init(init_size);
  intl.objects.resize(init_size);
  graph::init(init_size * 2);
}

void cleanup() {
  intl.manager.clear();
  intl.objects.clear();
  intl.active.points.clear();
  intl.active.lines.clear();
  intl.active.circles.clear();
  graph::cleanup();
}

Handle new_object(const GeomType type, const GeomId *args, const GeomId define,
                  const GeomId soln_id) {
  const GeomId id = intl.manager.alloc();
  if (id >= intl.objects.size()) intl.objects.resize(id * 2);
  intl.objects[id].init(id, type, args, define, soln_id);
  return Handle(id);
}

void draw_all() {
  for (const Handle handle : intl.active.circles) handle->draw();
  for (const Handle handle : intl.active.lines) handle->draw();
  for (const Handle handle : intl.active.points) handle->draw();

  for (const Handle handle : intl.active.circles) handle->draw_name();
  for (const Handle handle : intl.active.lines) handle->draw_name();
  for (const Handle handle : intl.active.points) handle->draw_name();
}

void remove_all() {
  for (auto &obj : intl.objects) obj.remove();
}

void update_all() {
  for (auto &obj : intl.objects) obj.update();
}

Handle get_hovered_object(const Vec2 pos) {
  for (const Handle handle : intl.active.points) {
    if (handle->hovered(pos)) return handle;
  }
  for (const Handle handle : intl.active.lines) {
    if (handle->hovered(pos)) return handle;
  }
  for (const Handle handle : intl.active.circles) {
    if (handle->hovered(pos)) return handle;
  }
  return {};
}

void set_xform(const Transform &xform) {
  intl.xform = &xform;
}

Vec2 Transform::forth(const Vec2 &v) const {
  const auto [sx, sy] = v * scale;
  const Vec2 rotated = {
      rotate[0][0] * sx + rotate[0][1] * sy,
      rotate[1][0] * sx + rotate[1][1] * sy,
  };
  const Vec2 translated = rotated + translate;
  return translated;
}

Vec2 Transform::inv(const Vec2 &v) const {
  const auto [tx, ty] = v - translate;
  const float cross = rotate[0][0] * rotate[1][1] - rotate[0][1] * rotate[1][0];
  const Vec2 rotated = {
      (rotate[1][1] * tx - rotate[0][1] * ty) / cross,
      (-rotate[1][0] * tx + rotate[0][0] * ty) / cross};
  const Vec2 scaled = rotated / scale;
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

void Geometry::init(const GeomId id_, const GeomType type_, const GeomId *args,
                    const GeomId define, const GeomId soln_id) {
  type = type_;
  handle = Handle(id_);
  color = type_ == POINT ? rl::DARKBLUE : rl::GRAY;
  get_default_name(name, type_);

  data.define = define;
  data.soln_id = soln_id;

  if (define != -1) graph::ref_node(define);
  for (int i = 0; i < object_arg_count[type]; i++) {
    data.args[i] = args[i];
    graph::ref_node(args[i]);
  }

  version = 0;
  update();
}

void Geometry::remove() {
  if (!handle.valid()) return;

  deselect();
  deactivate();

  if (data.define != -1) graph::unref_node(data.define);
  for (int i = 0; i < object_arg_count[type]; i++) {
    graph::unref_node(data.args[i]);
  }

  intl.manager.free(handle.get_id());
  handle.reset();
}

void Geometry::activate() {
  if (active) return;
  active = true;

  switch (type) {
  case POINT:
    intl.active.points.push_back(handle);
    break;
  case LINE:
    intl.active.lines.push_back(handle);
    break;
  default:
    intl.active.circles.push_back(handle);
  }
}

static void active_pop(std::vector<Handle> &active_, const Handle id) {
  for (auto &id_ : active_) {
    if (id_ == id) {
      id_ = active_.back();
      active_.pop_back();
      return;
    }
  }
}

void Geometry::deactivate() {
  if (!active) return;
  active = false;

  switch (type) {
  case POINT:
    active_pop(intl.active.points, handle);
    break;
  case LINE:
    active_pop(intl.active.lines, handle);
    break;
  default:
    active_pop(intl.active.circles, handle);
  }
}

void Geometry::update() {
  if (!handle.valid()) return;

  static constexpr int check_args[] = {0, 2, 5, 0, 3};
  const unsigned lastest = graph::get_version(check_args[type], data.args);
  if (version == lastest) return;
  version = lastest;

  valid = data.define == -1 || !graph::is_degenerate(data.define, data.soln_id);
  if (!valid) return;

  switch (type) {
  case POINT: {
    Vec2 pos;
    valid = graph::get_values(2, data.args, reinterpret_cast<float *>(&pos));
    if (!valid) return;

    pos = intl.xform->forth(pos);
    render.pt = pos;
    name_pos = pos + 4.f;
    break;
  }
  case LINE: {
    float args[5];
    valid = graph::get_values(5, data.args, args);
    if (!valid) return;

    const float nx = args[0], ny = args[1], dd = args[2];
    const float t1 = args[3], t2 = args[4];
    Vec2 p = {nx * dd + ny * t1, ny * dd - nx * t1};
    Vec2 q = {nx * dd + ny * t2, ny * dd - nx * t2};
    p = intl.xform->forth(p);
    q = intl.xform->forth(q);

    render.ln.point1 = p;
    render.ln.point2 = q;
    name_pos = (p + q) / 2.f + 2.f;
    break;
  }
  default: {
    float args[3];
    valid = graph::get_values(3, data.args, args);
    if (!valid) return;

    const Vec2 center = intl.xform->forth({args[0], args[1]});
    const float radius = intl.xform->forth(args[2]);

    render.cr.center = center;
    render.cr.radius = radius;
    name_pos = center + (radius / 1.414f + 2.f);
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
    return rl::check_collision_point_circle(pos, render.pt, 10);
  case LINE:
    return rl::check_collision_point_line(pos, render.ln.point1, render.ln.point2, 6);
  default:
    return fabsf(dist(pos, render.cr.center) - render.cr.radius) <= 6;
  }
}

void Geometry::draw() const {
  if (!visible()) return;

  switch (type) {
  case POINT: {
    rl::draw_circle_v(render.pt, 5, color);
    if (selected) {
      rl::draw_circle_lines_v(render.pt, 5.4f, selected_color);
    }
    break;
  }
  case LINE: {
    const Vec2 point1 = render.ln.point1;
    const Vec2 point2 = render.ln.point2;
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
    const auto &[center, radius] = render.cr;
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