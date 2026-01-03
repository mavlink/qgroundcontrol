#include "MAVLinkLogManagerTest.h"
#include "MAVLinkLogManager.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

#include <QtCore/QStandardPaths>
#include <QtTest/QTest>

void MAVLinkLogManagerTest::_testInitMAVLinkLogManager()
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager *const vehicleMgr = MultiVehicleManager::instance();
    Vehicle *const vehicle = vehicleMgr->activeVehicle();
    MAVLinkLogManager *const mavlinkLogManager = new MAVLinkLogManager(vehicle, this);
    QVERIFY(mavlinkLogManager);
}
