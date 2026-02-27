#include "MAVLinkLogManagerTest.h"

#include <QtCore/QStandardPaths>

#include "MAVLinkLogManager.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

void MAVLinkLogManagerTest::_testInitMAVLinkLogManager()
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* const vehicleMgr = MultiVehicleManager::instance();
    Vehicle* const vehicle = vehicleMgr->activeVehicle();
    MAVLinkLogManager* const mavlinkLogManager = new MAVLinkLogManager(vehicle, this);
    QVERIFY(mavlinkLogManager);
}

UT_REGISTER_TEST(MAVLinkLogManagerTest, TestLabel::Integration, TestLabel::Vehicle)
