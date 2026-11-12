#include "NTRIPConnectedVehiclesTest.h"

#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "ManualScheduler.h"
#include "MultiVehicleManager.h"
#include "NTRIP/Support/MockNTRIPTransport.h"
#include "NTRIPConnectedVehicles.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "PositionManager.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Support/GPSTestHelpers.h"

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

void NTRIPConnectedVehiclesTest::_gpsManagerStartsNtripWithTheVehiclesKnown()
{
    using Status = NTRIPManager::ConnectionStatus;
    TestFixtures::SettingsFixture saved;
    NTRIPSettings* const settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->ntripServerPort(), 2101);
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(settings->ntripUseTls(), false);
    saved.setFactValue(settings->ntripDataCapMB(), 0);
    saved.setFactValue(settings->ntripRequireVehicle(), true);
    saved.setFactValue(settings->ntripServerConnectEnabled(), true);
    // What the connection does until the manager is shut down, which its destruction does as well.
    int attempts = 0;
    QList<Status> statuses;
    // Receiver connection polling runs on this clock, which the test never advances.
    ManualScheduler scheduler;
    GPSManager manager(nullptr, &scheduler);
    manager.positionManager()->setPlatformSourceFactory([](QObject* parent) {
        auto* const source = new GPSTest::PositionSource;
        source->setParent(parent);
        return source;
    });
    NTRIPManager* const ntrip = manager.ntrip();
    ntrip->setTransportFactory([&attempts](const NTRIPManager::Configuration&, QObject* parent) {
        ++attempts;
        return new GPSTest::MockNTRIPTransport(parent);
    });
    connect(ntrip, &NTRIPManager::connectionStatusChanged, this,
            [&statuses, ntrip]() { statuses.append(ntrip->connectionStatus()); });

    // The vehicle of this suite is connected. init() follows the vehicles before it starts the connection, which
    // therefore opens at once instead of first waiting for a vehicle that is there.
    QCOMPARE(MultiVehicleManager::instance()->vehicles()->count(), 1);
    manager.init();
    QVERIFY(ntrip->configuration().requireVehicle);
    QCOMPARE(statuses, QList<Status>{Status::Connected});
    QCOMPARE(attempts, 1);
}

UT_REGISTER_TEST(NTRIPConnectedVehiclesTest, TestLabel::Integration, TestLabel::Vehicle)
