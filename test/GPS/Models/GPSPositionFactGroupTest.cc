#include "GPSPositionFactGroupTest.h"

#include <memory>

#include "GPSPositionFactGroup.h"
#include "VehicleGPS2FactGroup.h"
#include "VehicleGPSAggregateFactGroup.h"
#include "development/mavlink_msg_gnss_integrity.h"

void GPSPositionFactGroupTest::_vehicleMessages_data()
{
    QTest::addColumn<bool>("secondary");
    QTest::addColumn<bool>("unknown");
    QTest::newRow("gps1") << false << false;
    QTest::newRow("gps2") << true << false;
    QTest::newRow("gps1-unknown") << false << true;
    QTest::newRow("gps2-unknown") << true << true;
}

void GPSPositionFactGroupTest::_vehicleMessages()
{
    QFETCH(bool, secondary);
    QFETCH(bool, unknown);
    std::unique_ptr<VehicleGPSFactGroup> vehicleGps;
    if (secondary) {
        vehicleGps = std::make_unique<VehicleGPS2FactGroup>();
    } else {
        vehicleGps = std::make_unique<VehicleGPSFactGroup>();
    }
    const auto populate = [unknown](auto& raw) {
        raw.lat = 473000000;
        raw.lon = 85400000;
        raw.eph = unknown ? UINT16_MAX : 160;
        raw.epv = unknown ? UINT16_MAX : 210;
        raw.cog = unknown ? UINT16_MAX : 9000;
        raw.yaw = unknown ? UINT16_MAX : 18000;
        raw.fix_type = 6;
        raw.satellites_visible = unknown ? UINT8_MAX : 17;
    };
    mavlink_message_t message{};
    if (secondary) {
        mavlink_gps2_raw_t raw{};
        populate(raw);
        mavlink_msg_gps2_raw_encode(1, 1, &message, &raw);
    } else {
        mavlink_gps_raw_int_t raw{};
        populate(raw);
        mavlink_msg_gps_raw_int_encode(1, 1, &message, &raw);
    }
    vehicleGps->handleMessage(nullptr, message);
    GPSPositionFactGroup* common = vehicleGps.get();
    QCOMPARE(common->lat()->rawValue().toDouble(), 47.3);
    QCOMPARE(common->lon()->rawValue().toDouble(), 8.54);
    QVERIFY(!common->mgrs()->rawValue().toString().isEmpty());
    QCOMPARE(common->count()->rawValue().toInt(), unknown ? -1 : 17);
    QCOMPARE(common->lock()->rawValue().toInt(), 6);
    QVERIFY(common->telemetryAvailable());
    if (unknown) {
        QVERIFY(qIsNaN(common->hdop()->rawValue().toDouble()));
        QVERIFY(qIsNaN(common->vdop()->rawValue().toDouble()));
        QVERIFY(qIsNaN(common->courseOverGround()->rawValue().toDouble()));
        QVERIFY(qIsNaN(common->yaw()->rawValue().toDouble()));
    } else {
        QCOMPARE(common->hdop()->rawValue().toDouble(), 1.6);
        QCOMPARE(common->vdop()->rawValue().toDouble(), 2.1);
        QCOMPARE(common->courseOverGround()->rawValue().toDouble(), 90.0);
        QCOMPARE(common->yaw()->rawValue().toDouble(), 180.0);
    }
    QCOMPARE(common->lock()->enumValues().size(), 8);
    const auto authenticationValues = vehicleGps->authenticationState()->enumValues();
    QCOMPARE(authenticationValues.size(), 6);
    for (const int value : {0, 1, 2, 3, 4, 255}) {
        QVERIFY(authenticationValues.contains(value));
    }
    QCOMPARE(vehicleGps->authenticationState()->rawValue().toInt(), 255);
    QCOMPARE(vehicleGps->authenticationState()->enumStringValue(), QStringLiteral("Unknown"));
}

void GPSPositionFactGroupTest::_vehicleSentinels_data()
{
    QTest::addColumn<bool>("secondary");
    QTest::addColumn<int>("yaw");
    QTest::addColumn<bool>("missingExtension");
    for (bool secondary : {false, true}) {
        const QByteArray source = secondary ? "gps2" : "gps1";
        for (int yaw : {0, 65535, 36000, 12345}) {
            QTest::newRow((source + '-' + QByteArray::number(yaw)).constData()) << secondary << yaw << false;
        }
        QTest::newRow((source + "-missing-extension").constData()) << secondary << 12345 << true;
    }
}

