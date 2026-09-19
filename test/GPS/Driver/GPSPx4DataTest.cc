#include <cmath>
#include <limits>

#include <QtTest/QTest>

#include "GPSPx4Data_p.h"
#include "PortableTest.h"
#include "satellite_info.h"
#include "sensor_gps.h"

class GPSPx4DataTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _initialization_data();
    void _initialization();
    void _positionValues_data();
    void _positionValues();
    void _optionalValues_data();
    void _optionalValues();
    void _velocityValidity_data();
    void _velocityValidity();
    void _fixTypes_data();
    void _fixTypes();
    void _jammingStates_data();
    void _jammingStates();
    void _spoofingStates_data();
    void _spoofingStates();
    void _correctionStates_data();
    void _correctionStates();
    void _satelliteCounts_data();
    void _satelliteCounts();
    void _satelliteMetadata_data();
    void _satelliteMetadata();
    void _countOnlySatellites();
};

void GPSPx4DataTest::_initialization_data()
{
    QTest::addColumn<bool>("reset");
    QTest::newRow("unreported") << false;
    QTest::newRow("reset-populated-report") << true;
}

void GPSPx4DataTest::_initialization()
{
    QFETCH(bool, reset);
    sensor_gps_s raw{};
    if (reset) {
        raw.timestamp = 123;
        raw.time_utc_usec = 456;
        raw.latitude_deg = 47.5;
        raw.longitude_deg = -8.2;
        raw.altitude_msl_m = -25;
        raw.altitude_ellipsoid_m = 7.5;
        raw.eph = 1.5f;
        raw.epv = 2.5f;
        raw.hdop = 0.75f;
        raw.vdop = 1.25f;
        raw.vel_m_s = 12.5f;
        raw.cog_rad = 1.25f;
        raw.vel_ned_valid = true;
        raw.heading = 2.5f;
        raw.heading_accuracy = 0.125f;
        raw.fix_type = sensor_gps_s::FIX_TYPE_RTK_FIXED;
        raw.satellites_used = 18;
        raw.noise_per_ms = 42;
        raw.automatic_gain_control = 123;
        raw.jamming_indicator = 77;
        raw.jamming_state = sensor_gps_s::JAMMING_STATE_WARNING;
        raw.spoofing_state = sensor_gps_s::SPOOFING_STATE_MULTIPLE;
        raw.rtcm_msg_used = sensor_gps_s::RTCM_MSG_USED_USED;
        raw.rtcm_crc_failed = true;
    }

    GPSPx4Data::initialize(raw);
    const auto report = GPSPx4Data::position(raw);
    QCOMPARE(report.timestampUs, uint64_t{0});
    QCOMPARE(report.utcTimeUs, uint64_t{0});
    QCOMPARE(report.fixType, GPSPositionReport::FixType::Unknown);
    QVERIFY(std::isnan(report.latitudeDegrees));
    QVERIFY(std::isnan(report.longitudeDegrees));
    QVERIFY(std::isnan(report.altitudeMslMeters));
    QVERIFY(std::isnan(report.altitudeEllipsoidMeters));
    QVERIFY(std::isnan(report.horizontalAccuracyMeters));
    QVERIFY(std::isnan(report.verticalAccuracyMeters));
    QVERIFY(std::isnan(report.horizontalDop));
    QVERIFY(std::isnan(report.verticalDop));
    QVERIFY(std::isnan(report.speedMetersPerSecond));
    QVERIFY(std::isnan(report.courseRadians));
    QVERIFY(std::isnan(report.headingRadians));
    QVERIFY(std::isnan(report.headingAccuracyRadians));
    QVERIFY(!report.satellitesUsed.has_value());
    QCOMPARE(report.integrity.timestampUs, uint64_t{0});
    QCOMPARE(report.integrity.jamming, GPSIntegrityReport::JammingState::Unknown);
    QCOMPARE(report.integrity.spoofing, GPSIntegrityReport::SpoofingState::Unknown);
    QCOMPARE(report.integrity.correctionUse, GPSIntegrityReport::CorrectionUse::Unknown);
    QVERIFY(!report.integrity.noisePerMillisecond.has_value());
    QVERIFY(!report.integrity.automaticGainControl.has_value());
    QVERIFY(!report.integrity.jammingIndicator.has_value());
    QVERIFY(!report.integrity.correctionCrcFailed.has_value());
}

