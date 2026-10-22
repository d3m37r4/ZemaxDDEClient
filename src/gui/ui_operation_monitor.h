#pragma once

#include <functional>

#include "app/services/operation_monitor_service.h"
#include "gui/imgui_utils.h"
#include "lib/imgui/imgui.h"

// ThemeManager and DDEConnectionManager live in the global namespace
// (see gui/theme_manager.h and dde/dde_connection_manager.h).
class ThemeManager;
class DDEConnectionManager;

namespace gui {
    using app::models::TaskSource;

    class UiOperationMonitor : public app::services::OperationMonitorService {
    public:
        float computeStatusBarHeight() const;
        void renderGlobalStatusBar();

        void setThemeManager(const ThemeManager* tm) noexcept { m_themeManager = tm; }
        void setConnectionManager(const DDEConnectionManager* cm) noexcept { m_connectionManager = cm; }
        /// Cancel for the selected task: GuiManager dispatches by TaskSource
        /// to the owning service (profile - cancelCalculation(source), map -
        /// cancelMapCalculation()). An empty callback hides the Cancel button.
        void setOnCancelTask(std::function<void(TaskSource)> cb) { m_onCancelTask = std::move(cb); }

    private:
        int m_selectedTaskIndex{0};
        const ThemeManager* m_themeManager = nullptr;
        const DDEConnectionManager* m_connectionManager = nullptr;
        std::function<void(TaskSource)> m_onCancelTask;
    };
}
