#include "VehicleUomRegistrationManagerTest.h"

#include <memory>

#include "Vehicle.h"
#include "VehicleUomRegistrationManager.h"

void VehicleUomRegistrationManagerTest::_unavailableAdapterDisablesOperations_data()
{
    QTest::addColumn<bool>("withVehicle");
    QTest::newRow("null-vehicle") << false;
    QTest::newRow("generic-firmware") << true;
}

void VehicleUomRegistrationManagerTest::_unavailableAdapterDisablesOperations()
{
    QFETCH(bool, withVehicle);
    const auto vehicle = withVehicle ? std::make_unique<Vehicle>(MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC) : nullptr;
    VehicleUomRegistrationManager manager(vehicle.get());
    QVERIFY(!manager.available());
    QVERIFY(!manager.supported());
    QVERIFY(manager.devices().isEmpty());
    QCOMPARE(manager.selectedDevice(), -1);
    QVERIFY(manager.serial().isEmpty());
    QVERIFY(manager.uid().isEmpty());
    QVERIFY(manager.productModel().isEmpty());
    QVERIFY(!manager.deviceStatus().isEmpty());

    manager.refreshDevice();
    manager.selectDevice(0);
    manager.verifyRegistration();
    manager.activate();
    manager.deactivateAndCancel(QStringLiteral("3"), QStringLiteral("Ownership change"));
    manager.retryPending();

    QVERIFY(!manager.busy());
    QVERIFY(!manager.pending());
    QVERIFY(!manager.canVerify());
    QVERIFY(!manager.canActivate());
    QVERIFY(!manager.canDeactivate());
    QVERIFY(!manager.canRetry());
}

UT_REGISTER_TEST(VehicleUomRegistrationManagerTest, TestLabel::Unit, TestLabel::Vehicle)