void GPSPositionFactGroupTest::_vehicleSentinels()
{
    QFETCH(bool, secondary);
    QFETCH(int, yaw);
    QFETCH(bool, missingExtension);
    std::unique_ptr<VehicleGPSFactGroup> facts;
    if (secondary) {
        facts = std::make_unique<VehicleGPS2FactGroup>();
    } else {
        facts = std::make_unique<VehicleGPSFactGroup>();
    }
    const auto populate = [yaw](auto& raw) {
        raw.lat = 0;
        raw.lon = 0;
        raw.eph = 0;
        raw.epv = 0;
        raw.cog = 0;
        raw.yaw = yaw;
        raw.fix_type = 7;
        raw.satellites_visible = 0;
    };
    mavlink_message_t message{};
    if (secondary) {
        mavlink_gps2_raw_t raw{};
        populate(raw);
        mavlink_msg_gps2_raw_encode(1, 1, &message, &raw);
        if (missingExtension) {
            message.len = MAVLINK_MSG_ID_GPS2_RAW_MIN_LEN;
        }
    } else {
        mavlink_gps_raw_int_t raw{};
        populate(raw);
        mavlink_msg_gps_raw_int_encode(1, 1, &message, &raw);
        if (missingExtension) {
            message.len = MAVLINK_MSG_ID_GPS_RAW_INT_MIN_LEN;
        }
    }
    facts->handleMessage(nullptr, message);
    QCOMPARE(facts->lat()->rawValue().toDouble(), 0.0);
    QCOMPARE(facts->lon()->rawValue().toDouble(), 0.0);
    QCOMPARE(facts->hdop()->rawValue().toDouble(), 0.0);
    QCOMPARE(facts->vdop()->rawValue().toDouble(), 0.0);
    QCOMPARE(facts->courseOverGround()->rawValue().toDouble(), 0.0);
    QCOMPARE(facts->count()->rawValue().toInt(), 0);
    QCOMPARE(facts->lock()->rawValue().toInt(), 7);
    if (missingExtension || yaw == 0 || yaw == 65535) {
        QVERIFY(qIsNaN(facts->yaw()->rawValue().toDouble()));
    } else {
        QCOMPARE(facts->yaw()->rawValue().toDouble(), yaw == 36000 ? 0.0 : yaw / 100.0);
    }
    QVERIFY(facts->telemetryAvailable());
}

void GPSPositionFactGroupTest::_localObservation()
{
    GPSPositionFactGroup facts;
    GPSObservation fix;
    fix.position = QGeoPositionInfo(QGeoCoordinate(47.3, 8.54), QDateTime::currentDateTimeUtc());
    fix.position.setAttribute(QGeoPositionInfo::Direction, 90);
    fix.trueHeadingDegrees = 180;
    fix.horizontalDop = 1.6;
    fix.verticalDop = 2.1;
    fix.fixQuality = GPSObservation::FixQuality::RTKFixed;
    facts.updatePosition(fix);
    QCOMPARE(facts.lat()->rawValue().toDouble(), 47.3);
    QCOMPARE(facts.lon()->rawValue().toDouble(), 8.54);
    QCOMPARE(facts.hdop()->rawValue().toDouble(), 1.6);
    QCOMPARE(facts.vdop()->rawValue().toDouble(), 2.1);
    QCOMPARE(facts.courseOverGround()->rawValue().toDouble(), 90.0);
    QCOMPARE(facts.yaw()->rawValue().toDouble(), 180.0);
    QCOMPARE(facts.lock()->rawValue().toInt(), 6);
    fix.horizontalDop.reset();
    fix.trueHeadingDegrees.reset();
    fix.position.removeAttribute(QGeoPositionInfo::Direction);
    fix.fixQuality = GPSObservation::FixQuality::Extrapolated;
    facts.updatePosition(fix);
    QVERIFY(qIsNaN(facts.hdop()->rawValue().toDouble()));
    QVERIFY(qIsNaN(facts.courseOverGround()->rawValue().toDouble()));
    QVERIFY(qIsNaN(facts.yaw()->rawValue().toDouble()));
    QCOMPARE(facts.lock()->rawValue().toInt(), 0);
    facts.resetPosition();
    QVERIFY(qIsNaN(facts.lat()->rawValue().toDouble()));
    QVERIFY(!facts.telemetryAvailable());
}

