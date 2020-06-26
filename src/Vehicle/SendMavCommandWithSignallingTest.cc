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

void SendMavCommandWithSignallingTest::_noFailure(void)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();
    QSignalSpy              spyResult(vehicle, &Vehicle::mavCommandResult);

    vehicle->sendMavCommand(MAV_COMP_ID_ALL, MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED, true /* showError */);

    QCOMPARE(spyResult.wait(10000), true);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(2).toInt(), (int)MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED);
    QCOMPARE(arguments.at(3).toInt(), (int)MAV_RESULT_ACCEPTED);
    QCOMPARE(arguments.at(4).toBool(), false);
}

void SendMavCommandWithSignallingTest::_failureShowError(void)
{
    // Will pop error about request failure
    setExpectedMessageBox(QMessageBox::Ok);

    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    vehicle->sendMavCommand(MAV_COMP_ID_ALL, MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED, true /* showError */);

    QSignalSpy spyResult(vehicle, SIGNAL(mavCommandResult(int, int, int, int, bool)));
    QCOMPARE(spyResult.wait(10000), true);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(2).toInt(), (int)MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED);
    QCOMPARE(arguments.at(3).toInt(), (int)MAV_RESULT_FAILED);
    QCOMPARE(arguments.at(4).toBool(), false);

    // User should have been notified
    checkExpectedMessageBox();
}

void SendMavCommandWithSignallingTest::_failureNoShowError(void)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    vehicle->sendMavCommand(MAV_COMP_ID_ALL, MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED, false /* showError */);

    QSignalSpy spyResult(vehicle, SIGNAL(mavCommandResult(int, int, int, int, bool)));
    QCOMPARE(spyResult.wait(10000), true);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(2).toInt(), (int)MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED);
    QCOMPARE(arguments.at(3).toInt(), (int)MAV_RESULT_FAILED);
    QCOMPARE(arguments.at(4).toBool(), false);
}

void SendMavCommandWithSignallingTest::_noFailureAfterRetry(void)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    vehicle->sendMavCommand(MAV_COMP_ID_ALL, MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED, true /* showError */);

    QSignalSpy spyResult(vehicle, SIGNAL(mavCommandResult(int, int, int, int, bool)));
    QCOMPARE(spyResult.wait(10000), true);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(2).toInt(), (int)MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED);
    QCOMPARE(arguments.at(3).toInt(), (int)MAV_RESULT_ACCEPTED);
    QCOMPARE(arguments.at(4).toBool(), false);
}

void SendMavCommandWithSignallingTest::_failureAfterRetry(void)
{
    // Will pop error about request failure
    setExpectedMessageBox(QMessageBox::Ok);

    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    vehicle->sendMavCommand(MAV_COMP_ID_ALL, MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED, true /* showError */);

    QSignalSpy spyResult(vehicle, SIGNAL(mavCommandResult(int, int, int, int, bool)));
    QCOMPARE(spyResult.wait(10000), true);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(2).toInt(), (int)MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED);
    QCOMPARE(arguments.at(3).toInt(), (int)MAV_RESULT_FAILED);
    QCOMPARE(arguments.at(4).toBool(), false);

    // User should have been notified
    checkExpectedMessageBox();
}

void SendMavCommandWithSignallingTest::_failureAfterNoReponse(void)
{
    // Will pop error about request failure
    setExpectedMessageBox(QMessageBox::Ok);

    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    vehicle->sendMavCommand(MAV_COMP_ID_ALL, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, true /* showError */);

    QSignalSpy spyResult(vehicle, SIGNAL(mavCommandResult(int, int, int, int, bool)));
    QCOMPARE(spyResult.wait(20000), true);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(2).toInt(), (int)MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE);
    QCOMPARE(arguments.at(3).toInt(), (int)MAV_RESULT_FAILED);
    QCOMPARE(arguments.at(4).toBool(), true);

    // User should have been notified
    checkExpectedMessageBox();
}

void SendMavCommandWithSignallingTest::_unexpectedAck(void)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();
    QSignalSpy              spyResult(vehicle, &Vehicle::mavCommandResult);

    _mockLink->sendUnexpectedCommandAck(MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED, MAV_RESULT_ACCEPTED);
    QCOMPARE(spyResult.wait(100), false);

    _mockLink->sendUnexpectedCommandAck(MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED, MAV_RESULT_FAILED);
    QCOMPARE(spyResult.wait(100), false);
}
