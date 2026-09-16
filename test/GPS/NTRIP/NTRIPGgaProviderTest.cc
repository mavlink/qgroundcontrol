#include "NTRIPGgaProviderTest.h"

#include <QtCore/QScopeGuard>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "GPSSourceHealth.h"
#include "MAVLinkLib.h"
#include "MockNTRIPTransport.h"
#include "MonotonicClock.h"
#include "MultiVehicleManager.h"
#include "NMEAUtils.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPSettings.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleGPSFactGroup.h"
#include "VehicleLinkManager.h"

namespace {

using Source = NTRIPGgaProvider::PositionSource;

mavlink_message_t gpsMessage(int vehicleId, int32_t altitude = 123000, uint8_t fixType = GPS_FIX_TYPE_3D_FIX,
                             int32_t latitude = 473977000, int32_t longitude = 85456000)
{
    mavlink_gps_raw_int_t raw{};
    raw.time_usec = 1234567;
    raw.lat = latitude;
    raw.lon = longitude;
    raw.alt = altitude;
    raw.fix_type = fixType;
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

}  // namespace

void NTRIPGgaProviderTest::testSourceClearedOnStopAndFreshStart()
{
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;

    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() {
        return PositionResult{QGeoCoordinate(47.3977, 8.5456, 450.0), QStringLiteral("Vehicle GPS")};
    });

    provider.start(&transport);
    QCOMPARE(provider.currentSource(), QStringLiteral("Vehicle GPS"));
    QCOMPARE(transport.sentNmea.size(), 1);

    provider.stop();
    QVERIFY(provider.currentSource().isEmpty());

    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() { return PositionResult{}; });
    provider.start(&transport);
    QVERIFY(provider.currentSource().isEmpty());
}

void NTRIPGgaProviderTest::testDefaultRTKBaseProvider()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    auto* facts = qobject_cast<GPSRTKFactGroup*>(GPSManager::instance()->gpsRtk()->gpsRtkFactGroup());
    QVERIFY(facts);
    saved.setFactValue(settings->ntripGgaPositionSource(), static_cast<int>(NTRIPGgaProvider::PositionSource::RTKBase));
    saved.setFactValue(facts->valid(), true);
    saved.setFactValue(facts->currentLatitude(), 47.3977);
    saved.setFactValue(facts->currentLongitude(), 8.5456);
    saved.setFactValue(facts->currentAltitude(), 450.0);

    MockNTRIPTransport transport;
    NTRIPGgaProvider provider;
    GPSManager::instance()->configureGgaProvider(provider, settings);
    provider.start(&transport);
    QCOMPARE(provider.currentSource(), QStringLiteral("RTK Base"));
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(transport.sentNmea.first().contains(",4723.8620,N,00832.7360,E,"));
    QVERIFY(transport.sentNmea.first().contains(",450.0,M,"));
    QVERIFY(NMEAUtils::verifyChecksum(transport.sentNmea.first()));
    provider.stop();

    facts->valid()->setRawValue(false);
    transport.sentNmea.clear();
    provider.start(&transport);
    QVERIFY(provider.currentSource().isEmpty());
    QVERIFY(transport.sentNmea.isEmpty());
}

