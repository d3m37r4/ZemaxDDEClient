#pragma once

#include "app/services/operation_monitor_service.h"
#include "gui/imgui_utils.h"
#include "lib/imgui/imgui.h"

namespace gui {
    using app::models::TaskSource;

    class UiOperationMonitor : public app::services::OperationMonitorService {
    public:
        float computeStatusBarHeight() const;
        void renderGlobalStatusBar();

    private:
        int m_selectedTaskIndex{0};
    };
}
