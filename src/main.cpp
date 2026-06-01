#include "board.hpp"
#include "toolbar.hpp"

using namespace app;

constexpr int WINDOW_WIDTH = 1600;
constexpr int WINDOW_HEIGHT = 1000;

int main() {
  rl::init_window(WINDOW_WIDTH, WINDOW_HEIGHT, "ggb");
  rl::set_target_fps(60);
  rl::set_exit_key(0);

  toolbar::init(0, 0, WINDOW_WIDTH, 80);
  board::init(0, 80, WINDOW_WIDTH, WINDOW_HEIGHT - 80);

  while (!rl::window_should_close()) {
    board::listen();
    toolbar::listen();

    rl::begin_drawing();
    rl::clear_background(WHITE);
    board::draw();
    toolbar::draw();
    rl::end_drawing();
  }

  toolbar::cleanup();
  board::cleanup();
  rl::close_window();
  return 0;
}