void GPSPx4DataTest::_positionValues_data()
{
    QTest::addColumn<uint64_t>("timestamp");
    QTest::addColumn<uint64_t>("utcTime");
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("longitude");
    QTest::addColumn<double>("altitudeMsl");
    QTest::addColumn<double>("altitudeEllipsoid");
    QTest::addColumn<float>("quality");
    QTest::newRow("equator-below-sea-level") << uint64_t{123} << uint64_t{456} << 0.0 << -8.2 << -25.0 << 7.5 << 1.25f;
    QTest::newRow("explicit-zero") << uint64_t{0} << uint64_t{0} << 0.0 << 0.0 << 0.0 << 0.0 << 0.0f;
    QTest::newRow("precision-and-64-bit-time")
        << uint64_t{4'294'967'297} << uint64_t{1'726'000'000'123'456} << -47.123456789 << 179.987654321
        << 12'345.123456789 << 12'400.987654321 << 0.125f;
}

void GPSPx4DataTest::_positionValues()
{
    QFETCH(uint64_t, timestamp);
    QFETCH(uint64_t, utcTime);
    QFETCH(double, latitude);
    QFETCH(double, longitude);
    QFETCH(double, altitudeMsl);
    QFETCH(double, altitudeEllipsoid);
    QFETCH(float, quality);
    sensor_gps_s raw{};
    GPSPx4Data::initialize(raw);
    raw.timestamp = timestamp;
    raw.timestamp_sample = 987;
    raw.time_utc_usec = utcTime;
    raw.timestamp_time_relative = -123;
    raw.latitude_deg = latitude;
    raw.longitude_deg = longitude;
    raw.altitude_msl_m = altitudeMsl;
    raw.altitude_ellipsoid_m = altitudeEllipsoid;
    raw.eph = quality;
    raw.epv = quality * 2;
    raw.hdop = quality * 3;
    raw.vdop = quality * 4;
    raw.heading = quality * 5;
    raw.heading_accuracy = quality * 6;
    const auto report = GPSPx4Data::position(raw);
    QCOMPARE(report.timestampUs, timestamp);
    QCOMPARE(report.utcTimeUs, utcTime);
    QCOMPARE(report.latitudeDegrees, latitude);
    QCOMPARE(report.longitudeDegrees, longitude);
    QCOMPARE(report.altitudeMslMeters, altitudeMsl);
    QCOMPARE(report.altitudeEllipsoidMeters, altitudeEllipsoid);
    QCOMPARE(report.horizontalAccuracyMeters, quality);
    QCOMPARE(report.verticalAccuracyMeters, quality * 2);
    QCOMPARE(report.horizontalDop, quality * 3);
    QCOMPARE(report.verticalDop, quality * 4);
    QCOMPARE(report.headingRadians, quality * 5);
    QCOMPARE(report.headingAccuracyRadians, quality * 6);
    QCOMPARE(report.integrity.timestampUs, uint64_t{0});
}

void GPSPx4DataTest::_optionalValues_data()
{
    QTest::addColumn<uint8_t>("satellites");
    QTest::addColumn<int32_t>("noise");
    QTest::addColumn<uint16_t>("gain");
    QTest::addColumn<int32_t>("jamming");
    QTest::addColumn<bool>("known");
    QTest::newRow("unreported") << uint8_t{255} << int32_t{-1} << uint16_t{65535} << int32_t{-1} << false;
    QTest::newRow("explicit-zero") << uint8_t{0} << int32_t{0} << uint16_t{0} << int32_t{0} << true;
    QTest::newRow("reported") << uint8_t{18} << int32_t{42} << uint16_t{12345} << int32_t{77} << true;
    QTest::newRow("negative-diagnostics")
        << uint8_t{255} << (std::numeric_limits<int32_t>::min)() << uint16_t{65535} << int32_t{-2} << false;
    QTest::newRow("largest-reported") << uint8_t{254} << (std::numeric_limits<int32_t>::max)() << uint16_t{65534}
                                      << (std::numeric_limits<int32_t>::max)() << true;
}

