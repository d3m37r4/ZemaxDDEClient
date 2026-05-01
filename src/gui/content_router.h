#pragma once

#include "gui/forwards.h"

class GuiManager;

namespace gui {
    /**
     * @brief Routes content rendering for different GUI pages.
     */
    class ContentRouter {
    public:
        void renderContent(GuiPage currentPage, GuiManager* gui);
    };
}
