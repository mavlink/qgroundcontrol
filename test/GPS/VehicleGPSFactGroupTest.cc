#include "VehicleGPSFactGroupTest.h"
#include "UnitTest.h"
#include "QGCMAVLink.h"
#include "VehicleGPSFactGroup.h"
#include "VehicleGPS2FactGroup.h"

#include <QtCore/QtNumeric>

// Build a mavlink_message_t from a GPS_RAW_INT payload
static mavlink_message_t makeGpsRawInt(int32_t lat, int32_t lon, int32_t alt, uint16_t eph,
                                        uint16_t epv, uint16_t vel, uint16_t cog,
                                        uint8_t fix_type, uint8_t satellites_visible)
{
    mavlink_message_t msg{};
    mavlink_gps_raw_int_t raw{};
    raw.lat = lat;
    raw.lon = lon;
    raw.alt = alt;
    raw.eph = eph;
    raw.epv = epv;
    raw.vel = vel;
    raw.cog = cog;
    raw.fix_type = fix_type;
    raw.satellites_visible = satellites_visible;
    mavlink_msg_gps_raw_int_encode(1, 1, &msg, &raw);
    return msg;
}

static mavlink_message_t makeGpsRtk(uint8_t receiver_id, uint8_t health, uint8_t rate,
                                     uint8_t nsats, int32_t baseline_a_mm, int32_t baseline_b_mm,
                                     int32_t baseline_c_mm, uint32_t accuracy,
                                     int32_t iar_num_hypotheses)
{
    mavlink_message_t msg{};
    mavlink_gps_rtk_t rtk{};
    rtk.rtk_receiver_id = receiver_id;
    rtk.rtk_health = health;
    rtk.rtk_rate = rate;
    rtk.nsats = nsats;
    rtk.baseline_a_mm = baseline_a_mm;
    rtk.baseline_b_mm = baseline_b_mm;
    rtk.baseline_c_mm = baseline_c_mm;
    rtk.accuracy = accuracy;
    rtk.iar_num_hypotheses = iar_num_hypotheses;
    mavlink_msg_gps_rtk_encode(1, 1, &msg, &rtk);
    return msg;
}

static mavlink_message_t makeGps2Rtk(uint8_t receiver_id, uint8_t health, uint8_t rate,
                                      uint8_t nsats, int32_t baseline_a_mm, int32_t baseline_b_mm,
                                      int32_t baseline_c_mm, uint32_t accuracy,
                                      int32_t iar_num_hypotheses)
{
    mavlink_message_t msg{};
    mavlink_gps2_rtk_t rtk{};
    rtk.rtk_receiver_id = receiver_id;
    rtk.rtk_health = health;
    rtk.rtk_rate = rate;
    rtk.nsats = nsats;
    rtk.baseline_a_mm = baseline_a_mm;
    rtk.baseline_b_mm = baseline_b_mm;
    rtk.baseline_c_mm = baseline_c_mm;
    rtk.accuracy = accuracy;
    rtk.iar_num_hypotheses = iar_num_hypotheses;
    mavlink_msg_gps2_rtk_encode(1, 1, &msg, &rtk);
    return msg;
}

void VehicleGPSFactGroupTest::testInitialValues()
{
    VehicleGPSFactGroup fg;

    QVERIFY(qIsNaN(fg.lat()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.lon()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.hdop()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.vdop()->rawValue().toDouble()));
    QCOMPARE(fg.lock()->rawValue().toInt(), 0);
    QCOMPARE(fg.count()->rawValue().toInt(), 0);
    QCOMPARE(fg.spoofingState()->rawValue().toInt(), 255);
    QCOMPARE(fg.jammingState()->rawValue().toInt(), 255);
    QCOMPARE(fg.rtkHealth()->rawValue().toInt(), 0);
    QCOMPARE(fg.rtkRate()->rawValue().toInt(), 0);
    QVERIFY(qIsNaN(fg.rtkBaseline()->rawValue().toDouble()));
}

