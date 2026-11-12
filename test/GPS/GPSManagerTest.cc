#include "GPSManagerTest.h"

#include <chrono>
#include <memory>

#include <QtCore/QDateTime>
#include <QtCore/QMetaEnum>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>
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
#include "PositionReceiverSettings.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "QmlTesting/QmlFixtures.h"
#include "RTKSettings.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "Receiver/Support/ScriptedReceiverWorker.h"
#include "SettingsManager.h"
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
    const auto allowConnections =
        qScopeGuard([links] { links->setConnectionsAllowed(LinkManager::ConnectionSuspension::FirmwareUpgrade); });
    manager.init();

    // The saved receiver connects at the first poll, which waits while new connections are suspended.
    links->setConnectionsSuspended(LinkManager::ConnectionSuspension::FirmwareUpgrade);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(workers.count(), 0);
    links->setConnectionsAllowed(LinkManager::ConnectionSuspension::FirmwareUpgrade);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(999)));
    QCOMPARE(workers.count(), 0);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(workers.count(), 1);
    QVERIFY(manager.receiver()->hasReceiver());

    manager.shutdown();
    QVERIFY(!manager.receiver()->hasReceiver());
}

void GPSManagerTest::_positionReceiverGivesGcsPosition_data()
{
    QTest::addColumn<PositionManager::SourceMode>("mode");
    // The latitude published while the position receiver is in use and has no position; none is NaN. The base
    // station reports 47 all along, this device 48 and the position receiver 49.
    QTest::addColumn<double>("silent");
    QTest::newRow("best-available") << PositionManager::SourceMode::Automatic << 48.0;
    QTest::newRow("receiver-only") << PositionManager::SourceMode::ReceiverOnly << qQNaN();
}

