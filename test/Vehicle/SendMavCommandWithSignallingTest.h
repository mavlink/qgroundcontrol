#pragma once

#include "BaseClasses/VehicleTest.h"
#include "Vehicle.h"

class SendMavCommandWithSignallingTest : public VehicleTestNoInitialConnect
{
    Q_OBJECT

public:
    explicit SendMavCommandWithSignallingTest(QObject* parent = nullptr)
        : VehicleTestNoInitialConnect(parent)
    {
        setAutopilotType(MAV_AUTOPILOT_INVALID);
    }

private slots:
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
