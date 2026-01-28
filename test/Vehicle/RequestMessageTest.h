#pragma once

#include "UnitTest.h"
#include "MAVLinkLib.h"
#include "Vehicle.h"

/// Tests for Vehicle request message functionality.
/// Uses UnitTest directly since tests iterate through test cases that each need
/// their own connection lifecycle.
class RequestMessageTest : public UnitTest
{
    Q_OBJECT

public:
    RequestMessageTest() = default;

signals:
    void resultHandlerCalled();

private slots:
    void cleanup() override;

    void _performTestCases();
    void _compIdAllFailure();
    void _duplicateCommand();

private:
    struct TestCase_t {
        MockLink::RequestMessageFailureMode_t failureMode;
        MAV_RESULT expectedCommandResult;
        Vehicle::RequestMessageResultHandlerFailureCode_t expectedFailureCode;
        int expectedSendCount;
        bool resultHandlerCalled;
    };

    void _testCaseWorker(TestCase_t& testCase);

    static void _requestMessageResultHandler(void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message);
    static void _compIdAllRequestMessageResultHandler(void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message);

    bool _resultHandlerCalled;

    static TestCase_t _rgTestCases[];
};
