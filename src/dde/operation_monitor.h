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
    TimedOut
};

struct OperationInfo {
    uint64_t id;
    std::string command;
    OperationStatus status = OperationStatus::Pending;
    DWORD startTime = 0;
    DWORD elapsed = 0;
    std::string error;
};

class OperationMonitor {
public:
    uint64_t registerRequest(const std::string& command);
    void updateStatus(uint64_t id, OperationStatus status, const std::string& error = "");
    const std::vector<OperationInfo>& getOperations() const { return m_operations; }
    void clearCompleted();

private:
    std::vector<OperationInfo> m_operations;
    uint64_t m_nextId = 1;

    int findIndex(uint64_t id);
};

} // namespace ZemaxDDE