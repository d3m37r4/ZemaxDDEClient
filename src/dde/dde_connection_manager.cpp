#include "dde_connection_manager.h"
#include "dde/client.h"
#include "logger/logger.h"

DdeConnectionManager::DdeConnectionManager(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger)
    : m_ddeClient(ddeClient)
    , m_logger(logger)
{
}

bool DdeConnectionManager::connect() {
    if (!m_ddeClient) return false;
    if (isConnected()) return true;
    m_ddeClient->initiateDDE();
    return isConnected();
}

void DdeConnectionManager::disconnect() {
    if (isConnected()) {
        m_ddeClient->terminateDDE();
    }
}

bool DdeConnectionManager::isConnected() const {
    return m_ddeClient ? m_ddeClient->isConnected() : false;
}

void DdeConnectionManager::setOnConnected(std::function<void()> cb) {
    m_onConnected = std::move(cb);
}

void DdeConnectionManager::setOnDisconnected(std::function<void()> cb) {
    m_onDisconnected = std::move(cb);
}
