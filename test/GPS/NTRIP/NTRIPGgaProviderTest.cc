#include "NTRIPGgaProviderTest.h"

#include <QtCore/QScopeGuard>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fact.h"
#include "FactGroup.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "GPSSourceHealth.h"
#include "MAVLinkLib.h"
#include "ManualScheduler.h"
#include "MockNTRIPTransport.h"
#include "MultiVehicleManager.h"
#include "NMEASentence.h"
#include "NMEAUtils.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "PositionManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleGPSFactGroup.h"
#include "VehicleLinkManager.h"

namespace {

using Source = NTRIPGgaProvider::PositionSource;

mavlink_message_t gpsMessage(int vehicleId, int32_t altitude = 123000, int32_t latitude = 473977000,
                             int32_t longitude = 85456000, uint8_t fixType = GPS_FIX_TYPE_3D_FIX)
{
    mavlink_gps_raw_int_t raw{};
    raw.time_usec = 1234567;
    raw.lat = latitude;
    raw.lon = longitude;
    raw.alt = altitude;
    raw.fix_type = fixType;
    raw.eph = 110;
    raw.epv = 220;
    mavlink_message_t message{};
    mavlink_msg_gps_raw_int_encode(vehicleId, MAV_COMP_ID_AUTOPILOT1, &message, &raw);
    return message;
}

mavlink_message_t fusedMessage(int vehicleId, int32_t altitude = 450000, int32_t latitude = 473977000,
                               int32_t longitude = 85456000)
{
    mavlink_global_position_int_t global{};
    global.lat = latitude;
    global.lon = longitude;
    global.alt = altitude;
    mavlink_message_t message{};
    mavlink_msg_global_position_int_encode(vehicleId, MAV_COMP_ID_AUTOPILOT1, &message, &global);
    return message;
}

bool receiveMessage(Vehicle& vehicle, const mavlink_message_t& message)
{
    return QMetaObject::invokeMethod(&vehicle, "_mavlinkMessageReceived", Qt::DirectConnection,
                                     Q_ARG(LinkInterface*, nullptr), Q_ARG(mavlink_message_t, message));
}

void configureNtrip(TestFixtures::SettingsFixture& saved, Source source)
{
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerConnectEnabled(), false);
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("localhost"));
    saved.setFactValue(settings->ntripServerPort(), 2101);
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("test"));
    saved.setFactValue(settings->ntripGgaPositionSource(), static_cast<int>(source));
}

}  // namespace

void NTRIPGgaProviderTest::initTestCase()
{
    UnitTest::initTestCase();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->ntripSettings()->ntripServerConnectEnabled(), false);
    GPSManager::instance()->init();
}

void NTRIPGgaProviderTest::cleanup()
{
    GPSManager::instance()->ntrip()->stopNTRIP();
    UnitTest::cleanup();
}

void NTRIPGgaProviderTest::testSourceClearedOnStopAndFreshStart()
{
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;

    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() {
        return PositionResult{QGeoCoordinate(47.3977, 8.5456, 450.0), QStringLiteral("Vehicle GPS"),
                              GPSAltitudeDatum::MeanSeaLevel};
    });

    provider.start(&transport);
    QCOMPARE(provider.currentSource(), QStringLiteral("Vehicle GPS"));
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(transport.sentNmea.first().startsWith("$GPGGA,"));
    const auto fields = transport.sentNmea.first().split(',');
    QCOMPARE(fields.at(NMEA::Field::GGA_QUALITY).toUInt(), NMEA::GgaQuality::ESTIMATED);
    QVERIFY(fields.at(NMEA::Field::GGA_HDOP).isEmpty());
    QVERIFY(fields.at(NMEA::Field::GGA_SATELLITES_USED).isEmpty());

    provider.stop();
    QVERIFY(provider.currentSource().isEmpty());

    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() { return PositionResult{}; });
    provider.start(&transport);
    QVERIFY(provider.currentSource().isEmpty());
}

