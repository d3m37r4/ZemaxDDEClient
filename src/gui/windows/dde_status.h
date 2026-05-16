#pragma once

#include <functional>

#include "gui/constants.h"
#include "logger/logger.h"
#include "dde/client.h"

class ZemaxDDEClient;

namespace gui {
    class DDEStatus {
    public:
        DDEStatus(ZemaxDDE::ZemaxDDEClient* ddeClient) : m_ddeClient(ddeClient) {}
        void render(Logger& logger);
    private:
        ZemaxDDE::ZemaxDDEClient* m_ddeClient;
    };
}
