#pragma once

#include "BaseClasses/VehicleTest.h"
#include "Vehicle.h"

class RequestMessageTest : public VehicleTestNoInitialConnect
{
    Q_OBJECT

public:
    explicit RequestMessageTest(QObject* parent = nullptr)
        : VehicleTestNoInitialConnect(parent)
    {
        setAutopilotType(MAV_AUTOPILOT_INVALID);
    }

signals:
    void resultHandlerCalled();

private slots:
    void _performTestCases();
    void _compIdAllFailure();
    void _duplicateCommand();
    void _duplicateWhileQueued();

private:
    typedef struct
    {
        MockLink::RequestMessageFailureMode_t failureMode;
        MAV_RESULT expectedCommandResult;
        Vehicle::RequestMessageResultHandlerFailureCode_t expectedFailureCode;
        int expectedSendCount;
        int expectedMessageId;
        bool resultHandlerCalled;
        int callbackCount;
    } TestCase_t;

    void _testCaseWorker(TestCase_t& testCase);

    static void _requestMessageResultHandler(void* resultHandlerData, MAV_RESULT commandResult,
                                             Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
                                             const mavlink_message_t& message);
    static void _compIdAllRequestMessageResultHandler(void* resultHandlerData, MAV_RESULT commandResult,
                                                      Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
                                                      const mavlink_message_t& message);

    static TestCase_t _rgTestCases[];
};