void VehicleGPSFactGroupTest::testGpsRawInt()
{
    VehicleGPSFactGroup fg;

    // lat=47.123456, lon=8.654321, alt=550500mm, eph=120, epv=200, vel=500, cog=18000, fix=6, sats=12
    auto msg = makeGpsRawInt(471234560, 86543210, 550500, 120, 200, 500, 18000, 6, 12);
    fg.handleMessage(nullptr, msg);

    QCOMPARE(fg.lat()->rawValue().toDouble(), 47.123456);
    QCOMPARE(fg.lon()->rawValue().toDouble(), 8.654321);
    QCOMPARE(fg.altitudeMSL()->rawValue().toDouble(), 550.5);
    QCOMPARE(fg.hdop()->rawValue().toDouble(), 1.2);
    QCOMPARE(fg.vdop()->rawValue().toDouble(), 2.0);
    QCOMPARE(fg.groundSpeed()->rawValue().toDouble(), 5.0);
    QCOMPARE(fg.courseOverGround()->rawValue().toDouble(), 180.0);
    QCOMPARE(fg.lock()->rawValue().toInt(), 6);
    QCOMPARE(fg.count()->rawValue().toInt(), 12);
}

void VehicleGPSFactGroupTest::testHighLatency2()
{
    VehicleGPSFactGroup fg;

    mavlink_message_t msg{};
    mavlink_high_latency2_t hl2{};
    hl2.latitude = 371234560;
    hl2.longitude = -1224194000;
    hl2.altitude = 100;
    hl2.eph = 25;
    hl2.epv = 30;
    mavlink_msg_high_latency2_encode(1, 1, &msg, &hl2);

    fg.handleMessage(nullptr, msg);

    QCOMPARE(fg.lat()->rawValue().toDouble(), 37.123456);
    QCOMPARE(fg.lon()->rawValue().toDouble(), -122.4194);
    QCOMPARE(fg.altitudeMSL()->rawValue().toDouble(), 100.0);
    QCOMPARE(fg.hdop()->rawValue().toDouble(), 2.5);
    QCOMPARE(fg.vdop()->rawValue().toDouble(), 3.0);
}

void VehicleGPSFactGroupTest::testGpsRtk()
{
    VehicleGPSFactGroup fg;

    // receiver_id=0 (matches default), health=1, rate=5Hz, nsats=14
    // baseline: 3000mm north, 4000mm east, 0mm down → 5.0m
    // accuracy=1500 (1.5m), iar=42
    auto msg = makeGpsRtk(0, 1, 5, 14, 3000, 4000, 0, 1500, 42);
    fg.handleMessage(nullptr, msg);

    QCOMPARE(fg.rtkHealth()->rawValue().toInt(), 1);
    QCOMPARE(fg.rtkRate()->rawValue().toInt(), 5);
    QCOMPARE(fg.rtkNumSats()->rawValue().toInt(), 14);
    QCOMPARE(fg.rtkIAR()->rawValue().toInt(), 42);
    QVERIFY(qFuzzyCompare(fg.rtkAccuracy()->rawValue().toDouble(), 1.5));
    QVERIFY(qFuzzyCompare(fg.rtkBaseline()->rawValue().toDouble(), 5.0));
}

void VehicleGPSFactGroupTest::testGps2Raw()
{
    VehicleGPS2FactGroup fg;

    mavlink_message_t msg{};
    mavlink_gps2_raw_t raw{};
    raw.lat = 471234560;
    raw.lon = 86543210;
    raw.alt = 550500;
    raw.fix_type = 3;
    raw.satellites_visible = 8;
    raw.eph = 150;
    raw.epv = 250;
    mavlink_msg_gps2_raw_encode(1, 1, &msg, &raw);

    fg.handleMessage(nullptr, msg);

    QCOMPARE(fg.lat()->rawValue().toDouble(), 47.123456);
    QCOMPARE(fg.lock()->rawValue().toInt(), 3);
    QCOMPARE(fg.count()->rawValue().toInt(), 8);
    QCOMPARE(fg.hdop()->rawValue().toDouble(), 1.5);
}

