/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FollowMeTest.h"
#include "FollowMe.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "PositionManager.h"
#include "Vehicle.h"
#include "SettingsManager.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void FollowMeTest::_testFollowMe()
{
    FollowMe::instance()->init();
    QGCPositionManager::instance()->init();

    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager *vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle *vehicle = vehicleMgr->activeVehicle();
    vehicle->setFlightMode(vehicle->followFlightMode());
    qgcApp()->toolbox()->settingsManager()->appSettings()->followTarget()->setRawValue(1);

    QSignalSpy spyGCSMotionReport(vehicle, &Vehicle::messagesSentChanged);

    QVERIFY(spyGCSMotionReport.wait(1500));

    _disconnectMockLink();
}
