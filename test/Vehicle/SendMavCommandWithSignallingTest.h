#pragma once

#include "UnitTest.h"
#include "Vehicle.h"

class SendMavCommandWithSignallingTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _performTestCases(void);
    void _duplicateCommand(void);

private:
    typedef struct {
        MAV_CMD                             command;
        MAV_RESULT                          expectedCommandResult;
        Vehicle::MavCmdResultFailureCode_t  expectedFailureCode;
        int                                 expectedSendCount;
    } TestCase_t;

    void _testCaseWorker(TestCase_t& testCase);

    static TestCase_t _rgTestCases[];
};
