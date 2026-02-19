#include "SendMavCommandWithSignallingTest.h"

#include <QtTest/QSignalSpy>

#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"

#include <QtTest/QTest>

void SendMavCommandWithSignallingTest::init()
{
    VehicleTestManualConnect::init();
    LinkManager::instance()->setConnectionsAllowed();
    MAVLinkProtocol::deleteTempLogFiles();
}

SendMavCommandWithSignallingTest::TestCase_t SendMavCommandWithSignallingTest::_rgTestCases[] = {
    {MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED, MAV_RESULT_ACCEPTED, Vehicle::MavCmdResultCommandResultOnly, 1},
    {MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED, MAV_RESULT_FAILED, Vehicle::MavCmdResultCommandResultOnly, 1},
    {MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED, MAV_RESULT_ACCEPTED,
     Vehicle::MavCmdResultCommandResultOnly, 2},
    {MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED, MAV_RESULT_FAILED, Vehicle::MavCmdResultCommandResultOnly,
     2},
    {MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, MAV_RESULT_FAILED, Vehicle::MavCmdResultFailureNoResponseToCommand,
     Vehicle::_mavCommandMaxRetryCount},
    {MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE_NO_RETRY, MAV_RESULT_FAILED,
     Vehicle::MavCmdResultFailureNoResponseToCommand, 1},
    {MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_ACCEPTED, MAV_RESULT_ACCEPTED,
     Vehicle::MavCmdResultCommandResultOnly, 1},
    {MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_FAILED, MAV_RESULT_FAILED, Vehicle::MavCmdResultCommandResultOnly,
     1},
    {MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_NO_ACK, MAV_RESULT_FAILED,
     Vehicle::MavCmdResultFailureNoResponseToCommand, 1},
};

void SendMavCommandWithSignallingTest::_testCaseWorker(TestCase_t& testCase)
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QSignalSpy spyResult(vehicle, &Vehicle::mavCommandResult);
    _mockLink->clearReceivedMavCommandCounts();
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, testCase.command, true /* showError */);
    QVERIFY_SIGNAL_WAIT(spyResult, TestTimeout::longMs());
    QList<QVariant> arguments = spyResult.takeFirst();
    // Gimbal controler requests MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION on vehicle connection,
    // and that messes with this test, as it receives response to that command instead. So if we
    // are taking the response to that MAV_CMD_REQUEST_MESSAGE, we discard it and take the next
    // Also, the camera manager requests MAVLINK_MSG_ID_CAMERA_INFORMATION on vehicle connection,
    // so we need to ignore that as well.
    while (arguments.at(2).toInt() == MAV_CMD_REQUEST_MESSAGE ||
           arguments.at(2).toInt() == MAV_CMD_REQUEST_CAMERA_INFORMATION) {
        TEST_DEBUG(QStringLiteral("Ignoring response for command %1 while waiting for %2")
                       .arg(arguments.at(2).toInt())
                       .arg(static_cast<int>(testCase.command)));
        QVERIFY_SIGNAL_WAIT(spyResult, TestTimeout::longMs());
        arguments = spyResult.takeFirst();
    }
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(1).toInt(), MAV_COMP_ID_AUTOPILOT1);
    QCOMPARE(arguments.at(2).toInt(), testCase.command);
    QCOMPARE(arguments.at(3).toInt(), testCase.expectedCommandResult);
    QCOMPARE(arguments.at(4).value<Vehicle::MavCmdResultFailureCode_t>(), testCase.expectedFailureCode);
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1,
                                                    MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED),
             -1);
    QCOMPARE(_mockLink->receivedMavCommandCount(testCase.command), testCase.expectedSendCount);
    _disconnectMockLink();
}

void SendMavCommandWithSignallingTest::_performTestCases_data()
{
    QTest::addColumn<int>("command");
    QTest::addColumn<int>("expectedCommandResult");
    QTest::addColumn<int>("expectedFailureCode");
    QTest::addColumn<int>("expectedSendCount");

    int i = 0;
    for (const TestCase_t& testCase : _rgTestCases) {
        QTest::addRow("case_%d", i)
            << static_cast<int>(testCase.command)
            << static_cast<int>(testCase.expectedCommandResult)
            << static_cast<int>(testCase.expectedFailureCode)
            << testCase.expectedSendCount;
        ++i;
    }
}

void SendMavCommandWithSignallingTest::_performTestCases()
{
    QFETCH(int, command);
    QFETCH(int, expectedCommandResult);
    QFETCH(int, expectedFailureCode);
    QFETCH(int, expectedSendCount);

    TestCase_t testCase = {static_cast<MAV_CMD>(command),
                           static_cast<MAV_RESULT>(expectedCommandResult),
                           static_cast<Vehicle::MavCmdResultFailureCode_t>(expectedFailureCode),
                           expectedSendCount};
    TEST_DEBUG(QStringLiteral("Testing case %1").arg(QTest::currentDataTag() ? QTest::currentDataTag() : "unknown"));
    _testCaseWorker(testCase);
}

void SendMavCommandWithSignallingTest::_duplicateCommand()
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, true /* showError */);
    QVERIFY_TRUE_WAIT(_mockLink->receivedMavCommandCount(MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE) == 1,
                      TestTimeout::shortMs());
    QSignalSpy spyResult(vehicle, &Vehicle::mavCommandResult);
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, true /* showError */);
    // Duplicate command returns immediately
    QCOMPARE(spyResult.count(), 1);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(2).toInt(), (int)MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE);
    QCOMPARE(arguments.at(3).toInt(), (int)MAV_RESULT_FAILED);
    QCOMPARE(arguments.at(4).value<Vehicle::MavCmdResultFailureCode_t>(), Vehicle::MavCmdResultFailureDuplicateCommand);
    QCOMPARE(_mockLink->receivedMavCommandCount(MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE), 1);
    QVERIFY(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE) !=
            -1);
}

UT_REGISTER_TEST(SendMavCommandWithSignallingTest, TestLabel::Integration, TestLabel::Vehicle)
