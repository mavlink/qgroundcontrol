#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"
#include "Vehicle.h"

class RequestMessageTest : public VehicleTestManualConnect
{
    Q_OBJECT

signals:
    void resultHandlerCalled();

private slots:
    void init() override;
    void _performTestCases();
    void _performTestCasesStress();
    void _compIdAllFailure();
    void _duplicateCommand();

private:
    typedef struct
    {
        MockLink::RequestMessageFailureMode_t failureMode;
        MAV_RESULT expectedCommandResult;
        Vehicle::RequestMessageResultHandlerFailureCode_t expectedFailureCode;
        int expectedSendCount;
        bool resultHandlerCalled;
    } TestCase_t;

    void _testCaseWorker(TestCase_t& testCase);

    static void _requestMessageResultHandler(void* resultHandlerData, MAV_RESULT commandResult,
                                             Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
                                             const mavlink_message_t& message);
    static void _compIdAllRequestMessageResultHandler(void* resultHandlerData, MAV_RESULT commandResult,
                                                      Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
                                                      const mavlink_message_t& message);

    bool _resultHandlerCalled;

    static TestCase_t _rgTestCases[];
};
