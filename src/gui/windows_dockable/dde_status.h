#pragma once

#include <memory>

#include "gui/constants.h"
#include "gui/popups/connect_dde.h"
#include "dde/dde_connection_manager.h"

namespace gui {
    class DDEStatus {
    public:
        explicit DDEStatus(DDEConnectionManager* connectionManager)
            : m_connectionManager(connectionManager)
            , m_connectPopup(std::make_unique<ConnectDDEPopup>(connectionManager)) {}

        void render(Logger& logger);

    private:
        DDEConnectionManager* m_connectionManager;
        std::unique_ptr<ConnectDDEPopup> m_connectPopup;
        bool m_showConnectPopup = false;
    };
}