void GPSPx4DataTest::_optionalValues()
{
    QFETCH(uint8_t, satellites);
    QFETCH(int32_t, noise);
    QFETCH(uint16_t, gain);
    QFETCH(int32_t, jamming);
    QFETCH(bool, known);
    sensor_gps_s raw{};
    GPSPx4Data::initialize(raw);
    raw.satellites_used = satellites;
    raw.noise_per_ms = noise;
    raw.automatic_gain_control = gain;
    raw.jamming_indicator = jamming;
    const auto report = GPSPx4Data::position(raw);
    QCOMPARE(report.satellitesUsed.has_value(), known);
    QCOMPARE(report.integrity.noisePerMillisecond.has_value(), known);
    QCOMPARE(report.integrity.automaticGainControl.has_value(), known);
    QCOMPARE(report.integrity.jammingIndicator.has_value(), known);
    if (known) {
        QCOMPARE(*report.satellitesUsed, satellites);
        QCOMPARE(*report.integrity.noisePerMillisecond, noise);
        QCOMPARE(*report.integrity.automaticGainControl, gain);
        QCOMPARE(*report.integrity.jammingIndicator, jamming);
    }
}

void GPSPx4DataTest::_velocityValidity_data()
{
    QTest::addColumn<uint8_t>("nativeFix");
    QTest::addColumn<GPSPositionReport::FixType>("fix");
    QTest::addColumn<bool>("valid");
    using Fix = GPSPositionReport::FixType;
    QTest::newRow("no-fix-invalid-velocity") << sensor_gps_s::FIX_TYPE_NONE << Fix::NoFix << false;
    QTest::newRow("no-fix-valid-velocity") << sensor_gps_s::FIX_TYPE_NONE << Fix::NoFix << true;
    QTest::newRow("rtk-fixed-invalid-velocity") << sensor_gps_s::FIX_TYPE_RTK_FIXED << Fix::RTKFixed << false;
    QTest::newRow("rtk-fixed-valid-velocity") << sensor_gps_s::FIX_TYPE_RTK_FIXED << Fix::RTKFixed << true;
}

void GPSPx4DataTest::_velocityValidity()
{
    QFETCH(uint8_t, nativeFix);
    QFETCH(GPSPositionReport::FixType, fix);
    QFETCH(bool, valid);
    sensor_gps_s raw{};
    GPSPx4Data::initialize(raw);
    raw.fix_type = nativeFix;
    raw.vel_m_s = 12.5f;
    raw.cog_rad = 1.25f;
    raw.vel_ned_valid = valid;
    const auto report = GPSPx4Data::position(raw);
    QCOMPARE(report.fixType, fix);
    if (valid) {
        QCOMPARE(report.speedMetersPerSecond, 12.5f);
        QCOMPARE(report.courseRadians, 1.25f);
    } else {
        QVERIFY(std::isnan(report.speedMetersPerSecond));
        QVERIFY(std::isnan(report.courseRadians));
    }
}

void GPSPx4DataTest::_fixTypes_data()
{
    QTest::addColumn<uint8_t>("native");
    QTest::addColumn<GPSPositionReport::FixType>("expected");
    using Fix = GPSPositionReport::FixType;
    QTest::newRow("unknown") << uint8_t{0} << Fix::Unknown;
    QTest::newRow("none") << sensor_gps_s::FIX_TYPE_NONE << Fix::NoFix;
    QTest::newRow("2d") << sensor_gps_s::FIX_TYPE_2D << Fix::Fix2D;
    QTest::newRow("3d") << sensor_gps_s::FIX_TYPE_3D << Fix::Fix3D;
    QTest::newRow("differential") << sensor_gps_s::FIX_TYPE_RTCM_CODE_DIFFERENTIAL << Fix::Differential;
    QTest::newRow("rtk-float") << sensor_gps_s::FIX_TYPE_RTK_FLOAT << Fix::RTKFloat;
    QTest::newRow("rtk-fixed") << sensor_gps_s::FIX_TYPE_RTK_FIXED << Fix::RTKFixed;
    QTest::newRow("reserved-7") << uint8_t{7} << Fix::Unknown;
    QTest::newRow("extrapolated") << sensor_gps_s::FIX_TYPE_EXTRAPOLATED << Fix::Extrapolated;
    QTest::newRow("reserved-9") << uint8_t{9} << Fix::Unknown;
    QTest::newRow("reserved-255") << uint8_t{255} << Fix::Unknown;
}

