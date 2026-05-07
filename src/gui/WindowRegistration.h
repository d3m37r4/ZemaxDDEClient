#pragma once

#include "WindowManager.h"

namespace gui {
class GuiManager;
}

void RegisterAllWindows(WindowManager& mgr, gui::GuiManager* guiMgr);