#include "NTRIPConnectedVehiclesTest.h"

#include <QtTest/QSignalSpy>

#include "MultiVehicleManager.h"
#include "NTRIPConnectedVehicles.h"
#include "QmlObjectListModel.h"

void NTRIPConnectedVehiclesTest::_followsTheConnectedVehicles()
{
    const auto* const manager = MultiVehicleManager::instance();
    QVERIFY(manager);
    QCOMPARE(manager->vehicles()->count(), 1);

    NTRIPConnectedVehicles vehicles;
    QSignalSpy changes(&vehicles, &NTRIPVehiclePresence::presentChanged);
    // Until it follows the application, it knows of no vehicle.
    QVERIFY(!vehicles.present());
    vehicles.attach();
    QVERIFY(vehicles.present());
    QCOMPARE(changes.size(), 1);
    // Following twice tells nothing twice.
    vehicles.attach();

    _disconnectMockLink();
    QCOMPARE(manager->vehicles()->count(), 0);
    QVERIFY(!vehicles.present());
    QCOMPARE(changes.size(), 2);
    QVERIFY(!changes.constLast().constFirst().toBool());

    // The vehicle kept for planning without a connection does not count.
    QVERIFY(manager->offlineEditingVehicle());
    QVERIFY(!vehicles.present());

    _connectMockLink();
    QCOMPARE(manager->vehicles()->count(), 1);
    QVERIFY(vehicles.present());
    QCOMPARE(changes.size(), 3);
}

UT_REGISTER_TEST(NTRIPConnectedVehiclesTest, TestLabel::Integration, TestLabel::Vehicle)