void GPSPx4DataTest::_fixTypes()
{
    QFETCH(uint8_t, native);
    QFETCH(GPSPositionReport::FixType, expected);
    sensor_gps_s raw{};
    GPSPx4Data::initialize(raw);
    raw.fix_type = native;
    QCOMPARE(GPSPx4Data::position(raw).fixType, expected);
}

void GPSPx4DataTest::_jammingStates_data()
{
    QTest::addColumn<uint8_t>("native");
    QTest::addColumn<GPSIntegrityReport::JammingState>("expected");
    using State = GPSIntegrityReport::JammingState;
    QTest::newRow("unknown") << sensor_gps_s::JAMMING_STATE_UNKNOWN << State::Unknown;
    QTest::newRow("ok") << sensor_gps_s::JAMMING_STATE_OK << State::Ok;
    QTest::newRow("warning") << sensor_gps_s::JAMMING_STATE_WARNING << State::Warning;
    QTest::newRow("critical") << sensor_gps_s::JAMMING_STATE_CRITICAL << State::Critical;
    QTest::newRow("reserved-4") << uint8_t{4} << State::Unknown;
    QTest::newRow("reserved-255") << uint8_t{255} << State::Unknown;
}

void GPSPx4DataTest::_jammingStates()
{
    QFETCH(uint8_t, native);
    QFETCH(GPSIntegrityReport::JammingState, expected);
    sensor_gps_s raw{};
    GPSPx4Data::initialize(raw);
    raw.jamming_state = native;
    QCOMPARE(GPSPx4Data::position(raw).integrity.jamming, expected);
}

void GPSPx4DataTest::_spoofingStates_data()
{
    QTest::addColumn<uint8_t>("native");
    QTest::addColumn<GPSIntegrityReport::SpoofingState>("expected");
    using State = GPSIntegrityReport::SpoofingState;
    QTest::newRow("unknown") << sensor_gps_s::SPOOFING_STATE_UNKNOWN << State::Unknown;
    QTest::newRow("none") << sensor_gps_s::SPOOFING_STATE_NONE << State::None;
    QTest::newRow("indicated") << sensor_gps_s::SPOOFING_STATE_INDICATED << State::Indicated;
    QTest::newRow("multiple") << sensor_gps_s::SPOOFING_STATE_MULTIPLE << State::Multiple;
    QTest::newRow("reserved-4") << uint8_t{4} << State::Unknown;
    QTest::newRow("reserved-255") << uint8_t{255} << State::Unknown;
}

void GPSPx4DataTest::_spoofingStates()
{
    QFETCH(uint8_t, native);
    QFETCH(GPSIntegrityReport::SpoofingState, expected);
    sensor_gps_s raw{};
    GPSPx4Data::initialize(raw);
    raw.spoofing_state = native;
    QCOMPARE(GPSPx4Data::position(raw).integrity.spoofing, expected);
}

void GPSPx4DataTest::_correctionStates_data()
{
    QTest::addColumn<uint8_t>("native");
    QTest::addColumn<bool>("crcFailed");
    QTest::addColumn<GPSIntegrityReport::CorrectionUse>("expected");
    QTest::addColumn<bool>("crcKnown");
    using Use = GPSIntegrityReport::CorrectionUse;
    QTest::newRow("unknown-clean") << sensor_gps_s::RTCM_MSG_USED_UNKNOWN << false << Use::Unknown << false;
    QTest::newRow("unknown-failed") << sensor_gps_s::RTCM_MSG_USED_UNKNOWN << true << Use::Unknown << true;
    QTest::newRow("not-used-clean") << sensor_gps_s::RTCM_MSG_USED_NOT_USED << false << Use::NotUsed << true;
    QTest::newRow("not-used-failed") << sensor_gps_s::RTCM_MSG_USED_NOT_USED << true << Use::NotUsed << true;
    QTest::newRow("used-clean") << sensor_gps_s::RTCM_MSG_USED_USED << false << Use::Used << true;
    QTest::newRow("used-failed") << sensor_gps_s::RTCM_MSG_USED_USED << true << Use::Used << true;
    QTest::newRow("reserved-3-clean") << uint8_t{3} << false << Use::Unknown << false;
    QTest::newRow("reserved-3-failed") << uint8_t{3} << true << Use::Unknown << true;
    QTest::newRow("reserved-255-clean") << uint8_t{255} << false << Use::Unknown << false;
    QTest::newRow("reserved-255-failed") << uint8_t{255} << true << Use::Unknown << true;
}

