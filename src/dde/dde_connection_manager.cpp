#include "dde_connection_manager.h"

#include <algorithm>
#include <format>
#include <string>

#include "dde/client.h"
#include "dde/utils.h"
#include "logger/logger.h"

namespace {

    extern "C" LRESULT CALLBACK DDEClientWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
        if (iMsg >= WM_DDE_FIRST && iMsg <= WM_DDE_LAST) {
            auto* client = reinterpret_cast<ZemaxDDE::ZemaxDDEClient*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (client) {
                return client->handleDDEMessages(iMsg, wParam, lParam);
            }
        }
        return DefWindowProcW(hwnd, iMsg, wParam, lParam);
    }

    ATOM registerDDEClientClass() {
        WNDCLASSW wndClass{};
        wndClass.lpfnWndProc = DDEClientWndProc;
        wndClass.hInstance = GetModuleHandleW(nullptr);
        wndClass.lpszClassName = L"ZEMAX_DDE_Client";
        return RegisterClassW(&wndClass);
    }

    HWND createDDEClientWindow() {
        registerDDEClientClass();
        return CreateWindowExW(0, L"ZEMAX_DDE_Client", L"DDE Client",
            0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr);
    }

}

DDEConnectionManager::DDEConnectionManager(Logger& logger)
    : m_logger(logger)
{
}

int DDEConnectionManager::findFreeSlot() {
    for (int i = 0; i < m_maxConnections && i < MAX_CONNECTIONS; ++i) {
        if (m_connections[i].isDisconnected()) {
            return i;
        }
    }
    return -1;
}

int DDEConnectionManager::connectToZemax(HWND targetHwnd, const std::wstring& title) {
    if (!targetHwnd) {
        m_logger.addLog("[DDE] connectToZemax: null target HWND");
        return -1;
    }

    int idx = findFreeSlot();
    if (idx < 0) {
        m_logger.addLog("[DDE] connectToZemax: no free slots available");
        return -1;
    }

    auto& conn = m_connections[idx];

    conn.hwndClient = createDDEClientWindow();
    if (!conn.hwndClient) {
        m_logger.addLog("[DDE] connectToZemax: failed to create DDE client window");
        return -1;
    }

    conn.client = std::make_unique<ZemaxDDE::ZemaxDDEClient>(conn.hwndClient, m_logger);
    SetWindowLongPtrW(conn.hwndClient, GWLP_USERDATA,
        reinterpret_cast<LONG_PTR>(conn.client.get()));

    // Propagate per-request timeouts to new client.
    conn.client->setGetNameTimeoutMs(m_getNameTimeoutMs);
    conn.client->setGetFileTimeoutMs(m_getFileTimeoutMs);
    conn.client->setGetSystemTimeoutMs(m_getSystemTimeoutMs);
    conn.client->setGetFieldTimeoutMs(m_getFieldTimeoutMs);
    conn.client->setGetWaveTimeoutMs(m_getWaveTimeoutMs);
    conn.client->setGetSurfaceDataProfileTimeoutMs(m_getSurfaceDataProfileTimeoutMs);
    conn.client->setGetSagProfileTimeoutMs(m_getSagProfileTimeoutMs);
    conn.client->setGetSurfaceDataMapTimeoutMs(m_getSurfaceDataMapTimeoutMs);
    conn.client->setGetSagMapTimeoutMs(m_getSagMapTimeoutMs);

    try {
        conn.client->initiateDDE(targetHwnd);
    } catch (const std::exception& e) {
        m_logger.addLog(std::format("[DDE] connectToZemax: initiateDDE failed: {}", e.what()));
        conn.client.reset();
        DestroyWindow(conn.hwndClient);
        conn.hwndClient = nullptr;
        return -1;
    }

    conn.hwndServer = targetHwnd;
    conn.serverTitle = title;
    conn.connectionState = ZemaxDDE::ConnectionState::Connected;

    DWORD pid = 0;
    GetWindowThreadProcessId(targetHwnd, &pid);
    conn.serverPid = pid;
    conn.client->setServerPid(pid);

    // Set up connection lost callback to flag for GUI popup
    conn.client->setOnConnectionLostCallback([this, idx](const std::string& reason) {
        if (idx >= 0 && idx < MAX_CONNECTIONS && m_connections[idx].isConnected()) {
            m_connectionLostIndex = idx;
            m_connectionLostReason = reason;
            m_logger.addLog(std::format("[DDE] Connection lost flagged for slot {}: {}", idx, reason));
        }
    });

    m_activeIndex = idx;

    m_logger.addLog(std::format("[DDE] Connected slot {}: '{}' (PID: {}, hwndClient={:#010x}, hwndServer={:#010x})",
        idx, ZemaxDDE::wstring_to_utf8(title), pid,
        reinterpret_cast<uintptr_t>(conn.hwndClient),
        reinterpret_cast<uintptr_t>(conn.hwndServer)));
    m_logger.addLog(std::format("[DDE] Switched active connection to slot {}: '{}' (PID: {}, hwndClient={:#010x}, hwndServer={:#010x})",
        idx, ZemaxDDE::wstring_to_utf8(conn.serverTitle), conn.serverPid,
        reinterpret_cast<uintptr_t>(conn.hwndClient),
        reinterpret_cast<uintptr_t>(conn.hwndServer)));

    return idx;
}

