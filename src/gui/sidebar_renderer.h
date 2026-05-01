#pragma once
#include <functional>
#include "gui/forwards.h"

namespace ZemaxDDE { class ZemaxDDEClient; }
class Logger;

namespace gui {
    class SidebarRenderer {
    public:
        void renderSidebar(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger);
        void renderDDEStatusFrame(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger);
        void setPageSwitcher(std::function<void(gui::GuiPage)> cb);
    private:
        std::function<void(gui::GuiPage)> m_onPageSwitch;
    };
}
