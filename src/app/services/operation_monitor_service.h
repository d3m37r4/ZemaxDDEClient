#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "dde/operation_monitor.h"
#include "app/models/types.h"

namespace app::services {

    class OperationMonitorService {
    public:
        struct TaskRecord {
            uint64_t taskId;
            app::models::TaskSource source;
            std::string label;
            uint64_t ddeOperationId;
        };

        void setMonitor(ZemaxDDE::OperationMonitor* monitor) { m_monitor = monitor; }

        uint64_t startTask(app::models::TaskSource source, const std::string& label, int totalSteps);
        void reportProgress(uint64_t taskId, int currentStep, const std::string& message);
        bool isCancelled(uint64_t taskId) const;
        void completeTask(uint64_t taskId);
        void failTask(uint64_t taskId, const std::string& error);
        void requestCancel(uint64_t taskId);

        bool isActive(app::models::TaskSource source) const;
        bool hasActiveTasks() const;

        const std::vector<TaskRecord>& getTasks() const { return m_tasks; }
        const ZemaxDDE::OperationInfo* findDdeOp(uint64_t ddeId) const;

    private:
        ZemaxDDE::OperationMonitor* m_monitor = nullptr;
        std::vector<TaskRecord> m_tasks;
        uint64_t m_nextId = 1;

        TaskRecord* findRecord(uint64_t taskId);
        const TaskRecord* findRecord(uint64_t taskId) const;
    };
}
