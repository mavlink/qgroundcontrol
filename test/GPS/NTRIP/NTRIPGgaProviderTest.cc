#include "NTRIPGgaProviderTest.h"

#include <QtCore/QStringList>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QTest>

#include "Fixtures/RAIIFixtures.h"
#include "GPSBaseReference.h"
#include "MockNTRIPStream.h"
#include "NMEAUtils.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPSettings.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleGPSPositionProvider.h"

// Tests delegate checksum validation to the canonical NMEAUtils implementation
// — no local XOR loop, so there is one source of truth.
static inline bool validateChecksum(const QByteArray& sentence)
{
    return NMEAUtils::verifyChecksum(sentence);
}

static int countFields(const QByteArray& gga)
{
    // Count comma-separated fields between '$' and '*'.
    int star = gga.lastIndexOf('*');
    if (star < 0) {
        star = gga.size();
    }
    const QByteArray body = gga.mid(1, star - 1);  // strip $ and *XX
    return body.count(',') + 1;
}

// ---------------------------------------------------------------------------
// Format / structure
// ---------------------------------------------------------------------------

void NTRIPGgaProviderTest::testMakeGGA_basicCoordinate()
{
    const QGeoCoordinate coord(47.3977, 8.5456, 450.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 450.0);

    QVERIFY(gga.startsWith("$GPGGA,"));
    QVERIFY(gga.contains(",N,"));
    QVERIFY(gga.contains(",E,"));
    QVERIFY(gga.contains("450.0"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_checksumFormat()
{
    const QGeoCoordinate coord(35.6762, 139.6503, 40.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 40.0);

    const int starIdx = gga.indexOf('*');
    QVERIFY(starIdx > 0);
    // $...BODY*XX\r\n — 2 checksum hex digits + CRLF terminator = starIdx + 5
    QCOMPARE(gga.size(), starIdx + 5);
    QVERIFY(gga.endsWith("\r\n"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_fieldCount()
{
    // GGA body (between $ and *) has 15 comma-separated fields:
    // GPGGA, hhmmss, lat, N/S, lon, E/W, qual, nsat, hdop, alt, M, geoid, M, age, refid
    const QGeoCoordinate coord(40.0, -74.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 10.0);

    QCOMPARE(countFields(gga), 15);
}

void NTRIPGgaProviderTest::testMakeGGA_timeFormat()
{
    const QGeoCoordinate coord(40.0, -74.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 10.0);

    const QString ggaStr = QString::fromUtf8(gga);
    const QStringList fields = ggaStr.mid(1, ggaStr.indexOf('*') - 1).split(',');
    QVERIFY(fields.size() >= 2);

    const QString timeField = fields[1];
    QVERIFY2(timeField.size() >= 6, qPrintable(QString("Time field too short: %1").arg(timeField)));
    for (int i = 0; i < 6; ++i) {
        QVERIFY2(timeField[i].isDigit(), qPrintable(QString("Non-digit at pos %1: %2").arg(i).arg(timeField)));
    }
    const int hh = timeField.left(2).toInt();
    const int mm = timeField.mid(2, 2).toInt();
    const int ss = timeField.mid(4, 2).toInt();
    QVERIFY2(hh >= 0 && hh <= 23, qPrintable(QString("Invalid hour: %1").arg(hh)));
    QVERIFY2(mm >= 0 && mm <= 59, qPrintable(QString("Invalid minute: %1").arg(mm)));
    QVERIFY2(ss >= 0 && ss <= 59, qPrintable(QString("Invalid second: %1").arg(ss)));
}

// ---------------------------------------------------------------------------
// Hemisphere encoding
// ---------------------------------------------------------------------------

void NTRIPGgaProviderTest::testMakeGGA_negativeCoordinates()
{
    const QGeoCoordinate coord(-33.8688, -151.2093, 10.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 10.0);

    QVERIFY(gga.contains(",S,"));
    QVERIFY(gga.contains(",W,"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_equator()
{
    const QGeoCoordinate coord(0.0, 0.0, 0.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 0.0);

    // Exactly 0.0 degrees encodes as N / E (>= 0.0 convention).
    QVERIFY(gga.contains(",0000.0000,N,00000.0000,E,"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_dateLine()
{
    const QGeoCoordinate coordEast(17.7134, 178.065);
    const QByteArray ggaEast = NMEAUtils::makeGGA(coordEast, 5.0);
    QVERIFY(ggaEast.contains(",E,"));
    QVERIFY(validateChecksum(ggaEast));

    const QGeoCoordinate coordWest(17.7134, -179.5);
    const QByteArray ggaWest = NMEAUtils::makeGGA(coordWest, 5.0);
    QVERIFY(ggaWest.contains(",W,"));
    QVERIFY(validateChecksum(ggaWest));
}

// ---------------------------------------------------------------------------
// Altitude edge cases
// ---------------------------------------------------------------------------

void NTRIPGgaProviderTest::testMakeGGA_zeroAltitude()
{
    const QGeoCoordinate coord(51.5074, -0.1278, 0.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 0.0);

    QVERIFY(gga.contains(",0.0,M,"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_highAltitude()
{
    const QGeoCoordinate coord(27.9881, 86.9250, 8848.9);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 8848.9);

    QVERIFY(gga.contains(",8848.9,M,"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_negativeAltitude()
{
    // Dead Sea: ~-430 m below mean sea level.
    const QGeoCoordinate coord(31.5, 35.5);
    const QByteArray gga = NMEAUtils::makeGGA(coord, -430.5);

    QVERIFY(gga.contains(",-430.5,M,"));
    QVERIFY(validateChecksum(gga));
}

// ---------------------------------------------------------------------------
// Coordinate precision (DDMM.mmmm)
// ---------------------------------------------------------------------------

void NTRIPGgaProviderTest::testMakeGGA_dmmPrecision()
{
    // 47.3977 deg = 47 deg 23.8620 min ; 8.5456 deg = 008 deg 32.7360 min
    const QGeoCoordinate coord(47.3977, 8.5456);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 100.0);

    const QString ggaStr = QString::fromUtf8(gga);
    const QStringList fields = ggaStr.mid(1, ggaStr.indexOf('*') - 1).split(',');
    // fields: [0]=GPGGA [1]=time [2]=lat [3]=N/S [4]=lon [5]=E/W ...
    QVERIFY(fields.size() >= 6);

    QVERIFY2(fields[2].startsWith(QStringLiteral("4723")), qPrintable(QString("Lat field: %1").arg(fields[2])));
    QCOMPARE(fields[3], QStringLiteral("N"));
    QVERIFY2(fields[4].startsWith(QStringLiteral("00832")), qPrintable(QString("Lon field: %1").arg(fields[4])));
    QCOMPARE(fields[5], QStringLiteral("E"));

    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testSourceClearedOnStopAndFreshStart()
{
    NTRIPGgaProvider provider;
    MockNTRIPStream transport;

    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() {
        GPSObservation observation;
        observation.position =
            QGeoPositionInfo(QGeoCoordinate(47.3977, 8.5456, 450.0), QDateTime::currentDateTimeUtc());
        observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
        return PositionResult{observation, QStringLiteral("Vehicle GPS")};
    });

    provider.start([&transport](const QByteArray& sentence) { transport.sendNMEA(sentence); });
    QCOMPARE(provider.currentSource(), QStringLiteral("Vehicle GPS"));
    QCOMPARE(transport.sentNmea.size(), 1);

    provider.stop();
    QVERIFY(provider.currentSource().isEmpty());

    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() { return PositionResult{}; });
    provider.start([&transport](const QByteArray& sentence) { transport.sendNMEA(sentence); });
    QVERIFY(provider.currentSource().isEmpty());
}

void NTRIPGgaProviderTest::testDefaultRTKBaseProvider()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripGgaPositionSource(), static_cast<int>(NTRIPGgaProvider::PositionSource::RTKBase));
    GPSBaseReference reference;
    reference.valid = true;
    reference.observation.sessionId = 1;
    reference.observation.position =
        QGeoPositionInfo(QGeoCoordinate(47.3977, 8.5456, 450.0), QDateTime::currentDateTimeUtc());

    MockNTRIPStream transport;
    NTRIPGgaProvider provider;
    provider.init(settings);
    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::RTKBase, [&]() {
        return reference.isValid() ? PositionResult{reference.observation, QStringLiteral("RTK Base"), true}
                                   : PositionResult{};
    });
    provider.start([&transport](const QByteArray& sentence) { transport.sendNMEA(sentence); });
    QCOMPARE(provider.currentSource(), QStringLiteral("RTK Base"));
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(transport.sentNmea.first().contains(",4723.8620,N,00832.7360,E,"));
    const auto fields = transport.sentNmea.first().mid(1).split(',');
    QVERIFY(fields[9].isEmpty());
    QVERIFY(fields[11].isEmpty());
    QVERIFY(validateChecksum(transport.sentNmea.first()));
    provider.stop();

    reference.valid = false;
    transport.sentNmea.clear();
    provider.start([&transport](const QByteArray& sentence) { transport.sendNMEA(sentence); });
    QVERIFY(provider.currentSource().isEmpty());
    QVERIFY(transport.sentNmea.isEmpty());
}

void NTRIPGgaProviderTest::testObservationMetadata()
{
    GPSObservation observation;
    observation.position =
        QGeoPositionInfo(QGeoCoordinate(47.3977, 8.5456, 450.0),
                         QDateTime::fromString(QStringLiteral("2026-09-08T12:34:56.000Z"), Qt::ISODate));
    observation.fixQuality = GPSObservation::FixQuality::RTKFixed;
    observation.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
    observation.satellitesUsed = 18;
    observation.horizontalDop = 0.7;
    observation.altitudeEllipsoidMeters = 497.5;
    const QByteArray encoded = NMEAUtils::makeGGA(observation);
    const auto fields = encoded.mid(1, encoded.indexOf('*') - 1).split(',');
    QCOMPARE(fields[1], QByteArray("123456.000"));
    QCOMPARE(fields[6], QByteArray("4"));
    QCOMPARE(fields[7], QByteArray("18"));
    QCOMPARE(fields[8], QByteArray("0.7"));
    QCOMPARE(fields[9], QByteArray("450.0"));
    QCOMPARE(fields[11], QByteArray("47.5"));
    QVERIFY(validateChecksum(encoded));

    observation.satellitesUsed.reset();
    observation.horizontalDop.reset();
    observation.fixQuality = GPSObservation::FixQuality::Unknown;
    observation.altitudeDatum = GPSObservation::AltitudeDatum::Unknown;
    const QByteArray unknown = NMEAUtils::makeGGA(observation);
    const auto absent = unknown.mid(1, unknown.indexOf('*') - 1).split(',');
    QVERIFY(absent[6].isEmpty());
    QVERIFY(absent[7].isEmpty());
    QVERIFY(absent[8].isEmpty());
    QVERIFY(absent[9].isEmpty());
    QVERIFY(absent[11].isEmpty());
    QVERIFY(validateChecksum(unknown));
}

void NTRIPGgaProviderTest::testPositionFreshness()
{
    GPSObservation observation;
    observation.position = QGeoPositionInfo(QGeoCoordinate(47.3977, 8.5456), QDateTime::currentDateTimeUtc());
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    PositionResult result{observation, QStringLiteral("receiver")};
    QVERIFY(result.isValid());
    result.observation.monotonicTimestampUs -= 6000000;
    QVERIFY(!result.isValid());
    result.fixedReference = true;
    QVERIFY(result.isValid());
    result.observation.fixQuality = GPSObservation::FixQuality::NoFix;
    QVERIFY(!result.isValid());
    result.observation.fixQuality = GPSObservation::FixQuality::Fix3D;
    result.fixedReference = false;
    result.observation.monotonicTimestampUs = 0;
    QVERIFY(!result.isValid());
}

void NTRIPGgaProviderTest::testVehicleMessageFreshness()
{
    _connectMockLink();
    QVERIFY(vehicle());
    QVERIFY(mockLink());
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripGgaPositionSource(),
                       static_cast<int>(NTRIPGgaProvider::PositionSource::VehicleGPS));
    MockNTRIPStream transport;
    NTRIPGgaProvider provider;
    provider.init(settings);
    VehicleGPSPositionProvider positions;
    positions.setVehicle(vehicle());
    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, [&]() {
        return PositionResult{positions.gpsPosition(), QStringLiteral("Vehicle GPS")};
    });
    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleEKF, [&]() {
        return PositionResult{positions.ekfPosition(), QStringLiteral("Vehicle EKF")};
    });
    provider.start([&transport](const QByteArray& sentence) { transport.sendNMEA(sentence); });
    QVERIFY(transport.sentNmea.isEmpty());

    const auto deliver = [&](const mavlink_message_t& message) {
        return QMetaObject::invokeMethod(vehicle(), "_mavlinkMessageReceived", Qt::DirectConnection,
                                         Q_ARG(LinkInterface*, mockLink()), Q_ARG(mavlink_message_t, message));
    };
    mavlink_gps_raw_int_t fix{};
    fix.fix_type = GPS_FIX_TYPE_3D_FIX;
    fix.lat = 473977000;
    fix.lon = 85456000;
    fix.alt = 450000;
    fix.eph = UINT16_MAX;
    fix.satellites_visible = UINT8_MAX;
    mavlink_message_t message{};
    mavlink_msg_gps_raw_int_encode(vehicle()->id(), vehicle()->defaultComponentId(), &message, &fix);
    QVERIFY(deliver(message));
    provider._sendGGA();
    QCOMPARE(transport.sentNmea.size(), 1);
    const auto fields = transport.sentNmea.last().mid(1).split(',');
    QCOMPARE(fields[6], QByteArray("1"));
    QVERIFY(fields[7].isEmpty());
    QVERIFY(fields[8].isEmpty());
    QCOMPARE(fields[9], QByteArray("450.0"));

    positions._gpsPosition.monotonicTimestampUs -= 6000000;
    provider._sendGGA();
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(provider.currentSource().isEmpty());
    // An identical fresh message must refresh the observation even if no Fact value changes.
    QVERIFY(deliver(message));
    provider._sendGGA();
    QCOMPARE(transport.sentNmea.size(), 2);
    fix.fix_type = GPS_FIX_TYPE_NO_FIX;
    mavlink_msg_gps_raw_int_encode(vehicle()->id(), vehicle()->defaultComponentId(), &message, &fix);
    QVERIFY(deliver(message));
    provider._sendGGA();
    QCOMPARE(transport.sentNmea.size(), 2);
    QVERIFY(provider.currentSource().isEmpty());

    settings->ntripGgaPositionSource()->setRawValue(static_cast<int>(NTRIPGgaProvider::PositionSource::VehicleEKF));
    mavlink_global_position_int_t global{};
    global.lat = fix.lat;
    global.lon = fix.lon;
    global.alt = fix.alt;
    mavlink_msg_global_position_int_encode(vehicle()->id(), vehicle()->defaultComponentId(), &message, &global);
    QVERIFY(deliver(message));
    provider._sendGGA();
    QCOMPARE(transport.sentNmea.size(), 3);
    positions._ekfPosition.monotonicTimestampUs -= 6000000;
    mavlink_msg_global_position_int_encode(vehicle()->id(), MAV_COMP_ID_CAMERA, &message, &global);
    QVERIFY(deliver(message));
    provider._sendGGA();
    QCOMPARE(transport.sentNmea.size(), 3);
    global.lat = 0;
    global.lon = 0;
    mavlink_msg_global_position_int_encode(vehicle()->id(), vehicle()->defaultComponentId(), &message, &global);
    QVERIFY(deliver(message));
    provider._sendGGA();
    QCOMPARE(transport.sentNmea.size(), 3);
    QVERIFY(provider.currentSource().isEmpty());
    settings->ntripGgaPositionSource()->setRawValue(static_cast<int>(NTRIPGgaProvider::PositionSource::VehicleGPS));
    fix.fix_type = 0;
    mavlink_msg_gps_raw_int_encode(vehicle()->id(), vehicle()->defaultComponentId(), &message, &fix);
    QVERIFY(deliver(message));
    provider._sendGGA();
    QCOMPARE(transport.sentNmea.size(), 3);
    mavlink_high_latency_t highLatency = {};
    highLatency.latitude = fix.lat;
    highLatency.longitude = fix.lon;
    highLatency.altitude_amsl = 450;
    highLatency.gps_fix_type = 3;
    mavlink_msg_high_latency_encode(vehicle()->id(), vehicle()->defaultComponentId(), &message, &highLatency);
    QVERIFY(deliver(message));
    provider._sendGGA();
    QCOMPARE(transport.sentNmea.size(), 4);
    mavlink_high_latency2_t highLatency2 = {};
    highLatency2.latitude = fix.lat;
    highLatency2.longitude = fix.lon;
    highLatency2.altitude = 450;
    highLatency2.failure_flags = HL_FAILURE_FLAG_GPS;
    mavlink_msg_high_latency2_encode(vehicle()->id(), vehicle()->defaultComponentId(), &message, &highLatency2);
    QVERIFY(deliver(message));
    provider._sendGGA();
    QCOMPARE(transport.sentNmea.size(), 4);
    settings->ntripGgaPositionSource()->setRawValue(static_cast<int>(NTRIPGgaProvider::PositionSource::VehicleEKF));
    provider._sendGGA();
    QCOMPARE(transport.sentNmea.size(), 5);
    positions.setVehicle(nullptr);
    QVERIFY(!positions.gpsPosition().position.isValid());
    QVERIFY(!positions.ekfPosition().position.isValid());
    positions.setVehicle(vehicle());
    QVERIFY(!positions.gpsPosition().position.isValid());
    provider.stop();
}

UT_REGISTER_TEST(NTRIPGgaProviderTest, TestLabel::Unit)

void NTRIPGgaProviderTest::testWriterCanStopOrReplace_data()
{
    QTest::addColumn<bool>("replace");
    QTest::newRow("stop") << false;
    QTest::newRow("replace") << true;
}

void NTRIPGgaProviderTest::testWriterCanStopOrReplace()
{
    QFETCH(bool, replace);
    NTRIPGgaProvider provider;
    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() {
        GPSObservation observation;
        observation.position =
            QGeoPositionInfo(QGeoCoordinate(47.3977, 8.5456, 450.0), QDateTime::currentDateTimeUtc());
        observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
        return PositionResult{observation, QStringLiteral("Vehicle GPS")};
    });
    int replacedWrites = 0;
    provider.start([&](const QByteArray& sentence) {
        QVERIFY(validateChecksum(sentence));
        provider.stop();
        if (replace) {
            provider.start([&](const QByteArray&) { ++replacedWrites; });
        }
    });
    QCOMPARE(replacedWrites, replace ? 1 : 0);
    QCOMPARE(provider._timer.isActive(), replace);
    QCOMPARE(provider.currentSource().isEmpty(), !replace);
    provider.stop();
    provider._sendGGA();
    QCOMPARE(replacedWrites, replace ? 1 : 0);
}

void NTRIPGgaProviderTest::testWriterCanDestroyProvider()
{
    QPointer<NTRIPGgaProvider> provider = new NTRIPGgaProvider;
    provider->setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() {
        GPSObservation observation;
        observation.position =
            QGeoPositionInfo(QGeoCoordinate(47.3977, 8.5456, 450.0), QDateTime::currentDateTimeUtc());
        observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
        return PositionResult{observation, QStringLiteral("Vehicle GPS")};
    });
    provider->start([&](const QByteArray&) { delete provider.data(); });
    QVERIFY(!provider);
}
