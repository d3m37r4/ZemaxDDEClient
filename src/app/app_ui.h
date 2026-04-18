#pragma once

#include <memory>

class SagAnalysisController;
namespace gui {
    class GuiManager;
}

namespace app {
    class AppUI {
    public:
    explicit AppUI(gui::GuiManager& gui, SagAnalysisController& sag);
        void initialize();
        void render();
        void updateDpiStyle(float dpiScale);
        [[nodiscard]] bool shouldClose() const noexcept;
    private:
        gui::GuiManager& m_gui;
    };
}
