#pragma once

#include "WindowManager.hpp"

namespace gui {
class GuiManager;
}

void RegisterAllWindows(WindowManager& mgr, gui::GuiManager* guiMgr);