void GPSManagerTest::_positionReceiverGivesGcsPosition()
{
    using namespace std::chrono_literals;
    QFETCH(PositionManager::SourceMode, mode);
    QFETCH(double, silent);
    TestFixtures::SettingsFixture saved;
    SettingsManager* const settings = SettingsManager::instance();
    RTKSettings* const rtk = settings->rtkSettings();
    PositionReceiverSettings* const position = settings->positionReceiverSettings();
    saved.setFactValue(settings->ntripSettings()->ntripServerConnectEnabled(), false);
    // A base station on a tripod and the operator's receiver, each on a TCP address scripted workers never open.
    saved.setFactValue(rtk->receiverRole(), RTKSettings::ConfiguredBase);
    saved.setFactValue(rtk->baseReceiverManufacturer(), gpsReceiverManufacturerForType(GPSType::UBlox));
    saved.setFactValue(rtk->baseMode(), static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn));
    saved.setFactValue(rtk->connectionType(), RTKSettings::Tcp);
    saved.setFactValue(rtk->tcpHost(), QStringLiteral("base.test"));
    saved.setFactValue(rtk->tcpPort(), 2101);
    saved.setFactValue(rtk->autoConnect(), true);
    saved.setFactValue(rtk->gcsPositionSource(), static_cast<int>(mode));
    // As a new installation has it: out of use.
    saved.setFactValue(position->enabled(), position->enabled()->rawDefaultValue());
    saved.setFactValue(position->connectionType(), RTKSettings::Tcp);
    saved.setFactValue(position->tcpHost(), QStringLiteral("position.test"));
    saved.setFactValue(position->tcpPort(), 2102);

    GPSTest::PositionSource device;
    // Every position published from here on, the last ones as the manager shuts down.
    QList<double> published;
    ManualScheduler scheduler;
    GPSManager manager(nullptr, &scheduler);
    PositionManager* const positions = manager.positionManager();
    positions->setPlatformSourceFactory([&device](QObject*) { return &device; });
    GPSTest::ScriptedReceiverWorkerFactory baseWorkers;
    GPSTest::ScriptedReceiverWorkerFactory positionWorkers;
    manager.receiver()->setWorkerFactory(baseWorkers.workerFactory());
    manager.positionReceiver()->setWorkerFactory(positionWorkers.workerFactory());
    manager.init();
    // Whether or not the platform has granted the location permission.
    positions->setInternalPositionSource(&device, PositionManager::SourceStatus::WaitingForFix);
    connect(positions, &PositionManager::gcsPositionChanged, this, [&published](const QGeoCoordinate& coordinate) {
        if (coordinate.isValid()) {
            published.append(coordinate.latitude());
        }
    });

    const auto fixAt = [](double latitude) {
        GPSPositionReport report = GPSTest::fixReport(GPSFixQuality::Fix3D);
        report.navigation.latitudeDegrees = latitude;
        report.navigation.longitudeDegrees = 8;
        report.navigation.horizontalAccuracyMeters = 1.0f;
        return report;
    };
    // The base station and this device keep reporting, as they do, and the position receiver while @a reporting.
    bool reporting = false;
    const auto report = [&]() {
        // A base with a surveyed position reports a time fix: its position is the antenna's.
        baseWorkers.current()->position(GPSTest::fixReport(GPSFixQuality::NoFix));
        if (device.active) {
            QGeoPositionInfo fix(QGeoCoordinate(48, 8), QDateTime::currentDateTimeUtc());
            fix.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
            device.publish(fix);
        }
        if (reporting) {
            positionWorkers.current()->position(fixAt(49));
        }
    };
    // Long enough for a receiver's last position to go stale, and for a receiver that returned while this device
    // has a position to take over.
    const auto run = [&]() {
        for (int second = 0; second < 6; ++second) {
            (void) scheduler.advanceBy(1s);
            report();
        }
    };
    const auto latitude = [positions]() {
        return positions->acceptedObservation() ? positions->gcsPosition().latitude() : qQNaN();
    };

    // Base only, as it was: the antenna position of the base is the ground station's.
    QVERIFY(scheduler.advanceBy(1s));
    QCOMPARE(baseWorkers.count(), 1);
    QCOMPARE(positionWorkers.count(), 0);
    baseWorkers.current()->ready();
    baseWorkers.current()->baseStatus({.state = GPSBaseState::Valid,
                                       .position = {.latitudeDegrees = 47, .longitudeDegrees = 8},
                                       .meanAccuracyMeters = 1.0});
    run();
    QCOMPARE(latitude(), 47.0);
    QCOMPARE(positions->selectedSourceName(), PositionManager::tr("GNSS receiver"));

    // In use, the position receiver connects by itself, as a receiver that is sent nothing. Before it has a
    // position, the base's is already withdrawn.
    published.clear();
    position->enabled()->setRawValue(true);
    report();
    QCOMPARE(latitude(), silent);
    QVERIFY(scheduler.advanceBy(1s));
    QCOMPARE(positionWorkers.count(), 1);
    QCOMPARE(positionWorkers.current()->type(), GPSType::Passive);
    positionWorkers.current()->ready();
    report();
    QCOMPARE(latitude(), silent);
    QVERIFY(!manager.positionReceiver()->forwardingCorrections());

    reporting = true;
    run();
    QCOMPARE(latitude(), 49.0);
    QCOMPARE(positions->selectedSourceName(), PositionManager::tr("Position receiver"));

    // Only the base's RTCM is a correction source.
    GPSCorrectionManager* const corrections = manager.corrections();
    positionWorkers.current()->rtcm(GPSTest::rtcmMessage(1005, 20), scheduler.nowMs());
    QVERIFY(scheduler.advanceBy(1s));
    QCOMPARE(corrections->state(), GPSCorrectionManager::State::Waiting);
    baseWorkers.current()->rtcm(GPSTest::rtcmMessage(1005, 20), scheduler.nowMs());
    QVERIFY(scheduler.advanceBy(1s));
    QCOMPARE(corrections->state(), GPSCorrectionManager::State::Fresh);

    // The position receiver falls silent.
    reporting = false;
    run();
    QCOMPARE(latitude(), silent);
    reporting = true;
    run();
    QCOMPARE(latitude(), 49.0);

    // Its connection is lost, and returns.
    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
    positionWorkers.current()->fail(GPSConnectionError::DeviceError);
    verifyExpectedLogMessage();
    positionWorkers.finishRetired();
    reporting = false;
    report();
    QCOMPARE(latitude(), silent);
    QVERIFY(scheduler.advanceBy(1s));
    QCOMPARE(positionWorkers.count(), 2);
    positionWorkers.current()->ready();
    reporting = true;
    run();
    QCOMPARE(latitude(), 49.0);
    // Through all of it the base kept reporting, and its position was never the ground station's.
    QVERIFY2(!published.contains(47.0), "the base's antenna position was published");

    // Out of use, the position receiver is disconnected and stays so, and the base provides the position again.
    position->enabled()->setRawValue(false);
    QVERIFY(!manager.positionReceiver()->hasReceiver());
    positionWorkers.finishRetired();
    reporting = false;
    run();
    QCOMPARE(positionWorkers.count(), 2);
    QCOMPARE(latitude(), 47.0);
    QCOMPARE(positions->selectedSourceName(), PositionManager::tr("GNSS receiver"));
}

