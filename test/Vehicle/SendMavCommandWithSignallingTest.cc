#include "SendMavCommandWithSignallingTest.h"
#include "MultiVehicleManager.h"
#include "MockLink.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void SendMavCommandWithSignallingTest::cleanup()
{
    _disconnectMockLink();
    UnitTest::cleanup();
}

SendMavCommandWithSignallingTest::TestCase_t SendMavCommandWithSignallingTest::_rgTestCases[] = {
    {  MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED,           MAV_RESULT_ACCEPTED,    Vehicle::MavCmdResultCommandResultOnly,             1 },
    {  MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED,             MAV_RESULT_FAILED,      Vehicle::MavCmdResultCommandResultOnly,             1 },
    {  MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED,   MAV_RESULT_ACCEPTED,    Vehicle::MavCmdResultCommandResultOnly,             2 },
    {  MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED,     MAV_RESULT_FAILED,      Vehicle::MavCmdResultCommandResultOnly,             2 },
    {  MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE,                      MAV_RESULT_FAILED,      Vehicle::MavCmdResultFailureNoResponseToCommand,    Vehicle::_mavCommandMaxRetryCount },
    {  MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE_NO_RETRY,             MAV_RESULT_FAILED,      Vehicle::MavCmdResultFailureNoResponseToCommand,    1 },
    {  MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_ACCEPTED,      MAV_RESULT_ACCEPTED,    Vehicle::MavCmdResultCommandResultOnly,             1 },
    {  MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_FAILED,        MAV_RESULT_FAILED,      Vehicle::MavCmdResultCommandResultOnly,             1 },
    {  MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_NO_ACK,        MAV_RESULT_FAILED,      Vehicle::MavCmdResultFailureNoResponseToCommand,    1 },
};

void SendMavCommandWithSignallingTest::_testCaseWorker(TestCase_t& testCase)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager*    vehicleMgr  = MultiVehicleManager::instance();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();
    VERIFY_NOT_NULL(vehicle);
    QSignalSpy              spyResult(vehicle, &Vehicle::mavCommandResult);
    QGC_VERIFY_SPY_VALID(spyResult);

    _mockLink->clearReceivedMavCommandCounts();

    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, testCase.command, true /* showError */);

    QVERIFY(spyResult.wait(Vehicle::kTestMavCommandMaxWaitMs));
    QList<QVariant> arguments = spyResult.takeFirst();

    // Gimbal controler requests MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION on vehicle connection,
    // and that messes with this test, as it receives response to that command instead. So if we
    // are taking the response to that MAV_CMD_REQUEST_MESSAGE, we discard it and take the next
    // Also, the camera manager requests MAVLINK_MSG_ID_CAMERA_INFORMATION on vehicle connection,
    // so we need to ignore that as well.
    while (arguments.at(2).toInt() == MAV_CMD_REQUEST_MESSAGE || arguments.at(2).toInt() == MAV_CMD_REQUEST_CAMERA_INFORMATION) {
        qDebug() << "Received response to MAV_CMD_REQUEST_MESSAGE(512), ignoring, waiting for: " << testCase.command;
        QVERIFY(spyResult.wait(Vehicle::kTestMavCommandMaxWaitMs));
        arguments = spyResult.takeFirst();
    }

    QCOMPARE_EQ(arguments.count(), 5);
    QCOMPARE_EQ(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE_EQ(arguments.at(1).toInt(), static_cast<int>(MAV_COMP_ID_AUTOPILOT1));
    QCOMPARE_EQ(arguments.at(2).toInt(), static_cast<int>(testCase.command));
    QCOMPARE_EQ(arguments.at(3).toInt(), static_cast<int>(testCase.expectedCommandResult));
    QCOMPARE_EQ(arguments.at(4).value<Vehicle::MavCmdResultFailureCode_t>(), testCase.expectedFailureCode);
    QCOMPARE_EQ(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED), -1);
    QCOMPARE_EQ(_mockLink->receivedMavCommandCount(testCase.command), testCase.expectedSendCount);

    _disconnectMockLink();
}

void SendMavCommandWithSignallingTest::_performTestCases()
{
    TestHelpers::runTestCases(_rgTestCases, [this](TestCase_t& testCase) {
        _testCaseWorker(testCase);
    });
}

void SendMavCommandWithSignallingTest::_duplicateCommand()
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager*    vehicleMgr  = MultiVehicleManager::instance();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();
    VERIFY_NOT_NULL(vehicle);

    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, true /* showError */);
    // Wait for command to be sent - use reasonable timeout for CI systems
    WAIT_FOR_CONDITION(_mockLink->receivedMavCommandCount(MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE) == 1, TestHelpers::kShortTimeoutMs);
    QSignalSpy spyResult(vehicle, &Vehicle::mavCommandResult);
    QGC_VERIFY_SPY_VALID(spyResult);
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, true /* showError */);

    // Duplicate command returns immediately
    QCOMPARE_EQ(spyResult.count(), 1);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE_EQ(arguments.count(), 5);
    QCOMPARE_EQ(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE_EQ(arguments.at(2).toInt(), static_cast<int>(MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE));
    QCOMPARE_EQ(arguments.at(3).toInt(), static_cast<int>(MAV_RESULT_FAILED));
    QCOMPARE_EQ(arguments.at(4).value<Vehicle::MavCmdResultFailureCode_t>(), Vehicle::MavCmdResultFailureDuplicateCommand);
    QCOMPARE_EQ(_mockLink->receivedMavCommandCount(MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE), 1);
    QCOMPARE_NE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE), -1);
}