void GPSPx4DataTest::_correctionStates()
{
    QFETCH(uint8_t, native);
    QFETCH(bool, crcFailed);
    QFETCH(GPSIntegrityReport::CorrectionUse, expected);
    QFETCH(bool, crcKnown);
    sensor_gps_s raw{};
    GPSPx4Data::initialize(raw);
    raw.timestamp = 123;
    raw.rtcm_msg_used = native;
    raw.rtcm_crc_failed = crcFailed;
    const auto integrity = GPSPx4Data::position(raw).integrity;
    QCOMPARE(integrity.correctionUse, expected);
    QCOMPARE(integrity.correctionCrcFailed.has_value(), crcKnown);
    if (crcKnown) {
        QCOMPARE(*integrity.correctionCrcFailed, crcFailed);
    }
    QCOMPARE(integrity.timestampUs, uint64_t{0});
}

void GPSPx4DataTest::_satelliteCounts_data()
{
    QTest::addColumn<uint8_t>("nativeCount");
    QTest::addColumn<uint16_t>("expectedCount");
    constexpr uint16_t NATIVE_LIMIT = satellite_info_s::SAT_INFO_MAX_SATELLITES;
    QTest::newRow("empty") << uint8_t{0} << uint16_t{0};
    QTest::newRow("one") << uint8_t{1} << uint16_t{1};
    QTest::newRow("native-limit") << uint8_t{NATIVE_LIMIT} << NATIVE_LIMIT;
    QTest::newRow("above-native-limit") << uint8_t{NATIVE_LIMIT + 1} << NATIVE_LIMIT;
    QTest::newRow("maximum-byte") << uint8_t{255} << NATIVE_LIMIT;
}

void GPSPx4DataTest::_satelliteCounts()
{
    QFETCH(uint8_t, nativeCount);
    QFETCH(uint16_t, expectedCount);
    satellite_info_s raw{};
    raw.count = nativeCount;
    raw.timestamp = 42;
    for (uint8_t i = 0; i < satellite_info_s::SAT_INFO_MAX_SATELLITES; ++i) {
        raw.svid[i] = i + 1;
        raw.used[i] = 1;
    }
    const auto report = GPSPx4Data::satellites(raw, GPSType::ublox);
    QCOMPARE(report.count, expectedCount);
    QCOMPARE(report.timestampUs, uint64_t{42});
    for (uint16_t i = 0; i < expectedCount; ++i) {
        QCOMPARE(report.satellites[i].id, i + 1);
        QVERIFY(report.satellites[i].used.has_value());
        QVERIFY(*report.satellites[i].used);
    }
    for (uint16_t i = expectedCount; i < GPSSatelliteReport::MAX_SATELLITES; ++i) {
        const auto& satellite = report.satellites[i];
        QCOMPARE(satellite.id, uint16_t{0});
        QCOMPARE(satellite.prn, uint16_t{0});
        QVERIFY(!satellite.used.has_value());
        QVERIFY(!satellite.signalStrength.has_value());
        QVERIFY(!satellite.elevationDegrees.has_value());
        QVERIFY(!satellite.azimuthDegrees.has_value());
    }
}