void GPSManagerTest::_discoveryLeavesPositionReceiverDevice()
{
#ifdef QGC_NO_SERIAL_LINK
    QSKIP("Receivers are discovered on serial devices");
#else
    using namespace std::chrono_literals;
    TestFixtures::SettingsFixture saved;
    SettingsManager* const settings = SettingsManager::instance();
    RTKSettings* const rtk = settings->rtkSettings();
    PositionReceiverSettings* const position = settings->positionReceiverSettings();
    saved.setFactValue(settings->ntripSettings()->ntripServerConnectEnabled(), false);
    // A configured base without a saved device connects the RTK receiver it finds on USB, and configures it.
    saved.setFactValue(rtk->receiverRole(), RTKSettings::ConfiguredBase);
    saved.setFactValue(rtk->baseReceiverManufacturer(), gpsReceiverManufacturerForType(GPSType::UBlox));
    saved.setFactValue(rtk->connectionType(), RTKSettings::Serial);
    saved.setFactValue(rtk->serialDevice(), QString());
    saved.setFactValue(rtk->autoConnect(), true);
    saved.setFactValue(position->connectionType(), RTKSettings::Serial);
    saved.setFactValue(position->serialDevice(), QStringLiteral("/test/position"));
    saved.setFactValue(position->enabled(), true);
    // Both receivers are RTK receivers on USB. The position receiver's has two serial interfaces and is listed
    // first, so discovery would take it.
    SerialPortManager::Port positionFirst = GPSTest::rtkPort(QStringLiteral("/test/position-first"));
    SerialPortManager::Port positionSaved = GPSTest::rtkPort(QStringLiteral("/test/position"));
    positionFirst.physicalDeviceId = positionSaved.physicalDeviceId = QStringLiteral("5446:425:position");
    const SerialPortManager::Port base = GPSTest::rtkPort(QStringLiteral("/test/base"));
    GPSTest::TestSerialPorts ports({positionFirst, positionSaved, base});
    ManualScheduler scheduler;
    GPSManager manager(nullptr, &scheduler, &ports.manager);
    manager.positionManager()->setPlatformSourceFactory([](QObject* parent) {
        auto* const source = new GPSTest::PositionSource;
        source->setParent(parent);
        return source;
    });
    GPSTest::ScriptedReceiverWorkerFactory baseWorkers;
    GPSTest::ScriptedReceiverWorkerFactory positionWorkers;
    manager.receiver()->setWorkerFactory(baseWorkers.workerFactory());
    manager.positionReceiver()->setWorkerFactory(positionWorkers.workerFactory());
    manager.init();
    // A newly listed device settles before discovery opens it.
    const auto settle = [&scheduler]() { return scheduler.advanceBy(SerialPortManager::NEW_PORT_SETTLE_DELAY + 2s); };

    // The user disconnected the position receiver, so nothing holds its device: discovery still leaves it, and
    // its other interface, alone.
    manager.positionReceiver()->disconnectReceiver();
    QVERIFY(settle());
    QCOMPARE(positionWorkers.count(), 0);
    QCOMPARE(baseWorkers.count(), 1);
    QCOMPARE(manager.receiver()->activeEndpoint(), QStringLiteral("/test/base"));

    // The position receiver connects its device as a receiver that is sent nothing.
    QVERIFY(manager.positionReceiver()->connectReceiver());
    QCOMPARE(positionWorkers.current()->type(), GPSType::Passive);
    QCOMPARE(manager.positionReceiver()->activeEndpoint(), QStringLiteral("/test/position"));

    // Out of use, its device is any other: with the base unplugged, discovery connects it.
    position->enabled()->setRawValue(false);
    positionWorkers.finishRetired();
    ports.plug({positionFirst, positionSaved});
    baseWorkers.finishRetired();
    QVERIFY(settle());
    QCOMPARE(baseWorkers.count(), 2);
    QCOMPARE(manager.receiver()->activeEndpoint(), QStringLiteral("/test/position-first"));
#endif
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

void GPSManagerTest::_vehicleGpsEllipsoidHeight_data()
{
    // GPS_RAW_INT.alt_ellipsoid of each report in millimeters; zero when the vehicle does not report the height.
    QTest::addColumn<QList<int>>("ellipsoidMm");
    // The height above the ellipsoid of the last report's observation, beside 450 m above mean sea level; none is NaN.
    QTest::addColumn<double>("ellipsoid");
    // The receiver's own geoid separation of 30 m, where the EGM96 model has 48.4 m.
    QTest::newRow("reported") << QList<int>{480000} << 480.0;
    QTest::newRow("not-reported") << QList<int>{0} << qQNaN();
    QTest::newRow("no-longer-reported") << QList<int>{480000, 0} << qQNaN();
}

void GPSManagerTest::_vehicleGpsEllipsoidHeight()
{
    QFETCH(QList<int>, ellipsoidMm);
    QFETCH(double, ellipsoid);
    ManualScheduler scheduler;
    NTRIPVehicleGgaSource source(&scheduler);
    Vehicle vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    source.setVehicle(&vehicle);
    for (const int reported : ellipsoidMm) {
        mavlink_gps_raw_int_t raw{};
        raw.lat = 470000000;
        raw.lon = 80000000;
        raw.alt = 450000;
        raw.alt_ellipsoid = reported;
        raw.fix_type = GPS_FIX_TYPE_3D_FIX;
        mavlink_message_t message{};
        mavlink_msg_gps_raw_int_encode(1, MAV_COMP_ID_AUTOPILOT1, &message, &raw);
        // As Vehicle hands a message on: to its fact groups, then to its observers.
        vehicle.gpsFactGroup()->handleMessage(&vehicle, message);
        emit vehicle.mavlinkMessageReceived(message);
    }
    const auto observation = source.gpsObservation();
    QVERIFY(observation);
    QCOMPARE(observation->altitudeDatum, GPSAltitudeDatum::MeanSeaLevel);
    QCOMPARE(observation->position.coordinate().altitude(), 450.0);
    QCOMPARE(observation->altitudeEllipsoidMeters.value_or(qQNaN()), ellipsoid);
    if (qIsFinite(ellipsoid)) {
        // What the GGA sent to a caster carries as the geoid separation.
        QCOMPARE(observation->geoidSeparationMeters().value_or(qQNaN()), ellipsoid - 450.0);
    }
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

void GPSManagerTest::_groundStationGgaReportsAnyAltitudeDatum_data()
{
    // The datum of the device position differs by platform; a caster needs its latitude and longitude.
    QTest::addColumn<GPSAltitudeDatum>("datum");
    // The altitude and geoid separation fields for 450 m at 47N 8E, where the geoid is 48.4 m above the ellipsoid.
    QTest::addColumn<QByteArray>("heights");
    QTest::newRow("mean-sea-level") << GPSAltitudeDatum::MeanSeaLevel << QByteArray(",450.0,M,48.4,M,");
    QTest::newRow("ellipsoid") << GPSAltitudeDatum::Ellipsoid << QByteArray(",401.6,M,48.4,M,");
    QTest::newRow("unknown") << GPSAltitudeDatum::Unknown << QByteArray(",450.0,M,,M,");
}

void GPSManagerTest::_groundStationGgaReportsAnyAltitudeDatum()
{
    QFETCH(GPSAltitudeDatum, datum);
    QFETCH(QByteArray, heights);
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
    QCOMPARE(transport->sentNmea.size(), 1);
    QVERIFY2(transport->sentNmea.first().contains(heights), transport->sentNmea.first().constData());
}

void GPSManagerTest::_qmlServicesAvailableBeforeInit()
{
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> root = engine.create(QByteArray(R"(
        import QtQml
        import QGroundControl
        QtObject {
            readonly property var corrections: QGroundControl.gpsManager.corrections
            readonly property var vehicleBytes: QGroundControl.gpsManager.corrections.vehicleBytesSubmitted
            readonly property var baseFacts: QGroundControl.gpsManager.receiver.facts
            readonly property var positionFacts: QGroundControl.gpsManager.positionReceiver.facts
        }
    )"));
    QVERIFY2(root, qPrintable(engine.lastError()));
    GPSCorrectionManager* const corrections = GPSManager::instance()->corrections();
    QCOMPARE(root->property("corrections").value<GPSCorrectionManager*>(), corrections);
    QVERIFY(root->property("vehicleBytes").isValid());
    QCOMPARE(root->property("vehicleBytes").toULongLong(), corrections->vehicleBytesSubmitted());
    QCOMPARE(root->property("baseFacts").value<FactGroup*>(), GPSManager::instance()->receiver()->facts());
    QCOMPARE(root->property("positionFacts").value<FactGroup*>(), GPSManager::instance()->positionReceiver()->facts());
}

UT_REGISTER_TEST(GPSManagerTest, TestLabel::Unit)
