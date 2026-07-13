#pragma once

#include "app/services/operation_monitor.h"

namespace gui {
    using app::models::TaskSource;

    class UiOperationMonitor : public app::services::OperationMonitor {
    public:
        void renderGlobalStatusBar();
    };
}
