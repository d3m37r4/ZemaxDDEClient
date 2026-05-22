#pragma once

#include "gui/constants.h"
#include "dde/dde_connection_manager.h"

namespace gui {
    class DDEStatus {
    public:
        explicit DDEStatus(DDEConnectionManager* connectionManager)
            : m_connectionManager(connectionManager) {}

        void render(Logger& logger);

    private:
        void renderConnectPopup(Logger& logger);
        void renderChangeTargetPopup(Logger& logger);

        DDEConnectionManager* m_connectionManager;
        bool m_showConnectPopup = false;
        int m_selectedTargetIndex = -1;
        int m_selectedWindowIndex = -1;
    };
}
