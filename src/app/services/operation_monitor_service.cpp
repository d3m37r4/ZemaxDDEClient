#include "operation_monitor_service.h"

namespace app::services {
    uint64_t OperationMonitorService::startTask(app::models::TaskSource source, const std::string& label, int totalSteps, int clientSlot) {
        uint64_t ddeId = 0;
        if (m_monitor) {
            ddeId = m_monitor->registerOperation(label, totalSteps);
        }

        TaskRecord rec;
        rec.taskId = m_nextId++;
        rec.source = source;
        rec.label = label;
        rec.ddeOperationId = ddeId;
        rec.clientSlot = clientSlot;
        rec.boundMonitor = m_monitor;
        m_tasks.emplace(rec.taskId, rec);

        return rec.taskId;
    }

    void OperationMonitorService::reportProgress(uint64_t taskId, int currentStep, const std::string& message) {
        auto* rec = findRecord(taskId);
        if (!rec || !rec->boundMonitor) return;
        rec->boundMonitor->reportProgress(rec->ddeOperationId, currentStep, message);
    }

    bool OperationMonitorService::isCancelled(uint64_t taskId) const {
        auto* rec = findRecord(taskId);
        if (!rec || !rec->boundMonitor) return false;
        return rec->boundMonitor->isCancelled(rec->ddeOperationId);
    }

    void OperationMonitorService::completeTask(uint64_t taskId) {
        auto* rec = findRecord(taskId);
        if (!rec) return;
        if (rec->boundMonitor && rec->ddeOperationId > 0)
            rec->boundMonitor->onCompleted(rec->ddeOperationId);
        m_tasks.erase(taskId);
    }

    void OperationMonitorService::failTask(uint64_t taskId, const std::string& error) {
        auto* rec = findRecord(taskId);
        if (!rec) return;
        if (rec->boundMonitor && rec->ddeOperationId > 0)
            rec->boundMonitor->onError(rec->ddeOperationId, error);
        m_tasks.erase(taskId);
    }

    void OperationMonitorService::requestCancel(uint64_t taskId) {
        auto* rec = findRecord(taskId);
        if (!rec || !rec->boundMonitor) return;
        if (rec->ddeOperationId > 0)
            rec->boundMonitor->requestCancel(rec->ddeOperationId);
    }

    bool OperationMonitorService::hasActiveTasks(std::optional<app::models::TaskSource> filter) const {
        for (const auto& [id, t] : m_tasks) {
            if (filter && t.source != *filter) continue;
            if (t.ddeOperationId == 0) continue;
            if (!t.boundMonitor) continue;
            for (const auto& op : t.boundMonitor->getOperations()) {
                if (op.id == t.ddeOperationId) {
                    if (op.status == ZemaxDDE::OperationStatus::Pending ||
                        op.status == ZemaxDDE::OperationStatus::InFlight) {
                        return true;
                    }
                    break;
                }
            }
        }
        return false;
    }

    bool OperationMonitorService::hasActiveTasksOnSlot(int slot, std::optional<app::models::TaskSource> filter) const {
        for (const auto& [id, t] : m_tasks) {
            if (t.clientSlot != slot) continue;
            if (filter && t.source != *filter) continue;
            if (t.ddeOperationId == 0) continue;
            if (!t.boundMonitor) continue;
            for (const auto& op : t.boundMonitor->getOperations()) {
                if (op.id == t.ddeOperationId) {
                    if (op.status == ZemaxDDE::OperationStatus::Pending ||
                        op.status == ZemaxDDE::OperationStatus::InFlight) {
                        return true;
                    }
                    break;
                }
            }
        }
        return false;
    }

    int OperationMonitorService::getTaskClientSlot(uint64_t taskId) const {
        auto* rec = findRecord(taskId);
        return rec ? rec->clientSlot : -1;
    }

    OperationMonitorService::TaskRecord* OperationMonitorService::findRecord(uint64_t taskId) {
        auto it = m_tasks.find(taskId);
        return (it != m_tasks.end()) ? &it->second : nullptr;
    }

    const OperationMonitorService::TaskRecord* OperationMonitorService::findRecord(uint64_t taskId) const {
        auto it = m_tasks.find(taskId);
        return (it != m_tasks.end()) ? &it->second : nullptr;
    }

    const ZemaxDDE::OperationInfo* OperationMonitorService::findDdeOp(uint64_t ddeId) const {
        if (!m_monitor) return nullptr;
        for (const auto& op : m_monitor->getOperations()) {
            if (op.id == ddeId) return &op;
        }
        return nullptr;
    }
}