void NTRIPGgaProviderTest::testRTKReceiverProvider()
{
    TestFixtures::SettingsFixture saved;
    configureNtrip(saved, Source::RTKReceiver);
    auto* receiver = GPSManager::instance()->gpsRtk();
    auto* manager = GPSManager::instance()->ntrip();
    const auto start = [manager]() {
        auto* transport = new MockNTRIPTransport(manager);
        manager->setTransportForTest(transport);
        manager->startNTRIP();
        return transport;
    };
    auto* transport = start();
    QCOMPARE(manager->connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    QVERIFY(manager->ggaSource().isEmpty());
    QVERIFY(transport->sentNmea.isEmpty());
    manager->stopNTRIP();

    GPSPositionReport report;
    report.navigation.fixType = GPSFixQuality::RTKFixed;
    report.navigation.latitudeDegrees = 47.3977;
    report.navigation.longitudeDegrees = 8.5456;
    report.navigation.altitudeMslMeters = 450.0;
    report.navigation.horizontalAccuracyMeters = 0.02f;
    report.navigation.horizontalDop = 0.7f;
    report.navigation.satellitesUsed = 21;
    const auto retire = qScopeGuard([receiver]() { receiver->disconnectGPS(); });
    QVERIFY(
        QMetaObject::invokeMethod(receiver, "_positionUpdate", Qt::DirectConnection, Q_ARG(GPSPositionReport, report)));
    transport = start();
    QCOMPARE(manager->ggaSource(), QStringLiteral("RTK Receiver"));
    QCOMPARE(transport->sentNmea.size(), 1);
    const auto& wire = transport->sentNmea.first();
    const auto decoded = NMEA::sentence(std::string_view(wire.constData(), wire.size()));
    QVERIFY(decoded);
    const auto fix = NMEA::gga(*decoded);
    QVERIFY(fix);
    QVERIFY(qAbs(fix->latitude - 47.3977) < 1e-6);
    QVERIFY(qAbs(fix->longitude - 8.5456) < 1e-6);
    QCOMPARE(fix->quality, NMEA::GgaQuality::RTK_FIXED);
    QCOMPARE(fix->satellitesUsed, std::optional<unsigned>(21));
    manager->stopNTRIP();

    receiver->disconnectGPS();
    transport = start();
    QVERIFY(manager->ggaSource().isEmpty());
    QVERIFY(transport->sentNmea.isEmpty());
}

void NTRIPGgaProviderTest::_invalidProviderAltitude_data()
{
    QTest::addColumn<double>("altitude");
    QTest::newRow("unknown") << qQNaN();
    QTest::newRow("positive-infinity") << qInf();
    QTest::newRow("negative-infinity") << -qInf();
}

void NTRIPGgaProviderTest::_invalidProviderAltitude()
{
    QFETCH(double, altitude);
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.setPositionProvider(Source::VehicleGPS, [altitude]() {
        return PositionResult{QGeoCoordinate(47, 8, altitude), QStringLiteral("Vehicle GPS"),
                              GPSAltitudeDatum::MeanSeaLevel};
    });
    provider.setPositionProvider(Source::GCSPosition, []() {
        return PositionResult{QGeoCoordinate(48, 9, 0), QStringLiteral("GCS"), GPSAltitudeDatum::MeanSeaLevel};
    });

    provider.configure({Source::VehicleGPS});
    provider.start(&transport);
    QVERIFY(transport.sentNmea.isEmpty());
    QVERIFY(provider.currentSource().isEmpty());
    provider.stop();

    provider.configure({Source::Auto});
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), 1);
    QCOMPARE(provider.currentSource(), QStringLiteral("GCS"));
    const auto fields = transport.sentNmea.first().split(',');
    QCOMPARE(fields.size(), 15);
    QCOMPARE(fields.at(NMEA::Field::GGA_ALTITUDE), QByteArray("0.0"));
    QCOMPARE(fields.at(NMEA::Field::GGA_ALTITUDE_UNITS), QByteArray("M"));
    QVERIFY(fields.at(NMEA::Field::GGA_GEOID_SEPARATION).isEmpty());
    QVERIFY(NMEAUtils::verifyChecksum(transport.sentNmea.first()));
}

