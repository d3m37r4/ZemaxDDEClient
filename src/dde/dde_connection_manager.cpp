#include "dde_connection_manager.h"
#include "dde/client.h"
#include "logger/logger.h"

DDEConnectionManager::DDEConnectionManager(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger)
    : m_ddeClient(ddeClient)
    , m_logger(logger)
{
}

bool DDEConnectionManager::connect() {
    if (!m_ddeClient) return false;
    if (isConnected()) return true;
    m_ddeClient->initiateDDE();
    return isConnected();
}

void DDEConnectionManager::disconnect() {
    if (isConnected()) {
        m_ddeClient->terminateDDE();
    }
}

bool DDEConnectionManager::isConnected() const {
    return m_ddeClient ? m_ddeClient->isConnected() : false;
}

void DDEConnectionManager::setOnConnected(std::function<void()> cb) {
    m_onConnected = std::move(cb);
}

void DDEConnectionManager::setOnDisconnected(std::function<void()> cb) {
    m_onDisconnected = std::move(cb);
}
