#pragma once

#include <functional>

#include "gui/constants.h"
#include "logger/logger.h"
#include "dde/client.h"

class ZemaxDDEClient;

namespace gui {
    class DDEStatusRenderer {
    public:
        void renderDDEStatus(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger);
        void renderDDEStatusFrame(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger);
    };
}