void NTRIPGgaProviderTest::_activeVehicleAndCommunicationLoss()
{
    TestFixtures::SettingsFixture saved;
    configureNtrip(saved, Source::VehicleGPS);
    auto* settings = SettingsManager::instance()->ntripSettings();
    auto* ntrip = GPSManager::instance()->ntrip();
    Vehicle first(nullptr, 17, MAV_COMP_ID_AUTOPILOT1, MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    Vehicle second(nullptr, 18, MAV_COMP_ID_AUTOPILOT1, MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    auto* manager = MultiVehicleManager::instance();
    Vehicle* previous = manager->activeVehicle();
    const auto restoreActiveVehicle = qScopeGuard([manager, previous]() {
        manager->setActiveVehicle(previous);
        QVERIFY(QTest::qWaitFor([manager, previous]() { return manager->activeVehicle() == previous; },
                                TestTimeout::shortMs()));
    });
    QVERIFY(receiveMessage(first, gpsMessage(first.id())));
    QVERIFY(receiveMessage(first, fusedMessage(first.id(), 450000, 485000000, 97500000)));
    QVERIFY(receiveMessage(second, gpsMessage(second.id(), 234000, 483977000, 95456000)));
    QVERIFY(receiveMessage(second, fusedMessage(second.id(), 678000, 495000000, 107500000)));
    for (Vehicle* vehicle : {&first, &second}) {
        manager->setActiveVehicle(vehicle);
        QTRY_COMPARE_WITH_TIMEOUT(manager->activeVehicle(), vehicle, TestTimeout::shortMs());
        for (const auto source : {Source::VehicleGPS, Source::VehicleEKF}) {
            settings->ntripGgaPositionSource()->setRawValue(static_cast<int>(source));
            auto* transport = new MockNTRIPTransport(ntrip);
            ntrip->setTransportForTest(transport);
            ntrip->startNTRIP();
            QCOMPARE(transport->sentNmea.size(), 1);
            const auto* gps = qobject_cast<VehicleGPSFactGroup*>(vehicle->gpsFactGroup());
            QVERIFY(gps);
            const auto observation =
                source == Source::VehicleGPS ? gps->acceptedObservation() : vehicle->acceptedPositionObservation();
            QVERIFY(observation);
            const auto expected = observation->position.coordinate();
            const NMEA::GGA fix{
                .latitude = expected.latitude(),
                .longitude = expected.longitude(),
                .altitude = expected.altitude(),
                .hdop = observation->horizontalDop.value_or(qQNaN()),
                .quality = source == Source::VehicleGPS ? NMEA::GgaQuality::GPS : NMEA::GgaQuality::ESTIMATED,
            };
            const auto expectedFields = NMEAUtils::makeGGA(fix, QTime(12, 0)).split(',');
            const auto fields = transport->sentNmea.first().split(',');
            QCOMPARE(fields.size(), 15);
            QCOMPARE(fields.mid(2, 11), expectedFields.mid(2, 11));
            QVERIFY(fields.at(NMEA::Field::GGA_GEOID_SEPARATION).isEmpty());
            QCOMPARE(ntrip->ggaSource(),
                     source == Source::VehicleGPS ? QStringLiteral("Vehicle GPS") : QStringLiteral("Vehicle EKF"));
            QVERIFY(NMEAUtils::verifyChecksum(transport->sentNmea.first()));
            ntrip->stopNTRIP();
        }
    }

    QVERIFY(QMetaObject::invokeMethod(second.vehicleLinkManager(), "_commLostCheck", Qt::DirectConnection));
    QVERIFY(second.vehicleLinkManager()->communicationLost());
    QVERIFY(receiveMessage(second, gpsMessage(second.id())));
    QVERIFY(receiveMessage(second, fusedMessage(second.id())));
    QVERIFY(second.vehicleLinkManager()->communicationLost());
    for (const auto source : {Source::VehicleGPS, Source::VehicleEKF}) {
        settings->ntripGgaPositionSource()->setRawValue(static_cast<int>(source));
        auto* transport = new MockNTRIPTransport(ntrip);
        ntrip->setTransportForTest(transport);
        ntrip->startNTRIP();
        QVERIFY(ntrip->ggaSource().isEmpty());
        QVERIFY(transport->sentNmea.isEmpty());
        ntrip->stopNTRIP();
    }
    manager->setActiveVehicle(nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(!manager->activeVehicle(), TestTimeout::shortMs());
    auto* transport = new MockNTRIPTransport(ntrip);
    ntrip->setTransportForTest(transport);
    ntrip->startNTRIP();
    QVERIFY(transport->sentNmea.isEmpty());
    QVERIFY(ntrip->ggaSource().isEmpty());
    ntrip->stopNTRIP();
}

void NTRIPGgaProviderTest::_gcsObservation_data()
{
    QTest::addColumn<QGeoCoordinate>("coordinate");
    QTest::addColumn<GPSAltitudeDatum>("datum");
    QTest::addColumn<double>("horizontalAccuracy");
    QTest::addColumn<bool>("fixValid");
    QTest::addColumn<bool>("accepted");
    const QGeoCoordinate position(47.3977, 8.5456, 450);
    QTest::newRow("missing-vertical-accuracy") << position << GPSAltitudeDatum::MeanSeaLevel << 1.0 << true << true;
    QTest::newRow("missing-all-accuracy") << position << GPSAltitudeDatum::MeanSeaLevel << qQNaN() << true << true;
    QTest::newRow("unknown-datum") << position << GPSAltitudeDatum::Unknown << 1.0 << true << false;
    QTest::newRow("ellipsoid-only") << position << GPSAltitudeDatum::Ellipsoid << 1.0 << true << false;
    QTest::newRow("unknown-altitude") << QGeoCoordinate(47.3977, 8.5456) << GPSAltitudeDatum::MeanSeaLevel << 1.0
                                      << true << false;
    QTest::newRow("zero-island") << QGeoCoordinate(0, 0, 450) << GPSAltitudeDatum::MeanSeaLevel << 1.0 << true << true;
    QTest::newRow("invalid-coordinate") << QGeoCoordinate() << GPSAltitudeDatum::MeanSeaLevel << 1.0 << true << false;
    QTest::newRow("no-fix") << position << GPSAltitudeDatum::MeanSeaLevel << 1.0 << false << false;
}

void NTRIPGgaProviderTest::_gcsObservation()
{
    QFETCH(QGeoCoordinate, coordinate);
    QFETCH(GPSAltitudeDatum, datum);
    QFETCH(double, horizontalAccuracy);
    QFETCH(bool, fixValid);
    QFETCH(bool, accepted);
    TestFixtures::SettingsFixture saved;
    configureNtrip(saved, Source::GCSPosition);
    auto* positioning = QGCPositionManager::instance();
    const auto savedMode = positioning->sourceMode();
    const auto restoreMode = qScopeGuard([&]() { positioning->setSourceMode(savedMode); });
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    auto registration = positioning->registerPositionSource(GPSPositionService::SelectedSource::Receiver, &health);
    positioning->setSourceMode(GPSPositionService::SourceMode::ReceiverOnly);

    GPSObservation observation;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.position = QGeoPositionInfo(coordinate, observation.receivedAt);
    observation.altitudeDatum = datum;
    observation.receiverFixValid = fixValid;
    if (qIsFinite(horizontalAccuracy)) {
        observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, horizontalAccuracy);
    }
    health.updateObservation(observation);
    QVERIFY(!qIsFinite(positioning->gcsPosition().altitude()));

    auto* manager = GPSManager::instance()->ntrip();
    auto* transport = new MockNTRIPTransport(manager);
    manager->setTransportForTest(transport);
    manager->startNTRIP();
    QCOMPARE(manager->connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    QCOMPARE(transport->sentNmea.size(), accepted ? 1 : 0);
    QCOMPARE(manager->ggaSource(), accepted ? QStringLiteral("GCS Position") : QString());
    if (accepted) {
        const auto& wire = transport->sentNmea.first();
        const auto decoded = NMEA::sentence(std::string_view(wire.constData(), wire.size()));
        QVERIFY(decoded);
        const auto fix = NMEA::gga(*decoded);
        QVERIFY(fix);
        QVERIFY(qAbs(fix->latitude - coordinate.latitude()) < 1e-6);
        QVERIFY(qAbs(fix->longitude - coordinate.longitude()) < 1e-6);
        const auto fields = transport->sentNmea.first().split(',');
        QCOMPARE(fields.size(), 15);
        QCOMPARE(fields.at(NMEA::Field::GGA_ALTITUDE), QByteArray("450.0"));
        QCOMPARE(fields.at(NMEA::Field::GGA_ALTITUDE_UNITS), QByteArray("M"));
        QVERIFY(fields.at(NMEA::Field::GGA_GEOID_SEPARATION).isEmpty());
        QVERIFY(NMEAUtils::verifyChecksum(transport->sentNmea.first()));
    }
    manager->stopNTRIP();
}

void NTRIPGgaProviderTest::_gcsSelectionAndFreshness()
{
    TestFixtures::SettingsFixture saved;
    configureNtrip(saved, Source::Auto);
    auto* rtkFacts = qobject_cast<GPSRTKFactGroup*>(GPSManager::instance()->gpsRtk()->gpsRtkFactGroup());
    QVERIFY(rtkFacts);
    saved.setFactValue(rtkFacts->valid(), false);
    auto* vehicles = MultiVehicleManager::instance();
    Vehicle* previous = vehicles->activeVehicle();
    const auto restoreVehicle = qScopeGuard([&]() {
        vehicles->setActiveVehicle(previous);
        QVERIFY(QTest::qWaitFor([&]() { return vehicles->activeVehicle() == previous; }, TestTimeout::shortMs()));
    });
    vehicles->setActiveVehicle(nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(!vehicles->activeVehicle(), TestTimeout::shortMs());

    auto* positioning = QGCPositionManager::instance();
    const auto savedMode = positioning->sourceMode();
    const auto restoreMode = qScopeGuard([&]() { positioning->setSourceMode(savedMode); });
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSObservation observation;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.position = QGeoPositionInfo(QGeoCoordinate(47.3977, 8.5456, 450), observation.receivedAt);
    observation.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
    observation.receiverFixValid = true;
    health.updateObservation(observation);
    auto registration = positioning->registerPositionSource(GPSPositionService::SelectedSource::Receiver, &health);
    positioning->setSourceMode(GPSPositionService::SourceMode::ReceiverOnly);

    auto* manager = GPSManager::instance()->ntrip();
    const auto checkGga = [&](bool accepted) {
        auto* transport = new MockNTRIPTransport(manager);
        manager->setTransportForTest(transport);
        manager->startNTRIP();
        QCOMPARE(manager->connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
        QCOMPARE(transport->sentNmea.size(), accepted ? 1 : 0);
        QCOMPARE(manager->ggaSource(), accepted ? QStringLiteral("GCS Position") : QString());
        manager->stopNTRIP();
    };
    checkGga(false);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds{1}));
    observation.monotonicTimestampUs = scheduler.nowUs();
    health.updateObservation(observation);
    QVERIFY(!positioning->gcsPosition().isValid());
    checkGga(true);

    // Reselecting a pinned receiver requires an observation made after the selection.
    positioning->setSourceMode(GPSPositionService::SourceMode::InternalOnly);
    positioning->setSourceMode(GPSPositionService::SourceMode::ReceiverOnly);
    checkGga(false);
    health.updateObservation(observation);
    checkGga(true);

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds{GPSSourceHealth::FRESHNESS_TIMEOUT_MS}));
    checkGga(false);
    observation.monotonicTimestampUs = scheduler.nowUs();
    health.updateObservation(observation);
    checkGga(true);
    observation.receiverFixValid = false;
    health.updateObservation(observation);
    checkGga(false);
}

void NTRIPGgaProviderTest::_vehicleFixLossAndExpiry()
{
    TestFixtures::SettingsFixture saved;
    configureNtrip(saved, Source::Auto);
    auto* vehicles = MultiVehicleManager::instance();
    Vehicle* previous = vehicles->activeVehicle();
    Vehicle vehicle(nullptr, 17, MAV_COMP_ID_AUTOPILOT1, MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    const auto restore = qScopeGuard([&]() {
        vehicles->setActiveVehicle(previous);
        QVERIFY(QTest::qWaitFor([&]() { return vehicles->activeVehicle() == previous; }, TestTimeout::shortMs()));
    });
    saved.setFactValue(GPSManager::instance()->gpsRtk()->gpsRtkFactGroup()->getFact(QStringLiteral("valid")), false);
    auto* positioning = QGCPositionManager::instance();
    const auto savedMode = positioning->sourceMode();
    const auto restoreMode = qScopeGuard([&]() { positioning->setSourceMode(savedMode); });
    // No receiver is registered, so the pinned GCS position stays unavailable.
    positioning->setSourceMode(GPSPositionService::SourceMode::ReceiverOnly);
    vehicles->setActiveVehicle(&vehicle);
    QTRY_COMPARE_WITH_TIMEOUT(vehicles->activeVehicle(), &vehicle, TestTimeout::shortMs());
    QVERIFY(receiveMessage(vehicle, gpsMessage(vehicle.id())));
    QVERIFY(receiveMessage(vehicle, fusedMessage(vehicle.id())));

    auto* manager = GPSManager::instance()->ntrip();
    const auto checkSource = [&](const QString& expected) {
        auto* transport = new MockNTRIPTransport(manager);
        manager->setTransportForTest(transport);
        manager->startNTRIP();
        QCOMPARE(manager->ggaSource(), expected);
        QCOMPARE(transport->sentNmea.size(), expected.isEmpty() ? 0 : 1);
        manager->stopNTRIP();
    };
    checkSource(QStringLiteral("Vehicle GPS"));
    QVERIFY(receiveMessage(vehicle, gpsMessage(vehicle.id(), 123000, 473977000, 85456000, GPS_FIX_TYPE_NO_FIX)));
    checkSource(QStringLiteral("Vehicle EKF"));
    vehicle._positionHealth->setFreshnessTimeoutMs(1);
    QTRY_VERIFY_WITH_TIMEOUT(!vehicle.acceptedPositionObservation(), TestTimeout::shortMs());
    checkSource(QString());
    QVERIFY(receiveMessage(vehicle, gpsMessage(vehicle.id())));
    checkSource(QStringLiteral("Vehicle GPS"));
}

void NTRIPGgaProviderTest::_providerMetadata_data()
{
    QTest::addColumn<GPSObservation::FixQuality>("quality");
    QTest::addColumn<unsigned>("expectedQuality");
    using Quality = GPSObservation::FixQuality;
    QTest::newRow("gps") << Quality::Fix3D << NMEA::GgaQuality::GPS;
    QTest::newRow("differential") << Quality::Differential << NMEA::GgaQuality::DIFFERENTIAL;
    QTest::newRow("rtk-float") << Quality::RTKFloat << NMEA::GgaQuality::RTK_FLOAT;
    QTest::newRow("rtk-fixed") << Quality::RTKFixed << NMEA::GgaQuality::RTK_FIXED;
    QTest::newRow("estimated") << Quality::Extrapolated << NMEA::GgaQuality::ESTIMATED;
    QTest::newRow("unknown") << Quality::Unknown << NMEA::GgaQuality::ESTIMATED;
}

void NTRIPGgaProviderTest::_providerMetadata()
{
    QFETCH(GPSObservation::FixQuality, quality);
    QFETCH(unsigned, expectedQuality);
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.setPositionProvider(Source::VehicleGPS, [quality]() {
        return PositionResult{
            QGeoCoordinate(47, 8, 450), QStringLiteral("GPS"), GPSAltitudeDatum::MeanSeaLevel, quality, 18, 0.7};
    });
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), 1);
    const auto fields = transport.sentNmea.first().split(',');
    QCOMPARE(fields.at(NMEA::Field::GGA_QUALITY).toUInt(), expectedQuality);
    QCOMPARE(fields.at(NMEA::Field::GGA_SATELLITES_USED).toUInt(), 18u);
    QCOMPARE(fields.at(NMEA::Field::GGA_HDOP).toDouble(), 0.7);
}

UT_REGISTER_TEST(NTRIPGgaProviderTest, TestLabel::Unit)