void NTRIPGgaProviderTest::_vehicleSourcesAndFreshness()
{
    Vehicle vehicle(nullptr, 17, MAV_COMP_ID_AUTOPILOT1, MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    QVERIFY(!GPSManager::vehicleGgaPosition(&vehicle, Source::VehicleGPS, MonotonicClock::nowUs()).isValid());
    QVERIFY(receiveMessage(vehicle, gpsMessage(vehicle.id())));
    QCOMPARE(vehicle.coordinate().altitude(), 123.0);
    QVERIFY(!GPSManager::vehicleGgaPosition(&vehicle, Source::VehicleEKF, MonotonicClock::nowUs()).isValid());
    QVERIFY(receiveMessage(vehicle, fusedMessage(vehicle.id())));
    QCOMPARE(vehicle.coordinate().altitude(), 450.0);
    const auto gps = vehicle.gpsObservation();
    const auto fused = vehicle.fusedPositionObservation();
    QCOMPARE(
        GPSManager::vehicleGgaPosition(&vehicle, Source::VehicleGPS, gps.monotonicTimestampUs).coordinate.altitude(),
        123.0);
    QCOMPARE(
        GPSManager::vehicleGgaPosition(&vehicle, Source::VehicleEKF, fused.monotonicTimestampUs).coordinate.altitude(),
        450.0);
    QVERIFY(!gps.position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));

    mavlink_altitude_t altitude{};
    altitude.altitude_amsl = 999.0f;
    mavlink_message_t message{};
    mavlink_msg_altitude_encode(vehicle.id(), MAV_COMP_ID_AUTOPILOT1, &message, &altitude);
    QVERIFY(receiveMessage(vehicle, message));
    QCOMPARE(vehicle.gpsObservation().position.coordinate().altitude(), 123.0);
    QCOMPARE(vehicle.fusedPositionObservation().position.coordinate().altitude(), 450.0);
    QCOMPARE(vehicle.fusedPositionObservation().monotonicTimestampUs, fused.monotonicTimestampUs);

    constexpr quint64 LIFETIME_US = GPSSourceHealth::FRESHNESS_TIMEOUT_MS * quint64(1000);
    for (const auto source : {Source::VehicleGPS, Source::VehicleEKF}) {
        const auto receipt = source == Source::VehicleGPS ? gps.monotonicTimestampUs : fused.monotonicTimestampUs;
        QVERIFY(GPSManager::vehicleGgaPosition(&vehicle, source, receipt + LIFETIME_US - 1).isValid());
        QVERIFY(!GPSManager::vehicleGgaPosition(&vehicle, source, receipt + LIFETIME_US).isValid());
        QVERIFY(!GPSManager::vehicleGgaPosition(&vehicle, source, receipt + LIFETIME_US + 1).isValid());
        QVERIFY(!GPSManager::vehicleGgaPosition(&vehicle, source, receipt - 1).isValid());
    }
    QCOMPARE(vehicle.gpsObservation().monotonicTimestampUs, gps.monotonicTimestampUs);
    QCOMPARE(vehicle.fusedPositionObservation().monotonicTimestampUs, fused.monotonicTimestampUs);

    QSignalSpy coordinateChanged(&vehicle, &Vehicle::coordinateChanged);
    const auto beforeReceipt = MonotonicClock::nowUs();
    QVERIFY(receiveMessage(vehicle, gpsMessage(vehicle.id())));
    QVERIFY(receiveMessage(vehicle, fusedMessage(vehicle.id())));
    QCOMPARE(coordinateChanged.count(), 0);
    QVERIFY(vehicle.gpsObservation().monotonicTimestampUs >= beforeReceipt);
    QVERIFY(vehicle.fusedPositionObservation().monotonicTimestampUs >= beforeReceipt);
    QVERIFY(GPSManager::vehicleGgaPosition(&vehicle, Source::VehicleGPS, beforeReceipt + LIFETIME_US - 1).isValid());
    QVERIFY(GPSManager::vehicleGgaPosition(&vehicle, Source::VehicleEKF, beforeReceipt + LIFETIME_US - 1).isValid());

    const auto gpsReceipt = vehicle.gpsObservation().monotonicTimestampUs;
    message = gpsMessage(vehicle.id(), 765000);
    message.compid = MAV_COMP_ID_GPS;
    QVERIFY(receiveMessage(vehicle, message));
    QCOMPARE(vehicle.gpsObservation().monotonicTimestampUs, gpsReceipt);
    QCOMPARE(vehicle.gpsObservation().position.coordinate().altitude(), 123.0);
    QVERIFY(receiveMessage(vehicle, gpsMessage(0, 765000)));
    QCOMPARE(vehicle.gpsObservation().monotonicTimestampUs, gpsReceipt);
    const auto fusedReceipt = vehicle.fusedPositionObservation().monotonicTimestampUs;
    QVERIFY(receiveMessage(vehicle, fusedMessage(0, 765000)));
    QCOMPARE(vehicle.fusedPositionObservation().monotonicTimestampUs, fusedReceipt);
    QCOMPARE(vehicle.fusedPositionObservation().position.coordinate().altitude(), 450.0);

    vehicle.closeVehicle();
    QVERIFY(!GPSManager::vehicleGgaPosition(&vehicle, Source::VehicleGPS, MonotonicClock::nowUs()).isValid());
    QVERIFY(!GPSManager::vehicleGgaPosition(&vehicle, Source::VehicleEKF, MonotonicClock::nowUs()).isValid());
}

