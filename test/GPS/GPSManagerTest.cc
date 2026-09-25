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
#include "GPSRtk.h"
#include "GPSSettingsBindings.h"
#include "GPSSourceHealth.h"
#include "GpsTestHelpers.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "PositionManager.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

class GPSManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _correctionStatus();
    void _correctionStateFacade();
    void _positionManagerLifecycle();
    void _vehicleEstimateTracking();
    void _ntripSettingsBinding();
};

void GPSManagerTest::_correctionStatus()
{
    using State = GPSManager::CorrectionState;
    GPSCorrectionManager corrections;
    NTRIPManager ntrip;
    GPSRtk rtk;
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

    auto source = corrections.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster"));
    corrections.acceptIngress(
        source.event(GpsTestHelpers::buildRtcmFrame(1005, 20), GPSCorrectionFrame::monotonicNowMs(), 1005, true));
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
    QGCPositionManager* const positions = manager.positionManager();
    QCOMPARE(positions->sourceMode(), Mode::Automatic);

    manager.init();
    QCOMPARE(positions->sourceMode(), Mode::InternalOnly);

    manager.shutdown();
    sourceSetting->setRawValue(static_cast<int>(Mode::Automatic));
    QCOMPARE(positions->sourceMode(), Mode::InternalOnly);
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

    const NTRIPConfiguration expected{.connection = {.host = QStringLiteral("caster.example.com"),
                                                     .port = 443,
                                                     .username = QStringLiteral("user"),
                                                     .password = QStringLiteral("pass"),
                                                     .mountpoint = QStringLiteral("MOUNT"),
                                                     .useTls = true,
                                                     .allowSelfSignedCerts = true},
                                      .filter = {.whitelist = QStringLiteral("1005,1077")}};
    saved.setFactValue(settings->ntripServerHostAddress(), expected.connection.host);
    saved.setFactValue(settings->ntripServerPort(), expected.connection.port);
    saved.setFactValue(settings->ntripUsername(), expected.connection.username);
    saved.setFactValue(settings->ntripPassword(), expected.connection.password);
    saved.setFactValue(settings->ntripMountpoint(), expected.connection.mountpoint);
    saved.setFactValue(settings->ntripUseTls(), expected.connection.useTls);
    saved.setFactValue(settings->ntripAllowSelfSignedCerts(), expected.connection.allowSelfSignedCerts);
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

UT_REGISTER_TEST(GPSManagerTest, TestLabel::Unit)

#include "GPSManagerTest.moc"
