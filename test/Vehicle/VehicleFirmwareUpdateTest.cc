#include "VehicleFirmwareUpdateTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QString>

#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

void VehicleFirmwareUpdateTest::_acknowledgementPersistsUntilNewerRelease()
{
    Vehicle* const vehicle = createMockLinkAndWaitForVehicle();
    QVERIFY(vehicle);
    QTRY_VERIFY_WITH_TIMEOUT(vehicle->isInitialConnectComplete(), TestTimeout::longMs());

    vehicle->setNewStableFirmwareVersion(QStringLiteral("4.5.0"));
    QVERIFY(!vehicle->newStableFirmwareVersionAcknowledged());

    vehicle->acknowledgeNewStableFirmwareVersion();
    QVERIFY(vehicle->newStableFirmwareVersionAcknowledged());

    // Clearing then re-reporting the same release re-reads the persisted acknowledgement, as on reconnect
    vehicle->setNewStableFirmwareVersion(QString());
    QVERIFY(!vehicle->newStableFirmwareVersionAcknowledged());
    vehicle->setNewStableFirmwareVersion(QStringLiteral("4.5.0"));
    QVERIFY(vehicle->newStableFirmwareVersionAcknowledged());

    vehicle->setNewStableFirmwareVersion(QStringLiteral("4.6.0"));
    QVERIFY(!vehicle->newStableFirmwareVersionAcknowledged());
}

void VehicleFirmwareUpdateTest::_acknowledgementAppliesToMatchingConnectedVehicles()
{
    Vehicle* const first = createMockLinkAndWaitForVehicle(QStringLiteral("First"));
    QVERIFY(first);
    QTRY_VERIFY_WITH_TIMEOUT(first->isInitialConnectComplete(), TestTimeout::longMs());

    expectAppMessage(QRegularExpression(QStringLiteral("Connected to Vehicle [0-9]+")));
    const SharedLinkInterfacePtr secondLink = createMockLink(QStringLiteral("Second"));
    auto* const secondMockLink = qobject_cast<MockLink*>(secondLink.get());
    QVERIFY(secondMockLink);
    QTRY_VERIFY_WITH_TIMEOUT(MultiVehicleManager::instance()->getVehicleById(secondMockLink->vehicleId()),
                             TestTimeout::longMs());
    verifyExpectedLogMessage();
    Vehicle* const second = MultiVehicleManager::instance()->getVehicleById(secondMockLink->vehicleId());
    QVERIFY(second != first);
    QTRY_VERIFY_WITH_TIMEOUT(second->isInitialConnectComplete(), TestTimeout::longMs());

    first->setNewStableFirmwareVersion(QStringLiteral("4.5.0"));
    second->setNewStableFirmwareVersion(QStringLiteral("4.5.0"));
    QVERIFY(!second->newStableFirmwareVersionAcknowledged());

    first->acknowledgeNewStableFirmwareVersion();
    QVERIFY(first->newStableFirmwareVersionAcknowledged());
    QVERIFY(second->newStableFirmwareVersionAcknowledged());
}

UT_REGISTER_TEST(VehicleFirmwareUpdateTest, TestLabel::Integration, TestLabel::Vehicle)
