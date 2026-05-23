#include "operation_monitor.h"
#include <algorithm>

namespace ZemaxDDE {

uint64_t OperationMonitor::registerOperation(const std::string& serviceId, int totalSteps) {
    OperationInfo info;
    info.id = m_nextId++;
    info.serviceId = serviceId;
    info.status = OperationStatus::Pending;
    info.startTime = GetTickCount();
    info.totalSteps = totalSteps;
    info.currentStep = 0;
    info.cancelRequested = false;
    m_operations.push_back(info);
    return info.id;
}

void OperationMonitor::onRequestQueued(uint64_t operationId, const std::string& command) {
    int idx = findIndex(operationId);
    if (idx < 0) return;
    m_operations[idx].command = command;
    m_operations[idx].status = OperationStatus::InFlight;
    m_operations[idx].startTime = GetTickCount();
}

void OperationMonitor::reportProgress(uint64_t operationId, int currentStep, const std::string& message) {
    int idx = findIndex(operationId);
    if (idx < 0) return;

    auto& op = m_operations[idx];
    op.currentStep = currentStep;
    op.message = message;
    op.elapsed = GetTickCount() - op.startTime;
}

void OperationMonitor::onCompleted(uint64_t operationId) {
    int idx = findIndex(operationId);
    if (idx < 0) return;

    auto& op = m_operations[idx];
    op.status = OperationStatus::Completed;
    op.elapsed = GetTickCount() - op.startTime;
    op.message = "Completed";
}

void OperationMonitor::onError(uint64_t operationId, const std::string& error) {
    int idx = findIndex(operationId);
    if (idx < 0) return;

    auto& op = m_operations[idx];
    op.status = OperationStatus::Failed;
    op.elapsed = GetTickCount() - op.startTime;
    op.error = error;
    op.message = "Error";
}

void OperationMonitor::requestCancel(uint64_t operationId) {
    int idx = findIndex(operationId);
    if (idx < 0) return;
    m_operations[idx].cancelRequested = true;
    m_operations[idx].message = "Cancelling...";
}

bool OperationMonitor::isCancelled(uint64_t operationId) const {
    int idx = findIndex(operationId);
    if (idx < 0) return false;
    return m_operations[idx].cancelRequested;
}

int OperationMonitor::findIndex(uint64_t id) const {
    for (size_t i = 0; i < m_operations.size(); ++i) {
        if (m_operations[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

void OperationMonitor::clearCompleted() {
    m_operations.erase(
        std::remove_if(m_operations.begin(), m_operations.end(),
            [](const OperationInfo& op) {
                return op.status == OperationStatus::Completed ||
                       op.status == OperationStatus::Failed ||
                       op.status == OperationStatus::Cancelled;
            }),
        m_operations.end());
}

} // namespace ZemaxDDE