void VehicleGPSFactGroupTest::testGps2Rtk()
{
    VehicleGPS2FactGroup fg;

    // receiver_id=1 (matches GPS2 default), 3-4-0 baseline = 5.0m
    auto msg = makeGps2Rtk(1, 2, 10, 16, 3000, 4000, 0, 2000, 100);
    fg.handleMessage(nullptr, msg);

    QCOMPARE(fg.rtkHealth()->rawValue().toInt(), 2);
    QCOMPARE(fg.rtkRate()->rawValue().toInt(), 10);
    QCOMPARE(fg.rtkNumSats()->rawValue().toInt(), 16);
    QVERIFY(qFuzzyCompare(fg.rtkBaseline()->rawValue().toDouble(), 5.0));
}

void VehicleGPSFactGroupTest::testGnssIntegrity()
{
    VehicleGPSFactGroup fg;

    mavlink_message_t msg{};
    mavlink_gnss_integrity_t integrity{};
    integrity.id = 0; // matches default for GPS1
    integrity.jamming_state = 1;
    integrity.spoofing_state = 2;
    integrity.authentication_state = 3;
    integrity.corrections_quality = 4;
    integrity.system_status_summary = 5;
    integrity.gnss_signal_quality = 6;
    integrity.post_processing_quality = 7;
    mavlink_msg_gnss_integrity_encode(1, 1, &msg, &integrity);

    fg.handleMessage(nullptr, msg);

    QCOMPARE(fg.jammingState()->rawValue().toInt(), 1);
    QCOMPARE(fg.spoofingState()->rawValue().toInt(), 2);
    QCOMPARE(fg.authenticationState()->rawValue().toInt(), 3);
    QCOMPARE(fg.correctionsQuality()->rawValue().toInt(), 4);
    QCOMPARE(fg.systemQuality()->rawValue().toInt(), 5);
    QCOMPARE(fg.gnssSignalQuality()->rawValue().toInt(), 6);
    QCOMPARE(fg.postProcessingQuality()->rawValue().toInt(), 7);
}

void VehicleGPSFactGroupTest::testGnssIntegrityFiltering()
{
    VehicleGPSFactGroup fg;

    // Send integrity for id=1 — should be ignored by GPS1 (expects id=0)
    mavlink_message_t msg{};
    mavlink_gnss_integrity_t integrity{};
    integrity.id = 1;
    integrity.jamming_state = 3;
    mavlink_msg_gnss_integrity_encode(1, 1, &msg, &integrity);

    fg.handleMessage(nullptr, msg);

    // Should still be default (255 = Invalid)
    QCOMPARE(fg.jammingState()->rawValue().toInt(), 255);

    // GPS2 should accept id=1
    VehicleGPS2FactGroup fg2;
    fg2.handleMessage(nullptr, msg);
    QCOMPARE(fg2.jammingState()->rawValue().toInt(), 3);
}

void VehicleGPSFactGroupTest::testSentinelValues()
{
    VehicleGPSFactGroup fg;

    // All sentinel/unknown values: should produce NaN or 0
    mavlink_gps_raw_int_t raw{};
    raw.lat = 471234560; raw.lon = 86543210; raw.alt = 550500;
    raw.eph = UINT16_MAX; raw.epv = UINT16_MAX; raw.vel = UINT16_MAX;
    raw.cog = UINT16_MAX; raw.yaw = UINT16_MAX;
    raw.h_acc = 0; raw.v_acc = 0; raw.vel_acc = 0; raw.hdg_acc = 0;
    raw.fix_type = 3; raw.satellites_visible = 255;
    mavlink_message_t rawMsg{};
    mavlink_msg_gps_raw_int_encode(1, 1, &rawMsg, &raw);
    fg.handleMessage(nullptr, rawMsg);

    QVERIFY(qIsNaN(fg.hdop()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.vdop()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.groundSpeed()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.courseOverGround()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.yaw()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.hAcc()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.vAcc()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.velAcc()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.hdgAcc()->rawValue().toDouble()));
    QCOMPARE(fg.count()->rawValue().toInt(), 0); // 255 → 0
}