void GPSPx4DataTest::_satelliteMetadata_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<uint8_t>("id");
    QTest::addColumn<uint8_t>("used");
    QTest::addColumn<uint8_t>("signal");
    QTest::addColumn<int>("elevation");
    QTest::addColumn<uint8_t>("azimuth");
    QTest::addColumn<bool>("elevationKnown");
    QTest::addColumn<bool>("azimuthKnown");
    QTest::addColumn<float>("azimuthDegrees");
    QTest::newRow("ublox-negative-elevation-zero-signal") << GPSType::ublox << uint8_t{9} << uint8_t{1} << uint8_t{0}
                                                          << -10 << uint8_t{128} << true << true << 180.70588f;
    QTest::newRow("ublox-explicit-zero-false")
        << GPSType::ublox << uint8_t{9} << uint8_t{0} << uint8_t{0} << 0 << uint8_t{0} << true << true << 0.0f;
    QTest::newRow("ublox-lower-elevation-limit")
        << GPSType::ublox << uint8_t{9} << uint8_t{255} << uint8_t{45} << -90 << uint8_t{255} << true << true << 360.0f;
    QTest::newRow("ublox-upper-elevation-limit")
        << GPSType::ublox << uint8_t{9} << uint8_t{1} << uint8_t{45} << 90 << uint8_t{0} << true << true << 0.0f;
    QTest::newRow("ublox-below-elevation-limit")
        << GPSType::ublox << uint8_t{9} << uint8_t{1} << uint8_t{45} << -91 << uint8_t{0} << false << true << 0.0f;
    QTest::newRow("ublox-above-elevation-limit")
        << GPSType::ublox << uint8_t{9} << uint8_t{1} << uint8_t{45} << 91 << uint8_t{0} << false << true << 0.0f;
    QTest::newRow("missing-id") << GPSType::ublox << uint8_t{0} << uint8_t{0} << uint8_t{45} << 10 << uint8_t{128}
                                << false << false << 0.0f;
    QTest::newRow("ashtech-unknown-azimuth")
        << GPSType::trimble << uint8_t{9} << uint8_t{1} << uint8_t{0} << -10 << uint8_t{128} << true << false << 0.0f;
    QTest::newRow("ashtech-zero-is-not-known-north")
        << GPSType::trimble << uint8_t{9} << uint8_t{0} << uint8_t{45} << 0 << uint8_t{0} << true << false << 0.0f;
}

void GPSPx4DataTest::_satelliteMetadata()
{
    QFETCH(GPSType, type);
    QFETCH(uint8_t, id);
    QFETCH(uint8_t, used);
    QFETCH(uint8_t, signal);
    QFETCH(int, elevation);
    QFETCH(uint8_t, azimuth);
    QFETCH(bool, elevationKnown);
    QFETCH(bool, azimuthKnown);
    QFETCH(float, azimuthDegrees);
    satellite_info_s raw{};
    raw.count = 1;
    raw.svid[0] = id;
    raw.prn[0] = 23;
    raw.used[0] = used;
    raw.snr[0] = signal;
    raw.elevation[0] = static_cast<uint8_t>(elevation);
    raw.azimuth[0] = azimuth;
    const auto report = GPSPx4Data::satellites(raw, type);
    const auto& satellite = report.satellites[0];
    QCOMPARE(satellite.id, id);
    QCOMPARE(satellite.prn, uint16_t{23});
    QVERIFY(satellite.used.has_value());
    QCOMPARE(*satellite.used, used != 0);
    QCOMPARE(satellite.signalStrength.has_value(), id != 0);
    if (id != 0) {
        QCOMPARE(*satellite.signalStrength, signal);
    }
    QCOMPARE(satellite.elevationDegrees.has_value(), elevationKnown);
    if (elevationKnown) {
        QCOMPARE(*satellite.elevationDegrees, static_cast<float>(elevation));
    }
    QCOMPARE(satellite.azimuthDegrees.has_value(), azimuthKnown);
    if (azimuthKnown) {
        QCOMPARE(*satellite.azimuthDegrees, azimuthDegrees);
    }
}

void GPSPx4DataTest::_countOnlySatellites()
{
    satellite_info_s raw{};
    raw.count = 255;
    raw.timestamp = 42;
    for (uint8_t i = 0; i < satellite_info_s::SAT_INFO_MAX_SATELLITES; ++i) {
        raw.svid[i] = i + 1;
        raw.prn[i] = i + 21;
        raw.used[i] = 1;
        raw.snr[i] = 45;
        raw.elevation[i] = 10;
        raw.azimuth[i] = 128;
    }
    const auto report = GPSPx4Data::satellites(raw, GPSType::septentrio);
    QCOMPARE(report.count, satellite_info_s::SAT_INFO_MAX_SATELLITES);
    QCOMPARE(report.timestampUs, uint64_t{42});
    for (const auto& satellite : report.satellites) {
        QCOMPARE(satellite.id, uint16_t{0});
        QCOMPARE(satellite.prn, uint16_t{0});
        QVERIFY(!satellite.used.has_value());
        QVERIFY(!satellite.signalStrength.has_value());
        QVERIFY(!satellite.elevationDegrees.has_value());
        QVERIFY(!satellite.azimuthDegrees.has_value());
    }
}

QGC_REGISTER_PORTABLE_TEST(GPSPx4DataTest, TestLabel::Unit)

#include "GPSPx4DataTest.moc"
