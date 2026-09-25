#include <memory>

#include <QtTest/QTest>

#include "GpsMavlinkTestHelpers.h"
#include "MAVLinkLib.h"
#include "ManualScheduler.h"
#include "UnitTest.h"
#include "VehicleGPSFactGroup.h"
#include "development/mavlink_msg_gnss_integrity.h"

namespace {

mavlink_message_t gpsMessage(bool secondary, uint16_t yaw, uint8_t fixType)
{
    using GpsTestHelpers::GpsReceiver;
    return GpsTestHelpers::gpsRawMessage({.latitudeE7 = 470000000,
                                          .longitudeE7 = 80000000,
                                          .altitudeMm = 123456,
                                          .fixType = fixType,
                                          .eph = 120,
                                          .epv = 240,
                                          .cog = 9000,
                                          .satellitesVisible = 18,
                                          .yaw = yaw,
                                          .horizontalAccuracyMm = 3500,
                                          .verticalAccuracyMm = 6000},
                                         secondary ? GpsReceiver::Secondary : GpsReceiver::Primary);
}

mavlink_message_t rtkMessage(bool secondary)
{
    const auto fill = [](auto& rtk) {
        rtk.baseline_a_mm = 3000;
        rtk.baseline_b_mm = -4000;
        rtk.baseline_c_mm = 0;
        rtk.rtk_rate = 1;
        rtk.nsats = 17;
    };
    mavlink_message_t message{};
    if (secondary) {
        mavlink_gps2_rtk_t rtk{};
        fill(rtk);
        mavlink_msg_gps2_rtk_encode(1, 1, &message, &rtk);
    } else {
        mavlink_gps_rtk_t rtk{};
        fill(rtk);
        mavlink_msg_gps_rtk_encode(1, 1, &message, &rtk);
    }
    return message;
}

std::unique_ptr<VehicleGPSFactGroup> receiver(bool secondary, RuntimeScheduler* scheduler)
{
    using Index = VehicleGPSFactGroup::ReceiverIndex;
    return std::make_unique<VehicleGPSFactGroup>(nullptr, scheduler, secondary ? Index::Secondary : Index::Primary);
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
    void _receiverDispatch_data();
    void _receiverDispatch();
    void _rtkStatus_data();
    void _rtkStatus();
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

void VehicleGPSFactGroupTest::_receiverDispatch_data()
{
    _fixValidityAndFreshness_data();
}

void VehicleGPSFactGroupTest::_receiverDispatch()
{
    QFETCH(bool, secondary);
    ManualScheduler scheduler;
    auto gps = receiver(secondary, &scheduler);
    gps->handleMessage(nullptr, gpsMessage(!secondary, 0, GPS_FIX_TYPE_3D_FIX));
    QVERIFY(!gps->acceptedObservation());
    QVERIFY(qIsNaN(gps->lat()->rawValue().toDouble()));
    gps->handleMessage(nullptr, gpsMessage(secondary, 0, GPS_FIX_TYPE_3D_FIX));
    QVERIFY(gps->acceptedObservation());

    mavlink_high_latency_t highLatency{};
    highLatency.latitude = 480000000;
    highLatency.longitude = 90000000;
    highLatency.altitude_amsl = 300;
    highLatency.gps_fix_type = GPS_FIX_TYPE_3D_FIX;
    mavlink_message_t message{};
    mavlink_msg_high_latency_encode(1, 1, &message, &highLatency);
    gps->handleMessage(nullptr, message);
    QCOMPARE(gps->lat()->rawValue().toDouble(), secondary ? 47.0 : 48.0);
    mavlink_high_latency2_t highLatency2{};
    highLatency2.latitude = 490000000;
    highLatency2.longitude = 100000000;
    highLatency2.altitude = 400;
    mavlink_msg_high_latency2_encode(1, 1, &message, &highLatency2);
    gps->handleMessage(nullptr, message);
    QCOMPARE(gps->lat()->rawValue().toDouble(), secondary ? 47.0 : 49.0);

    mavlink_gnss_integrity_t integrity{};
    integrity.id = secondary ? 0 : 1;
    integrity.jamming_state = 3;
    mavlink_msg_gnss_integrity_encode(1, 1, &message, &integrity);
    gps->handleMessage(nullptr, message);
    QCOMPARE(gps->jammingState()->rawValue().toInt(), 255);
    QCOMPARE(gps->gnssIntegrityTimestampUs(), quint64(0));
    integrity.id = secondary ? 1 : 0;
    mavlink_msg_gnss_integrity_encode(1, 1, &message, &integrity);
    gps->handleMessage(nullptr, message);
    QCOMPARE(gps->jammingState()->rawValue().toInt(), 3);
    QCOMPARE(gps->gnssIntegrityTimestampUs(), scheduler.nowUs());
}

UT_REGISTER_TEST(VehicleGPSFactGroupTest, TestLabel::Unit)

void VehicleGPSFactGroupTest::_rtkStatus_data()
{
    QTest::addColumn<bool>("secondary");
    QTest::newRow("primary") << false;
    QTest::newRow("secondary") << true;
}

void VehicleGPSFactGroupTest::_rtkStatus()
{
    QFETCH(bool, secondary);
    ManualScheduler scheduler;
    auto gps = receiver(secondary, &scheduler);
    QVERIFY(qIsNaN(gps->rtkBaseline()->rawValue().toDouble()));
    QCOMPARE(gps->rtkSatellites()->rawValue().toInt(), -1);
    gps->handleMessage(nullptr, rtkMessage(!secondary));
    QVERIFY(qIsNaN(gps->rtkBaseline()->rawValue().toDouble()));
    gps->handleMessage(nullptr, rtkMessage(secondary));
    QCOMPARE(gps->rtkBaseline()->rawValue().toDouble(), 5.0);
    QCOMPARE(gps->rtkRate()->rawValue().toDouble(), 1.0);
    QCOMPARE(gps->rtkSatellites()->rawValue().toInt(), 17);
    QVERIFY(scheduler.advanceBy(VehicleGPSFactGroup::RTK_STATUS_TIMEOUT - std::chrono::milliseconds(1)));
    gps->handleMessage(nullptr, rtkMessage(secondary));
    QVERIFY(scheduler.advanceBy(VehicleGPSFactGroup::RTK_STATUS_TIMEOUT - std::chrono::milliseconds(1)));
    QCOMPARE(gps->rtkSatellites()->rawValue().toInt(), 17);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY(qIsNaN(gps->rtkBaseline()->rawValue().toDouble()));
    QVERIFY(qIsNaN(gps->rtkRate()->rawValue().toDouble()));
    QCOMPARE(gps->rtkSatellites()->rawValue().toInt(), -1);
}

#include "VehicleGPSFactGroupTest.moc"
