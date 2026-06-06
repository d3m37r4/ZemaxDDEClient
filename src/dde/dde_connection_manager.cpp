#include "dde_connection_manager.h"

#include <algorithm>
#include <format>
#include <string>

#include "app/zemax_window_enumerator.h"
#include "dde/client.h"
#include "logger/logger.h"

namespace {

    std::string ws2s(const std::wstring& wstr) {
        if (wstr.empty()) return {};
        int len = WideCharToMultiByte(CP_ACP, 0, wstr.data(), static_cast<int>(wstr.size()),
            nullptr, 0, nullptr, nullptr);
        std::string result(static_cast<size_t>(len), '\0');
        WideCharToMultiByte(CP_ACP, 0, wstr.data(), static_cast<int>(wstr.size()),
            result.data(), len, nullptr, nullptr);
        return result;
    }

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
        if (!m_connections[i].isConnected) {
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
    conn.isConnected = true;

    DWORD pid = 0;
    GetWindowThreadProcessId(targetHwnd, &pid);
    conn.serverPid = pid;

    m_activeIndex = idx;

    m_logger.addLog(std::format("[DDE] Connected slot {}: '{}' (PID: {}, hwndClient={:#010x}, hwndServer={:#010x})",
        idx, ws2s(title), pid,
        reinterpret_cast<uintptr_t>(conn.hwndClient),
        reinterpret_cast<uintptr_t>(conn.hwndServer)));
    m_logger.addLog(std::format("[DDE] Switched active connection to slot {}: '{}' (PID: {}, hwndClient={:#010x}, hwndServer={:#010x})",
        idx, ws2s(conn.serverTitle), conn.serverPid,
        reinterpret_cast<uintptr_t>(conn.hwndClient),
        reinterpret_cast<uintptr_t>(conn.hwndServer)));

    return idx;
}

void DDEConnectionManager::disconnect(int index) {
    if (index < 0 || index >= MAX_CONNECTIONS) return;

    auto& conn = m_connections[index];
    if (!conn.isConnected) return;

    m_logger.addLog(std::format("[DDE] Disconnected slot {}: '{}' (PID: {}, hwndClient={:#010x}, hwndServer={:#010x})",
        index, ws2s(conn.serverTitle), conn.serverPid,
        reinterpret_cast<uintptr_t>(conn.hwndClient),
        reinterpret_cast<uintptr_t>(conn.hwndServer)));

    if (conn.client) {
        conn.client->terminateDDE();
        conn.client.reset();
    }

    if (conn.hwndClient) {
        DestroyWindow(conn.hwndClient);
        conn.hwndClient = nullptr;
    }

    conn.hwndServer = nullptr;
    conn.serverTitle.clear();
    conn.serverPid = 0;
    conn.isConnected = false;

    if (m_activeIndex == index) {
        m_activeIndex = -1;
        for (int i = 0; i < MAX_CONNECTIONS; ++i) {
            if (m_connections[i].isConnected) {
                m_activeIndex = i;
                break;
            }
        }
    }
}

void DDEConnectionManager::disconnectAll() {
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (m_connections[i].isConnected) {
            disconnect(i);
        }
    }
}

void DDEConnectionManager::setActiveConnection(int index) {
    if (index >= 0 && index < MAX_CONNECTIONS && m_connections[index].isConnected) {
        m_activeIndex = index;
        auto& conn = m_connections[index];
        m_logger.addLog(std::format("[DDE] Switched active connection to slot {}: '{}' (PID: {}, hwndClient={:#010x}, hwndServer={:#010x})",
            index, ws2s(conn.serverTitle), conn.serverPid,
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

bool DDEConnectionManager::isAnyConnected() const {
    for (const auto& conn : m_connections) {
        if (conn.isConnected) return true;
    }
    return false;
}

void DDEConnectionManager::pumpAllMessages() {
    for (auto& conn : m_connections) {
        if (conn.client) {
            conn.client->pumpMessages();
        }
    }
}

void DDEConnectionManager::processAllTimeouts() {
    for (auto& conn : m_connections) {
        if (conn.client) {
            conn.client->processTimeouts();
        }
    }
}

std::vector<app::ZemaxWindowInfo> DDEConnectionManager::enumerateAvailableTargets() {
    return app::ZemaxWindowEnumerator::enumerate();
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
    propagateDefaultTimeout(ms);
}

void DDEConnectionManager::setDefaultRetries(int n) {
    propagateDefaultRetries(n);
}

void DDEConnectionManager::setMaxConnections(int n) {
    if (n < 1) n = 1;
    if (n > MAX_CONNECTIONS) n = MAX_CONNECTIONS;
    m_maxConnections = n;
    m_logger.addLog(std::format("[DDE] Max connections set to {}", n));
}

void DDEConnectionManager::setAutoReconnect(bool enabled) {
    m_autoReconnect = enabled;
    m_logger.addLog(std::format("[DDE] Auto-reconnect {}", enabled ? "enabled" : "disabled"));
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
