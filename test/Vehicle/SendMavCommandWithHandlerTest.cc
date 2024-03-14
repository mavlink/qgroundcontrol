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
    {  MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED,           MAV_RESULT_ACCEPTED,    false,  Vehicle::MavCmdResultCommandResultOnly,             1 },
    {  MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED,             MAV_RESULT_FAILED,      false,  Vehicle::MavCmdResultCommandResultOnly,             1 },
    {  MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED,   MAV_RESULT_ACCEPTED,    false,  Vehicle::MavCmdResultCommandResultOnly,             2 },
    {  MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED,     MAV_RESULT_FAILED,      false,  Vehicle::MavCmdResultCommandResultOnly,             2 },
    {  MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE,                      MAV_RESULT_FAILED,      false,  Vehicle::MavCmdResultFailureNoResponseToCommand,    Vehicle::_mavCommandMaxRetryCount },
    {  MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE_NO_RETRY,             MAV_RESULT_FAILED,      false,  Vehicle::MavCmdResultFailureNoResponseToCommand,    1 },
    {  MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_ACCEPTED,      MAV_RESULT_ACCEPTED,    true,   Vehicle::MavCmdResultCommandResultOnly,             1 },
    {  MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_FAILED,        MAV_RESULT_FAILED,      true,   Vehicle::MavCmdResultCommandResultOnly,             1 },
    {  MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_NO_ACK,        MAV_RESULT_FAILED,      true,   Vehicle::MavCmdResultFailureNoResponseToCommand,    1 },
};

bool SendMavCommandWithHandlerTest::_resultHandlerCalled        = false;
bool SendMavCommandWithHandlerTest::_progressHandlerCalled    = false;

void SendMavCommandWithHandlerTest::_mavCmdResultHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    TestCase_t* testCase = static_cast<TestCase_t*>(resultHandlerData);

    _resultHandlerCalled = true;

    QCOMPARE(MAV_COMP_ID_AUTOPILOT1,            compId);
    QCOMPARE(testCase->expectedCommandResult,   ack.result);
    QCOMPARE(testCase->expectedFailureCode,     failureCode);
}

void SendMavCommandWithHandlerTest::_mavCmdProgressHandler(void* progressHandlerData, int compId, const mavlink_command_ack_t& ack)
{
    TestCase_t*             testCase    = static_cast<TestCase_t*>(progressHandlerData);
    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();

    _progressHandlerCalled = true;

    QCOMPARE(MAV_COMP_ID_AUTOPILOT1,                    compId);
    QCOMPARE(MAV_RESULT_IN_PROGRESS,                    ack.result);
    QCOMPARE(1,                                         ack.progress);

    // Command should still be in list
    QVERIFY(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, testCase->command) != -1);
}

void SendMavCommandWithHandlerTest::_testCaseWorker(TestCase_t& testCase)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();

    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler       = _mavCmdResultHandler;
    handlerInfo.resultHandlerData   = &testCase;
    handlerInfo.progressHandler     = _mavCmdProgressHandler;
    handlerInfo.progressHandlerData = &testCase;

    _resultHandlerCalled    = false;
    _progressHandlerCalled  = false;

    _mockLink->clearReceivedMavCommandCounts();
    vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_AUTOPILOT1, testCase.command);

    if (testCase.expectInProgressResult) {
        QVERIFY(QTest::qWaitFor([&]() { return _progressHandlerCalled; }, 10000));
    }
    
    QVERIFY(QTest::qWaitFor([&]() { return _resultHandlerCalled; }, 10000));
    QCOMPARE(_mockLink->receivedMavCommandCount(testCase.command), testCase.expectedSendCount);
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, testCase.command), -1);

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

    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler       = _mavCmdResultHandler;
    handlerInfo.resultHandlerData   = &testCase;
    handlerInfo.progressHandler     = _mavCmdProgressHandler;
    handlerInfo.progressHandlerData = &testCase;

    _resultHandlerCalled    = false;
    _progressHandlerCalled  = false;

    _mockLink->clearReceivedMavCommandCounts();
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, testCase.command, true /* showError */);
    QVERIFY(QTest::qWaitFor([&]() { return _mockLink->receivedMavCommandCount(testCase.command) == 1; }, 10));
    QVERIFY(!_resultHandlerCalled);
    QVERIFY(!_progressHandlerCalled);
    vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_AUTOPILOT1, testCase.command);

    // Duplicate command response should happen immediately
    QVERIFY(_resultHandlerCalled);
    QVERIFY(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, testCase.command) != -1);
    QCOMPARE(_mockLink->receivedMavCommandCount(testCase.command), 1);
}

void SendMavCommandWithHandlerTest::_compIdAllFailureMavCmdResultHandler(void* /*resultHandlerData*/, int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    _resultHandlerCalled = true;

    QCOMPARE(compId,        MAV_COMP_ID_ALL);
    QCOMPARE(ack.result,    MAV_RESULT_FAILED);
    QCOMPARE(failureCode,   Vehicle::MavCmdResultCommandResultOnly);
}

void SendMavCommandWithHandlerTest::_compIdAllFailure(void)
{
    _connectMockLinkNoInitialConnectSequence();

    SendMavCommandWithHandlerTest::TestCase_t testCase = {
        MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, MAV_RESULT_FAILED, 0, Vehicle::MavCmdResultFailureDuplicateCommand, 0
    };

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();

    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler = _compIdAllFailureMavCmdResultHandler;

    _resultHandlerCalled    = false;

    _mockLink->clearReceivedMavCommandCounts();
    vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_ALL, testCase.command);

    QCOMPARE(_resultHandlerCalled,                                                      true);
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_ALL, testCase.command), -1);
    QCOMPARE(_mockLink->receivedMavCommandCount(testCase.command),                      testCase.expectedSendCount);

    _disconnectMockLink();
}
