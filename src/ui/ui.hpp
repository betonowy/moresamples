#pragma once

struct Ctx;

#include <vector>

namespace nodes {
struct INode;
}

namespace ui {
void menuBar(Ctx &);
void nodeEditor(Ctx &);
void grid(Ctx &, float mouse_diff_x, float mouse_diff_y, bool grabbed, bool stop);
void bkgContextual(Ctx &);

namespace popups {
bool infiniteLoop(Ctx &, std::vector<nodes::INode *>);
}

namespace style {
void setup(Ctx &);

void pushMenuBar(Ctx &);
void popMenuBar(Ctx &);

void pushNodes(Ctx &, float margin);
void popNodes(Ctx &);
} // namespace style
} // namespace ui
