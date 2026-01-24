#include "SendMavCommandWithHandlerTest.h"
#include "MultiVehicleManager.h"
#include "MockLink.h"

#include <QtTest/QTest>

void SendMavCommandWithHandlerTest::cleanup()
{
    _disconnectMockLink();
    UnitTest::cleanup();
}

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

    QCOMPARE_EQ(compId, static_cast<int>(MAV_COMP_ID_AUTOPILOT1));
    QCOMPARE_EQ(ack.result, testCase->expectedCommandResult);
    QCOMPARE_EQ(failureCode, testCase->expectedFailureCode);
}

void SendMavCommandWithHandlerTest::_mavCmdProgressHandler(void* progressHandlerData, int compId, const mavlink_command_ack_t& ack)
{
    TestCase_t*             testCase    = static_cast<TestCase_t*>(progressHandlerData);
    MultiVehicleManager*    vehicleMgr  = MultiVehicleManager::instance();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();

    _progressHandlerCalled = true;

    QCOMPARE_EQ(compId, static_cast<int>(MAV_COMP_ID_AUTOPILOT1));
    QCOMPARE_EQ(ack.result, static_cast<decltype(ack.result)>(MAV_RESULT_IN_PROGRESS));
    QCOMPARE_EQ(ack.progress, static_cast<decltype(ack.progress)>(1));

    // Command should still be in list
    QCOMPARE_NE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, testCase->command), -1);
}

void SendMavCommandWithHandlerTest::_testCaseWorker(TestCase_t& testCase)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager*    vehicleMgr  = MultiVehicleManager::instance();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();
    VERIFY_NOT_NULL(vehicle);

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
        WAIT_FOR_CONDITION(_progressHandlerCalled, Vehicle::kTestMavCommandMaxWaitMs);
    }

    WAIT_FOR_CONDITION(_resultHandlerCalled, Vehicle::kTestMavCommandMaxWaitMs);
    QCOMPARE_EQ(_mockLink->receivedMavCommandCount(testCase.command), testCase.expectedSendCount);
    QCOMPARE_EQ(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, testCase.command), -1);

    _disconnectMockLink();
}

void SendMavCommandWithHandlerTest::_performTestCases()
{
    TestHelpers::runTestCases(_rgTestCases, [this](TestCase_t& testCase) {
        _testCaseWorker(testCase);
    });
}

void SendMavCommandWithHandlerTest::_duplicateCommand()
{
    _connectMockLinkNoInitialConnectSequence();

    SendMavCommandWithHandlerTest::TestCase_t testCase = {
        MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, MAV_RESULT_FAILED, 0, Vehicle::MavCmdResultFailureDuplicateCommand, 1
    };

    MultiVehicleManager*    vehicleMgr  = MultiVehicleManager::instance();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();
    VERIFY_NOT_NULL(vehicle);

    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler       = _mavCmdResultHandler;
    handlerInfo.resultHandlerData   = &testCase;
    handlerInfo.progressHandler     = _mavCmdProgressHandler;
    handlerInfo.progressHandlerData = &testCase;

    _resultHandlerCalled    = false;
    _progressHandlerCalled  = false;

    _mockLink->clearReceivedMavCommandCounts();
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, testCase.command, true /* showError */);
    // Wait for command to be sent - use reasonable timeout for CI systems
    WAIT_FOR_CONDITION(_mockLink->receivedMavCommandCount(testCase.command) == 1, TestHelpers::kShortTimeoutMs);
    QVERIFY(!_resultHandlerCalled);
    QVERIFY(!_progressHandlerCalled);
    vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_AUTOPILOT1, testCase.command);

    // Duplicate command response should happen immediately
    QVERIFY(_resultHandlerCalled);
    QCOMPARE_NE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, testCase.command), -1);
    QCOMPARE_EQ(_mockLink->receivedMavCommandCount(testCase.command), 1);
}

void SendMavCommandWithHandlerTest::_compIdAllFailureMavCmdResultHandler(void* /*resultHandlerData*/, int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    _resultHandlerCalled = true;

    QCOMPARE_EQ(compId, static_cast<int>(MAV_COMP_ID_ALL));
    QCOMPARE_EQ(ack.result, static_cast<decltype(ack.result)>(MAV_RESULT_FAILED));
    QCOMPARE_EQ(failureCode, Vehicle::MavCmdResultCommandResultOnly);
}

void SendMavCommandWithHandlerTest::_compIdAllFailure()
{
    _connectMockLinkNoInitialConnectSequence();

    SendMavCommandWithHandlerTest::TestCase_t testCase = {
        MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, MAV_RESULT_FAILED, 0, Vehicle::MavCmdResultFailureDuplicateCommand, 0
    };

    MultiVehicleManager*    vehicleMgr  = MultiVehicleManager::instance();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();
    VERIFY_NOT_NULL(vehicle);

    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler = _compIdAllFailureMavCmdResultHandler;

    _resultHandlerCalled    = false;

    _mockLink->clearReceivedMavCommandCounts();
    vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_ALL, testCase.command);

    QCOMPARE_EQ(_resultHandlerCalled, true);
    QCOMPARE_EQ(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_ALL, testCase.command), -1);
    QCOMPARE_EQ(_mockLink->receivedMavCommandCount(testCase.command), testCase.expectedSendCount);

    _disconnectMockLink();
}
