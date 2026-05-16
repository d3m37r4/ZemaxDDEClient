#pragma once

#include "window_manager.h"

namespace gui {
    class GuiManager;
}

void RegisterAllWindows(WindowManager& mgr, gui::GuiManager* guiMgr);