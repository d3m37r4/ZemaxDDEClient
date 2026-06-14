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

    void setGetNameTimeoutMs(DWORD ms);
    void setGetFileTimeoutMs(DWORD ms);
    void setGetSystemTimeoutMs(DWORD ms);
    void setGetFieldTimeoutMs(DWORD ms);
    void setGetWaveTimeoutMs(DWORD ms);
    void setGetSurfaceDataProfileTimeoutMs(DWORD ms);
    void setGetSagProfileTimeoutMs(DWORD ms);
    void setGetSurfaceDataMapTimeoutMs(DWORD ms);
    void setGetSagMapTimeoutMs(DWORD ms);

    [[nodiscard]] DWORD getDefaultTimeoutMs() const;
    [[nodiscard]] int   getDefaultRetries()   const;
    [[nodiscard]] int   getMaxConnections()   const { return m_maxConnections; }
    [[nodiscard]] bool  getAutoReconnect()    const { return m_autoReconnect; }

    [[nodiscard]] DWORD getGetNameTimeoutMs() const { return m_getNameTimeoutMs; }
    [[nodiscard]] DWORD getGetFileTimeoutMs() const { return m_getFileTimeoutMs; }
    [[nodiscard]] DWORD getGetSystemTimeoutMs() const { return m_getSystemTimeoutMs; }
    [[nodiscard]] DWORD getGetFieldTimeoutMs() const { return m_getFieldTimeoutMs; }
    [[nodiscard]] DWORD getGetWaveTimeoutMs() const { return m_getWaveTimeoutMs; }
    [[nodiscard]] DWORD getGetSurfaceDataProfileTimeoutMs() const { return m_getSurfaceDataProfileTimeoutMs; }
    [[nodiscard]] DWORD getGetSagProfileTimeoutMs() const { return m_getSagProfileTimeoutMs; }
    [[nodiscard]] DWORD getGetSurfaceDataMapTimeoutMs() const { return m_getSurfaceDataMapTimeoutMs; }
    [[nodiscard]] DWORD getGetSagMapTimeoutMs() const { return m_getSagMapTimeoutMs; }

private:
    std::array<DDEConnection, MAX_CONNECTIONS> m_connections;
    Logger& m_logger;
    int m_activeIndex = -1;
    int m_maxConnections = MAX_CONNECTIONS;
    bool m_autoReconnect = true;

    DWORD m_getNameTimeoutMs = 2000;
    DWORD m_getFileTimeoutMs = 2000;
    DWORD m_getSystemTimeoutMs = 2000;
    DWORD m_getFieldTimeoutMs = 2000;
    DWORD m_getWaveTimeoutMs = 2000;
    DWORD m_getSurfaceDataProfileTimeoutMs = 2000;
    DWORD m_getSagProfileTimeoutMs = 1000;
    DWORD m_getSurfaceDataMapTimeoutMs = 2000;
    DWORD m_getSagMapTimeoutMs = 1000;

    int findFreeSlot();
    HWND createClientWindow();

    void propagateDefaultTimeout(DWORD ms);
    void propagateDefaultRetries(int n);
    void propagatePerRequestTimeouts();
};