void NTRIPGgaProviderTest::_invalidVehicleObservations_data()
{
    QTest::addColumn<bool>("rawGps");
    QTest::addColumn<int>("fixType");
    QTest::addColumn<int>("latitude");
    QTest::addColumn<int>("longitude");
    QTest::addColumn<int>("altitude");
    QTest::newRow("no-gps") << true << 0 << 473977000 << 85456000 << 123000;
    QTest::newRow("no-fix") << true << 1 << 473977000 << 85456000 << 123000;
    QTest::newRow("2d-no-altitude") << true << 2 << 473977000 << 85456000 << 123000;
    QTest::newRow("unknown-fix") << true << 255 << 473977000 << 85456000 << 123000;
    QTest::newRow("gps-zero-island") << true << 3 << 0 << 0 << 123000;
    QTest::newRow("gps-invalid-latitude") << true << 3 << 910000000 << 85456000 << 123000;
    QTest::newRow("gps-invalid-longitude") << true << 3 << 473977000 << 1810000000 << 123000;
    QTest::newRow("gps-unknown-altitude") << true << 3 << 473977000 << 85456000 << INT32_MAX;
    QTest::newRow("gps-invalid-altitude") << true << 3 << 473977000 << 85456000 << INT32_MIN;
    QTest::newRow("fused-zero-island") << false << 3 << 0 << 0 << 450000;
    QTest::newRow("fused-invalid-latitude") << false << 3 << 910000000 << 85456000 << 450000;
    QTest::newRow("fused-unknown-altitude") << false << 3 << 473977000 << 85456000 << INT32_MAX;
}

