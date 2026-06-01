extern "C" {
#include "raylib.h"
}
#include "raylib.hpp"

namespace rl {
void init_window(int width, int height, const char *title) { InitWindow(width, height, title); }
void close_window() { CloseWindow(); }
bool window_should_close() { return WindowShouldClose(); }

void clear_background(Color color) { ClearBackground(color); }
void begin_drawing() { BeginDrawing(); }
void end_drawing() { EndDrawing(); }

void set_target_fps(int fps) { SetTargetFPS(fps); }
float get_frame_time() { return GetFrameTime(); }

bool is_key_pressed(int key) { return IsKeyPressed(key); }
bool is_key_down(int key) { return IsKeyDown(key); }
int get_char_pressed() { return GetCharPressed(); }
void set_exit_key(int key) { SetExitKey(key); }

bool is_mouse_button_pressed(int button) { return IsMouseButtonPressed(button); }
bool is_mouse_button_down(int button) { return IsMouseButtonDown(button); }
bool is_mouse_button_released(int button) { return IsMouseButtonReleased(button); }
bool is_mouse_button_up(int button) { return IsMouseButtonUp(button); }
Vector2 get_mouse_position() { return GetMousePosition(); }
Vector2 get_mouse_delta() { return GetMouseDelta(); }

void set_mouse_cursor(int cursor) { SetMouseCursor(cursor); }

void draw_line_v(Vector2 start, Vector2 end, Color color) { DrawLineV(start, end, color); }

void draw_line_ex(Vector2 start, Vector2 end, float thick, Color color) {
  DrawLineEx(start, end, thick, color);
}

void draw_circle_v(Vector2 center, float radius, Color color) {
  DrawCircleV(center, radius, color);
}

void draw_circle_lines_v(Vector2 center, float radius, Color color) {
  DrawCircleLinesV(center, radius, color);
}

void draw_ring(Vector2 center, float inner_radius, float outer_radius, float start_angle,
               float end_angle, int segments, Color color) {
  DrawRing(center, inner_radius, outer_radius, start_angle, end_angle, segments, color);
}

void draw_ring_lines(Vector2 center, float inner_radius, float outer_radius, float start_angle,
                     float end_angle, int segments, Color color) {
  DrawRingLines(center, inner_radius, outer_radius, start_angle, end_angle, segments, color);
}

void draw_rectangle(int x, int y, int width, int height, Color color) {
  DrawRectangle(x, y, width, height, color);
}

void draw_rectangle_v(Vector2 pos, Vector2 size, Color color) {
  DrawRectangleV(pos, size, color);
}

void draw_rectangle_rec(Rectangle rec, Color color) { DrawRectangleRec(rec, color); }

bool check_collision_point_rec(Vector2 point, Rectangle rec) {
  return CheckCollisionPointRec(point, rec);
}

bool check_collision_point_circle(Vector2 point, Vector2 center, float radius) {
  return CheckCollisionPointCircle(point, center, radius);
}

bool check_collision_point_line(Vector2 point, Vector2 p1, Vector2 p2, int threshold) {
  return CheckCollisionPointLine(point, p1, p2, threshold);
}

Font get_font_default() { return GetFontDefault(); }

void draw_text(const char *text, int x, int y, int size, Color color) {
  DrawText(text, x, y, size, color);
}

void draw_text_ex(Font font, const char *text, Vector2 pos, float size, float spacing,
                  Color color) { DrawTextEx(font, text, pos, size, spacing, color); }
}