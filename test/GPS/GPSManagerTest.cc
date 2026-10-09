#include "GPSManagerTest.h"

#include <memory>

#include <QtCore/QMetaEnum>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "FactGroup.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "GPSSourceHealth.h"
#include "LinkManager.h"
#include "ManualScheduler.h"
#include "NTRIP/Support/MockNTRIPTransport.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "NTRIPVehicleGgaSource.h"
#include "PositionManager.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "RTKSettings.h"
#include "Receiver/Support/ScriptedReceiverWorker.h"
#include "SettingsManager.h"
#include "Support/GPSQmlTestHelpers.h"
#include "Support/GPSTestHelpers.h"
#include "Vehicle.h"
#include "VehicleGPSFactGroup.h"

void GPSManagerTest::_correctionState()
{
    using State = GPSCorrectionManager::State;
    ManualScheduler scheduler;
    GPSManager manager(nullptr, &scheduler);
    const auto advanceTick = [&scheduler] { return scheduler.advanceBy(std::chrono::seconds(1)); };
    GPSCorrectionManager* const corrections = manager.corrections();
    QSignalSpy changes(corrections, &GPSCorrectionManager::stateChanged);
    QCOMPARE(corrections->state(), State::Inactive);

    // An enabled source that has delivered nothing is waiting.
    corrections->setConfiguration({.udpInput = {.enabled = true, .port = 0}, .udpOutput = {}});
    QCOMPARE(corrections->state(), State::Waiting);
    corrections->setConfiguration({});
    QCOMPARE(corrections->state(), State::Inactive);

    // So is an open source until its first frame.
    auto ntrip = corrections->openSource(GPSCorrectionSettings::Ntrip, QStringLiteral("caster"));
    QVERIFY(advanceTick());
    QCOMPARE(corrections->state(), State::Waiting);
    ntrip->submit(GPSTest::rtcmMessage(1005, 20), scheduler.nowMs());
    QVERIFY(advanceTick());
    QCOMPARE(corrections->state(), State::Fresh);
    ntrip.reset();
    QVERIFY(advanceTick());
    QCOMPARE(corrections->state(), State::Inactive);
    QCOMPARE(changes.count(), 5);
}

void GPSManagerTest::_positionManagerLifecycle()
{
    using Mode = PositionManager::SourceMode;
    using Status = PositionManager::SourceStatus;
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->ntripSettings()->ntripServerConnectEnabled(), false);
    Fact* const sourceSetting = SettingsManager::instance()->rtkSettings()->gcsPositionSource();
    saved.setFactValue(sourceSetting, static_cast<int>(Mode::InternalOnly));
    // Receiver connection polling runs on this clock, which the test never advances.
    ManualScheduler scheduler;
    GPSManager manager(nullptr, &scheduler);
    PositionManager* const positions = manager.positionManager();
    // Unit tests never create Qt's platform source.
    positions->setPlatformSourceFactory([](QObject* parent) {
        auto* const source = new GPSTest::PositionSource;
        source->setParent(parent);
        return source;
    });
    QCOMPARE(positions->configuration().sourceMode, Mode::Automatic);
    QCOMPARE(positions->sourceStatus(), Status::NoSource);

    manager.init();
    QCOMPARE(positions->configuration().sourceMode, Mode::InternalOnly);
    // init() has either requested location permission or bound the platform source.
    QVERIFY(positions->sourceStatus() != Status::NoSource);
    sourceSetting->setRawValue(static_cast<int>(Mode::ReceiverOnly));
    QCOMPARE(positions->configuration().sourceMode, Mode::ReceiverOnly);

    manager.shutdown();
    sourceSetting->setRawValue(static_cast<int>(Mode::Automatic));
    QCOMPARE(positions->configuration().sourceMode, Mode::ReceiverOnly);
}

void GPSManagerTest::_sourceModeMatchesSetting()
{
    const Fact* const setting = SettingsManager::instance()->rtkSettings()->gcsPositionSource();
    const QMetaEnum modes = QMetaEnum::fromType<PositionManager::SourceMode>();
    QCOMPARE(setting->enumValues().size(), modes.keyCount());
    for (int i = 0; i < modes.keyCount(); ++i) {
        QCOMPARE(setting->enumValues().at(i).toInt(), modes.value(i));
    }
}

