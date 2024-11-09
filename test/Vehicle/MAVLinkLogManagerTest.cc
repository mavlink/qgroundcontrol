/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkLogManagerTest.h"
#include "MAVLinkLogManager.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"

#include <QtCore/QStandardPaths>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void MAVLinkLogManagerTest::_testInitMAVLinkLogManager()
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager *const vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle *const vehicle = vehicleMgr->activeVehicle();
    MAVLinkLogManager *const mavlinkLogManager = new MAVLinkLogManager(vehicle, this);
    QVERIFY(mavlinkLogManager);
}
