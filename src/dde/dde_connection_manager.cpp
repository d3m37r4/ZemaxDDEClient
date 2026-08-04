#include "dde_connection_manager.h"

#include <algorithm>
#include <format>
#include <string>

#include "dde/client.h"
#include "dde/operation_monitor.h"
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
        // Defensive: clean up GWLP_USERDATA if the window is destroyed outside of disconnect().
        // Currently all DestroyWindow calls go through disconnect() which handles this explicitly.
        // This handler is a safety net for future changes that may destroy the window differently.
        if (iMsg == WM_NCDESTROY) {
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
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
    // Follow the client's actual state instead of assuming Connected. For a
    // synchronous DDE handshake this is already Connected; for an asynchronous
    // ACK the slot stays in Connecting until the ACK handler promotes the client
    // and the per-frame sync in checkAllConnectionHealth() copies it here.
    conn.connectionState = conn.client->connectionState();

    DWORD pid = 0;
    GetWindowThreadProcessId(targetHwnd, &pid);
    conn.serverPid = pid;
    conn.client->setServerPid(pid);

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

    // Notify upper layers before the client is destroyed so that any bookkeeping
    // referencing this connection's OperationMonitor can be released.
    if (m_onClientDisconnect) {
        m_onClientDisconnect(index);
    }

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
    if (index < 0 || index >= MAX_CONNECTIONS || !m_connections[index].isConnected()) return;
    if (index == m_activeIndex) return;

    m_activeIndex = index;
    auto& conn = m_connections[index];
    m_logger.addLog(std::format("[DDE] Switched active connection to slot {}: '{}' (PID: {}, hwndClient={:#010x}, hwndServer={:#010x})",
        index, ZemaxDDE::wstring_to_utf8(conn.serverTitle), conn.serverPid,
        reinterpret_cast<uintptr_t>(conn.hwndClient),
        reinterpret_cast<uintptr_t>(conn.hwndServer)));
}

ZemaxDDE::ZemaxDDEClient* DDEConnectionManager::getActiveClient() const {
    if (m_activeIndex >= 0 && m_activeIndex < MAX_CONNECTIONS) {
        return m_connections[m_activeIndex].client.get();
    }
    return nullptr;
}

DDEConnection* DDEConnectionManager::getConnection(int index) {
    if (index >= 0 && index < m_maxConnections) {
        return &m_connections[index];
    }
    return nullptr;
}

void DDEConnectionManager::processAllTimeouts() {
    for (auto& conn : m_connections) {
        if (conn.client && conn.isConnected()) {
            conn.client->processTimeouts();
        }
    }
}

void DDEConnectionManager::checkAllConnectionHealth() {
    if (m_connectionLostIndex >= 0) return;

    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        auto& conn = m_connections[i];
        if (conn.isDisconnected() || !conn.client) continue;

        // Sync the slot's externally-visible state with the actual client state
        // (covers asynchronous WM_DDE_ACK handshakes promoted during pump/glfw).
        ZemaxDDE::ConnectionState actual = conn.client->connectionState();
        if (conn.connectionState != actual) {
            conn.connectionState = actual;
        }

        conn.client->checkConnectionHealth();

        if (conn.client->hasConnectionLost()) {
            m_connectionLostIndex = i;
            m_connectionLostReason = conn.client->getConnectionLostReason();
            conn.client->clearConnectionLost();
            m_logger.addLog(std::format("[DDE] Connection lost flagged for slot {}: {}", i, m_connectionLostReason));
        }
    }
}

int DDEConnectionManager::findActiveTaskSlot(app::models::TaskSource source) const {
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        auto* conn = const_cast<DDEConnectionManager*>(this)->getConnection(i);
        if (!conn || !conn->isConnected() || !conn->client) continue;

        auto* monitor = conn->client->getOperationMonitor();
        if (!monitor) continue;

        bool matches = false;
        for (const auto& op : monitor->getOperations()) {
            if (op.status != ZemaxDDE::OperationStatus::Pending &&
                op.status != ZemaxDDE::OperationStatus::InFlight) continue;

            bool labelMatch = false;
            switch (source) {
                case app::models::TaskSource::NominalSurfaceProfile:
                    labelMatch = op.serviceId == "Nominal Profile";
                    break;
                case app::models::TaskSource::TolerancedSurfaceProfile:
                    labelMatch = op.serviceId == "Toleranced Profile";
                    break;
                case app::models::TaskSource::SurfaceIrregularityMap:
                    labelMatch = op.serviceId == "Surface Irregularity Map"
                              || op.serviceId.starts_with("Section ");
                    break;
                default:
                    break;
            }
            if (labelMatch) {
                matches = true;
                break;
            }
        }
        if (matches) return i;
    }
    return -1;
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
