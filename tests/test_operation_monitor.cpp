#include <gtest/gtest.h>

#include "dde/operation_monitor.h"

namespace ZemaxDDE {
namespace {

TEST(OperationMonitor, RegisterReturnsIncrementingId) {
    OperationMonitor mon;
    uint64_t id1 = mon.registerOperation("svc1", 5);
    uint64_t id2 = mon.registerOperation("svc2", 3);
    uint64_t id3 = mon.registerOperation("svc3", 1);

    EXPECT_EQ(id1, 1);
    EXPECT_EQ(id2, 2);
    EXPECT_EQ(id3, 3);
}

TEST(OperationMonitor, TransitionPendingToInFlightToCompleted) {
    OperationMonitor mon;
    uint64_t id = mon.registerOperation("svc", 3);

    EXPECT_EQ(mon.getOperations().size(), 1u);
    EXPECT_EQ(mon.getOperations()[0].status, OperationStatus::Pending);

    mon.onRequestQueued(id, "GetName");
    EXPECT_EQ(mon.getOperations()[0].status, OperationStatus::InFlight);
    EXPECT_EQ(mon.getOperations()[0].command, "GetName");

    mon.reportProgress(id, 1, "step 1");
    EXPECT_EQ(mon.getOperations()[0].currentStep, 1);

    mon.onCompleted(id);
    EXPECT_EQ(mon.getOperations()[0].status, OperationStatus::Completed);
    EXPECT_EQ(mon.getOperations()[0].message, "Completed");
}

TEST(OperationMonitor, TransitionToFailed) {
    OperationMonitor mon;
    uint64_t id = mon.registerOperation("svc", 1);
    mon.onRequestQueued(id, "GetFile");
    mon.onError(id, "timeout");

    EXPECT_EQ(mon.getOperations()[0].status, OperationStatus::Failed);
    EXPECT_EQ(mon.getOperations()[0].error, "timeout");
}

TEST(OperationMonitor, ClearCompletedRemovesDone) {
    OperationMonitor mon;
    uint64_t id1 = mon.registerOperation("svc", 1);
    uint64_t id2 = mon.registerOperation("svc", 1);
    uint64_t id3 = mon.registerOperation("svc", 1);

    mon.onCompleted(id1);
    mon.onError(id2, "err");
    mon.onRequestQueued(id3, "cmd");

    mon.clearCompleted();

    EXPECT_EQ(mon.getOperations().size(), 1u);
    EXPECT_EQ(mon.getOperations()[0].id, id3);
    EXPECT_EQ(mon.getOperations()[0].status, OperationStatus::InFlight);
}

TEST(OperationMonitor, IsCancelledFalseByDefault) {
    OperationMonitor mon;
    uint64_t id = mon.registerOperation("svc", 1);

    EXPECT_FALSE(mon.isCancelled(id));

    mon.requestCancel(id);
    EXPECT_TRUE(mon.isCancelled(id));
    EXPECT_EQ(mon.getOperations()[0].message, "Cancelling...");
}

} // namespace
} // namespace ZemaxDDE
