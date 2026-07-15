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
        m_tasks.push_back(rec);

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
        rec->ddeOperationId = 0;
    }

    void OperationMonitorService::failTask(uint64_t taskId, const std::string& error) {
        auto* rec = findRecord(taskId);
        if (!rec || !m_monitor) return;
        m_monitor->onError(rec->ddeOperationId, error);
        rec->ddeOperationId = 0;
    }

    void OperationMonitorService::requestCancel(uint64_t taskId) {
        auto* rec = findRecord(taskId);
        if (!rec) return;
        if (m_monitor && rec->ddeOperationId > 0)
            m_monitor->requestCancel(rec->ddeOperationId);
    }

    bool OperationMonitorService::isActive(app::models::TaskSource source) const {
        for (const auto& t : m_tasks) {
            if (t.source != source) continue;
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

    bool OperationMonitorService::hasActiveTasks() const {
        for (const auto& t : m_tasks) {
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
        for (auto& t : m_tasks) {
            if (t.taskId == taskId) return &t;
        }
        return nullptr;
    }

    const OperationMonitorService::TaskRecord* OperationMonitorService::findRecord(uint64_t taskId) const {
        for (const auto& t : m_tasks) {
            if (t.taskId == taskId) return &t;
        }
        return nullptr;
    }

    const ZemaxDDE::OperationInfo* OperationMonitorService::findDdeOp(uint64_t ddeId) const {
        if (!m_monitor) return nullptr;
        for (const auto& op : m_monitor->getOperations()) {
            if (op.id == ddeId) return &op;
        }
        return nullptr;
    }
}
