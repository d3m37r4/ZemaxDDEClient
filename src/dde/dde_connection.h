#pragma once

#include <memory>
#include <string>
#include <windows.h>

namespace ZemaxDDE {
    class ZemaxDDEClient;
}

struct DDEConnection {
    HWND hwndClient = nullptr;
    HWND hwndServer = nullptr;
    std::wstring serverTitle;
    DWORD serverPid = 0;
    std::unique_ptr<ZemaxDDE::ZemaxDDEClient> client;
    bool isConnected = false;
};
