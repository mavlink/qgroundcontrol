#pragma once

#include "TestFixtures.h"
#include "Vehicle.h"

/// Tests for Vehicle MAV command sending with handler callbacks.
/// Uses UnitTest directly since tests iterate through test cases that each need
/// their own connection lifecycle.
class SendMavCommandWithHandlerTest : public UnitTest
{
    Q_OBJECT

public:
    SendMavCommandWithHandlerTest() = default;

private slots:
    void cleanup() override;

    void _performTestCases();
    void _compIdAllFailure();
    void _duplicateCommand();

private:
    struct TestCase_t {
        MAV_CMD command;
        MAV_RESULT expectedCommandResult;
        bool expectInProgressResult;
        Vehicle::MavCmdResultFailureCode_t expectedFailureCode;
        int expectedSendCount;
    };

    void _testCaseWorker(TestCase_t& testCase);

    static void _mavCmdResultHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode);
    static void _mavCmdProgressHandler(void* progressHandlerData, int compId, const mavlink_command_ack_t& ack);
    static void _compIdAllFailureMavCmdResultHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode);

    static bool _resultHandlerCalled;
    static bool _progressHandlerCalled;

    static TestCase_t _rgTestCases[];
};
