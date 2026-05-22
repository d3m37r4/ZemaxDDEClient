#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <windows.h>

#include "dde/dde_connection.h"

namespace app { struct ZemaxWindowInfo; }
namespace ZemaxDDE { class ZemaxDDEClient; }
class Logger;

class DDEConnectionManager {
public:
    static constexpr int MAX_CONNECTIONS = 2;

    explicit DDEConnectionManager(Logger& logger);

    int connectToZemax(HWND targetHwnd, const std::wstring& title);
    void disconnect(int index);
    void disconnectAll();

    void setActiveConnection(int index);
    ZemaxDDE::ZemaxDDEClient* getActiveClient() const;
    int getActiveIndex() const { return m_activeIndex; }

    DDEConnection* getConnection(int index);
    const std::array<DDEConnection, MAX_CONNECTIONS>& getConnections() const { return m_connections; }

    bool isAnyConnected() const;

    void pumpAllMessages();
    void processAllTimeouts();

    std::vector<app::ZemaxWindowInfo> enumerateAvailableTargets();

private:
    std::array<DDEConnection, MAX_CONNECTIONS> m_connections;
    Logger& m_logger;
    int m_activeIndex = -1;

    int findFreeSlot();
    HWND createClientWindow();
};
