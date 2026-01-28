#pragma once

#include "TestFixtures.h"
#include "Vehicle.h"

/// Tests for Vehicle MAV command sending with signal-based results.
/// Uses UnitTest directly since tests iterate through test cases that each need
/// their own connection lifecycle.
class SendMavCommandWithSignallingTest : public UnitTest
{
    Q_OBJECT

public:
    SendMavCommandWithSignallingTest() = default;

private slots:
    void cleanup() override;

    void _performTestCases();
    void _duplicateCommand();

private:
    struct TestCase_t {
        MAV_CMD command;
        MAV_RESULT expectedCommandResult;
        Vehicle::MavCmdResultFailureCode_t expectedFailureCode;
        int expectedSendCount;
    };

    void _testCaseWorker(TestCase_t& testCase);

    static TestCase_t _rgTestCases[];
};
