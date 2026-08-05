#include "MultiVehicleManagerTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

void MultiVehicleManagerTest::_testDuplicateAllLinksRemoved()
{
    // The duplicate delivery must hit the not-found guard and warn instead of
    // double-scheduling vehicle deletion.
    expectLogMessage("Vehicle.MultiVehicleManager", QtWarningMsg,
                     QRegularExpression("Vehicle not found in map!"));

    // The forced deletion below bypasses the normal closeVehicle link-release path, so
    // the MockLink teardown warns about a leftover vehicle reference.
    ignoreLogMessage("Comms.LinkInterface", QtWarningMsg,
                     QRegularExpression("still have vehicle references:"));

    Vehicle* vehicle = this->vehicle();
    QVERIFY(vehicle);

    // Stop heartbeats so a new vehicle isn't created after this one is removed
    simulateCommLoss(true);

    QSignalSpy destroyedSpy(vehicle, &QObject::destroyed);
    QVERIFY(destroyedSpy.isValid());

    // Simulate the race from issue #13251 crash 2: allLinksRemoved delivered twice for
    // the same vehicle.
    VehicleLinkManager* vehicleLinkManager = vehicle->vehicleLinkManager();
    QVERIFY(vehicleLinkManager);
    emit vehicleLinkManager->allLinksRemoved(vehicle);
    emit vehicleLinkManager->allLinksRemoved(vehicle);

    // The duplicate delivery must have hit the not-found guard
    verifyExpectedLogMessage();

    QTRY_COMPARE_WITH_TIMEOUT(destroyedSpy.count(), 1, TestTimeout::longMs());
    QVERIFY(!MultiVehicleManager::instance()->activeVehicle());

    // The vehicle is already gone but the MockLink is still connected. Disconnect it
    // ourselves so the base class cleanup (which waits for a vehicle removal) doesn't
    // time out.
    _vehicle = nullptr;
    if (_mockLink) {
        _mockLink->disconnect();
        _mockLink = nullptr;
    }
    settleEventLoopForCleanup();
}

UT_REGISTER_TEST(MultiVehicleManagerTest, TestLabel::Integration, TestLabel::Vehicle)
