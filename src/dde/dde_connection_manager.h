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

    // Runtime-configurable DDE settings. Forwarded to active clients by
    // SettingsManager::applyDDE. Clamped to safe ranges.
    void setDefaultTimeoutMs(DWORD ms);
    void setDefaultRetries(int n);
    void setMaxConnections(int n);
    void setAutoReconnect(bool enabled);

    [[nodiscard]] DWORD getDefaultTimeoutMs() const;
    [[nodiscard]] int   getDefaultRetries()   const;
    [[nodiscard]] int   getMaxConnections()   const { return m_maxConnections; }
    [[nodiscard]] bool  getAutoReconnect()    const { return m_autoReconnect; }

private:
    std::array<DDEConnection, MAX_CONNECTIONS> m_connections;
    Logger& m_logger;
    int m_activeIndex = -1;
    int m_maxConnections = MAX_CONNECTIONS;
    bool m_autoReconnect = true;

    int findFreeSlot();
    HWND createClientWindow();

    void propagateDefaultTimeout(DWORD ms);
    void propagateDefaultRetries(int n);
};
