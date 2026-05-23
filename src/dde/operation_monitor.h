#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>

namespace ZemaxDDE {

enum class OperationStatus {
    Pending,
    InFlight,
    Completed,
    Failed,
    Cancelled
};

struct OperationInfo {
    uint64_t id;
    std::string serviceId;
    std::string command;
    OperationStatus status = OperationStatus::Pending;
    DWORD startTime = 0;
    DWORD elapsed = 0;
    int currentStep = 0;
    int totalSteps = 0;
    std::string message;
    std::string error;
    bool cancelRequested = false;
};

class OperationMonitor {
public:
    uint64_t registerOperation(const std::string& serviceId, int totalSteps);
    void onRequestQueued(uint64_t operationId, const std::string& command);
    void reportProgress(uint64_t operationId, int currentStep, const std::string& message);
    void onCompleted(uint64_t operationId);
    void onError(uint64_t operationId, const std::string& error);
    void requestCancel(uint64_t operationId);
    bool isCancelled(uint64_t operationId) const;

    const std::vector<OperationInfo>& getOperations() const { return m_operations; }
    void clearCompleted();

private:
    std::vector<OperationInfo> m_operations;
    uint64_t m_nextId = 1;

    int findIndex(uint64_t id) const;
};

} // namespace ZemaxDDE
