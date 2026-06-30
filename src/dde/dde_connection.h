#pragma once

#include <memory>
#include <string>
#include <windows.h>

#include "types.h"

namespace ZemaxDDE {
    class ZemaxDDEClient;
}

struct DDEConnection {
    HWND hwndClient = nullptr;
    HWND hwndServer = nullptr;
    std::wstring serverTitle;
    DWORD serverPid = 0;
    std::unique_ptr<ZemaxDDE::ZemaxDDEClient> client;
    ZemaxDDE::ConnectionState connectionState = ZemaxDDE::ConnectionState::Disconnected;

    [[nodiscard]] bool isConnected() const noexcept {
        return connectionState == ZemaxDDE::ConnectionState::Connected;
    }

    [[nodiscard]] bool isDisconnected() const noexcept {
        return connectionState == ZemaxDDE::ConnectionState::Disconnected;
    }
};
