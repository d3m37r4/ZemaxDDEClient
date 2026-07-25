#include "operation_monitor_service.h"

namespace app::services {
    uint64_t OperationMonitorService::startTask(app::models::TaskSource source, const std::string& label, int totalSteps) {
        uint64_t ddeId = 0;
        if (m_monitor) {
            ddeId = m_monitor->registerOperation(label, totalSteps);
        }

        TaskRecord rec;
        rec.taskId = m_nextId++;
        rec.source = source;
        rec.label = label;
        rec.ddeOperationId = ddeId;
        m_tasks.emplace(rec.taskId, rec);

        return rec.taskId;
    }

    void OperationMonitorService::reportProgress(uint64_t taskId, int currentStep, const std::string& message) {
        auto* rec = findRecord(taskId);
        if (!rec || !m_monitor) return;
        m_monitor->reportProgress(rec->ddeOperationId, currentStep, message);
    }

    bool OperationMonitorService::isCancelled(uint64_t taskId) const {
        auto* rec = findRecord(taskId);
        if (!rec || !m_monitor) return false;
        return m_monitor->isCancelled(rec->ddeOperationId);
    }

    void OperationMonitorService::completeTask(uint64_t taskId) {
        auto* rec = findRecord(taskId);
        if (!rec || !m_monitor) return;
        m_monitor->onCompleted(rec->ddeOperationId);
        m_tasks.erase(taskId);
    }

    void OperationMonitorService::failTask(uint64_t taskId, const std::string& error) {
        auto* rec = findRecord(taskId);
        if (!rec || !m_monitor) return;
        m_monitor->onError(rec->ddeOperationId, error);
        m_tasks.erase(taskId);
    }

    void OperationMonitorService::requestCancel(uint64_t taskId) {
        auto* rec = findRecord(taskId);
        if (!rec) return;
        if (m_monitor && rec->ddeOperationId > 0)
            m_monitor->requestCancel(rec->ddeOperationId);
    }

    bool OperationMonitorService::hasActiveTasks(std::optional<app::models::TaskSource> filter) const {
        for (const auto& [id, t] : m_tasks) {
            if (filter && t.source != *filter) continue;
            if (t.ddeOperationId == 0) continue;
            auto* op = findDdeOp(t.ddeOperationId);
            if (!op) continue;
            if (op->status == ZemaxDDE::OperationStatus::Pending ||
                op->status == ZemaxDDE::OperationStatus::InFlight) {
                return true;
            }
        }
        return false;
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
