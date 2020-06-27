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
    {  MockLink::FailRequestMessageNone,                                MAV_RESULT_ACCEPTED,    Vehicle::RequestMessageNoFailure,                   false },
    {  MockLink::FailRequestMessageCommandAcceptedMsgNotSent,           MAV_RESULT_FAILED,      Vehicle::RequestMessageFailureMessageNotReceived,   false },
    {  MockLink::FailRequestMessageCommandUnsupported,                  MAV_RESULT_UNSUPPORTED, Vehicle::RequestMessageFailureCommandError,         false },
    {  MockLink::FailRequestMessageCommandNoResponse,                   MAV_RESULT_FAILED,      Vehicle::RequestMessageFailureCommandNotAcked,      false },
    //{  MockLink::FailRequestMessageCommandAcceptedSecondAttempMsgSent,  MAV_RESULT_FAILED,      Vehicle::RequestMessageNoFailure,   false },
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

    _mockLink->setRequestMessageFailureMode(testCase.failureMode);
    vehicle->requestMessage(_requestMessageResultHandler, &testCase, MAV_COMP_ID_ALL, MAVLINK_MSG_ID_DEBUG);
    for (int i=0; i<100; i++) {
        QTest::qWait(100);
        if (testCase.resultHandlerCalled) {
            break;
        }
    }
    QVERIFY(testCase.resultHandlerCalled);

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
