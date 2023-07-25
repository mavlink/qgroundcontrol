/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SendMavCommandWithHandlerTest.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "MockLink.h"

SendMavCommandWithHandlerTest::TestCase_t SendMavCommandWithHandlerTest::_rgTestCases[] = {
    {  MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED,           MAV_RESULT_ACCEPTED,    0,   Vehicle::MavCmdResultCommandResultOnly,             1 },
    {  MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED,             MAV_RESULT_FAILED,      0,   Vehicle::MavCmdResultCommandResultOnly,             1 },
    {  MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED,   MAV_RESULT_ACCEPTED,    0,   Vehicle::MavCmdResultCommandResultOnly,             2 },
    {  MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED,     MAV_RESULT_FAILED,      0,   Vehicle::MavCmdResultCommandResultOnly,             2 },
    {  MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE,                      MAV_RESULT_FAILED,      0,   Vehicle::MavCmdResultFailureNoResponseToCommand,    Vehicle::_mavCommandMaxRetryCount },
    {  MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE_NO_RETRY,             MAV_RESULT_FAILED,      0,   Vehicle::MavCmdResultFailureNoResponseToCommand,    1 },
};

bool SendMavCommandWithHandlerTest::_handlerCalled = false;

void SendMavCommandWithHandlerTest::_mavCmdResultHandler(void* resultHandlerData, int compId, MAV_RESULT commandResult, uint8_t progress, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    TestCase_t* testCase = static_cast<TestCase_t*>(resultHandlerData);

    _handlerCalled = true;

    QCOMPARE(compId,                            MAV_COMP_ID_AUTOPILOT1);
    QCOMPARE(testCase->expectedCommandResult,   commandResult);
    QCOMPARE(testCase->progress,                progress);
    QCOMPARE(testCase->expectedFailureCode,     failureCode);
}

void SendMavCommandWithHandlerTest::_testCaseWorker(TestCase_t& testCase)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();

    _handlerCalled = false;
    _mockLink->clearSendMavCommandCounts();
    vehicle->sendMavCommandWithHandler(_mavCmdResultHandler, &testCase, MAV_COMP_ID_AUTOPILOT1, testCase.command);
    QVERIFY(QTest::qWaitFor([&]() { return _handlerCalled; }, 10000));
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, testCase.command),  -1);
    QCOMPARE(_mockLink->sendMavCommandCount(testCase.command),                  testCase.expectedSendCount);

    _disconnectMockLink();
}

void SendMavCommandWithHandlerTest::_performTestCases(void)
{
    int index = 0;
    for (TestCase_t& testCase: _rgTestCases) {
        qDebug() << "Testing case" << index++;
        _testCaseWorker(testCase);
    }
}

void SendMavCommandWithHandlerTest::_duplicateCommand(void)
{
    _connectMockLinkNoInitialConnectSequence();

    SendMavCommandWithHandlerTest::TestCase_t testCase = {
        MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, MAV_RESULT_FAILED, 0, Vehicle::MavCmdResultFailureDuplicateCommand, 1
    };

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();

    _handlerCalled = false;
    _mockLink->clearSendMavCommandCounts();
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, testCase.command, true /* showError */);
    QVERIFY(QTest::qWaitFor([&]() { return _mockLink->sendMavCommandCount(testCase.command) == 1; }, 10));
    QVERIFY(!_handlerCalled);
    vehicle->sendMavCommandWithHandler(_mavCmdResultHandler, &testCase, MAV_COMP_ID_AUTOPILOT1, testCase.command);

    // Duplicate command response should happen immediately
    QVERIFY(_handlerCalled);
    QVERIFY(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, testCase.command) != -1);
    QCOMPARE(_mockLink->sendMavCommandCount(testCase.command), 1);
}

void SendMavCommandWithHandlerTest::_compIdAllMavCmdResultHandler(void* /*resultHandlerData*/, int compId, MAV_RESULT commandResult, uint8_t progress, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    _handlerCalled = true;

    QCOMPARE(compId,            MAV_COMP_ID_ALL);
    QCOMPARE(commandResult,     MAV_RESULT_FAILED);
    QCOMPARE(progress,          0);
    QCOMPARE(failureCode,       Vehicle::MavCmdResultCommandResultOnly);
}

void SendMavCommandWithHandlerTest::_compIdAllFailure(void)
{
    _connectMockLinkNoInitialConnectSequence();

    SendMavCommandWithHandlerTest::TestCase_t testCase = {
        MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, MAV_RESULT_FAILED, 0, Vehicle::MavCmdResultFailureDuplicateCommand, 0
    };

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();

    _handlerCalled = false;
    _mockLink->clearSendMavCommandCounts();
    vehicle->sendMavCommandWithHandler(_compIdAllMavCmdResultHandler, nullptr, MAV_COMP_ID_ALL, testCase.command);
    QCOMPARE(_handlerCalled,                                                            true);
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_ALL, testCase.command), -1);
    QCOMPARE(_mockLink->sendMavCommandCount(testCase.command),                          testCase.expectedSendCount);

    _disconnectMockLink();
}