void DDEConnectionManager::disconnect(int index) {
    if (index < 0 || index >= MAX_CONNECTIONS) return;

    auto& conn = m_connections[index];
    if (conn.isDisconnected()) return;

    m_logger.addLog(std::format("[DDE] Disconnected slot {}: '{}' (PID: {}, hwndClient={:#010x}, hwndServer={:#010x})",
        index, ZemaxDDE::wstring_to_utf8(conn.serverTitle), conn.serverPid,
        reinterpret_cast<uintptr_t>(conn.hwndClient),
        reinterpret_cast<uintptr_t>(conn.hwndServer)));

    if (conn.client) {
        conn.client->terminateDDE();
    }

    if (conn.hwndClient) {
        SetWindowLongPtrW(conn.hwndClient, GWLP_USERDATA, 0);
        DestroyWindow(conn.hwndClient);
        conn.hwndClient = nullptr;
    }

    if (conn.client) {
        conn.client.reset();
    }

    conn.hwndServer = nullptr;
    conn.serverTitle.clear();
    conn.serverPid = 0;
    conn.connectionState = ZemaxDDE::ConnectionState::Disconnected;

    if (m_activeIndex == index) {
        m_activeIndex = -1;
        for (int i = 0; i < MAX_CONNECTIONS; ++i) {
            if (m_connections[i].isConnected()) {
                m_activeIndex = i;
                break;
            }
        }
    }
}

void DDEConnectionManager::disconnectAll() {
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (m_connections[i].isConnected()) {
            disconnect(i);
        }
    }
}

void DDEConnectionManager::setActiveConnection(int index) {
    if (index >= 0 && index < MAX_CONNECTIONS && m_connections[index].isConnected()) {
        m_activeIndex = index;
        auto& conn = m_connections[index];
        m_logger.addLog(std::format("[DDE] Switched active connection to slot {}: '{}' (PID: {}, hwndClient={:#010x}, hwndServer={:#010x})",
            index, ZemaxDDE::wstring_to_utf8(conn.serverTitle), conn.serverPid,
            reinterpret_cast<uintptr_t>(conn.hwndClient),
            reinterpret_cast<uintptr_t>(conn.hwndServer)));
    }
}

ZemaxDDE::ZemaxDDEClient* DDEConnectionManager::getActiveClient() const {
    if (m_activeIndex >= 0 && m_activeIndex < MAX_CONNECTIONS) {
        return m_connections[m_activeIndex].client.get();
    }
    return nullptr;
}

DDEConnection* DDEConnectionManager::getConnection(int index) {
    if (index >= 0 && index < MAX_CONNECTIONS) {
        return &m_connections[index];
    }
    return nullptr;
}

void DDEConnectionManager::processAllTimeouts() {
    for (auto& conn : m_connections) {
        if (conn.client) {
            conn.client->processTimeouts();
        }
    }
}

void DDEConnectionManager::checkAllConnectionHealth() {
    // If already flagged and not yet handled by GUI, skip re-checking
    if (m_connectionLostIndex >= 0) return;

    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        auto& conn = m_connections[i];
        if (conn.isDisconnected() || !conn.client) continue;

        // Delegate health check to the client — it knows its own HWND and PID.
        // If client detects loss, it calls handleConnectionLost() which:
        //   1. Sets m_connectionState = Disconnected (on client)
        //   2. Drains the request queue
        //   3. Fires the callback → sets m_connectionLostIndex
        // We do NOT set conn.connectionState here — disconnect() will handle it.
        conn.client->checkConnectionHealth();
    }
}

DWORD DDEConnectionManager::getDefaultTimeoutMs() const {
    for (const auto& conn : m_connections) {
        if (conn.client) return conn.client->getDefaultTimeoutMs();
    }
    return 1000;
}