void GPSManagerTest::_connectionPolling()
{
    TestFixtures::SettingsFixture saved;
    SettingsManager* const settings = SettingsManager::instance();
    RTKSettings* const rtk = settings->rtkSettings();
    saved.setFactValue(settings->ntripSettings()->ntripServerConnectEnabled(), false);
    saved.setFactValue(rtk->receiverRole(), RTKSettings::Passive);
    saved.setFactValue(rtk->connectionType(), RTKSettings::Tcp);
    saved.setFactValue(rtk->tcpHost(), QStringLiteral("rtk.test"));
    saved.setFactValue(rtk->tcpPort(), 2101);
    saved.setFactValue(rtk->autoConnect(), true);
    ManualScheduler scheduler;
    GPSManager manager(nullptr, &scheduler);
    manager.positionManager()->setPlatformSourceFactory([](QObject* parent) {
        auto* const source = new GPSTest::PositionSource;
        source->setParent(parent);
        return source;
    });
    GPSTest::ScriptedReceiverWorkerFactory workers;
    manager.receiver()->setWorkerFactory(workers.workerFactory());
    LinkManager* const links = LinkManager::instance();
    const auto allowConnections = qScopeGuard([links] { links->setConnectionsAllowed(); });
    manager.init();

    // The saved receiver connects at the first poll, which waits while new connections are suspended.
    links->setConnectionsSuspended(QStringLiteral("GPSManagerTest"));
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(workers.count(), 0);
    links->setConnectionsAllowed();
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(999)));
    QCOMPARE(workers.count(), 0);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(workers.count(), 1);
    QVERIFY(manager.receiver()->hasReceiver());

    manager.shutdown();
    QVERIFY(!manager.receiver()->hasReceiver());
}

