#pragma once

#include "gui/forwards.h"

namespace gui {
    class GuiManager;

    class ContentRouter {
    public:
        void renderContent(GuiPage currentPage, GuiManager* gui);
    };
}
