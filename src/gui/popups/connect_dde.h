#pragma once

#include "dde/dde_connection_manager.h"

namespace gui {
    class ConnectDDEPopup {
    public:
        explicit ConnectDDEPopup(DDEConnectionManager* connectionManager)
            : m_connectionManager(connectionManager) {}

        void render(bool& showFlag, Logger& logger);

    private:
        DDEConnectionManager* m_connectionManager;
        int m_selectedWindowIndex = -1;
    };
}