void GPSManagerTest::_vehicleGpsTracking()
{
    ManualScheduler scheduler;
    NTRIPVehicleGgaSource source(&scheduler);
    Vehicle first(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    Vehicle second(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    const auto report = [](Vehicle& vehicle, uint8_t fixType) {
        vehicle.gpsFactGroup()->handleMessage(&vehicle, GPSTest::gpsRawMessage({.latitudeE7 = 470000000,
                                                                                .longitudeE7 = 80000000,
                                                                                .altitudeMm = 123456,
                                                                                .fixType = fixType,
                                                                                .eph = 120}));
    };
    const auto observation = [&source]() { return source.gpsObservation(); };

    source.setVehicle(&first);
    report(second, GPS_FIX_TYPE_RTK_FIXED);
    QVERIFY(!observation());
    report(first, GPS_FIX_TYPE_RTK_FIXED);
    auto accepted = observation();
    QVERIFY(accepted);
    QCOMPARE(accepted->position.coordinate(), QGeoCoordinate(47, 8, 123.456));
    QCOMPARE(accepted->altitudeDatum, GPSAltitudeDatum::MeanSeaLevel);
    QCOMPARE(accepted->fixQuality, GPSObservation::FixQuality::RTKFixed);
    QCOMPARE(accepted->horizontalDop, 1.2);

    // Reports go stale without an update, and a report without a fix is not used.
    QVERIFY(scheduler.advanceBy(GPSSourceHealth::FRESHNESS_TIMEOUT - std::chrono::milliseconds(1)));
    QVERIFY(observation());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY(!observation());
    report(first, GPS_FIX_TYPE_3D_FIX);
    QVERIFY(observation());
    report(first, GPS_FIX_TYPE_NO_FIX);
    QVERIFY(!observation());

    report(first, GPS_FIX_TYPE_3D_FIX);
    source.setVehicle(&second);
    QVERIFY(!observation());
    report(second, GPS_FIX_TYPE_3D_FIX);
    QVERIFY(observation());
    source.setVehicle(nullptr);
    QVERIFY(!observation());
}

void GPSManagerTest::_vehicleEstimateTracking()
{
    ManualScheduler scheduler;
    NTRIPVehicleGgaSource source(&scheduler);
    Vehicle first(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    Vehicle second(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    const QGeoCoordinate coordinate(47.3977, 8.5456, 450.0);
    const auto report = [](Vehicle& vehicle, const QGeoCoordinate& position) {
        mavlink_message_t message;
        (void) mavlink_msg_global_position_int_pack_chan(
            1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_0, &message, 0, qRound(position.latitude() * 1e7),
            qRound(position.longitude() * 1e7), qRound(position.altitude() * 1000), 0, 0, 0, 0, 0);
        emit vehicle.mavlinkMessageReceived(message);
    };
    const QGeoCoordinate noPosition(0, 0, 450.0);
    const auto estimate = [&source]() { return source.estimateObservation(); };

    source.setVehicle(&first);
    report(second, coordinate);
    QVERIFY(!estimate());
    report(first, coordinate);
    auto observation = estimate();
    QVERIFY(observation);
    QCOMPARE(observation->position.coordinate(), coordinate);
    QCOMPARE(observation->altitudeDatum, GPSAltitudeDatum::MeanSeaLevel);
    QCOMPARE(observation->fixQuality, GPSObservation::FixQuality::Extrapolated);

    // ArduPilot's 0/0 report means the vehicle has no position.
    report(first, noPosition);
    QVERIFY(!estimate());

    report(first, coordinate);
    QVERIFY(scheduler.advanceBy(GPSSourceHealth::FRESHNESS_TIMEOUT));
    QVERIFY(!estimate());

    // Switching vehicles drops the previous estimate and ignores the new vehicle's earlier reports.
    report(first, coordinate);
    QVERIFY(estimate());
    source.setVehicle(&second);
    QVERIFY(!estimate());
    report(first, coordinate);
    QVERIFY(!estimate());
    report(second, coordinate);
    QVERIFY(estimate());

    source.setVehicle(nullptr);
    QVERIFY(!estimate());
}

void GPSManagerTest::_groundStationGgaNeedsMeanSeaLevel_data()
{
    QTest::addColumn<GPSAltitudeDatum>("datum");
    QTest::addColumn<bool>("sent");
    QTest::newRow("mean-sea-level") << GPSAltitudeDatum::MeanSeaLevel << true;
    QTest::newRow("ellipsoid") << GPSAltitudeDatum::Ellipsoid << false;
    QTest::newRow("unknown") << GPSAltitudeDatum::Unknown << false;
}

void GPSManagerTest::_groundStationGgaNeedsMeanSeaLevel()
{
    QFETCH(GPSAltitudeDatum, datum);
    QFETCH(bool, sent);
    GPSTest::PositionSource source;
    GPSManager manager;
    PositionManager* const groundStation = manager.positionManager();
    groundStation->setConfiguration({.sourceMode = PositionManager::SourceMode::InternalOnly});
    groundStation->setInternalPositionSource(&source, PositionManager::SourceStatus::WaitingForFix, false, datum);
    QGeoPositionInfo fix(QGeoCoordinate(47, 8, 450), QDateTime::currentDateTimeUtc());
    fix.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    fix.setAttribute(QGeoPositionInfo::VerticalAccuracy, 1);
    source.publish(fix);

    NTRIPManager* const ntrip = manager.ntrip();
    manager._initGgaSources();
    auto configuration = GPSTest::mockCasterConfiguration();
    configuration.gga.source = NTRIPGgaReporter::PositionSource::GCSPosition;
    ntrip->setConfiguration(configuration);
    auto* transport = GPSTest::injectMockTransport(*ntrip, true);
    ntrip->init();
    QCOMPARE(ntrip->connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    QCOMPARE(transport->sentNmea.size(), sent ? 1 : 0);
}

void GPSManagerTest::_qmlServicesAvailableBeforeInit()
{
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> root = engine.create(QByteArray(R"(
        import QtQml
        import QGroundControl
        QtObject {
            readonly property var corrections: QGroundControl.gpsManager.corrections
            readonly property var vehicleBytes: QGroundControl.gpsManager.corrections.vehicleBytesSubmitted
            readonly property var baseFacts: QGroundControl.gpsManager.receiver.facts
        }
    )"));
    QVERIFY2(root, qPrintable(engine.lastError()));
    GPSCorrectionManager* const corrections = GPSManager::instance()->corrections();
    QCOMPARE(root->property("corrections").value<GPSCorrectionManager*>(), corrections);
    QVERIFY(root->property("vehicleBytes").isValid());
    QCOMPARE(root->property("vehicleBytes").toULongLong(), corrections->vehicleBytesSubmitted());
    QCOMPARE(root->property("baseFacts").value<FactGroup*>(), GPSManager::instance()->receiver()->facts());
}

UT_REGISTER_TEST(GPSManagerTest, TestLabel::Unit)
