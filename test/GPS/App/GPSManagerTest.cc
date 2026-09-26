#include <QtCore/QMetaProperty>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSCorrectionStatus.h"
#include "GPSGgaSources.h"
#include "GPSManager.h"
#include "GPSReceiver.h"
#include "GPSSettingsBindings.h"
#include "GPSSourceHealth.h"
#include "GPSTestHelpers.h"
#include "ManualScheduler.h"
#include "MultiVehicleManager.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "PositionManager.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "SimulatedPosition.h"
#include "UnitTest.h"
#include "Vehicle.h"

class GPSManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _correctionStatus();
    void _correctionStateFacade();
    void _positionManagerLifecycle();
    void _simulatedPositionFollowsVehicleHome_data();
    void _simulatedPositionFollowsVehicleHome();
    void _vehicleEstimateTracking();
    void _ntripSettingsBinding();
    void _settingsBindingsCoverEveryFact_data();
    void _settingsBindingsCoverEveryFact();
};

void GPSManagerTest::_correctionStatus()
{
    using State = GPSManager::CorrectionState;
    GPSCorrectionManager corrections;
    NTRIPManager ntrip;
    GPSReceiver rtk;
    Fact udpInputEnabled(0, QStringLiteral("udpInputEnabled"), FactMetaData::valueTypeBool);
    udpInputEnabled.setRawValue(false);
    GPSCorrectionStatus status(&corrections, &ntrip, &rtk, &udpInputEnabled);
    QSignalSpy changes(&status, &GPSCorrectionStatus::stateChanged);
    QCOMPARE(status.state(), State::Inactive);

    // An enabled source that has delivered nothing is waiting.
    udpInputEnabled.setRawValue(true);
    QCOMPARE(status.state(), State::Waiting);
    udpInputEnabled.setRawValue(false);
    QCOMPARE(status.state(), State::Inactive);

    auto source = corrections.registerSource(GPSCorrectionSource::NTRIP, QStringLiteral("caster"));
    corrections.acceptIngress(
        source.event(GPSTestHelpers::buildRtcmFrame(1005, 20), GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QTRY_COMPARE_WITH_TIMEOUT(status.state(), State::Fresh, TestTimeout::mediumMs());
    source.reset();
    QTRY_COMPARE_WITH_TIMEOUT(status.state(), State::Inactive, TestTimeout::mediumMs());
    QCOMPARE(changes.count(), 4);
}

void GPSManagerTest::_correctionStateFacade()
{
    TestFixtures::SettingsFixture saved;
    Fact* const udpInputEnabled = SettingsManager::instance()->gpsCorrectionSettings()->rtcmUdpInputEnabled();
    saved.setFactValue(udpInputEnabled, false);
    saved.setFactValue(SettingsManager::instance()->ntripSettings()->ntripServerConnectEnabled(), false);
    GPSManager manager;
    QSignalSpy changes(&manager, &GPSManager::correctionStateChanged);
    QCOMPARE(manager.correctionState(), GPSManager::CorrectionState::Inactive);
    udpInputEnabled->setRawValue(true);
    QCOMPARE(manager.correctionState(), GPSManager::CorrectionState::Waiting);
    QCOMPARE(changes.count(), 1);
}

void GPSManagerTest::_positionManagerLifecycle()
{
    using Mode = GPSPositionService::SourceMode;
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->ntripSettings()->ntripServerConnectEnabled(), false);
    Fact* const sourceSetting = SettingsManager::instance()->rtkSettings()->gcsPositionSource();
    saved.setFactValue(sourceSetting, static_cast<int>(Mode::InternalOnly));
    GPSManager manager;
    PositionManager* const positions = manager.positionManager();
    QSignalSpy simulated(positions, &PositionManager::simulatedPositionCreated);
    QCOMPARE(positions->sourceMode(), Mode::Automatic);

    manager.init();
    QCOMPARE(positions->sourceMode(), Mode::InternalOnly);
    QCOMPARE(simulated.size(), 1);
    sourceSetting->setRawValue(static_cast<int>(Mode::ReceiverOnly));
    QCOMPARE(positions->sourceMode(), Mode::ReceiverOnly);

    manager.shutdown();
    sourceSetting->setRawValue(static_cast<int>(Mode::Automatic));
    QCOMPARE(positions->sourceMode(), Mode::ReceiverOnly);
}

void GPSManagerTest::_simulatedPositionFollowsVehicleHome_data()
{
    QTest::addColumn<bool>("latestAlreadyValid");
    QTest::addColumn<bool>("oldestFirst");
    QTest::newRow("pending-oldest-first") << false << true;
    QTest::newRow("pending-newest-first") << false << false;
    QTest::newRow("valid-oldest-first") << true << true;
    QTest::newRow("valid-newest-first") << true << false;
}

void GPSManagerTest::_simulatedPositionFollowsVehicleHome()
{
    QFETCH(bool, latestAlreadyValid);
    QFETCH(bool, oldestFirst);
    ManualScheduler scheduler;
    SimulatedPosition source(nullptr, &scheduler);
    MultiVehicleManager vehicles;
    GPSManager::_followVehicleHome(&vehicles, &source);
    Vehicle first(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    Vehicle second(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    Vehicle latest(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    QGeoCoordinate latestHome(48, 9, 550);
    if (latestAlreadyValid) {
        latest._setHomePosition(latestHome);
    }
    const auto origin = source.lastKnownPosition(false).coordinate();
    for (auto* vehicle : {&first, &second, &latest}) {
        emit vehicles.vehicleAdded(vehicle);
    }
    const auto updateOlderHomes = [&]() {
        QGeoCoordinate firstHome(46, 7, 450);
        QGeoCoordinate secondHome(47, 8, 500);
        first._setHomePosition(firstHome);
        second._setHomePosition(secondHome);
    };
    if (oldestFirst) {
        updateOlderHomes();
        QCOMPARE(source.lastKnownPosition(false).coordinate(), latestAlreadyValid ? latestHome : origin);
    }
    latest._setHomePosition(latestHome);
    QCOMPARE(source.lastKnownPosition(false).coordinate(), latestHome);
    if (!oldestFirst) {
        updateOlderHomes();
    }
    QGeoCoordinate changedHome(49, 10, 600);
    for (auto* vehicle : {&first, &second, &latest}) {
        vehicle->_setHomePosition(changedHome);
        QCOMPARE(source.lastKnownPosition(false).coordinate(), latestHome);
    }
}

void GPSManagerTest::_vehicleEstimateTracking()
{
    GPSGgaSources sources(nullptr, nullptr, nullptr);
    Vehicle first(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    Vehicle second(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    const QGeoCoordinate coordinate(47.3977, 8.5456, 450.0);
    const auto estimate = [&sources]() {
        return sources.vehicleEstimateHealth()->acceptedObservation(GPSObservation::PositionUse::Gga);
    };

    sources.setActiveVehicle(&first);
    emit second.positionReported(coordinate);
    QVERIFY(!estimate());
    emit first.positionReported(coordinate);
    auto observation = estimate();
    QVERIFY(observation);
    QVERIFY(observation->position.coordinate() == coordinate);
    QCOMPARE(observation->altitudeDatum, GPSAltitudeDatum::MeanSeaLevel);
    QCOMPARE(observation->fixQuality, GPSObservation::FixQuality::Extrapolated);
    QCOMPARE(observation->sourceId, QStringLiteral("vehicle/%1/ekf").arg(first.id()));

    emit first.positionReported(QGeoCoordinate());
    QVERIFY(!estimate());

    // Switching vehicles drops the previous estimate and ignores the new vehicle's earlier reports.
    emit first.positionReported(coordinate);
    QVERIFY(estimate());
    sources.setActiveVehicle(&second);
    QVERIFY(!estimate());
    emit first.positionReported(coordinate);
    QVERIFY(!estimate());
    emit second.positionReported(coordinate);
    QVERIFY(estimate());

    sources.setActiveVehicle(nullptr);
    QVERIFY(!estimate());
}

void GPSManagerTest::_ntripSettingsBinding()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerConnectEnabled(), false);

    const NTRIPConfiguration expected{
        .connection = {.host = QStringLiteral("caster.example.com"),
                       .port = 443,
                       .username = QStringLiteral("user"),
                       .password = QStringLiteral("pass"),
                       .mountpoint = QStringLiteral("MOUNT"),
                       .useTls = true,
                       .allowSelfSignedCerts = true,
                       .pinnedCertificate = QStringLiteral("caster.example.com:443|00ff")},
        .filter = {.whitelist = QStringLiteral("1005,1077")}};
    saved.setFactValue(settings->ntripServerHostAddress(), expected.connection.host);
    saved.setFactValue(settings->ntripServerPort(), expected.connection.port);
    saved.setFactValue(settings->ntripUsername(), expected.connection.username);
    saved.setFactValue(settings->ntripPassword(), expected.connection.password);
    saved.setFactValue(settings->ntripMountpoint(), expected.connection.mountpoint);
    saved.setFactValue(settings->ntripUseTls(), expected.connection.useTls);
    saved.setFactValue(settings->ntripAllowSelfSignedCerts(), expected.connection.allowSelfSignedCerts);
    saved.setFactValue(settings->ntripPinnedCertificate(), expected.connection.pinnedCertificate);
    saved.setFactValue(settings->ntripWhitelist(), expected.filter.whitelist);
    saved.setFactValue(settings->ntripGgaPositionSource(),
                       static_cast<int>(NTRIPGgaProvider::PositionSource::GCSPosition));
    saved.setFactValue(settings->ntripGgaIntervalSec(), 7);

    const NTRIPManager::Configuration configuration = GPSSettingsBindings::ntripConfiguration(settings);
    QVERIFY(!configuration.enabled);
    QCOMPARE(configuration.stream, expected);
    QCOMPARE(configuration.gga.source, NTRIPGgaProvider::PositionSource::GCSPosition);
    QCOMPARE(configuration.gga.interval, std::chrono::milliseconds(7000));

    NTRIPManager manager;
    GPSSettingsBindings::bindNtrip(settings, &manager);
    QCOMPARE(manager.configuration(), configuration);
    settings->ntripServerConnectEnabled()->setRawValue(true);
    QVERIFY(manager.configuration().enabled);
}

void GPSManagerTest::_settingsBindingsCoverEveryFact_data()
{
    QTest::addColumn<SettingsGroup*>("group");
    QTest::addColumn<QList<Fact*>>("bound");
    QTest::addColumn<QStringList>("ignored");

    // Facts the bindings deliberately leave to another consumer. A new setting must be bound or listed here.
    SettingsManager* const settings = SettingsManager::instance();
    QTest::newRow("RTK") << static_cast<SettingsGroup*>(settings->rtkSettings())
                         << GPSSettingsBindings::boundFacts(settings->rtkSettings())
                         << QStringList{
                                // GPSManager reads it once at startup.
                                QStringLiteral("connectOnStartup"),
                            };
    QTest::newRow("NTRIP") << static_cast<SettingsGroup*>(settings->ntripSettings())
                           << GPSSettingsBindings::boundFacts(settings->ntripSettings()) << QStringList{};
    QTest::newRow("GPSCorrection") << static_cast<SettingsGroup*>(settings->gpsCorrectionSettings())
                                   << GPSSettingsBindings::boundFacts(settings->gpsCorrectionSettings())
                                   << QStringList{};
}

void GPSManagerTest::_settingsBindingsCoverEveryFact()
{
    QFETCH(SettingsGroup*, group);
    QFETCH(QList<Fact*>, bound);
    QFETCH(QStringList, ignored);

    QStringList names;
    const QMetaObject* const metaObject = group->metaObject();
    for (int i = SettingsGroup::staticMetaObject.propertyCount(); i < metaObject->propertyCount(); ++i) {
        const QMetaProperty property = metaObject->property(i);
        if (property.metaType() != QMetaType::fromType<Fact*>()) {
            continue;
        }
        const QString name = QString::fromLatin1(property.name());
        names.append(name);
        const qsizetype expected = ignored.contains(name) ? 0 : 1;
        QVERIFY2(bound.count(property.read(group).value<Fact*>()) == expected,
                 qPrintable(QStringLiteral("%1 must be bound once or listed as ignored, not both").arg(name)));
    }
    for (const QString& name : ignored) {
        QVERIFY2(names.contains(name), qPrintable(QStringLiteral("Ignored %1 is not a Fact of the group").arg(name)));
    }
}

UT_REGISTER_TEST(GPSManagerTest, TestLabel::Unit)

#include "GPSManagerTest.moc"
