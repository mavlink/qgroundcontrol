#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

#include <QtCore/QList>
#include <QtTest/QTest>

#include "GPSNativeData_p.h"
#include "GPSProtocol.h"
#include "UnitTest.h"

Q_DECLARE_METATYPE(GPSNativeIntegrityReport)
Q_DECLARE_METATYPE(GPSNativeSatelliteData)
Q_DECLARE_METATYPE(GPSEllipsoidPosition)

static_assert(std::is_same_v<decltype(GPSNativePositionReport::fix_type), GPSPositionReport::FixType>);
static_assert(std::is_same_v<decltype(GPSNativeIntegrityReport::jamming_state), GPSIntegrityReport::JammingState>);
static_assert(std::is_same_v<decltype(GPSNativeIntegrityReport::spoofing_state), GPSIntegrityReport::SpoofingState>);
static_assert(
    std::is_same_v<decltype(GPSNativeIntegrityReport::corrections_msg_used), GPSIntegrityReport::CorrectionUse>);

namespace {

struct CoordinateConversions : GPSProtocol
{
    using GPSProtocol::EcefMeters;
    using GPSProtocol::fromEcef;
    using GPSProtocol::toEcef;
};

QList<uint16_t> satelliteIds(const GPSSatelliteReport& report)
{
    QList<uint16_t> ids;
    for (uint16_t i = 0; i < report.count; ++i) {
        ids.append(report.satellites[i].id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

}  // namespace

class GPSNativeDataTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _ellipsoidEcefConversion_data();
    void _ellipsoidEcefConversion();
    void _unreportedPosition();
    void _positionValues_data();
    void _positionValues();
    void _velocityValidity_data();
    void _velocityValidity();
    void _fixTypes_data();
    void _fixTypes();
    void _integrityStates_data();
    void _integrityStates();
    void _integrityReceipts();
    void _optionalValues_data();
    void _optionalValues();
    void _satelliteCounts_data();
    void _satelliteCounts();
    void _satelliteMetadata_data();
    void _satelliteMetadata();
    void _satelliteConstellationOverride();
    void _satelliteSnapshotScopes();
    void _satelliteSnapshotBounds();
    void _satelliteSnapshotExpiry();
    void _surveyProjection_data();
    void _surveyProjection();
};

void GPSNativeDataTest::_ellipsoidEcefConversion_data()
{
    QTest::addColumn<GPSEllipsoidPosition>("position");
    QTest::addColumn<double>("x");
    QTest::addColumn<double>("y");
    QTest::addColumn<double>("z");
    QTest::newRow("equator") << GPSEllipsoidPosition{0, 0, 0} << 6378137.0 << 0.0 << 0.0;
    QTest::newRow("equator-east-height") << GPSEllipsoidPosition{0, 90, 100} << 0.0 << 6378237.0 << 0.0;
    QTest::newRow("north-pole") << GPSEllipsoidPosition{90, 45, 0} << 0.0 << 0.0 << 6356752.314245;
    QTest::newRow("south-pole-below-ellipsoid")
        << GPSEllipsoidPosition{-90, -120, -30} << 0.0 << 0.0 << -6356722.314245;
    QTest::newRow("oblique") << GPSEllipsoidPosition{45, 45, 0} << 3194419.145061 << 3194419.145061 << 4487348.408866;
    QTest::newRow("antimeridian") << GPSEllipsoidPosition{0, 180, -30} << -6378107.0 << 0.0 << 0.0;
    QTest::newRow("unknown") << GPSEllipsoidPosition{} << qQNaN() << qQNaN() << qQNaN();
}

void GPSNativeDataTest::_ellipsoidEcefConversion()
{
    QFETCH(GPSEllipsoidPosition, position);
    QFETCH(double, x);
    QFETCH(double, y);
    QFETCH(double, z);
    const auto ecef = CoordinateConversions::toEcef(position);
    const auto restored = CoordinateConversions::fromEcef(ecef);
    if (std::isnan(x)) {
        QVERIFY(std::isnan(ecef.x));
        QVERIFY(std::isnan(ecef.y));
        QVERIFY(std::isnan(ecef.z));
        QVERIFY(std::isnan(restored.latitudeDegrees));
        QVERIFY(std::isnan(restored.longitudeDegrees));
        QVERIFY(std::isnan(restored.altitudeMeters));
        return;
    }
    QVERIFY(std::abs(ecef.x - x) < 0.000001);
    QVERIFY(std::abs(ecef.y - y) < 0.000001);
    QVERIFY(std::abs(ecef.z - z) < 0.000001);
    QVERIFY(std::abs(restored.latitudeDegrees - position.latitudeDegrees) < 1e-10);
    if (std::abs(position.latitudeDegrees) != 90) {
        QVERIFY(std::abs(restored.longitudeDegrees - position.longitudeDegrees) < 1e-10);
    }
    QVERIFY(std::abs(restored.altitudeMeters - position.altitudeMeters) < 0.00001f);
}

void GPSNativeDataTest::_unreportedPosition()
{
    const auto report = GPSNativeData::position({}, {});
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

void GPSNativeDataTest::_positionValues_data()
{
    QTest::addColumn<uint64_t>("timestamp");
    QTest::addColumn<uint64_t>("utcTime");
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("longitude");
    QTest::addColumn<double>("altitudeMsl");
    QTest::addColumn<double>("altitudeEllipsoid");
    QTest::addColumn<float>("quality");
    const double unknown = std::numeric_limits<double>::quiet_NaN();
    QTest::newRow("below-sea-level") << uint64_t{123} << uint64_t{456} << 0.0 << -8.2 << -25.0 << 7.5 << 1.25f;
    QTest::newRow("explicit-zero") << uint64_t{0} << uint64_t{0} << 0.0 << 0.0 << 0.0 << 0.0 << 0.0f;
    QTest::newRow("precision-and-64-bit-time")
        << uint64_t{4'294'967'297} << uint64_t{1'726'000'000'123'456} << -47.123456789 << 179.987654321
        << 12'345.123456789 << 12'400.987654321 << 0.125f;
    QTest::newRow("msl-only") << uint64_t{123} << uint64_t{456} << 47.5 << 8.2 << -25.0 << unknown << 1.25f;
    QTest::newRow("ellipsoid-only") << uint64_t{123} << uint64_t{456} << 47.5 << 8.2 << unknown << 7.5 << 1.25f;
}

void GPSNativeDataTest::_positionValues()
{
    QFETCH(uint64_t, timestamp);
    QFETCH(uint64_t, utcTime);
    QFETCH(double, latitude);
    QFETCH(double, longitude);
    QFETCH(double, altitudeMsl);
    QFETCH(double, altitudeEllipsoid);
    QFETCH(float, quality);
    GPSNativePositionReport source;
    source.timestamp = timestamp;
    source.time_utc_usec = utcTime;
    source.latitude_deg = latitude;
    source.longitude_deg = longitude;
    source.altitude_msl_m = altitudeMsl;
    source.altitude_ellipsoid_m = altitudeEllipsoid;
    source.eph = quality;
    source.epv = quality * 2;
    source.hdop = quality * 3;
    source.vdop = quality * 4;
    source.heading = quality * 5;
    source.heading_accuracy = quality * 6;
    GPSNativeIntegrityReport diagnostic;
    diagnostic.timestamp = 4'294'967'299;
    const auto report = GPSNativeData::position(source, diagnostic);
    QCOMPARE(report.timestampUs, timestamp);
    QCOMPARE(report.utcTimeUs, utcTime);
    QCOMPARE(report.latitudeDegrees, latitude);
    QCOMPARE(report.longitudeDegrees, longitude);
    if (std::isnan(altitudeMsl)) {
        QVERIFY(std::isnan(report.altitudeMslMeters));
    } else {
        QCOMPARE(report.altitudeMslMeters, altitudeMsl);
    }
    if (std::isnan(altitudeEllipsoid)) {
        QVERIFY(std::isnan(report.altitudeEllipsoidMeters));
    } else {
        QCOMPARE(report.altitudeEllipsoidMeters, altitudeEllipsoid);
    }
    QCOMPARE(report.horizontalAccuracyMeters, quality);
    QCOMPARE(report.verticalAccuracyMeters, quality * 2);
    QCOMPARE(report.horizontalDop, quality * 3);
    QCOMPARE(report.verticalDop, quality * 4);
    QCOMPARE(report.headingRadians, quality * 5);
    QCOMPARE(report.headingAccuracyRadians, quality * 6);
    QCOMPARE(report.integrity.timestampUs, diagnostic.timestamp);
}

void GPSNativeDataTest::_velocityValidity_data()
{
    QTest::addColumn<GPSPositionReport::FixType>("fix");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<float>("speed");
    QTest::addColumn<float>("course");
    QTest::newRow("no-fix-invalid") << GPSPositionReport::FixType::NoFix << false << 12.5f << 1.25f;
    QTest::newRow("no-fix-valid") << GPSPositionReport::FixType::NoFix << true << 12.5f << 1.25f;
    QTest::newRow("rtk-fixed-invalid") << GPSPositionReport::FixType::RTKFixed << false << 12.5f << 1.25f;
    QTest::newRow("rtk-fixed-valid") << GPSPositionReport::FixType::RTKFixed << true << 12.5f << 1.25f;
    QTest::newRow("stationary-north") << GPSPositionReport::FixType::Fix3D << true << 0.0f << 0.0f;
}

void GPSNativeDataTest::_velocityValidity()
{
    QFETCH(GPSPositionReport::FixType, fix);
    QFETCH(bool, valid);
    QFETCH(float, speed);
    QFETCH(float, course);
    GPSNativePositionReport source;
    source.fix_type = fix;
    source.vel_ned_valid = valid;
    source.vel_m_s = speed;
    source.cog_rad = course;
    const auto report = GPSNativeData::position(source, {});
    if (valid) {
        QCOMPARE(report.speedMetersPerSecond, speed);
        QCOMPARE(report.courseRadians, course);
    } else {
        QVERIFY(std::isnan(report.speedMetersPerSecond));
        QVERIFY(std::isnan(report.courseRadians));
    }
}

void GPSNativeDataTest::_fixTypes_data()
{
    QTest::addColumn<int>("native");
    QTest::addColumn<GPSPositionReport::FixType>("expected");
    using Fix = GPSPositionReport::FixType;
    QTest::newRow("unknown") << 0 << Fix::Unknown;
    QTest::newRow("none") << 1 << Fix::NoFix;
    QTest::newRow("2d") << 2 << Fix::Fix2D;
    QTest::newRow("3d") << 3 << Fix::Fix3D;
    QTest::newRow("differential") << 4 << Fix::Differential;
    QTest::newRow("rtk-float") << 5 << Fix::RTKFloat;
    QTest::newRow("rtk-fixed") << 6 << Fix::RTKFixed;
    QTest::newRow("reserved-7") << 7 << Fix::Unknown;
    QTest::newRow("extrapolated") << 8 << Fix::Extrapolated;
    QTest::newRow("reserved-9") << 9 << Fix::Unknown;
    QTest::newRow("reserved-255") << 255 << Fix::Unknown;
    QTest::newRow("negative") << -1 << Fix::Unknown;
    QTest::newRow("out-of-byte-range") << 256 << Fix::Unknown;
}

void GPSNativeDataTest::_fixTypes()
{
    QFETCH(int, native);
    QFETCH(GPSPositionReport::FixType, expected);
    GPSNativePositionReport source;
    QCOMPARE(GPSPositionReport::fixTypeFromValue(native), expected);
    source.fix_type = static_cast<GPSPositionReport::FixType>(native);
    QCOMPARE(GPSNativeData::position(source, {}).fixType, expected);
}

void GPSNativeDataTest::_integrityStates_data()
{
    QTest::addColumn<int>("native");
    QTest::addColumn<GPSIntegrityReport::JammingState>("jamming");
    QTest::addColumn<GPSIntegrityReport::SpoofingState>("spoofing");
    QTest::addColumn<GPSIntegrityReport::CorrectionUse>("correctionUse");
    using Jamming = GPSIntegrityReport::JammingState;
    using Spoofing = GPSIntegrityReport::SpoofingState;
    using Use = GPSIntegrityReport::CorrectionUse;
    QTest::newRow("unknown") << 0 << Jamming::Unknown << Spoofing::Unknown << Use::Unknown;
    QTest::newRow("ok-not-used") << 1 << Jamming::Ok << Spoofing::None << Use::NotUsed;
    QTest::newRow("warning-used") << 2 << Jamming::Warning << Spoofing::Indicated << Use::Used;
    QTest::newRow("critical-reserved-use") << 3 << Jamming::Critical << Spoofing::Multiple << Use::Unknown;
    QTest::newRow("reserved-4") << 4 << Jamming::Unknown << Spoofing::Unknown << Use::Unknown;
    QTest::newRow("reserved-255") << 255 << Jamming::Unknown << Spoofing::Unknown << Use::Unknown;
    QTest::newRow("negative") << -1 << Jamming::Unknown << Spoofing::Unknown << Use::Unknown;
    QTest::newRow("out-of-byte-range") << 256 << Jamming::Unknown << Spoofing::Unknown << Use::Unknown;
}

void GPSNativeDataTest::_integrityStates()
{
    QFETCH(int, native);
    QFETCH(GPSIntegrityReport::JammingState, jamming);
    QFETCH(GPSIntegrityReport::SpoofingState, spoofing);
    QFETCH(GPSIntegrityReport::CorrectionUse, correctionUse);
    GPSNativeIntegrityReport diagnostic;
    QCOMPARE(GPSIntegrityReport::jammingStateFromValue(native), jamming);
    QCOMPARE(GPSIntegrityReport::spoofingStateFromValue(native), spoofing);
    QCOMPARE(GPSIntegrityReport::correctionUseFromValue(native), correctionUse);
    diagnostic.jamming_state = static_cast<GPSIntegrityReport::JammingState>(native);
    diagnostic.spoofing_state = static_cast<GPSIntegrityReport::SpoofingState>(native);
    diagnostic.corrections_msg_used = static_cast<GPSIntegrityReport::CorrectionUse>(native);
    diagnostic.jamming_state_timestamp = 100;
    diagnostic.spoofing_state_timestamp = 100;
    diagnostic.corrections_timestamp = 100;
    const auto integrity = GPSNativeData::position({.timestamp = 100}, diagnostic).integrity;
    QCOMPARE(integrity.jamming, jamming);
    QCOMPARE(integrity.spoofing, spoofing);
    QCOMPARE(integrity.correctionUse, correctionUse);
}

void GPSNativeDataTest::_integrityReceipts()
{
    GPSNativeIntegrityReport diagnostic;
    diagnostic.timestamp = 7'000'000;
    diagnostic.jamming_state = GPSIntegrityReport::JammingState::Critical;
    diagnostic.jamming_state_timestamp = 1'000'000;
    diagnostic.spoofing_state = GPSIntegrityReport::SpoofingState::Indicated;
    diagnostic.spoofing_state_timestamp = 7'000'000;
    diagnostic.rf_timestamp = 3'000'000;
    diagnostic.noise_per_ms = 0;
    diagnostic.corrections_timestamp = 6'000'000;
    diagnostic.corrections_crc_failed = false;
    diagnostic.corrections_msg_used = GPSIntegrityReport::CorrectionUse::Used;
    const auto report = GPSNativeData::position({.timestamp = 7'000'000}, diagnostic);
    QCOMPARE(report.integrity.jamming, GPSIntegrityReport::JammingState::Unknown);
    QCOMPARE(report.integrity.jammingTimestampUs, uint64_t{1'000'000});
    QCOMPARE(report.integrity.spoofing, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(report.integrity.spoofingTimestampUs, uint64_t{7'000'000});
    QCOMPARE(report.integrity.noisePerMillisecond, std::optional<int32_t>{0});
    QCOMPARE(report.integrity.rfTimestampUs, uint64_t{3'000'000});
    QCOMPARE(report.integrity.correctionCrcFailed, std::optional<bool>{false});
    QCOMPARE(report.integrity.correctionTimestampUs, uint64_t{6'000'000});
    const auto earlierEpoch = GPSNativeData::position({.timestamp = 6'500'000}, diagnostic);
    QCOMPARE(earlierEpoch.integrity.spoofing, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(earlierEpoch.integrity.jamming, GPSIntegrityReport::JammingState::Unknown);
    const auto delayedPublication = GPSNativeData::position({.timestamp = 6'500'000}, diagnostic, 8'000'000);
    QVERIFY(!delayedPublication.integrity.noisePerMillisecond);
    QCOMPARE(delayedPublication.integrity.spoofing, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(delayedPublication.integrity.rfTimestampUs, uint64_t{3'000'000});

    const auto later = report.integrity.freshAt(8'000'000);
    QVERIFY(!later.noisePerMillisecond);
    QCOMPARE(later.spoofing, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(later.correctionUse, GPSIntegrityReport::CorrectionUse::Used);
    QCOMPARE(report.integrity.noisePerMillisecond, std::optional<int32_t>{0});
    const auto future = report.integrity.freshAt(6'500'000);
    QCOMPARE(future.spoofing, GPSIntegrityReport::SpoofingState::Unknown);
    QCOMPARE(future.correctionUse, GPSIntegrityReport::CorrectionUse::Used);
}

void GPSNativeDataTest::_optionalValues_data()
{
    QTest::addColumn<uint8_t>("satellites");
    QTest::addColumn<GPSNativeIntegrityReport>("diagnostic");
    QTest::newRow("unreported") << uint8_t{255} << GPSNativeIntegrityReport{};
    QTest::newRow("explicit-zero-false") << uint8_t{0}
                                         << GPSNativeIntegrityReport{.noise_per_ms = 0,
                                                                     .automatic_gain_control = 0,
                                                                     .jamming_indicator = 0,
                                                                     .corrections_crc_failed = false};
    QTest::newRow("reported") << uint8_t{18}
                              << GPSNativeIntegrityReport{
                                     .noise_per_ms = 42,
                                     .automatic_gain_control = 12345,
                                     .jamming_indicator = 77,
                                     .corrections_crc_failed = true,
                                     .corrections_msg_used = GPSIntegrityReport::CorrectionUse::Used};
    QTest::newRow("largest-reported") << uint8_t{254}
                                      << GPSNativeIntegrityReport{
                                             .noise_per_ms = (std::numeric_limits<int32_t>::max)(),
                                             .automatic_gain_control = (std::numeric_limits<uint16_t>::max)(),
                                             .jamming_indicator = (std::numeric_limits<int32_t>::max)()};
    QTest::newRow("correction-use-without-crc")
        << uint8_t{255} << GPSNativeIntegrityReport{.corrections_msg_used = GPSIntegrityReport::CorrectionUse::Used};
}

void GPSNativeDataTest::_optionalValues()
{
    QFETCH(uint8_t, satellites);
    QFETCH(GPSNativeIntegrityReport, diagnostic);
    GPSNativePositionReport source;
    source.timestamp = 100;
    diagnostic.rf_timestamp = 100;
    diagnostic.corrections_timestamp = 100;
    source.satellites_used = satellites;
    const auto report = GPSNativeData::position(source, diagnostic);
    QCOMPARE(report.satellitesUsed.has_value(), satellites != 255);
    if (satellites != 255) {
        QCOMPARE(*report.satellitesUsed, satellites);
    }
    QCOMPARE(report.integrity.noisePerMillisecond, diagnostic.noise_per_ms);
    QCOMPARE(report.integrity.automaticGainControl, diagnostic.automatic_gain_control);
    QCOMPARE(report.integrity.jammingIndicator, diagnostic.jamming_indicator);
    QCOMPARE(report.integrity.correctionCrcFailed, diagnostic.corrections_crc_failed);
}

void GPSNativeDataTest::_satelliteCounts_data()
{
    QTest::addColumn<uint16_t>("nativeCount");
    QTest::addColumn<uint16_t>("expectedCount");
    constexpr uint16_t LIMIT = GPSSatelliteReport::MAX_SATELLITES;
    QTest::newRow("empty") << uint16_t{0} << uint16_t{0};
    QTest::newRow("one") << uint16_t{1} << uint16_t{1};
    QTest::newRow("limit") << LIMIT << LIMIT;
    QTest::newRow("above-limit") << uint16_t{LIMIT + 1} << LIMIT;
    QTest::newRow("maximum-count") << (std::numeric_limits<uint16_t>::max)() << LIMIT;
}

void GPSNativeDataTest::_satelliteCounts()
{
    QFETCH(uint16_t, nativeCount);
    QFETCH(uint16_t, expectedCount);
    GPSNativeSatelliteReport source;
    source.timestamp = 4'294'967'297;
    source.count = nativeCount;
    for (uint16_t i = 0; i < source.entries.size(); ++i) {
        source.entries[i].id = 300 + i;
        source.entries[i].prn = 500 + i;
        source.entries[i].used = (i % 2) != 0;
    }
    GPSNativeData::SatelliteSnapshot snapshot;
    const auto report = snapshot.update(source);
    QCOMPARE(report.timestampUs, source.timestamp);
    QCOMPARE(report.count, expectedCount);
    for (uint16_t i = 0; i < expectedCount; ++i) {
        QCOMPARE(report.satellites[i].id, uint16_t(300 + i));
        QCOMPARE(report.satellites[i].prn, uint16_t(500 + i));
        QCOMPARE(report.satellites[i].used, std::optional<bool>{(i % 2) != 0});
    }
    for (uint16_t i = expectedCount; i < report.satellites.size(); ++i) {
        const auto& entry = report.satellites[i];
        QCOMPARE(entry.id, uint16_t{0});
        QCOMPARE(entry.prn, uint16_t{0});
        QVERIFY(!entry.used.has_value());
        QVERIFY(!entry.elevationDegrees.has_value());
        QVERIFY(!entry.azimuthDegrees.has_value());
        QVERIFY(!entry.signalStrength.has_value());
    }
}

void GPSNativeDataTest::_satelliteMetadata_data()
{
    QTest::addColumn<GPSNativeSatelliteData>("entry");
    QTest::addColumn<bool>("elevationKnown");
    QTest::addColumn<bool>("azimuthKnown");
    QTest::addColumn<bool>("signalKnown");
    const auto row = [](const char* name, double elevation, double azimuth, int signal, bool elevationKnown,
                        bool azimuthKnown, bool signalKnown) {
        QTest::newRow(name) << GPSNativeSatelliteData{.id = 300,
                                                      .prn = 501,
                                                      .used = false,
                                                      .elevation = elevation,
                                                      .azimuth = azimuth,
                                                      .signal = signal}
                            << elevationKnown << azimuthKnown << signalKnown;
    };
    QTest::newRow("unreported") << GPSNativeSatelliteData{.id = 300, .prn = 501} << false << false << false;
    row("explicit-zero-false", 0, 0, 0, true, true, true);
    row("negative-elevation", -10.5, 180.25, 45, true, true, true);
    row("lower-boundaries", -90, 0, 0, true, true, true);
    row("upper-boundaries", 90, 360, 255, true, true, true);
    row("below-elevation-limit", -90.001, 180, 45, false, true, true);
    row("above-elevation-limit", 90.001, 180, 45, false, true, true);
    row("below-azimuth-limit", 45, -0.001, 45, true, false, true);
    row("above-azimuth-limit", 45, 360.001, 45, true, false, true);
    row("below-signal-limit", 45, 180, -1, true, true, false);
    row("above-signal-limit", 45, 180, 256, true, true, false);
    row("nan-angles", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), 45, false,
        false, true);
    row("infinite-angles", -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), 45, false,
        false, true);
}

void GPSNativeDataTest::_satelliteMetadata()
{
    QFETCH(GPSNativeSatelliteData, entry);
    QFETCH(bool, elevationKnown);
    QFETCH(bool, azimuthKnown);
    QFETCH(bool, signalKnown);
    GPSNativeSatelliteReport source;
    source.timestamp = 100;
    source.count = 1;
    source.entries[0] = entry;
    GPSNativeData::SatelliteSnapshot snapshot;
    const auto report = snapshot.update(source);
    QCOMPARE(report.count, uint16_t{1});
    const auto& satellite = report.satellites[0];
    QCOMPARE(satellite.id, entry.id);
    QCOMPARE(satellite.prn, entry.prn);
    QCOMPARE(satellite.used, entry.used);
    QCOMPARE(satellite.inViewTimestampUs, source.timestamp);
    QCOMPARE(satellite.inUseTimestampUs, entry.used ? source.timestamp : uint64_t{0});
    QCOMPARE(satellite.elevationDegrees.has_value(), elevationKnown);
    QCOMPARE(satellite.azimuthDegrees.has_value(), azimuthKnown);
    QCOMPARE(satellite.signalStrength.has_value(), signalKnown);
    if (elevationKnown) {
        QCOMPARE(*satellite.elevationDegrees, static_cast<float>(*entry.elevation));
    }
    if (azimuthKnown) {
        QCOMPARE(*satellite.azimuthDegrees, static_cast<float>(*entry.azimuth));
    }
    if (signalKnown) {
        QCOMPARE(*satellite.signalStrength, static_cast<uint8_t>(*entry.signal));
    }
}

void GPSNativeDataTest::_satelliteConstellationOverride()
{
    GPSNativeSatelliteReport source;
    source.timestamp = 100;
    source.count = 1;
    source.constellation = GPSConstellation::GPS;
    source.entries[0] = {.id = 1,
                         .prn = 2,
                         .constellation = GPSConstellation::GLONASS,
                         .used = false,
                         .elevation = 10.123456789,
                         .azimuth = 123.123456789,
                         .signal = 0};
    GPSNativeData::SatelliteSnapshot snapshot;
    const auto report = snapshot.update(source);
    QCOMPARE(report.count, uint16_t{1});
    const auto& satellite = report.satellites[0];
    QCOMPARE(satellite.constellation, GPSConstellation::GPS);
    QCOMPARE(satellite.prn, uint16_t{2});
    QCOMPARE(satellite.used, std::optional<bool>{false});
    QCOMPARE(satellite.signalStrength, std::optional<uint8_t>{0});
    QCOMPARE(satellite.elevationDegrees.value(), static_cast<float>(*source.entries[0].elevation));
    QCOMPARE(satellite.azimuthDegrees.value(), static_cast<float>(*source.entries[0].azimuth));
    QCOMPARE(satellite.inViewTimestampUs, source.timestamp);
    QCOMPARE(satellite.inUseTimestampUs, source.timestamp);
}

void GPSNativeDataTest::_satelliteSnapshotScopes()
{
    GPSNativeData::SatelliteSnapshot snapshot;
    GPSNativeSatelliteReport whole;
    whole.timestamp = 100;
    whole.count = 3;
    whole.entries[0] = {.id = 1, .constellation = GPSConstellation::GPS, .used = false, .signal = 0};
    whole.entries[1] = {.id = 33, .constellation = GPSConstellation::SBAS};
    whole.entries[2] = {.id = 65, .constellation = GPSConstellation::GLONASS};
    QCOMPARE(satelliteIds(snapshot.update(whole)), (QList<uint16_t>{1, 33, 65}));

    GPSNativeSatelliteReport scoped;
    scoped.timestamp = 200;
    scoped.constellation = GPSConstellation::GLONASS;
    scoped.count = 1;
    scoped.entries[0].id = 66;
    auto report = snapshot.update(scoped);
    QCOMPARE(report.timestampUs, uint64_t{200});
    QCOMPARE(satelliteIds(report), (QList<uint16_t>{1, 33, 66}));
    const auto gps = std::find_if(report.satellites.begin(), report.satellites.begin() + report.count,
                                  [](const auto& entry) { return entry.id == 1; });
    QVERIFY(gps != report.satellites.begin() + report.count);
    QCOMPARE(gps->used, std::optional<bool>{false});
    QCOMPARE(gps->signalStrength, std::optional<uint8_t>{0});

    scoped.count = 0;
    scoped.timestamp = 300;
    scoped.constellation = GPSConstellation::Galileo;
    report = snapshot.update(scoped);
    QCOMPARE(report.timestampUs, uint64_t{300});
    QCOMPARE(satelliteIds(report), (QList<uint16_t>{1, 33, 66}));
    scoped.constellation = GPSConstellation::GLONASS;
    QCOMPARE(satelliteIds(snapshot.update(scoped)), (QList<uint16_t>{1, 33}));

    whole.count = 1;
    whole.timestamp = 400;
    whole.entries[0] = {.id = 301, .constellation = GPSConstellation::Galileo};
    QCOMPARE(satelliteIds(snapshot.update(whole)), (QList<uint16_t>{301}));
    scoped.constellation = GPSConstellation::SBAS;
    scoped.timestamp = 500;
    QCOMPARE(satelliteIds(snapshot.update(scoped)), (QList<uint16_t>{301}));
    whole.count = 0;
    whole.timestamp = 600;
    QCOMPARE(snapshot.update(whole).count, uint16_t{0});
    QCOMPARE(snapshot.update(scoped).count, uint16_t{0});
}

void GPSNativeDataTest::_satelliteSnapshotBounds()
{
    GPSNativeData::SatelliteSnapshot snapshot;
    GPSNativeSatelliteReport full;
    full.timestamp = 4'294'967'296;
    full.constellation = GPSConstellation::GPS;
    full.count = GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES;
    for (uint16_t i = 0; i < full.count; ++i) {
        full.entries[i].id = i + 1;
    }
    QCOMPARE(snapshot.update(full).count, GPSSatelliteReport::MAX_SATELLITES);
    GPSNativeSatelliteReport extra;
    extra.constellation = GPSConstellation::Galileo;
    extra.timestamp = 4'294'967'297;
    extra.count = 1;
    extra.entries[0].id = 301;
    const auto report = snapshot.update(extra);
    QCOMPARE(report.count, GPSSatelliteReport::MAX_SATELLITES);
    QCOMPARE(report.timestampUs, extra.timestamp);
    full.count = 0;
    full.timestamp = extra.timestamp + 1;
    QCOMPARE(satelliteIds(snapshot.update(full)), (QList<uint16_t>{301}));
}

void GPSNativeDataTest::_satelliteSnapshotExpiry()
{
    GPSNativeData::SatelliteSnapshot state;
    GPSNativeSatelliteReport gps;
    gps.constellation = GPSConstellation::GPS;
    gps.timestamp = 1'000'000;
    gps.count = 1;
    gps.entries[0] = {.id = 1, .used = false, .signal = 0};
    state.update(gps);
    GPSNativeSatelliteReport glonass;
    glonass.constellation = GPSConstellation::GLONASS;
    glonass.timestamp = 4'000'000;
    glonass.count = 1;
    glonass.entries[0].id = 65;
    auto snapshot = state.update(glonass);
    QCOMPARE(snapshot.count, uint16_t{2});
    QCOMPARE(snapshot.satellites[0].inViewTimestampUs, gps.timestamp);
    QCOMPARE(snapshot.satellites[0].inUseTimestampUs, gps.timestamp);

    glonass.timestamp = 6'000'000;
    snapshot = state.update(glonass);
    QCOMPARE(satelliteIds(snapshot), (QList<uint16_t>{65}));
    QCOMPARE(satelliteIds(state.update(gps)), (QList<uint16_t>{65}));  // Retired receipts cannot resurrect a view.
    gps.timestamp = 6'000'001;
    snapshot = state.update(gps);
    QCOMPARE(snapshot.count, uint16_t{2});
    QCOMPARE(snapshot.satellites[0].signalStrength, std::optional<uint8_t>{0});
    QCOMPARE(snapshot.satellites[0].used, std::optional<bool>{false});

    GPSNativeSatelliteReport usage;
    usage.constellation = GPSConstellation::GPS;
    usage.usage = GPSNativeSatelliteReport::Usage{};
    usage.usage->timestamp = 7'000'000;
    usage.usage->count = 1;
    usage.usage->ids[0] = 1;
    GPSNativeData::SatelliteSnapshot usageOnly;
    const auto unavailableView = usageOnly.update(usage);
    QCOMPARE(unavailableView.timestampUs, uint64_t{0});
    QCOMPARE(unavailableView.count, uint16_t{0});
    snapshot = state.update(usage);
    QCOMPARE(snapshot.satellites[0].used, std::optional<bool>{true});
    QCOMPARE(snapshot.satellites[0].inViewTimestampUs, gps.timestamp);
    QCOMPARE(snapshot.satellites[0].inUseTimestampUs, usage.usage->timestamp);
    gps.timestamp = 7'500'000;
    gps.entries[0].used.reset();
    snapshot = state.update(gps);
    QCOMPARE(snapshot.satellites[0].used, std::optional<bool>{true});
    gps.timestamp = 12'000'000;
    snapshot = state.update(gps);
    QCOMPARE(snapshot.count, uint16_t{1});
    QVERIFY(!snapshot.satellites[0].used);
    QCOMPARE(snapshot.satellites[0].inUseTimestampUs, uint64_t{0});
    gps.count = 0;
    gps.timestamp++;
    QCOMPARE(state.update(gps).count, uint16_t{0});
    QVERIFY(!state.expire(gps.timestamp + 4'999'999));
    const auto expired = state.expire(gps.timestamp + 5'000'000);
    QVERIFY(expired);
    QCOMPARE(expired->count, uint16_t{0});
    QVERIFY(!state.expire(gps.timestamp + 6'000'000));

    gps.timestamp += 7'000'000;
    gps.count = 1;
    state.update(gps);
    const auto gone = state.expire(gps.timestamp + 5'000'000);
    QVERIFY(gone);
    QCOMPARE(gone->count, uint16_t{0});
    QVERIFY(!state.expire(gps.timestamp + 5'000'001));
}

void GPSNativeDataTest::_surveyProjection_data()
{
    QTest::addColumn<GPSNativeSurveyReport::AltitudeDatum>("datum");
    QTest::addColumn<bool>("accuracyKnown");
    QTest::addColumn<uint32_t>("accuracyMillimeters");
    QTest::addColumn<double>("accuracyMeters");
    QTest::addColumn<uint32_t>("duration");
    QTest::addColumn<uint8_t>("flags");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<bool>("active");
    using Datum = GPSNativeSurveyReport::AltitudeDatum;
    const double unknown = std::numeric_limits<double>::quiet_NaN();
    QTest::newRow("unknown-datum-and-accuracy")
        << Datum::Unknown << false << uint32_t{1250} << unknown << uint32_t{0} << uint8_t{0} << false << false;
    QTest::newRow("msl-is-not-ellipsoid")
        << Datum::MeanSeaLevel << false << uint32_t{0} << unknown << uint32_t{180} << uint8_t{1} << true << false;
    QTest::newRow("ellipsoid-and-known-zero-accuracy")
        << Datum::Ellipsoid << true << uint32_t{0} << 0.0 << uint32_t{180} << uint8_t{2} << false << true;
    QTest::newRow("millimeters-to-meters")
        << Datum::Ellipsoid << true << uint32_t{1250} << 1.25 << uint32_t{180} << uint8_t{3} << true << true;
    QTest::newRow("full-width-values") << Datum::Ellipsoid << true << (std::numeric_limits<uint32_t>::max)()
                                       << 4'294'967.295 << (std::numeric_limits<uint32_t>::max)() << uint8_t{0xfd}
                                       << true << false;
    QTest::newRow("reserved-flags") << Datum::Ellipsoid << false << uint32_t{0} << unknown << uint32_t{0}
                                    << uint8_t{0xfc} << false << false;
}

void GPSNativeDataTest::_surveyProjection()
{
    QFETCH(GPSNativeSurveyReport::AltitudeDatum, datum);
    QFETCH(bool, accuracyKnown);
    QFETCH(uint32_t, accuracyMillimeters);
    QFETCH(double, accuracyMeters);
    QFETCH(uint32_t, duration);
    QFETCH(uint8_t, flags);
    QFETCH(bool, valid);
    QFETCH(bool, active);
    const GPSNativeSurveyReport source{.altitudeDatum = datum,
                                       .accuracyKnown = accuracyKnown,
                                       .latitude = -47.123456789,
                                       .longitude = 179.987654321,
                                       .altitude = -25.5f,
                                       .mean_accuracy = accuracyMillimeters,
                                       .duration = duration,
                                       .flags = flags};
    const auto report = GPSNativeData::survey(source);
    QCOMPARE(report.position.latitudeDegrees, -47.123456789);
    QCOMPARE(report.position.longitudeDegrees, 179.987654321);
    if (datum == GPSNativeSurveyReport::AltitudeDatum::Ellipsoid) {
        QCOMPARE(report.position.altitudeMeters, -25.5f);
    } else {
        QVERIFY(std::isnan(report.position.altitudeMeters));
    }
    if (std::isnan(accuracyMeters)) {
        QVERIFY(!report.meanAccuracyMeters.has_value());
    } else {
        QVERIFY(report.meanAccuracyMeters.has_value());
        QCOMPARE(*report.meanAccuracyMeters, accuracyMeters);
    }
    QCOMPARE(report.duration, std::chrono::seconds(duration));
    QCOMPARE(report.valid, valid);
    QCOMPARE(report.active, active);
}

UT_REGISTER_TEST(GPSNativeDataTest, TestLabel::Unit)

#include "GPSNativeDataTest.moc"
