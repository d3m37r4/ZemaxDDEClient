#include <format>

#include "ui_operation_monitor.h"

namespace gui {

uint64_t UiOperationMonitor::startTask(TaskSource source, const std::string& label, int totalSteps) {
    uint64_t ddeId = 0;
    if (m_monitor) {
        ddeId = m_monitor->registerOperation(label, totalSteps);
    }

    TaskRecord rec;
    rec.taskId = m_nextId++;
    rec.source = source;
    rec.label = label;
    rec.ddeOperationId = ddeId;
    m_tasks.push_back(rec);

    return rec.taskId;
}

void UiOperationMonitor::reportProgress(uint64_t taskId, int currentStep, const std::string& message) {
    auto* rec = findRecord(taskId);
    if (!rec || !m_monitor) return;
    m_monitor->reportProgress(rec->ddeOperationId, currentStep, message);
}

bool UiOperationMonitor::isCancelled(uint64_t taskId) const {
    auto* rec = findRecord(taskId);
    if (!rec || !m_monitor) return false;
    return m_monitor->isCancelled(rec->ddeOperationId);
}

void UiOperationMonitor::completeTask(uint64_t taskId) {
    auto* rec = findRecord(taskId);
    if (!rec || !m_monitor) return;
    m_monitor->onCompleted(rec->ddeOperationId);
    rec->ddeOperationId = 0;
}

void UiOperationMonitor::failTask(uint64_t taskId, const std::string& error) {
    auto* rec = findRecord(taskId);
    if (!rec || !m_monitor) return;
    m_monitor->onError(rec->ddeOperationId, error);
    rec->ddeOperationId = 0;
}

void UiOperationMonitor::requestCancel(uint64_t taskId) {
    auto* rec = findRecord(taskId);
    if (!rec) return;
    if (m_monitor && rec->ddeOperationId > 0)
        m_monitor->requestCancel(rec->ddeOperationId);
}

uint64_t UiOperationMonitor::getDdeOperationId(uint64_t taskId) const {
    auto* rec = findRecord(taskId);
    return rec ? rec->ddeOperationId : 0;
}

UiOperationMonitor::TaskRecord* UiOperationMonitor::findRecord(uint64_t taskId) {
    for (auto& t : m_tasks) {
        if (t.taskId == taskId) return &t;
    }
    return nullptr;
}

const UiOperationMonitor::TaskRecord* UiOperationMonitor::findRecord(uint64_t taskId) const {
    for (const auto& t : m_tasks) {
        if (t.taskId == taskId) return &t;
    }
    return nullptr;
}

const ZemaxDDE::OperationInfo* UiOperationMonitor::findDdeOp(uint64_t ddeId) const {
    if (!m_monitor) return nullptr;
    for (const auto& op : m_monitor->getOperations()) {
        if (op.id == ddeId) return &op;
    }
    return nullptr;
}

}
