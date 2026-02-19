#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"
#include "Vehicle.h"

class SendMavCommandWithSignallingTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void init() override;
    void _performTestCases_data();
    void _performTestCases();
    void _duplicateCommand();

private:
    typedef struct
    {
        MAV_CMD command;
        MAV_RESULT expectedCommandResult;
        Vehicle::MavCmdResultFailureCode_t expectedFailureCode;
        int expectedSendCount;
    } TestCase_t;

    void _testCaseWorker(TestCase_t& testCase);

    static TestCase_t _rgTestCases[];
};