void NTRIPGgaProviderTest::_invalidVehicleObservations()
{
    QFETCH(bool, rawGps);
    QFETCH(int, fixType);
    QFETCH(int, latitude);
    QFETCH(int, longitude);
    QFETCH(int, altitude);
    Vehicle vehicle(nullptr, 17, MAV_COMP_ID_AUTOPILOT1, MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    const auto source = rawGps ? Source::VehicleGPS : Source::VehicleEKF;
    QVERIFY(receiveMessage(vehicle, gpsMessage(vehicle.id())));
    QVERIFY(receiveMessage(vehicle, fusedMessage(vehicle.id())));
    QVERIFY(GPSManager::vehicleGgaPosition(&vehicle, source, MonotonicClock::nowUs()).isValid());
    const auto invalid = rawGps ? gpsMessage(vehicle.id(), altitude, fixType, latitude, longitude)
                                : fusedMessage(vehicle.id(), altitude, latitude, longitude);
    QVERIFY(receiveMessage(vehicle, invalid));
    QVERIFY(!GPSManager::vehicleGgaPosition(&vehicle, source, MonotonicClock::nowUs()).isValid());
}

void NTRIPGgaProviderTest::_highLatencyObservations_data()
{
    QTest::addColumn<bool>("version2");
    QTest::addColumn<int>("fixOrFailure");
    QTest::addColumn<int>("altitude");
    QTest::addColumn<bool>("accepted");
    QTest::newRow("high-latency") << false << int(GPS_FIX_TYPE_3D_FIX) << 600 << true;
    QTest::newRow("high-latency-no-fix") << false << int(GPS_FIX_TYPE_NO_FIX) << 600 << false;
    QTest::newRow("high-latency-no-altitude") << false << int(GPS_FIX_TYPE_3D_FIX) << int(INT16_MAX) << false;
    QTest::newRow("high-latency2") << true << 0 << 700 << true;
    QTest::newRow("high-latency2-gps-failure") << true << int(HL_FAILURE_FLAG_GPS) << 700 << false;
    QTest::newRow("high-latency2-estimator-failure") << true << int(HL_FAILURE_FLAG_ESTIMATOR) << 700 << false;
}

void NTRIPGgaProviderTest::_highLatencyObservations()
{
    QFETCH(bool, version2);
    QFETCH(int, fixOrFailure);
    QFETCH(int, altitude);
    QFETCH(bool, accepted);
    Vehicle vehicle(nullptr, 17, MAV_COMP_ID_AUTOPILOT1, MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    QVERIFY(receiveMessage(vehicle, gpsMessage(vehicle.id(), 123000, GPS_FIX_TYPE_RTK_FIXED)));
    mavlink_message_t message{};
    if (version2) {
        mavlink_high_latency2_t report{};
        report.latitude = 473977000;
        report.longitude = 85456000;
        report.altitude = altitude;
        report.failure_flags = fixOrFailure;
        report.eph = UINT8_MAX;
        report.epv = UINT8_MAX;
        mavlink_msg_high_latency2_encode(vehicle.id(), MAV_COMP_ID_AUTOPILOT1, &message, &report);
    } else {
        mavlink_high_latency_t report{};
        report.latitude = 473977000;
        report.longitude = 85456000;
        report.altitude_amsl = altitude;
        report.gps_fix_type = fixOrFailure;
        mavlink_msg_high_latency_encode(vehicle.id(), MAV_COMP_ID_AUTOPILOT1, &message, &report);
    }
    QVERIFY(receiveMessage(vehicle, message));
    const auto position = GPSManager::vehicleGgaPosition(&vehicle, Source::VehicleEKF, MonotonicClock::nowUs());
    QCOMPARE(position.isValid(), accepted);
    if (accepted) {
        QCOMPARE(position.coordinate.altitude(), double(altitude));
    }
    QVERIFY(!GPSManager::vehicleGgaPosition(&vehicle, Source::VehicleGPS, MonotonicClock::nowUs()).isValid());
    QCOMPARE(vehicle.gpsFactGroup()->getFact(QStringLiteral("lock"))->rawValue().toInt(), int(GPS_FIX_TYPE_RTK_FIXED));
}

void NTRIPGgaProviderTest::_activeVehicleAndCommunicationLoss()
{
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
    QVERIFY(receiveMessage(first, fusedMessage(first.id())));
    QVERIFY(receiveMessage(second, gpsMessage(second.id(), 234000)));
    QVERIFY(receiveMessage(second, fusedMessage(second.id(), 678000)));
    MockNTRIPTransport transport;
    NTRIPGgaProvider provider;
    GPSManager::instance()->configureGgaProvider(provider, nullptr);

    for (Vehicle* vehicle : {&first, &second}) {
        manager->setActiveVehicle(vehicle);
        QTRY_COMPARE_WITH_TIMEOUT(manager->activeVehicle(), vehicle, TestTimeout::shortMs());
        for (const auto source : {Source::VehicleGPS, Source::VehicleEKF}) {
            transport.sentNmea.clear();
            provider.configure({source});
            provider.start(&transport);
            provider.stop();
            QCOMPARE(transport.sentNmea.size(), 1);
            const auto position = GPSManager::vehicleGgaPosition(vehicle, source, MonotonicClock::nowUs());
            const auto altitudeField = QByteArray::number(position.coordinate.altitude(), 'f', 1) + ",M,";
            QVERIFY(transport.sentNmea.first().contains(altitudeField));
            QVERIFY(NMEAUtils::verifyChecksum(transport.sentNmea.first()));
        }
    }

    QVERIFY(QMetaObject::invokeMethod(second.vehicleLinkManager(), "_commLostCheck", Qt::DirectConnection));
    QVERIFY(second.vehicleLinkManager()->communicationLost());
    QVERIFY(!second.gpsObservation().position.isValid());
    QVERIFY(!second.fusedPositionObservation().position.isValid());
    QVERIFY(receiveMessage(second, gpsMessage(second.id())));
    QVERIFY(receiveMessage(second, fusedMessage(second.id())));
    QVERIFY(second.vehicleLinkManager()->communicationLost());
    QVERIFY(second.gpsObservation().position.isValid());
    QVERIFY(second.fusedPositionObservation().position.isValid());
    for (const auto source : {Source::VehicleGPS, Source::VehicleEKF}) {
        transport.sentNmea.clear();
        provider.configure({source});
        provider.start(&transport);
        QVERIFY(provider.currentSource().isEmpty());
        QVERIFY(transport.sentNmea.isEmpty());
        provider.stop();
    }
    manager->setActiveVehicle(nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(!manager->activeVehicle(), TestTimeout::shortMs());
    provider.start(&transport);
    QVERIFY(transport.sentNmea.isEmpty());
    QVERIFY(provider.currentSource().isEmpty());
    provider.stop();
}

UT_REGISTER_TEST(NTRIPGgaProviderTest, TestLabel::Unit)
