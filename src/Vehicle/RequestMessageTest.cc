/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RequestMessageTest.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "MockLink.h"

RequestMessageTest::TestCase_t RequestMessageTest::_rgTestCases[] = {
    {  MockLink::FailRequestMessageNone,                                MAV_RESULT_ACCEPTED,    Vehicle::RequestMessageNoFailure,                   1,                                  false },
    {  MockLink::FailRequestMessageCommandAcceptedMsgNotSent,           MAV_RESULT_FAILED,      Vehicle::RequestMessageFailureMessageNotReceived,   1,                                  false },
    {  MockLink::FailRequestMessageCommandUnsupported,                  MAV_RESULT_UNSUPPORTED, Vehicle::RequestMessageFailureCommandError,         1,                                  false },
    {  MockLink::FailRequestMessageCommandNoResponse,                   MAV_RESULT_FAILED,      Vehicle::RequestMessageFailureCommandNotAcked,      Vehicle::_mavCommandMaxRetryCount,  false },
};

void RequestMessageTest::_requestMessageResultHandler(void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message)
{
    TestCase_t* testCase = static_cast<TestCase_t*>(resultHandlerData);

    testCase->resultHandlerCalled = true;
    QCOMPARE((int)testCase->expectedCommandResult, (int)commandResult);
    QCOMPARE((int)testCase->expectedFailureCode, (int)failureCode);
    if (testCase->expectedFailureCode == Vehicle::RequestMessageNoFailure) {
        QVERIFY(message.msgid == MAVLINK_MSG_ID_DEBUG);
    }
}

void RequestMessageTest::_testCaseWorker(TestCase_t& testCase)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();

    _mockLink->clearSendMavCommandCounts();
    _mockLink->setRequestMessageFailureMode(testCase.failureMode);

    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);
    QVERIFY(QTest::qWaitFor([&]() { return testCase.resultHandlerCalled; }, 10000));
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE),   -1);
    QCOMPARE(_mockLink->sendMavCommandCount(MAV_CMD_REQUEST_MESSAGE),                                   testCase.expectedSendCount);

    // We should be able to do it twice in a row without any duplicate command problems
    testCase.resultHandlerCalled = false;
    _mockLink->clearSendMavCommandCounts();
    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);
    QVERIFY(QTest::qWaitFor([&]() { return testCase.resultHandlerCalled; }, 10000));
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE),   -1);
    QCOMPARE(_mockLink->sendMavCommandCount(MAV_CMD_REQUEST_MESSAGE),                                   testCase.expectedSendCount);

    _disconnectMockLink();
}

void RequestMessageTest::_performTestCases(void)
{
    int index = 0;
    for (TestCase_t& testCase: _rgTestCases) {
        qDebug() << "Testing case" << index++;
        _testCaseWorker(testCase);
    }
}

void RequestMessageTest::_duplicateCommand(void)
{
    _connectMockLinkNoInitialConnectSequence();

    RequestMessageTest::TestCase_t testCase = {
        MockLink::FailRequestMessageCommandNoResponse, MAV_RESULT_FAILED, Vehicle::RequestMessageFailureDuplicateCommand, 1, false
    };

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();

    _mockLink->clearSendMavCommandCounts();
    _mockLink->setRequestMessageFailureMode(testCase.failureMode);

    QVERIFY(false == vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE));

    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);
    QVERIFY(QTest::qWaitFor([&]() { return _mockLink->sendMavCommandCount(MAV_CMD_REQUEST_MESSAGE) == 1; }, 10));
    QCOMPARE(testCase.resultHandlerCalled, false);
    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);

    // Duplicate command returns immediately
    QCOMPARE(testCase.resultHandlerCalled,                                                              true);
    QCOMPARE(_mockLink->sendMavCommandCount(MAV_CMD_REQUEST_MESSAGE),                                   testCase.expectedSendCount);
    QVERIFY(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE) != -1);
    QVERIFY(true == vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE));

    // MockLink does not ack messages?
    // So wait for Vehicle to exhaust retries and then report that failure.
    // We should then observe that the command is no longer pending and may send again.
    testCase.resultHandlerCalled = false;
    testCase.expectedFailureCode = Vehicle::RequestMessageFailureCommandNotAcked;
    auto timeout = Vehicle::_mavCommandMaxRetryCount * (Vehicle::_mavCommandResponseCheckTimeoutMSecs + Vehicle::_mavCommandAckTimeoutMSecs);
    QVERIFY(QTest::qWaitFor([&]() { return testCase.resultHandlerCalled; }, timeout));
    QVERIFY(false == vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE));
}

void RequestMessageTest::_compIdAllRequestMessageResultHandler(void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& /*message*/)
{
    TestCase_t* testCase = static_cast<TestCase_t*>(resultHandlerData);

    testCase->resultHandlerCalled = true;
    QCOMPARE((int)testCase->expectedCommandResult, (int)commandResult);
    QCOMPARE((int)testCase->expectedFailureCode, (int)failureCode);
}

void RequestMessageTest::_compIdAllFailure(void)
{
    _connectMockLinkNoInitialConnectSequence();

    RequestMessageTest::TestCase_t testCase = {
        MockLink::FailRequestMessageCommandNoResponse, MAV_RESULT_FAILED, Vehicle::RequestMessageFailureCommandError, 0, false
    };

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();

    _mockLink->clearSendMavCommandCounts();
    _mockLink->setRequestMessageFailureMode(testCase.failureMode);

    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_ALL, MAVLINK_MSG_ID_DEBUG);
    QCOMPARE(testCase.resultHandlerCalled,                                                      true);
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_ALL, MAV_CMD_REQUEST_MESSAGE),  -1);
    QCOMPARE(_mockLink->sendMavCommandCount(MAV_CMD_REQUEST_MESSAGE),                           0);

    _disconnectMockLink();
}
