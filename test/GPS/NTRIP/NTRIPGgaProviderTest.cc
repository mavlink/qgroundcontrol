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
#include "VehicleLinkManager.h"

namespace {

using Source = NTRIPGgaProvider::PositionSource;

mavlink_message_t gpsMessage(int vehicleId, int32_t altitude = 123000, int32_t latitude = 473977000,
                             int32_t longitude = 85456000)
{
    mavlink_gps_raw_int_t raw{};
    raw.time_usec = 1234567;
    raw.lat = latitude;
    raw.lon = longitude;
    raw.alt = altitude;
    raw.fix_type = GPS_FIX_TYPE_3D_FIX;
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
    NTRIPManager::instance()->init();
}

void NTRIPGgaProviderTest::cleanup()
{
    NTRIPManager::instance()->stopNTRIP();
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

    provider.stop();
    QVERIFY(provider.currentSource().isEmpty());

    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() { return PositionResult{}; });
    provider.start(&transport);
    QVERIFY(provider.currentSource().isEmpty());
}

void NTRIPGgaProviderTest::testDefaultRTKBaseProvider()
{
    TestFixtures::SettingsFixture saved;
    configureNtrip(saved, Source::RTKBase);
    auto* facts = qobject_cast<GPSRTKFactGroup*>(GPSManager::instance()->gpsRtk()->gpsRtkFactGroup());
    QVERIFY(facts);
    saved.setFactValue(facts->valid(), true);
    saved.setFactValue(facts->currentLatitude(), 47.3977);
    saved.setFactValue(facts->currentLongitude(), 8.5456);
    saved.setFactValue(facts->currentAltitude(), 450.0);

    auto* manager = NTRIPManager::instance();
    auto* transport = new MockNTRIPTransport(manager);
    manager->setTransportForTest(transport);
    manager->startNTRIP();
    QCOMPARE(manager->connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    QVERIFY(manager->ggaSource().isEmpty());
    QVERIFY(transport->sentNmea.isEmpty());
    QCOMPARE(facts->currentAltitude()->rawValue().toDouble(), 450.0);
    manager->stopNTRIP();

    facts->valid()->setRawValue(false);
    transport = new MockNTRIPTransport(manager);
    manager->setTransportForTest(transport);
    manager->startNTRIP();
    QVERIFY(manager->ggaSource().isEmpty());
    QVERIFY(transport->sentNmea.isEmpty());
    manager->stopNTRIP();
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
    auto* ntrip = NTRIPManager::instance();
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
            QGeoCoordinate expected = vehicle->coordinate();
            if (source == Source::VehicleGPS) {
                auto* gps = vehicle->gpsFactGroup();
                expected.setLatitude(gps->getFact(QStringLiteral("lat"))->rawValue().toDouble());
                expected.setLongitude(gps->getFact(QStringLiteral("lon"))->rawValue().toDouble());
            }
            const NMEA::GGA fix{
                .latitude = expected.latitude(),
                .longitude = expected.longitude(),
                .altitude = expected.altitude(),
                .hdop = 1.0,
                .quality = NMEA::GgaQuality::GPS,
                .satellitesUsed = 12,
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
    QTest::newRow("zero-island") << QGeoCoordinate(0, 0, 450) << GPSAltitudeDatum::MeanSeaLevel << 1.0 << true << false;
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
    QObject producer;
    GPSSourceHealth health(nullptr, &scheduler);
    auto registration =
        positioning->registerPositionSource(GPSPositionService::SelectedSource::Receiver, &producer, &health);
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

    auto* manager = NTRIPManager::instance();
    auto* transport = new MockNTRIPTransport(manager);
    manager->setTransportForTest(transport);
    manager->startNTRIP();
    QCOMPARE(manager->connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    QCOMPARE(transport->sentNmea.size(), accepted ? 1 : 0);
    QCOMPARE(manager->ggaSource(), accepted ? QStringLiteral("GCS Position") : QString());
    if (accepted) {
        QVERIFY(transport->sentNmea.first().contains(",4723.8620,N,00832.7360,E,"));
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
    QObject producer;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSObservation observation;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.position = QGeoPositionInfo(QGeoCoordinate(47.3977, 8.5456, 450), observation.receivedAt);
    observation.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
    observation.receiverFixValid = true;
    health.updateObservation(observation);
    auto registration =
        positioning->registerPositionSource(GPSPositionService::SelectedSource::Receiver, &producer, &health);
    positioning->setSourceMode(GPSPositionService::SourceMode::ReceiverOnly);

    auto* manager = NTRIPManager::instance();
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

    positioning->setSourceMode(GPSPositionService::SourceMode::NmeaOnly);
    checkGga(false);
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

UT_REGISTER_TEST(NTRIPGgaProviderTest, TestLabel::Unit)
