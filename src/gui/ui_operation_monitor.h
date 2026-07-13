#pragma once

#include "app/services/operation_monitor_service.h"

namespace gui {
    using app::models::TaskSource;

    class UiOperationMonitor : public app::services::OperationMonitorService {
    public:
        void renderGlobalStatusBar();
    };
}
