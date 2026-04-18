#include "app_ui.h"
#include "gui/sag_analysis_controller.h"
#include "gui/gui.h"

namespace app {
    AppUI::AppUI(gui::GuiManager& gui, SagAnalysisController& sag)
        : m_gui(gui) {
        (void)sag;
    }

    void AppUI::initialize() {
        m_gui.initialize();
    }

    void AppUI::render() {
        m_gui.render();
    }

    void AppUI::updateDpiStyle(float dpiScale) {
        m_gui.updateDpiStyle(dpiScale);
    }

    bool AppUI::shouldClose() const noexcept {
        return m_gui.shouldClose();
    }
}