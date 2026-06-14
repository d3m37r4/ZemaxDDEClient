#pragma once

#include "dde/dde_connection_manager.h"

class Logger;

namespace gui {
    class ConnectDDEPopup {
    public:
        explicit ConnectDDEPopup(DDEConnectionManager* connectionManager)
            : m_connectionManager(connectionManager) {}

        void open() noexcept;
        void close() noexcept;
        [[nodiscard]] bool isOpen() const noexcept { return m_open; }

        void setLogger(Logger* logger) noexcept { m_logger = logger; }

        void render();

    private:
        DDEConnectionManager* m_connectionManager;
        Logger* m_logger = nullptr;
        bool m_open = false;
        int m_selectedWindowIndex = -1;
    };
}
