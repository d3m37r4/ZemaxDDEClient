#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
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
            int clientSlot{-1};
            ZemaxDDE::OperationMonitor* boundMonitor{nullptr};
        };

        void setMonitor(ZemaxDDE::OperationMonitor* monitor) { m_monitor = monitor; }

        uint64_t startTask(app::models::TaskSource source, const std::string& label, int totalSteps, int clientSlot = -1);
        void reportProgress(uint64_t taskId, int currentStep, const std::string& message);
        bool isCancelled(uint64_t taskId) const;
        void completeTask(uint64_t taskId);
        void failTask(uint64_t taskId, const std::string& error);
        void requestCancel(uint64_t taskId);

        /// Fails and removes every task bound to the given client slot.
        /// Intended to be called when a DDE connection is torn down, so that
        /// records do not keep a dangling pointer to a destroyed OperationMonitor.
        /// Deliberately does NOT dereference boundMonitor (the client may already
        /// be gone or about to be destroyed) — it only drops the bookkeeping record.
        void failAllTasksOnSlot(int slot);

        struct TaskProgress {
            int currentStep;
            int totalSteps;
            int clientSlot{-1};
        };

        bool hasActiveTasks(std::optional<app::models::TaskSource> filter = std::nullopt) const;
        bool hasActiveTasksOnSlot(int slot, std::optional<app::models::TaskSource> filter = std::nullopt) const;
        int getTaskClientSlot(uint64_t taskId) const;
        std::optional<TaskProgress> getTaskProgress(app::models::TaskSource source) const;

        const std::unordered_map<uint64_t, TaskRecord>& getTasks() const { return m_tasks; }
        const ZemaxDDE::OperationInfo* findDdeOp(uint64_t ddeId) const;

    private:
        ZemaxDDE::OperationMonitor* m_monitor = nullptr;
        std::unordered_map<uint64_t, TaskRecord> m_tasks;
        uint64_t m_nextId = 1;

        TaskRecord* findRecord(uint64_t taskId);
        const TaskRecord* findRecord(uint64_t taskId) const;
    };
}
