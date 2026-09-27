#pragma once

#include <memory>

#include "gui/constants.h"
#include "gui/popups/connect_dde.h"
#include "gui/theme_manager.h"
#include "dde/dde_connection_manager.h"

namespace gui {
    class DDEStatus {
    public:
        explicit DDEStatus(DDEConnectionManager* connectionManager)
            : m_connectionManager(connectionManager)
            , m_connectPopup(std::make_unique<ConnectDDEPopup>(connectionManager)) {}

        void render(Logger& logger);

        /// Same action as the Connect/Disconnect button: opens the connect
        /// popup when disconnected, disconnects the active slot otherwise.
        void toggleConnection(Logger& logger);
        void openConnectPopup();
        void closeConnectPopup() noexcept;

        /// Non-owning; bound by GuiManager after graphics.initialize().
        void setThemeManager(const ThemeManager* themeManager) noexcept {
            m_themeManager = themeManager;
            m_connectPopup->setThemeManager(themeManager);
        }
        void setLogger(Logger* logger) noexcept { m_connectPopup->setLogger(logger); }

        [[nodiscard]] bool isConnectPopupOpen() const noexcept { return m_connectPopup->isOpen(); }

    private:
        DDEConnectionManager* m_connectionManager;
        const ThemeManager* m_themeManager = nullptr;
        std::unique_ptr<ConnectDDEPopup> m_connectPopup;
    };
}
