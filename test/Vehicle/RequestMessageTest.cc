#include "RequestMessageTest.h"

#include "MultiVehicleManager.h"
RequestMessageTest::TestCase_t RequestMessageTest::_rgTestCases[] = {
    {MockLink::FailRequestMessageNone, MAV_RESULT_ACCEPTED, Vehicle::RequestMessageNoFailure, 1, MAVLINK_MSG_ID_DEBUG, false, 0},
    {MockLink::FailRequestMessageCommandAcceptedMsgNotSent, MAV_RESULT_FAILED,
     Vehicle::RequestMessageFailureMessageNotReceived, 1, MAVLINK_MSG_ID_DEBUG, false, 0},
    {MockLink::FailRequestMessageCommandUnsupported, MAV_RESULT_UNSUPPORTED, Vehicle::RequestMessageFailureCommandError,
     1, MAVLINK_MSG_ID_DEBUG, false, 0},
    {MockLink::FailRequestMessageCommandNoResponse, MAV_RESULT_FAILED, Vehicle::RequestMessageFailureCommandNotAcked,
     Vehicle::_mavCommandMaxRetryCount, MAVLINK_MSG_ID_DEBUG, false, 0},
};

void RequestMessageTest::_requestMessageResultHandler(void* resultHandlerData, MAV_RESULT commandResult,
                                                      Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
                                                      const mavlink_message_t& message)
{
    TestCase_t* testCase = static_cast<TestCase_t*>(resultHandlerData);
    testCase->resultHandlerCalled = true;
    testCase->callbackCount++;
    QCOMPARE((int)testCase->expectedCommandResult, (int)commandResult);
    QCOMPARE((int)testCase->expectedFailureCode, (int)failureCode);
    if (testCase->expectedFailureCode == Vehicle::RequestMessageNoFailure) {
        QVERIFY(message.msgid == testCase->expectedMessageId);
    }
}

void RequestMessageTest::_testCaseWorker(TestCase_t& testCase)
{
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    // Gimbal controller sends message requests when receiving heartbeats, trying to find a gimbal, and it messes with
    // this test so we disable it
    vehicle->_deleteGimbalController();
    // Camera manager also messes with it.
    vehicle->_deleteCameraManager();
    _mockLink->clearReceivedMavCommandCounts();
    _mockLink->setRequestMessageFailureMode(testCase.failureMode);
    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);
    QVERIFY(QTest::qWaitFor([&]() { return testCase.resultHandlerCalled; }, 10000));
    QCOMPARE(testCase.callbackCount, 1);
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE), -1);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), testCase.expectedSendCount);
    // We should be able to do it twice in a row without any duplicate command problems
    testCase.resultHandlerCalled = false;
    testCase.callbackCount = 0;
    _mockLink->clearReceivedMavCommandCounts();
    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);
    QVERIFY(QTest::qWaitFor([&]() { return testCase.resultHandlerCalled; }, 10000));
    QCOMPARE(testCase.callbackCount, 1);
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE), -1);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), testCase.expectedSendCount);
}

void RequestMessageTest::_performTestCases()
{
    int index = 0;
    for (TestCase_t& testCase : _rgTestCases) {
        qDebug() << "Testing case" << index++;
        _testCaseWorker(testCase);
    }
}

void RequestMessageTest::_duplicateCommand()
{
    RequestMessageTest::TestCase_t firstRequestCase = {MockLink::FailRequestMessageCommandAcceptedMsgNotSent, MAV_RESULT_FAILED,
                                                                                                             Vehicle::RequestMessageFailureMessageNotReceived, 1, MAVLINK_MSG_ID_DEBUG, false, 0};
    RequestMessageTest::TestCase_t exactDuplicateCase = {MockLink::FailRequestMessageCommandAcceptedMsgNotSent, MAV_RESULT_FAILED,
                                                                                                                 Vehicle::RequestMessageFailureDuplicate, 1, MAVLINK_MSG_ID_DEBUG, false, 0};
    RequestMessageTest::TestCase_t queuedRequestCase = {MockLink::FailRequestMessageNone, MAV_RESULT_ACCEPTED,
                                                                                                                Vehicle::RequestMessageNoFailure, 2, MAVLINK_MSG_ID_AUTOPILOT_VERSION, false, 0};

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    _mockLink->clearReceivedMavCommandCounts();
    _mockLink->setRequestMessageFailureMode(firstRequestCase.failureMode);
    QVERIFY(false == vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE));
    vehicle->requestMessage(_requestMessageResultHandler, &firstRequestCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);
    QVERIFY(QTest::qWaitFor([&]() { return _mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE) == 1; }, 10));
    QCOMPARE(firstRequestCase.resultHandlerCalled, false);

    // Exact duplicate request should fail immediately and should not send a second MAV_CMD_REQUEST_MESSAGE.
    vehicle->requestMessage(_requestMessageResultHandler, &exactDuplicateCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);
    QCOMPARE(exactDuplicateCase.resultHandlerCalled, true);
    QCOMPARE(exactDuplicateCase.callbackCount, 1);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), exactDuplicateCase.expectedSendCount);

    // Different message id should queue and only send after the first request is done.
    vehicle->requestMessage(_requestMessageResultHandler, &queuedRequestCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_AUTOPILOT_VERSION);
    QCOMPARE(queuedRequestCase.resultHandlerCalled, false);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), exactDuplicateCase.expectedSendCount);

    // Let queued request succeed once it is dequeued after first request message-timeout.
    _mockLink->setRequestMessageFailureMode(MockLink::FailRequestMessageNone);

    // Wait for the first request to fail with MessageNotReceived.
    QVERIFY(QTest::qWaitFor([&]() { return firstRequestCase.resultHandlerCalled; }, 3000));
    QCOMPARE(firstRequestCase.callbackCount, 1);

    // Queued request should now be sent and complete successfully.
    QVERIFY(QTest::qWaitFor([&]() { return queuedRequestCase.resultHandlerCalled; }, 1000));
    QCOMPARE(queuedRequestCase.callbackCount, 1);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), queuedRequestCase.expectedSendCount);

    QVERIFY(false == vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE));
}