int DDEConnectionManager::getDefaultRetries() const {
    for (const auto& conn : m_connections) {
        if (conn.client) return conn.client->getDefaultRetries();
    }
    return 1;
}

void DDEConnectionManager::setDefaultTimeoutMs(DWORD ms) {
    if (ms == m_defaultTimeoutMs) return;
    m_defaultTimeoutMs = ms;
    propagateDefaultTimeout(ms);
}

void DDEConnectionManager::setDefaultRetries(int n) {
    if (n == m_defaultRetries) return;
    m_defaultRetries = n;
    propagateDefaultRetries(n);
}

void DDEConnectionManager::setMaxConnections(int n) {
    if (n < 1) n = 1;
    if (n > MAX_CONNECTIONS) n = MAX_CONNECTIONS;
    if (n == m_maxConnections) return;
    m_maxConnections = n;
    m_logger.addLog(std::format("[DDE] Max connections set to {}", n));
}

void DDEConnectionManager::setGetNameTimeoutMs(DWORD ms) {
    if (ms == m_getNameTimeoutMs) return;
    m_getNameTimeoutMs = ms;
    propagatePerRequestTimeouts();
}

void DDEConnectionManager::setGetFileTimeoutMs(DWORD ms) {
    if (ms == m_getFileTimeoutMs) return;
    m_getFileTimeoutMs = ms;
    propagatePerRequestTimeouts();
}

void DDEConnectionManager::setGetSystemTimeoutMs(DWORD ms) {
    if (ms == m_getSystemTimeoutMs) return;
    m_getSystemTimeoutMs = ms;
    propagatePerRequestTimeouts();
}

void DDEConnectionManager::setGetFieldTimeoutMs(DWORD ms) {
    if (ms == m_getFieldTimeoutMs) return;
    m_getFieldTimeoutMs = ms;
    propagatePerRequestTimeouts();
}

void DDEConnectionManager::setGetWaveTimeoutMs(DWORD ms) {
    if (ms == m_getWaveTimeoutMs) return;
    m_getWaveTimeoutMs = ms;
    propagatePerRequestTimeouts();
}

void DDEConnectionManager::setGetSurfaceDataProfileTimeoutMs(DWORD ms) {
    if (ms == m_getSurfaceDataProfileTimeoutMs) return;
    m_getSurfaceDataProfileTimeoutMs = ms;
    propagatePerRequestTimeouts();
}

void DDEConnectionManager::setGetSagProfileTimeoutMs(DWORD ms) {
    if (ms == m_getSagProfileTimeoutMs) return;
    m_getSagProfileTimeoutMs = ms;
    propagatePerRequestTimeouts();
}

void DDEConnectionManager::setGetSurfaceDataMapTimeoutMs(DWORD ms) {
    if (ms == m_getSurfaceDataMapTimeoutMs) return;
    m_getSurfaceDataMapTimeoutMs = ms;
    propagatePerRequestTimeouts();
}

void DDEConnectionManager::setGetSagMapTimeoutMs(DWORD ms) {
    if (ms == m_getSagMapTimeoutMs) return;
    m_getSagMapTimeoutMs = ms;
    propagatePerRequestTimeouts();
}

void DDEConnectionManager::propagateDefaultTimeout(DWORD ms) {
    for (auto& conn : m_connections) {
        if (conn.client) conn.client->setDefaultTimeoutMs(ms);
    }
}

void DDEConnectionManager::propagateDefaultRetries(int n) {
    for (auto& conn : m_connections) {
        if (conn.client) conn.client->setDefaultRetries(n);
    }
}

void DDEConnectionManager::propagatePerRequestTimeouts() {
    for (auto& conn : m_connections) {
        if (!conn.client) continue;
        conn.client->setGetNameTimeoutMs(m_getNameTimeoutMs);
        conn.client->setGetFileTimeoutMs(m_getFileTimeoutMs);
        conn.client->setGetSystemTimeoutMs(m_getSystemTimeoutMs);
        conn.client->setGetFieldTimeoutMs(m_getFieldTimeoutMs);
        conn.client->setGetWaveTimeoutMs(m_getWaveTimeoutMs);
        conn.client->setGetSurfaceDataProfileTimeoutMs(m_getSurfaceDataProfileTimeoutMs);
        conn.client->setGetSagProfileTimeoutMs(m_getSagProfileTimeoutMs);
        conn.client->setGetSurfaceDataMapTimeoutMs(m_getSurfaceDataMapTimeoutMs);
        conn.client->setGetSagMapTimeoutMs(m_getSagMapTimeoutMs);
    }
}
