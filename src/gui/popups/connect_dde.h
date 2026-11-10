#pragma once

#include "dde/dde_connection_manager.h"

class Logger;
class ThemeManager;

namespace gui {
    class ConnectDDEPopup {
    public:
        explicit ConnectDDEPopup(DDEConnectionManager* connectionManager)
            : m_connectionManager(connectionManager) {}

        void open() noexcept;
        void close() noexcept;
        [[nodiscard]] bool isOpen() const noexcept { return m_open; }

        void setLogger(Logger* logger) noexcept { m_logger = logger; }
        void setThemeManager(const ThemeManager* tm) noexcept { m_themeManager = tm; }

        void render();

    private:
        DDEConnectionManager* m_connectionManager;
        const ThemeManager* m_themeManager = nullptr;
        Logger* m_logger = nullptr;
        bool m_open = false;
        /// Selected row is remembered by HWND: the list is re-enumerated every
        /// frame, so a row index would silently point at another window.
        HWND m_selectedHwnd = nullptr;
    };
}
