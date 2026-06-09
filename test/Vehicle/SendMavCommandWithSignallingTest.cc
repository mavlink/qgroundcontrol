#include "SendMavCommandWithSignallingTest.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "MultiVehicleManager.h"

namespace {
bool _waitForExpectedCommandResult(QSignalSpy& spyResult, MAV_CMD expectedCommand, QList<QVariant>& arguments)
{
    const int commandResultTimeoutMs = TestTimeout::longMs();
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    while (true) {
        const int elapsed = static_cast<int>(elapsedTimer.elapsed());
        const int remainingMs = commandResultTimeoutMs - elapsed;
        if (remainingMs <= 0) {
            return false;
        }

        if (spyResult.isEmpty() && !spyResult.wait(remainingMs)) {
            return false;
        }

        arguments = spyResult.takeFirst();
        if (arguments.count() == 5 && arguments.at(2).toInt() == expectedCommand) {
            return true;
        }

        qCDebug(UnitTestLog) << "Received response to command" << arguments.at(2).toInt() << ", ignoring, waiting for:" << expectedCommand;
    }
}
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
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QSignalSpy spyResult(vehicle, &Vehicle::mavCommandResult);
    _mockLink->clearReceivedMavCommandCounts();
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, testCase.command, true /* showError */);
    QList<QVariant> arguments;
    QVERIFY(_waitForExpectedCommandResult(spyResult, testCase.command, arguments));
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
}

void SendMavCommandWithSignallingTest::_performTestCases()
{
    // Some test cases send showError=true and get a failed result, producing showAppMessage debug logs.
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression("command failed|did not respond to command"));
    // No-response test cases exhaust command retries, producing MavCommandQueue warnings.
    ignoreLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression("Giving up sending command after max retries:"));
    int index = 0;
    for (TestCase_t& testCase : _rgTestCases) {
        qCDebug(UnitTestLog) << "Testing case" << index++;
        _testCaseWorker(testCase);
    }
}

void SendMavCommandWithSignallingTest::_duplicateCommand()
{
    // Sending a duplicate command produces a showAppMessage debug log.
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression("Unable to send command"));
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
