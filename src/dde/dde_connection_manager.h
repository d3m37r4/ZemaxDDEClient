#pragma once

#include <functional>

namespace ZemaxDDE { class ZemaxDDEClient; }

class Logger;

// 4.1: Centralizes DDE connection management
class DdeConnectionManager {
public:
    DdeConnectionManager(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger);

    bool connect();
    void disconnect();
    bool isConnected() const;

    void setOnConnected(std::function<void()> cb);
    void setOnDisconnected(std::function<void()> cb);

private:
    ZemaxDDE::ZemaxDDEClient* m_ddeClient;
    Logger& m_logger;
    std::function<void()> m_onConnected;
    std::function<void()> m_onDisconnected;
};
