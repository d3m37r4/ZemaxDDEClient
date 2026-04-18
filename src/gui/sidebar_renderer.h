#pragma once
#include <functional>
#include "gui/forwards.h"

namespace ZemaxDDE { class ZemaxDDEClient; }

namespace gui {
    class SidebarRenderer {
    public:
        void renderSidebar(ZemaxDDE::ZemaxDDEClient* ddeClient);
        void renderDDEStatusFrame(ZemaxDDE::ZemaxDDEClient* ddeClient);
        void setPageSwitcher(std::function<void(gui::GuiPage)> cb);
    private:
        std::function<void(gui::GuiPage)> m_onPageSwitch;
    };
}
