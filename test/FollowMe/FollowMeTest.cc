#include "FollowMeTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "FollowMe.h"
#include "MultiVehicleManager.h"
#include "PositionManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"

void FollowMeTest::_testFollowMe()
{
    // The mock vehicle does not have a follow mode configured, so setFlightMode produces expected warnings.
    ignoreLogMessage("FirmwarePlugin.PX4FirmwarePlugin", QtWarningMsg,
                     QRegularExpression("Unknown flight Mode"));
    ignoreLogMessage("Vehicle.Vehicle", QtWarningMsg,
                     QRegularExpression("setFlightMode failed"));
    FollowMe::instance()->init();
    QGCPositionManager::instance()->init();
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    vehicle->setFlightMode(vehicle->followFlightMode());
    SettingsManager::instance()->appSettings()->followTarget()->setRawValue(1);
    QSignalSpy spyGCSMotionReport(vehicle, &Vehicle::messagesSentChanged);
    QVERIFY_SIGNAL_WAIT(spyGCSMotionReport, TestTimeout::mediumMs());
    _disconnectMockLink();
}

UT_REGISTER_TEST(FollowMeTest, TestLabel::Integration, TestLabel::Vehicle)
