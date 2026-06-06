#pragma once

#include <memory>

#include "gui/constants.h"
#include "gui/popups/connect_dde.h"
#include "dde/dde_connection_manager.h"

namespace gui {
    class ThemeManager;

    class DDEStatus {
    public:
        explicit DDEStatus(DDEConnectionManager* connectionManager)
            : m_connectionManager(connectionManager)
            , m_connectPopup(std::make_unique<ConnectDDEPopup>(connectionManager)) {}

        void render(Logger& logger);

        /// Non-owning; bound by GuiManager after graphics.initialize().
        void setThemeManager(const ThemeManager* themeManager) noexcept { m_themeManager = themeManager; }

    private:
        DDEConnectionManager* m_connectionManager;
        const ThemeManager* m_themeManager = nullptr;
        std::unique_ptr<ConnectDDEPopup> m_connectPopup;
        bool m_showConnectPopup = false;
    };
}
