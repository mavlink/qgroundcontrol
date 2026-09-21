#include <memory>

#include <QtTest/QTest>

#include "MAVLinkLib.h"
#include "ManualScheduler.h"
#include "UnitTest.h"
#include "VehicleGPS2FactGroup.h"
#include "VehicleGPSFactGroup.h"

namespace {

mavlink_message_t gpsMessage(bool secondary, uint16_t yaw, uint8_t fixType)
{
    const auto fill = [yaw, fixType](auto& raw) {
        raw.lat = 470000000;
        raw.lon = 80000000;
        raw.alt = 123456;
        raw.eph = 120;
        raw.epv = 240;
        raw.h_acc = 3500;
        raw.v_acc = 6000;
        raw.cog = 9000;
        raw.satellites_visible = 18;
        raw.yaw = yaw;
        raw.fix_type = fixType;
    };
    mavlink_message_t message{};
    if (secondary) {
        mavlink_gps2_raw_t raw{};
        fill(raw);
        mavlink_msg_gps2_raw_encode(1, 1, &message, &raw);
    } else {
        mavlink_gps_raw_int_t raw{};
        fill(raw);
        mavlink_msg_gps_raw_int_encode(1, 1, &message, &raw);
    }
    return message;
}

std::unique_ptr<VehicleGPSFactGroup> receiver(bool secondary, RuntimeScheduler* scheduler)
{
    if (secondary) {
        return std::make_unique<VehicleGPS2FactGroup>(nullptr, scheduler);
    }
    return std::make_unique<VehicleGPSFactGroup>(nullptr, scheduler);
}

}  // namespace

class VehicleGPSFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _rawNormalization_data();
    void _rawNormalization();
    void _fixValidityAndFreshness_data();
    void _fixValidityAndFreshness();
    void _highLatencyAccuracyUnits();
};

void VehicleGPSFactGroupTest::_rawNormalization_data()
{
    QTest::addColumn<bool>("secondary");
    QTest::addColumn<uint16_t>("yaw");
    QTest::addColumn<double>("expected");
    for (const bool secondary : {false, true}) {
        const QByteArray prefix = secondary ? "gps2-" : "gps1-";
        QTest::newRow((prefix + "unavailable").constData()) << secondary << uint16_t(0) << qQNaN();
        QTest::newRow((prefix + "no-solution").constData()) << secondary << uint16_t(UINT16_MAX) << qQNaN();
        QTest::newRow((prefix + "north").constData()) << secondary << uint16_t(36000) << 0.0;
        QTest::newRow((prefix + "east").constData()) << secondary << uint16_t(9000) << 90.0;
    }
}

void VehicleGPSFactGroupTest::_rawNormalization()
{
    QFETCH(bool, secondary);
    QFETCH(uint16_t, yaw);
    QFETCH(double, expected);
    ManualScheduler scheduler;
    auto gps = receiver(secondary, &scheduler);
    QVERIFY(qIsNaN(gps->yaw()->rawValue().toDouble()));
    gps->handleMessage(nullptr, gpsMessage(secondary, yaw, GPS_FIX_TYPE_3D_FIX));
    if (qIsNaN(expected)) {
        QVERIFY(qIsNaN(gps->yaw()->rawValue().toDouble()));
    } else {
        QCOMPARE(gps->yaw()->rawValue().toDouble(), expected);
    }
    QCOMPARE(gps->hdop()->rawValue().toDouble(), 1.2);
    QCOMPARE(gps->vdop()->rawValue().toDouble(), 2.4);
    QCOMPARE(gps->horizontalAccuracy()->rawValue().toDouble(), 3.5);
    QCOMPARE(gps->verticalAccuracy()->rawValue().toDouble(), 6.0);
    QCOMPARE(gps->count()->rawValue().toInt(), 18);
    const auto observation = gps->acceptedObservation();
    QVERIFY(observation);
    QCOMPARE(observation->position.coordinate(), QGeoCoordinate(47, 8, 123.456));
    QCOMPARE(observation->altitudeDatum, GPSAltitudeDatum::MeanSeaLevel);
    QVERIFY(!observation->satellitesUsed);
}

void VehicleGPSFactGroupTest::_fixValidityAndFreshness_data()
{
    QTest::addColumn<bool>("secondary");
    QTest::newRow("gps1") << false;
    QTest::newRow("gps2") << true;
}

void VehicleGPSFactGroupTest::_fixValidityAndFreshness()
{
    QFETCH(bool, secondary);
    ManualScheduler scheduler;
    auto gps = receiver(secondary, &scheduler);
    QVERIFY(!gps->acceptedObservation());
    gps->handleMessage(nullptr, gpsMessage(secondary, 0, GPS_FIX_TYPE_RTK_FIXED));
    QVERIFY(gps->acceptedObservation());
    QCOMPARE(gps->acceptedObservation()->fixQuality, GPSObservation::FixQuality::RTKFixed);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(4999)));
    QVERIFY(gps->acceptedObservation());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY(!gps->acceptedObservation());
    gps->handleMessage(nullptr, gpsMessage(secondary, 0, GPS_FIX_TYPE_3D_FIX));
    QVERIFY(gps->acceptedObservation());
    gps->handleMessage(nullptr, gpsMessage(secondary, 0, GPS_FIX_TYPE_NO_FIX));
    QVERIFY(!gps->acceptedObservation());
}

void VehicleGPSFactGroupTest::_highLatencyAccuracyUnits()
{
    ManualScheduler scheduler;
    VehicleGPSFactGroup gps(nullptr, &scheduler);
    gps.handleMessage(nullptr, gpsMessage(false, 9000, GPS_FIX_TYPE_3D_FIX));
    mavlink_high_latency2_t raw{};
    raw.latitude = 470000000;
    raw.longitude = 80000000;
    raw.altitude = 123;
    raw.eph = 123;
    raw.epv = 234;
    mavlink_message_t message{};
    mavlink_msg_high_latency2_encode(1, 1, &message, &raw);
    gps.handleMessage(nullptr, message);
    QVERIFY(qIsNaN(gps.hdop()->rawValue().toDouble()));
    QVERIFY(qIsNaN(gps.vdop()->rawValue().toDouble()));
    QVERIFY(qIsNaN(gps.yaw()->rawValue().toDouble()));
    QCOMPARE(gps.horizontalAccuracy()->rawValue().toDouble(), 12.3);
    QCOMPARE(gps.verticalAccuracy()->rawValue().toDouble(), 23.4);
    QVERIFY(!gps.acceptedObservation());
}

UT_REGISTER_TEST(VehicleGPSFactGroupTest, TestLabel::Unit)
#include "VehicleGPSFactGroupTest.moc"
