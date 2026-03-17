#include "raylib_.h"
#include "board.h"
#include "toolbar.h"

const int WINDOW_WIDTH = 1600;
const int WINDOW_HEIGHT = 1000;

int main() {
  rl_init_window(WINDOW_WIDTH, WINDOW_HEIGHT, "ggb");
  rl_set_target_fps(60);
  toolbar_init(0, 0, WINDOW_WIDTH, 80);
  board_init(0, 80, WINDOW_WIDTH, WINDOW_HEIGHT - 80);

  while (!rl_window_should_close()) {
    board_listen();
    toolbar_listen();

    rl_begin_drawing();
    rl_clear_background(WHITE);
    board_draw();
    toolbar_draw();
    rl_end_drawing();
  }

  toolbar_cleanup();
  board_cleanup();
  rl_close_window();
  return 0;
}