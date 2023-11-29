/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SendMavCommandWithSignallingTest.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "MockLink.h"

SendMavCommandWithSignallingTest::TestCase_t SendMavCommandWithSignallingTest::_rgTestCases[] = {
    {  MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED,           MAV_RESULT_ACCEPTED,    Vehicle::MavCmdResultCommandResultOnly,             1 },
    {  MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED,             MAV_RESULT_FAILED,      Vehicle::MavCmdResultCommandResultOnly,             1 },
    {  MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED,   MAV_RESULT_ACCEPTED,    Vehicle::MavCmdResultCommandResultOnly,             2 },
    {  MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED,     MAV_RESULT_FAILED,      Vehicle::MavCmdResultCommandResultOnly,             2 },
    {  MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE,                      MAV_RESULT_FAILED,      Vehicle::MavCmdResultFailureNoResponseToCommand,    Vehicle::_mavCommandMaxRetryCount },
    {  MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE_NO_RETRY,             MAV_RESULT_FAILED,      Vehicle::MavCmdResultFailureNoResponseToCommand,    1 },
};

void SendMavCommandWithSignallingTest::_testCaseWorker(TestCase_t& testCase)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();
    QSignalSpy              spyResult(vehicle, &Vehicle::mavCommandResult);

    _mockLink->clearSendMavCommandCounts();

    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, testCase.command, true /* showError */);

    QCOMPARE(spyResult.wait(10000), true);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(),                                             5);
    QCOMPARE(arguments.at(0).toInt(),                                       vehicle->id());
    QCOMPARE(arguments.at(1).toInt(),                                       MAV_COMP_ID_AUTOPILOT1);
    QCOMPARE(arguments.at(2).toInt(),                                       testCase.command);
    QCOMPARE(arguments.at(3).toInt(),                                       testCase.expectedCommandResult);
    QCOMPARE(arguments.at(4).value<Vehicle::MavCmdResultFailureCode_t>(),   testCase.expectedFailureCode);
    QCOMPARE(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED), -1);
    QCOMPARE(_mockLink->sendMavCommandCount(testCase.command),              testCase.expectedSendCount);

    _disconnectMockLink();
}

void SendMavCommandWithSignallingTest::_performTestCases(void)
{
    int index = 0;
    for (TestCase_t& testCase: _rgTestCases) {
        qDebug() << "Testing case" << index++;
        _testCaseWorker(testCase);
    }
}

void SendMavCommandWithSignallingTest::_duplicateCommand(void)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();

    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, true /* showError */);
    QVERIFY(QTest::qWaitFor([&]() { return _mockLink->sendMavCommandCount(MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE) == 1; }, 10));
    QSignalSpy spyResult(vehicle, &Vehicle::mavCommandResult);
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, true /* showError */);

    // Duplicate command returns immediately
    QCOMPARE(spyResult.count(),                                                         1);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(),                                                         5);
    QCOMPARE(arguments.at(0).toInt(),                                                   vehicle->id());
    QCOMPARE(arguments.at(2).toInt(),                                                   (int)MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE);
    QCOMPARE(arguments.at(3).toInt(),                                                   (int)MAV_RESULT_FAILED);
    QCOMPARE(arguments.at(4).value<Vehicle::MavCmdResultFailureCode_t>(),               Vehicle::MavCmdResultFailureDuplicateCommand);
    QCOMPARE(_mockLink->sendMavCommandCount(MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE),    1);
    QVERIFY(vehicle->_findMavCommandListEntryIndex(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE) != -1);
}