void RequestMessageTest::_compIdAllRequestMessageResultHandler(
    void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
    const mavlink_message_t& /*message*/)
{
    TestCase_t* testCase = static_cast<TestCase_t*>(resultHandlerData);
    testCase->resultHandlerCalled = true;
    testCase->callbackCount++;
    QCOMPARE((int)testCase->expectedCommandResult, (int)commandResult);
    QCOMPARE((int)testCase->expectedFailureCode, (int)failureCode);
}

void RequestMessageTest::_compIdAllFailure()
{
    RequestMessageTest::TestCase_t testCase = {MockLink::FailRequestMessageCommandNoResponse, MAV_RESULT_FAILED,
                                               Vehicle::RequestMessageFailureCommandError, 0, MAVLINK_MSG_ID_DEBUG, false, 0};
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    _mockLink->clearReceivedMavCommandCounts();
    _mockLink->setRequestMessageFailureMode(testCase.failureMode);
    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_ALL, MAVLINK_MSG_ID_DEBUG);
    QCOMPARE(testCase.resultHandlerCalled, true);
    QCOMPARE(testCase.callbackCount, 1);
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_ALL, MAV_CMD_REQUEST_MESSAGE), -1);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), 0);
}

void RequestMessageTest::_duplicateWhileQueued()
{
    RequestMessageTest::TestCase_t firstRequestCase = {MockLink::FailRequestMessageCommandAcceptedMsgNotSent, MAV_RESULT_FAILED,
                                                       Vehicle::RequestMessageFailureMessageNotReceived, 1, MAVLINK_MSG_ID_DEBUG, false, 0};
    RequestMessageTest::TestCase_t queuedRequestCase = {MockLink::FailRequestMessageNone, MAV_RESULT_ACCEPTED,
                                                        Vehicle::RequestMessageNoFailure, 2, MAVLINK_MSG_ID_AUTOPILOT_VERSION, false, 0};
    RequestMessageTest::TestCase_t duplicateQueuedCase = {MockLink::FailRequestMessageCommandAcceptedMsgNotSent, MAV_RESULT_FAILED,
                                                          Vehicle::RequestMessageFailureDuplicate, 1, MAVLINK_MSG_ID_AUTOPILOT_VERSION, false, 0};

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    _mockLink->clearReceivedMavCommandCounts();
    _mockLink->setRequestMessageFailureMode(firstRequestCase.failureMode);

    vehicle->requestMessage(_requestMessageResultHandler, &firstRequestCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);
    QVERIFY(QTest::qWaitFor([&]() { return _mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE) == 1; }, 50));

    vehicle->requestMessage(_requestMessageResultHandler, &queuedRequestCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_AUTOPILOT_VERSION);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), 1);
    QCOMPARE(queuedRequestCase.resultHandlerCalled, false);

    // Duplicate of an already queued request should fail immediately and should not send another command.
    vehicle->requestMessage(_requestMessageResultHandler, &duplicateQueuedCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_AUTOPILOT_VERSION);
    QCOMPARE(duplicateQueuedCase.resultHandlerCalled, true);
    QCOMPARE(duplicateQueuedCase.callbackCount, 1);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), 1);

    // Allow queued request to succeed once first request has completed its message timeout path.
    _mockLink->setRequestMessageFailureMode(MockLink::FailRequestMessageNone);
    QVERIFY(QTest::qWaitFor([&]() { return firstRequestCase.resultHandlerCalled; }, 3000));
    QCOMPARE(firstRequestCase.callbackCount, 1);

    QVERIFY(QTest::qWaitFor([&]() { return queuedRequestCase.resultHandlerCalled; }, 1000));
    QCOMPARE(queuedRequestCase.callbackCount, 1);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), queuedRequestCase.expectedSendCount);
}

UT_REGISTER_TEST(RequestMessageTest, TestLabel::Integration, TestLabel::Vehicle)
