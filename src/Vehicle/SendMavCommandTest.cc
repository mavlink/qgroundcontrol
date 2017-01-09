/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "SendMavCommandTest.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"

void SendMavCommandTest::_noFailure(void)
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);

    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    vehicle->sendMavCommand(MAV_COMP_ID_ALL, MAV_CMD_USER_1, true /* showError */);

    QSignalSpy spyResult(vehicle, SIGNAL(mavCommandResult(int, int, int, int, bool)));
    QCOMPARE(spyResult.wait(10000), true);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(2).toInt(), (int)MAV_CMD_USER_1);
    QCOMPARE(arguments.at(3).toInt(), (int)MAV_RESULT_ACCEPTED);
    QCOMPARE(arguments.at(4).toBool(), false);
}

void SendMavCommandTest::_failureShowError(void)
{
    // Will pop error about request failure
    setExpectedMessageBox(QMessageBox::Ok);

    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);

    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    vehicle->sendMavCommand(MAV_COMP_ID_ALL, MAV_CMD_USER_2, true /* showError */);

    QSignalSpy spyResult(vehicle, SIGNAL(mavCommandResult(int, int, int, int, bool)));
    QCOMPARE(spyResult.wait(10000), true);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(2).toInt(), (int)MAV_CMD_USER_2);
    QCOMPARE(arguments.at(3).toInt(), (int)MAV_RESULT_FAILED);
    QCOMPARE(arguments.at(4).toBool(), false);

    // User should have been notified
    checkExpectedMessageBox();
}

void SendMavCommandTest::_failureNoShowError(void)
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);

    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    vehicle->sendMavCommand(MAV_COMP_ID_ALL, MAV_CMD_USER_2, false /* showError */);

    QSignalSpy spyResult(vehicle, SIGNAL(mavCommandResult(int, int, int, int, bool)));
    QCOMPARE(spyResult.wait(10000), true);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(2).toInt(), (int)MAV_CMD_USER_2);
    QCOMPARE(arguments.at(3).toInt(), (int)MAV_RESULT_FAILED);
    QCOMPARE(arguments.at(4).toBool(), false);
}

void SendMavCommandTest::_noFailureAfterRetry(void)
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);

    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    vehicle->sendMavCommand(MAV_COMP_ID_ALL, MAV_CMD_USER_3, true /* showError */);

    QSignalSpy spyResult(vehicle, SIGNAL(mavCommandResult(int, int, int, int, bool)));
    QCOMPARE(spyResult.wait(10000), true);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(2).toInt(), (int)MAV_CMD_USER_3);
    QCOMPARE(arguments.at(3).toInt(), (int)MAV_RESULT_ACCEPTED);
    QCOMPARE(arguments.at(4).toBool(), false);
}

void SendMavCommandTest::_failureAfterRetry(void)
{
    // Will pop error about request failure
    setExpectedMessageBox(QMessageBox::Ok);

    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);

    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    vehicle->sendMavCommand(MAV_COMP_ID_ALL, MAV_CMD_USER_4, true /* showError */);

    QSignalSpy spyResult(vehicle, SIGNAL(mavCommandResult(int, int, int, int, bool)));
    QCOMPARE(spyResult.wait(10000), true);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(2).toInt(), (int)MAV_CMD_USER_4);
    QCOMPARE(arguments.at(3).toInt(), (int)MAV_RESULT_FAILED);
    QCOMPARE(arguments.at(4).toBool(), false);

    // User should have been notified
    checkExpectedMessageBox();
}

void SendMavCommandTest::_failureAfterNoReponse(void)
{
    // Will pop error about request failure
    setExpectedMessageBox(QMessageBox::Ok);

    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);

    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    vehicle->sendMavCommand(MAV_COMP_ID_ALL, MAV_CMD_USER_5, true /* showError */);

    QSignalSpy spyResult(vehicle, SIGNAL(mavCommandResult(int, int, int, int, bool)));
    QCOMPARE(spyResult.wait(20000), true);
    QList<QVariant> arguments = spyResult.takeFirst();
    QCOMPARE(arguments.count(), 5);
    QCOMPARE(arguments.at(0).toInt(), vehicle->id());
    QCOMPARE(arguments.at(2).toInt(), (int)MAV_CMD_USER_5);
    QCOMPARE(arguments.at(3).toInt(), (int)MAV_RESULT_FAILED);
    QCOMPARE(arguments.at(4).toBool(), true);

    // User should have been notified
    checkExpectedMessageBox();
}
