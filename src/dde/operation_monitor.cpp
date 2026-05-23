#include "operation_monitor.h"
#include <algorithm>

namespace ZemaxDDE {

uint64_t OperationMonitor::registerRequest(const std::string& command) {
    OperationInfo info;
    info.id = m_nextId++;
    info.command = command;
    info.status = OperationStatus::Pending;
    info.startTime = GetTickCount();
    m_operations.push_back(info);
    return info.id;
}

void OperationMonitor::updateStatus(uint64_t id, OperationStatus status, const std::string& error) {
    int idx = findIndex(id);
    if (idx < 0) return;

    auto& op = m_operations[idx];
    op.status = status;
    op.elapsed = GetTickCount() - op.startTime;
    if (!error.empty()) {
        op.error = error;
    }
}

int OperationMonitor::findIndex(uint64_t id) {
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
                       op.status == OperationStatus::TimedOut;
            }),
        m_operations.end());
}

} // namespace ZemaxDDE