void GPSPositionFactGroupTest::_resetDuringUpdate()
{
    GPSPositionFactGroup facts;
    bool reset = false;
    connect(facts.lat(), &Fact::rawValueChanged, this, [&]() {
        if (!reset) {
            reset = true;
            facts.resetPosition();
        }
    });
    GPSObservation fix;
    fix.position = QGeoPositionInfo(QGeoCoordinate(47.3, 8.54), QDateTime::currentDateTimeUtc());
    fix.fixQuality = GPSObservation::FixQuality::RTKFixed;
    facts.updatePosition(fix);
    QVERIFY(reset);
    QVERIFY(qIsNaN(facts.lat()->rawValue().toDouble()));
    QVERIFY(qIsNaN(facts.lon()->rawValue().toDouble()));
    QCOMPARE(facts.lock()->rawValue().toInt(), 0);
    QVERIFY(!facts.telemetryAvailable());
}

void GPSPositionFactGroupTest::_integrityProjection()
{
    GPSPositionFactGroup native;
    VehicleGPSFactGroup vehicle;
    VehicleGPSAggregateFactGroup aggregate;
    aggregate.bindToGps(&vehicle, nullptr);
    GPSObservation observation;
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    observation.jammingState = 3;
    observation.spoofingState = 1;
    observation.authenticationState = 4;
    observation.correctionsProtocol = 1;
    observation.correctionsUsed = 2;
    native.integrity()->update(GPSIntegrityObservation::fromPosition(observation));
    mavlink_gnss_integrity_t raw{};
    raw.jamming_state = 3;
    raw.spoofing_state = 1;
    raw.authentication_state = 4;
    raw.corrections_quality = 255;
    raw.system_status_summary = 255;
    raw.gnss_signal_quality = 255;
    raw.post_processing_quality = 255;
    mavlink_message_t message{};
    mavlink_msg_gnss_integrity_encode(1, 1, &message, &raw);
    vehicle.handleMessage(nullptr, message);
    QCOMPARE(native.integrity()->jammingState()->rawValue(), vehicle.jammingState()->rawValue());
    QCOMPARE(native.integrity()->spoofingState()->rawValue(), vehicle.spoofingState()->rawValue());
    QCOMPARE(native.integrity()->authenticationState()->rawValue(), vehicle.authenticationState()->rawValue());
    QCOMPARE(vehicle.getFact(QStringLiteral("jammingState")), vehicle.integrity()->jammingState());
    QVERIFY(native.integrity()->available());
    QVERIFY(!native.integrity()->systemErrorsKnown());
    QVERIFY(vehicle.integrity()->systemErrorsKnown());
    QCOMPARE(native.integrity()->correctionsUsed()->rawValue().toInt(), 2);
    QCOMPARE(vehicle.integrity()->correctionsUsed()->rawValue().toInt(), 255);
    vehicle.updatePosition({});
    QCOMPARE(vehicle.jammingState()->rawValue().toInt(), 3);
    QCOMPARE(aggregate.jammingState()->rawValue().toInt(), 3);
    vehicle.integrity()->reset();
    QCOMPARE(aggregate.jammingState()->rawValue().toInt(), 255);
    native.integrity()->reset();
    QVERIFY(!native.integrity()->available());
    QCOMPARE(native.integrity()->correctionsProtocol()->rawValue().toInt(), 255);
}

void GPSPositionFactGroupTest::_integrityExpiryAndReentrancy()
{
    GPSIntegrityFactGroup facts;
    GPSIntegrityObservation observation;
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs() - 4900000;
    observation.jammingState = 3;
    facts.update(observation);
    QVERIFY(facts.available());
    QTRY_VERIFY_WITH_TIMEOUT(!facts.available(), 2000);
    QCOMPARE(facts.jammingState()->rawValue().toInt(), 255);
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs() - 6000000;
    facts.update(observation);
    QVERIFY(!facts.available());
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    observation.spoofingState = 3;
    bool reset = false;
    connect(facts.spoofingState(), &Fact::rawValueChanged, this, [&]() {
        if (!reset) {
            reset = true;
            facts.reset();
        }
    });
    facts.update(observation);
    QVERIFY(reset);
    QVERIFY(!facts.available());
    QCOMPARE(facts.jammingState()->rawValue().toInt(), 255);
}

UT_REGISTER_TEST(GPSPositionFactGroupTest, TestLabel::Unit)
