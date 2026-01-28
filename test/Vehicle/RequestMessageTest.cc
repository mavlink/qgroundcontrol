#include "RequestMessageTest.h"
#include "TestHelpers.h"
#include "MultiVehicleManager.h"
#include "MockLink.h"

#include <QtTest/QTest>

RequestMessageTest::TestCase_t RequestMessageTest::_rgTestCases[] = {
    {  MockLink::FailRequestMessageNone,                                MAV_RESULT_ACCEPTED,    Vehicle::RequestMessageNoFailure,                   1,                                  false },
    {  MockLink::FailRequestMessageCommandAcceptedMsgNotSent,           MAV_RESULT_FAILED,      Vehicle::RequestMessageFailureMessageNotReceived,   1,                                  false },
    {  MockLink::FailRequestMessageCommandUnsupported,                  MAV_RESULT_UNSUPPORTED, Vehicle::RequestMessageFailureCommandError,         1,                                  false },
    {  MockLink::FailRequestMessageCommandNoResponse,                   MAV_RESULT_FAILED,      Vehicle::RequestMessageFailureCommandNotAcked,      Vehicle::_mavCommandMaxRetryCount,  false },
};

void RequestMessageTest::cleanup()
{
    _disconnectMockLink();
    UnitTest::cleanup();
}

void RequestMessageTest::_requestMessageResultHandler(void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message)
{
    TestCase_t* testCase = static_cast<TestCase_t*>(resultHandlerData);

    testCase->resultHandlerCalled = true;
    QCOMPARE_EQ(static_cast<int>(commandResult), static_cast<int>(testCase->expectedCommandResult));
    QCOMPARE_EQ(static_cast<int>(failureCode), static_cast<int>(testCase->expectedFailureCode));
    if (testCase->expectedFailureCode == Vehicle::RequestMessageNoFailure) {
        QCOMPARE_EQ(message.msgid, static_cast<decltype(message.msgid)>(MAVLINK_MSG_ID_DEBUG));
    }
}

void RequestMessageTest::_testCaseWorker(TestCase_t& testCase)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager*    vehicleMgr  = MultiVehicleManager::instance();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();
    VERIFY_NOT_NULL(vehicle);

    // Gimbal controller sends message requests when receiving heartbeats, trying to find a gimbal, and it messes with this test so we disable it
    vehicle->_deleteGimbalController();

    // Camera manager also messes with it.
    vehicle->_deleteCameraManager();

    _mockLink->clearReceivedMavCommandCounts();
    _mockLink->setRequestMessageFailureMode(testCase.failureMode);

    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);
    WAIT_FOR_CONDITION(testCase.resultHandlerCalled, Vehicle::kTestMavCommandMaxWaitMs);
    QCOMPARE_EQ(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE), -1);
    QCOMPARE_EQ(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), testCase.expectedSendCount);

    // We should be able to do it twice in a row without any duplicate command problems
    testCase.resultHandlerCalled = false;
    _mockLink->clearReceivedMavCommandCounts();
    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);
    WAIT_FOR_CONDITION(testCase.resultHandlerCalled, Vehicle::kTestMavCommandMaxWaitMs);
    QCOMPARE_EQ(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE), -1);
    QCOMPARE_EQ(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), testCase.expectedSendCount);

    _disconnectMockLink();
}

void RequestMessageTest::_performTestCases()
{
    TestHelpers::runTestCases(_rgTestCases, [this](TestCase_t& testCase) {
        _testCaseWorker(testCase);
    });
}

void RequestMessageTest::_duplicateCommand()
{
    _connectMockLinkNoInitialConnectSequence();

    RequestMessageTest::TestCase_t testCase = {
        MockLink::FailRequestMessageCommandNoResponse, MAV_RESULT_FAILED, Vehicle::RequestMessageFailureDuplicateCommand, 1, false
    };

    MultiVehicleManager*    vehicleMgr  = MultiVehicleManager::instance();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();
    VERIFY_NOT_NULL(vehicle);

    _mockLink->clearReceivedMavCommandCounts();
    _mockLink->setRequestMessageFailureMode(testCase.failureMode);

    QCOMPARE_EQ(vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE), false);

    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);
    // Wait for command to be sent - use reasonable timeout for CI systems
    WAIT_FOR_CONDITION(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE) == 1, TestHelpers::kShortTimeoutMs);
    QCOMPARE_EQ(testCase.resultHandlerCalled, false);
    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);

    // Duplicate command returns immediately
    QCOMPARE_EQ(testCase.resultHandlerCalled, true);
    QCOMPARE_EQ(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), testCase.expectedSendCount);
    QCOMPARE_NE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE), -1);
    QCOMPARE_EQ(vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE), true);

    // MockLink does not ack messages in NoResponse mode
    // So wait for Vehicle to exhaust retries and then report that failure.
    // We should then observe that the command is no longer pending and may send again.
    testCase.resultHandlerCalled = false;
    testCase.expectedFailureCode = Vehicle::RequestMessageFailureCommandNotAcked;
    auto timeout = Vehicle::_mavCommandMaxRetryCount * (Vehicle::_mavCommandResponseCheckTimeoutMSecs() + Vehicle::_mavCommandAckTimeoutMSecs());
    WAIT_FOR_CONDITION(testCase.resultHandlerCalled, timeout);
    QCOMPARE_EQ(vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE), false);
}

void RequestMessageTest::_compIdAllRequestMessageResultHandler(void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& /*message*/)
{
    TestCase_t* testCase = static_cast<TestCase_t*>(resultHandlerData);

    testCase->resultHandlerCalled = true;
    QCOMPARE_EQ(static_cast<int>(commandResult), static_cast<int>(testCase->expectedCommandResult));
    QCOMPARE_EQ(static_cast<int>(failureCode), static_cast<int>(testCase->expectedFailureCode));
}

void RequestMessageTest::_compIdAllFailure()
{
    _connectMockLinkNoInitialConnectSequence();

    RequestMessageTest::TestCase_t testCase = {
        MockLink::FailRequestMessageCommandNoResponse, MAV_RESULT_FAILED, Vehicle::RequestMessageFailureCommandError, 0, false
    };

    MultiVehicleManager*    vehicleMgr  = MultiVehicleManager::instance();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();
    VERIFY_NOT_NULL(vehicle);

    _mockLink->clearReceivedMavCommandCounts();
    _mockLink->setRequestMessageFailureMode(testCase.failureMode);

    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_ALL, MAVLINK_MSG_ID_DEBUG);
    QCOMPARE_EQ(testCase.resultHandlerCalled, true);
    QCOMPARE_EQ(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_ALL, MAV_CMD_REQUEST_MESSAGE), -1);
    QCOMPARE_EQ(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), 0);
}
