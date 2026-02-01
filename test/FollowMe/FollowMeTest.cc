#include "FollowMeTest.h"

#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "FollowMe.h"
#include "MultiVehicleManager.h"
#include "PositionManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"

void FollowMeTest::_testFollowMe()
{
    FollowMe::instance()->init();
    QGCPositionManager::instance()->init();
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    vehicle->setFlightMode(vehicle->followFlightMode());
    SettingsManager::instance()->appSettings()->followTarget()->setRawValue(1);
    QSignalSpy spyGCSMotionReport(vehicle, &Vehicle::messagesSentChanged);
    QVERIFY(spyGCSMotionReport.wait(1500));
    _disconnectMockLink();
}

UT_REGISTER_TEST(FollowMeTest, TestLabel::Integration, TestLabel::Vehicle)
