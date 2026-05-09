#pragma once

#include <functional>

#include "gui/constants.h"
#include "logger/logger.h"
#include "dde/client.h"

class ZemaxDDEClient;

namespace gui {
    class DdeStatusRenderer {
    public:
        void renderDdeStatus(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger);
        void renderDdeStatusFrame(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger);
    };
}
