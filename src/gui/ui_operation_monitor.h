#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "dde/operation_monitor.h"

namespace gui {

enum class TaskSource {
    None,
    NominalSurfaceProfile,
    TolerancedSurfaceProfile,
    SurfaceMapAnalysis
};

class UiOperationMonitor {
public:
    void setMonitor(ZemaxDDE::OperationMonitor* monitor) { m_monitor = monitor; }

    uint64_t startTask(TaskSource source, const std::string& label, int totalSteps);
    void reportProgress(uint64_t taskId, int currentStep, const std::string& message);
    bool isCancelled(uint64_t taskId) const;
    void completeTask(uint64_t taskId);
    void failTask(uint64_t taskId, const std::string& error);
    void requestCancel(uint64_t taskId);
    uint64_t getDdeOperationId(uint64_t taskId) const;

    bool isActive(TaskSource source) const;
    bool hasActiveTasks() const;

    void renderGlobalStatusBar();

private:
    struct TaskRecord {
        uint64_t taskId;
        TaskSource source;
        std::string label;
        uint64_t ddeOperationId;
    };

    ZemaxDDE::OperationMonitor* m_monitor = nullptr;
    std::vector<TaskRecord> m_tasks;
    uint64_t m_nextId = 1;

    TaskRecord* findRecord(uint64_t taskId);
    const TaskRecord* findRecord(uint64_t taskId) const;
    const ZemaxDDE::OperationInfo* findDdeOp(uint64_t ddeId) const;
};

}
