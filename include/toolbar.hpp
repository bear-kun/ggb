#ifndef GGB_TOOLBAR_H
#define GGB_TOOLBAR_H


#include "board.hpp"
#include <memory>

namespace app::toolbar {
class GeomTool : public board::Control {
public:
  std::string usage;

  virtual void reset() {
  }
};

void init(int x, int y, int w, int h);
void listen();
void draw();
void cleanup();

using ToolPtr = std::unique_ptr<GeomTool>;
ToolPtr angle_bisector();
ToolPtr circle();
ToolPtr circumcircle();
ToolPtr delete_();
ToolPtr intersection();
ToolPtr line();
ToolPtr midpoint();
ToolPtr move();
ToolPtr parallel();
ToolPtr perpendicular();
ToolPtr point();
ToolPtr tangent();
}

#endif // GGB_TOOLBAR_H