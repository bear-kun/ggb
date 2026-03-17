#include "raylib.h"

void rl_init_window(int width, int height, const char *title) { InitWindow(width, height, title); }
void rl_close_window() { CloseWindow(); }
bool rl_window_should_close() { return WindowShouldClose(); }

void rl_clear_background(Color color) { ClearBackground(color); }
void rl_begin_drawing() { BeginDrawing(); }
void rl_end_drawing() { EndDrawing(); }

void rl_set_target_fps(int fps) { SetTargetFPS(fps); }
float rl_get_frame_time() { return GetFrameTime(); }

bool rl_is_key_pressed(int key) { return IsKeyPressed(key); }
int rl_get_char_pressed() { return GetCharPressed(); }
void rl_set_exit_key(int key) { SetExitKey(key); }

bool rl_is_mouse_button_pressed(int button) { return IsMouseButtonPressed(button); }
bool rl_is_mouse_button_down(int button) { return IsMouseButtonDown(button); }
bool rl_is_mouse_button_released(int button) { return IsMouseButtonReleased(button); }
bool rl_is_mouse_button_up(int button) { return IsMouseButtonUp(button); }
Vector2 rl_get_mouse_position() { return GetMousePosition(); }
Vector2 rl_get_mouse_delta() { return GetMouseDelta(); }

void rl_set_mouse_cursor(int cursor) { SetMouseCursor(cursor); }

void rl_draw_line_v(Vector2 start, Vector2 end, Color color) { DrawLineV(start, end, color); }
void rl_draw_line_ex(Vector2 start, Vector2 end, float thick, Color color) { DrawLineEx(start, end, thick, color); }
void rl_draw_circle_v(Vector2 center, float radius, Color color) { DrawCircleV(center, radius, color); }
void rl_draw_circle_lines_v(Vector2 center, float radius, Color color) {DrawCircleLinesV(center, radius, color); }
void rl_draw_ring(Vector2 center, float inner_radius, float outer_radius, float start_angle, float end_angle, int segments, Color color) { DrawRing(center, inner_radius, outer_radius, start_angle, end_angle, segments, color); }
void rl_draw_ring_lines(Vector2 center, float inner_radius, float outer_radius, float start_angle, float end_angle, int segments, Color color) { DrawRingLines(center, inner_radius, outer_radius, start_angle, end_angle, segments, color); }
void rl_draw_rectangle(int x, int y, int width, int height, Color color) { DrawRectangle(x, y, width, height, color); }
void rl_draw_rectangle_v(Vector2 pos, Vector2 size, Color color) { DrawRectangleV(pos, size, color); }
void rl_draw_rectangle_rec(Rectangle rec, Color color) { DrawRectangleRec(rec, color); }

bool rl_check_collision_point_rec(Vector2 point, Rectangle rec) { return CheckCollisionPointRec(point, rec); }
bool rl_check_collision_point_circle(Vector2 point, Vector2 center, float radius) { return CheckCollisionPointCircle(point, center, radius); }
bool rl_check_collision_point_line(Vector2 point, Vector2 p1, Vector2 p2, int threshold) { return CheckCollisionPointLine(point, p1, p2, threshold); }

Font rl_get_font_default() { return GetFontDefault(); }
void rl_draw_text(const char *text, int x, int y, int size, Color color) { DrawText(text, x, y, size, color); }
void rl_draw_text_ex(Font font, const char *text, Vector2 pos, float size, float spacing, Color color) { DrawTextEx(font, text, pos, size, spacing, color); }