void VehicleGPSFactGroupTest::testHighLatency()
{
    VehicleGPSFactGroup fg;

    mavlink_message_t msg{};
    mavlink_high_latency_t hl{};
    hl.latitude = 371234560;
    hl.longitude = -1224194000;
    hl.altitude_amsl = 100;
    hl.gps_fix_type = 3;
    mavlink_msg_high_latency_encode(1, 1, &msg, &hl);

    fg.handleMessage(nullptr, msg);

    QCOMPARE(fg.lat()->rawValue().toDouble(), 37.123456);
    QCOMPARE(fg.lock()->rawValue().toInt(), 3);
    QCOMPARE(fg.count()->rawValue().toInt(), 0);
}

void VehicleGPSFactGroupTest::testRtkReceiverIdRejection()
{
    VehicleGPSFactGroup fg;

    // Send GPS_RTK with receiver_id=1 to GPS1 (expects id=0) — should be ignored
    auto msg = makeGpsRtk(1, 5, 10, 20, 1000, 2000, 3000, 500, 99);
    fg.handleMessage(nullptr, msg);

    QCOMPARE(fg.rtkHealth()->rawValue().toInt(), 0); // unchanged from default
    QCOMPARE(fg.rtkRate()->rawValue().toInt(), 0);
}

void VehicleGPSFactGroupTest::testQualityNone()
{
    VehicleGPSFactGroup fg;
    // No fix → QualityNone
    QCOMPARE(fg.quality(), VehicleGPSFactGroup::GPSQuality::QualityNone);

    // 2D fix still None (below Fix3D)
    auto msg = makeGpsRawInt(0, 0, 0, 500, 500, UINT16_MAX, UINT16_MAX, 1, 4);
    fg.handleMessage(nullptr, msg);
    QCOMPARE(fg.quality(), VehicleGPSFactGroup::GPSQuality::QualityNone);
}

void VehicleGPSFactGroupTest::testQualityPoor()
{
    VehicleGPSFactGroup fg;
    // 3D fix, high HDOP (5.0), few sats (5) → Poor
    auto msg = makeGpsRawInt(470000000, -1220000000, 100000, 500, 500, 100, 1800, 3, 5);
    fg.handleMessage(nullptr, msg);
    QCOMPARE(fg.quality(), VehicleGPSFactGroup::GPSQuality::QualityPoor);
}

void VehicleGPSFactGroupTest::testQualityGoodHdopAndSats()
{
    VehicleGPSFactGroup fg;
    // 3D fix, low HDOP (1.2), many sats (14) → Good
    auto msg = makeGpsRawInt(470000000, -1220000000, 100000, 120, 120, 100, 1800, 3, 14);
    fg.handleMessage(nullptr, msg);
    QCOMPARE(fg.quality(), VehicleGPSFactGroup::GPSQuality::QualityGood);
}

void VehicleGPSFactGroupTest::testQualityExcellentRtkFixed()
{
    VehicleGPSFactGroup fg;
    // RTK Fixed → Excellent regardless of other values
    auto msg = makeGpsRawInt(470000000, -1220000000, 100000, 80, 80, 100, 1800, 6, 20);
    fg.handleMessage(nullptr, msg);
    QCOMPARE(fg.quality(), VehicleGPSFactGroup::GPSQuality::QualityExcellent);
}

void VehicleGPSFactGroupTest::testMavlinkCoordinateHelpers()
{
    // Lat/lon round-trip
    QCOMPARE(QGCMAVLink::doubleToMavlinkLatLon(47.0), int32_t(470000000));
    QVERIFY(qFuzzyCompare(QGCMAVLink::mavlinkLatLonToDouble(470000000), 47.0));
    QVERIFY(qFuzzyCompare(QGCMAVLink::mavlinkLatLonToDouble(-1220000000), -122.0));

    // Altitude round-trip
    QCOMPARE(QGCMAVLink::metersToMavlinkMm(100.0), int32_t(100000));
    QVERIFY(qFuzzyCompare(QGCMAVLink::mavlinkMmToMeters(100000), 100.0));
    QVERIFY(qFuzzyCompare(QGCMAVLink::mavlinkMmToMeters(0), 0.0));
}

UT_REGISTER_TEST(VehicleGPSFactGroupTest, TestLabel::Unit)
