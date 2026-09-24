#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

#include <QtTest/QTest>

#include "GPSFixQuality.h"
#include "GPSNativeData_p.h"
#include "GPSProtocol.h"
#include "UnitTest.h"

Q_DECLARE_METATYPE(GPSIntegrityReport)
Q_DECLARE_METATYPE(GPSEllipsoidPosition)

static_assert(std::is_same_v<decltype(GPSNavigationValues::fixType), GPSPositionReport::FixType>);
static_assert(std::is_same_v<decltype(GPSIntegrityReport::Jamming::state), GPSIntegrityReport::JammingState>);
static_assert(std::is_same_v<decltype(GPSIntegrityReport::Spoofing::state), GPSIntegrityReport::SpoofingState>);
static_assert(std::is_same_v<decltype(GPSIntegrityReport::Corrections::use), GPSIntegrityReport::CorrectionUse>);

namespace {

struct CoordinateConversions : GPSProtocol
{
    using GPSProtocol::EcefMeters;
    using GPSProtocol::fromEcef;
    using GPSProtocol::toEcef;
};

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
    void _satelliteSnapshotScopes();
    void _satelliteSnapshotBounds();
    void _satelliteSnapshotExpiry();
    void _satelliteUsageCombination();
    void _satelliteUsageExpiryFallback();
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
    QCOMPARE(report.navigation.timestampUs, uint64_t{0});
    QCOMPARE(report.navigation.utcTimeUs, uint64_t{0});
    QCOMPARE(report.navigation.fixType, GPSPositionReport::FixType::Unknown);
    QVERIFY(std::isnan(report.navigation.latitudeDegrees));
    QVERIFY(std::isnan(report.navigation.longitudeDegrees));
    QVERIFY(std::isnan(report.navigation.altitudeMslMeters));
    QVERIFY(std::isnan(report.navigation.altitudeEllipsoidMeters));
    QVERIFY(std::isnan(report.navigation.horizontalAccuracyMeters));
    QVERIFY(std::isnan(report.navigation.verticalAccuracyMeters));
    QVERIFY(std::isnan(report.navigation.horizontalDop));
    QVERIFY(std::isnan(report.navigation.verticalDop));
    QVERIFY(std::isnan(report.navigation.speedMetersPerSecond));
    QVERIFY(std::isnan(report.navigation.courseRadians));
    QVERIFY(std::isnan(report.navigation.headingRadians));
    QVERIFY(std::isnan(report.navigation.headingAccuracyRadians));
    QVERIFY(!report.navigation.satellitesUsed.has_value());
    QCOMPARE(report.integrity.timestampUs, uint64_t{0});
    QCOMPARE(report.integrity.jamming.state, GPSIntegrityReport::JammingState::Unknown);
    QCOMPARE(report.integrity.spoofing.state, GPSIntegrityReport::SpoofingState::Unknown);
    QCOMPARE(report.integrity.corrections.use, GPSIntegrityReport::CorrectionUse::Unknown);
    QVERIFY(!report.integrity.rf.noisePerMillisecond.has_value());
    QVERIFY(!report.integrity.rf.automaticGainControl.has_value());
    QVERIFY(!report.integrity.rf.jammingIndicator.has_value());
    QVERIFY(!report.integrity.corrections.crcFailed.has_value());
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
    source.navigation.timestampUs = timestamp;
    source.navigation.utcTimeUs = utcTime;
    source.navigation.latitudeDegrees = latitude;
    source.navigation.longitudeDegrees = longitude;
    source.navigation.altitudeMslMeters = altitudeMsl;
    source.navigation.altitudeEllipsoidMeters = altitudeEllipsoid;
    source.navigation.horizontalAccuracyMeters = quality;
    source.navigation.verticalAccuracyMeters = quality * 2;
    source.navigation.horizontalDop = quality * 3;
    source.navigation.verticalDop = quality * 4;
    source.navigation.headingRadians = quality * 5;
    source.navigation.headingAccuracyRadians = quality * 6;
    GPSIntegrityReport diagnostic;
    diagnostic.timestampUs = 4'294'967'299;
    const auto report = GPSNativeData::position(source, diagnostic);
    QCOMPARE(report.navigation.timestampUs, timestamp);
    QCOMPARE(report.navigation.utcTimeUs, utcTime);
    QCOMPARE(report.navigation.latitudeDegrees, latitude);
    QCOMPARE(report.navigation.longitudeDegrees, longitude);
    if (std::isnan(altitudeMsl)) {
        QVERIFY(std::isnan(report.navigation.altitudeMslMeters));
    } else {
        QCOMPARE(report.navigation.altitudeMslMeters, altitudeMsl);
    }
    if (std::isnan(altitudeEllipsoid)) {
        QVERIFY(std::isnan(report.navigation.altitudeEllipsoidMeters));
    } else {
        QCOMPARE(report.navigation.altitudeEllipsoidMeters, altitudeEllipsoid);
    }
    QCOMPARE(report.navigation.horizontalAccuracyMeters, quality);
    QCOMPARE(report.navigation.verticalAccuracyMeters, quality * 2);
    QCOMPARE(report.navigation.horizontalDop, quality * 3);
    QCOMPARE(report.navigation.verticalDop, quality * 4);
    QCOMPARE(report.navigation.headingRadians, quality * 5);
    QCOMPARE(report.navigation.headingAccuracyRadians, quality * 6);
    QCOMPARE(report.integrity.timestampUs, diagnostic.timestampUs);
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
    source.navigation.fixType = fix;
    source.velocityValid = valid;
    source.navigation.speedMetersPerSecond = speed;
    source.navigation.courseRadians = course;
    const auto report = GPSNativeData::position(source, {});
    if (valid) {
        QCOMPARE(report.navigation.speedMetersPerSecond, speed);
        QCOMPARE(report.navigation.courseRadians, course);
    } else {
        QVERIFY(std::isnan(report.navigation.speedMetersPerSecond));
        QVERIFY(std::isnan(report.navigation.courseRadians));
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
    QCOMPARE(gpsFixQualityFromValue(native), expected);
    source.navigation.fixType = static_cast<GPSPositionReport::FixType>(native);
    QCOMPARE(GPSNativeData::position(source, {}).navigation.fixType, expected);
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
    GPSIntegrityReport diagnostic;
    QCOMPARE(GPSIntegrityReport::jammingStateFromValue(native), jamming);
    QCOMPARE(GPSIntegrityReport::spoofingStateFromValue(native), spoofing);
    QCOMPARE(GPSIntegrityReport::correctionUseFromValue(native), correctionUse);
    diagnostic.jamming.state = static_cast<GPSIntegrityReport::JammingState>(native);
    diagnostic.spoofing.state = static_cast<GPSIntegrityReport::SpoofingState>(native);
    diagnostic.corrections.use = static_cast<GPSIntegrityReport::CorrectionUse>(native);
    diagnostic.jamming.timestampUs = 100;
    diagnostic.spoofing.timestampUs = 100;
    diagnostic.corrections.timestampUs = 100;
    const auto integrity = GPSNativeData::position({.navigation = {.timestampUs = 100}}, diagnostic).integrity;
    QCOMPARE(integrity.jamming.state, jamming);
    QCOMPARE(integrity.spoofing.state, spoofing);
    QCOMPARE(integrity.corrections.use, correctionUse);
}

void GPSNativeDataTest::_integrityReceipts()
{
    GPSIntegrityReport diagnostic;
    diagnostic.timestampUs = 7'000'000;
    diagnostic.jamming.state = GPSIntegrityReport::JammingState::Critical;
    diagnostic.jamming.timestampUs = 1'000'000;
    diagnostic.spoofing.state = GPSIntegrityReport::SpoofingState::Indicated;
    diagnostic.spoofing.timestampUs = 7'000'000;
    diagnostic.rf.timestampUs = 3'000'000;
    diagnostic.rf.noisePerMillisecond = 0;
    diagnostic.corrections.timestampUs = 6'000'000;
    diagnostic.corrections.crcFailed = false;
    diagnostic.corrections.use = GPSIntegrityReport::CorrectionUse::Used;
    diagnostic.corrections.protocol = GPSIntegrityReport::CorrectionProtocol::RTCM3;
    const auto report = GPSNativeData::position({.navigation = {.timestampUs = 7'000'000}}, diagnostic);
    QCOMPARE(report.integrity.jamming.state, GPSIntegrityReport::JammingState::Unknown);
    QCOMPARE(report.integrity.jamming.timestampUs, uint64_t{1'000'000});
    QCOMPARE(report.integrity.spoofing.state, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(report.integrity.spoofing.timestampUs, uint64_t{7'000'000});
    QCOMPARE(report.integrity.rf.noisePerMillisecond, std::optional<int32_t>{0});
    QCOMPARE(report.integrity.rf.timestampUs, uint64_t{3'000'000});
    QCOMPARE(report.integrity.corrections.crcFailed, std::optional<bool>{false});
    QCOMPARE(report.integrity.corrections.protocol, GPSIntegrityReport::CorrectionProtocol::RTCM3);
    QCOMPARE(report.integrity.corrections.timestampUs, uint64_t{6'000'000});
    const auto earlierEpoch = GPSNativeData::position({.navigation = {.timestampUs = 6'500'000}}, diagnostic);
    QCOMPARE(earlierEpoch.integrity.spoofing.state, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(earlierEpoch.integrity.jamming.state, GPSIntegrityReport::JammingState::Unknown);
    const auto delayedPublication =
        GPSNativeData::position({.navigation = {.timestampUs = 6'500'000}}, diagnostic, 8'000'000);
    QVERIFY(!delayedPublication.integrity.rf.noisePerMillisecond);
    QCOMPARE(delayedPublication.integrity.spoofing.state, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(delayedPublication.integrity.rf.timestampUs, uint64_t{3'000'000});

    const auto later = report.integrity.freshAt(8'000'000);
    QVERIFY(!later.rf.noisePerMillisecond);
    QCOMPARE(later.spoofing.state, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(later.corrections.use, GPSIntegrityReport::CorrectionUse::Used);
    QCOMPARE(report.integrity.rf.noisePerMillisecond, std::optional<int32_t>{0});
    QCOMPARE(report.integrity.freshAt(11'000'000).corrections.protocol,
             GPSIntegrityReport::CorrectionProtocol::Unknown);
    const auto future = report.integrity.freshAt(6'500'000);
    QCOMPARE(future.spoofing.state, GPSIntegrityReport::SpoofingState::Unknown);
    QCOMPARE(future.corrections.use, GPSIntegrityReport::CorrectionUse::Used);
}

void GPSNativeDataTest::_optionalValues_data()
{
    QTest::addColumn<uint8_t>("satellites");
    QTest::addColumn<GPSIntegrityReport>("diagnostic");
    QTest::newRow("unreported") << uint8_t{255} << GPSIntegrityReport{};
    QTest::newRow("explicit-zero-false") << uint8_t{0}
                                         << GPSIntegrityReport{.rf = {.noisePerMillisecond = 0,
                                                                      .automaticGainControl = 0,
                                                                      .jammingIndicator = 0},
                                                               .corrections = {.crcFailed = false}};
    QTest::newRow("reported")
        << uint8_t{18}
        << GPSIntegrityReport{.rf = {.noisePerMillisecond = 42, .automaticGainControl = 12345, .jammingIndicator = 77},
                              .corrections = {.use = GPSIntegrityReport::CorrectionUse::Used, .crcFailed = true}};
    QTest::newRow("largest-reported") << uint8_t{254}
                                      << GPSIntegrityReport{
                                             .rf = {.noisePerMillisecond = (std::numeric_limits<int32_t>::max)(),
                                                    .automaticGainControl = (std::numeric_limits<uint16_t>::max)(),
                                                    .jammingIndicator = (std::numeric_limits<int32_t>::max)()}};
    QTest::newRow("correction-use-without-crc")
        << uint8_t{255} << GPSIntegrityReport{.corrections = {.use = GPSIntegrityReport::CorrectionUse::Used}};
}

void GPSNativeDataTest::_optionalValues()
{
    QFETCH(uint8_t, satellites);
    QFETCH(GPSIntegrityReport, diagnostic);
    GPSNativePositionReport source;
    source.navigation.timestampUs = 100;
    diagnostic.rf.timestampUs = 100;
    diagnostic.corrections.timestampUs = 100;
    source.navigation.satellitesUsed = satellites;
    const auto report = GPSNativeData::position(source, diagnostic);
    QCOMPARE(report.navigation.satellitesUsed.has_value(), satellites != 255);
    if (satellites != 255) {
        QCOMPARE(*report.navigation.satellitesUsed, satellites);
    }
    QCOMPARE(report.integrity.rf.noisePerMillisecond, diagnostic.rf.noisePerMillisecond);
    QCOMPARE(report.integrity.rf.automaticGainControl, diagnostic.rf.automaticGainControl);
    QCOMPARE(report.integrity.rf.jammingIndicator, diagnostic.rf.jammingIndicator);
    QCOMPARE(report.integrity.corrections.crcFailed, diagnostic.corrections.crcFailed);
}

void GPSNativeDataTest::_satelliteCounts_data()
{
    QTest::addColumn<int>("nativeCount");
    QTest::addColumn<int>("expectedCount");
    constexpr int LIMIT = GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES;
    QTest::newRow("empty") << 0 << 0;
    QTest::newRow("one") << 1 << 1;
    QTest::newRow("limit") << LIMIT << LIMIT;
    QTest::newRow("above-limit") << (LIMIT + 1) << LIMIT;
    QTest::newRow("negative") << -1 << 0;
}

void GPSNativeDataTest::_satelliteCounts()
{
    QFETCH(int, nativeCount);
    QFETCH(int, expectedCount);
    GPSNativeSatelliteReport source;
    auto* system = source.ensureConstellation(GPSConstellation::Unknown);
    QVERIFY(system);
    system->inViewTimestampUs = 4'294'967'297;
    system->inView = nativeCount;
    system->inUseTimestampUs = system->inViewTimestampUs;
    system->inUse = nativeCount;
    GPSNativeData::SatelliteSnapshot snapshot;
    const auto report = snapshot.update(source);
    QCOMPARE(report.timestampUs, system->inViewTimestampUs);
    QCOMPARE(report.inView, std::optional<int>{expectedCount});
    QCOMPARE(report.used, std::optional<int>{expectedCount});
}

void GPSNativeDataTest::_satelliteSnapshotScopes()
{
    GPSNativeData::SatelliteSnapshot snapshot;
    GPSNativeSatelliteReport whole;
    auto* gps = whole.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(gps);
    gps->inViewTimestampUs = 100;
    gps->inView = 1;
    gps->inUseTimestampUs = 100;
    gps->inUse = 0;
    auto* sbas = whole.ensureConstellation(GPSConstellation::SBAS);
    QVERIFY(sbas);
    sbas->inViewTimestampUs = 100;
    sbas->inView = 1;
    auto* glonass = whole.ensureConstellation(GPSConstellation::GLONASS);
    QVERIFY(glonass);
    glonass->inViewTimestampUs = 100;
    glonass->inView = 1;
    auto report = snapshot.update(whole);
    QCOMPARE(report.timestampUs, uint64_t{100});
    QCOMPARE(report.inView, std::optional<int>{3});
    QVERIFY(!report.used);

    GPSNativeSatelliteReport scoped;
    scoped.fullSnapshot = false;
    glonass = scoped.ensureConstellation(GPSConstellation::GLONASS);
    QVERIFY(glonass);
    glonass->inViewTimestampUs = 200;
    glonass->inView = 1;
    glonass->inUseTimestampUs = 200;
    glonass->inUse = 1;
    report = snapshot.update(scoped);
    QCOMPARE(report.timestampUs, uint64_t{200});
    QCOMPARE(report.inView, std::optional<int>{3});
    QVERIFY(!report.used);

    scoped = {};
    scoped.fullSnapshot = false;
    auto* galileo = scoped.ensureConstellation(GPSConstellation::Galileo);
    QVERIFY(galileo);
    galileo->inViewTimestampUs = 300;
    galileo->inView = 0;
    report = snapshot.update(scoped);
    QCOMPARE(report.timestampUs, uint64_t{300});
    QCOMPARE(report.inView, std::optional<int>{3});
    scoped.constellations[0].constellation = GPSConstellation::GLONASS;
    report = snapshot.update(scoped);
    QCOMPARE(report.inView, std::optional<int>{2});

    whole = {};
    galileo = whole.ensureConstellation(GPSConstellation::Galileo);
    QVERIFY(galileo);
    galileo->inViewTimestampUs = 400;
    galileo->inView = 1;
    galileo->inUseTimestampUs = 400;
    galileo->inUse = 0;
    report = snapshot.update(whole);
    QCOMPARE(report.inView, std::optional<int>{1});
    QCOMPARE(report.used, std::optional<int>{0});

    whole = {};
    auto* empty = whole.ensureConstellation(GPSConstellation::Unknown);
    QVERIFY(empty);
    empty->inViewTimestampUs = 600;
    empty->inView = 0;
    report = snapshot.update(whole);
    QCOMPARE(report.inView, std::optional<int>{0});
    QCOMPARE(report.used, std::optional<int>{0});
}

void GPSNativeDataTest::_satelliteSnapshotBounds()
{
    GPSNativeData::SatelliteSnapshot snapshot;
    GPSNativeSatelliteReport full;
    auto* gps = full.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(gps);
    gps->inViewTimestampUs = 4'294'967'296;
    gps->inView = GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES;
    gps->inUseTimestampUs = gps->inViewTimestampUs;
    gps->inUse = GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES;
    QCOMPARE(snapshot.update(full).inView, std::optional<int>{int(GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES)});
    GPSNativeSatelliteReport extra;
    extra.fullSnapshot = false;
    auto* galileo = extra.ensureConstellation(GPSConstellation::Galileo);
    QVERIFY(galileo);
    galileo->inViewTimestampUs = 4'294'967'297;
    galileo->inView = 1;
    galileo->inUseTimestampUs = galileo->inViewTimestampUs;
    galileo->inUse = 1;
    const auto report = snapshot.update(extra);
    QCOMPARE(report.inView, std::optional<int>{int(GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES) + 1});
    QCOMPARE(report.timestampUs, galileo->inViewTimestampUs);
    full = {};
    auto* emptyGps = full.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(emptyGps);
    emptyGps->inViewTimestampUs = galileo->inViewTimestampUs + 1;
    QCOMPARE(snapshot.update(full).inView, std::optional<int>{0});
}

void GPSNativeDataTest::_satelliteSnapshotExpiry()
{
    GPSNativeData::SatelliteSnapshot state;
    GPSNativeSatelliteReport gps;
    gps.fullSnapshot = false;
    auto* gpsGroup = gps.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(gpsGroup);
    gpsGroup->inViewTimestampUs = 1'000'000;
    gpsGroup->inView = 1;
    gpsGroup->inUseTimestampUs = 1'000'000;
    gpsGroup->inUse = 0;
    state.update(gps);
    GPSNativeSatelliteReport glonass;
    glonass.fullSnapshot = false;
    auto* glonassGroup = glonass.ensureConstellation(GPSConstellation::GLONASS);
    QVERIFY(glonassGroup);
    glonassGroup->inViewTimestampUs = 4'000'000;
    glonassGroup->inView = 1;
    glonassGroup->inUseTimestampUs = 4'000'000;
    glonassGroup->inUse = 1;
    auto snapshot = state.update(glonass);
    QCOMPARE(snapshot.inView, std::optional<int>{2});
    QCOMPARE(snapshot.used, std::optional<int>{1});

    glonassGroup->inViewTimestampUs = 6'000'000;
    glonassGroup->inUseTimestampUs = 6'000'000;
    snapshot = state.update(glonass);
    QCOMPARE(snapshot.inView, std::optional<int>{1});
    QCOMPARE(snapshot.used, std::optional<int>{1});
    QCOMPARE(state.update(gps).inView, std::optional<int>{1});  // Retired receipts cannot resurrect a view.
    gpsGroup->inViewTimestampUs = 6'000'001;
    gpsGroup->inUseTimestampUs = 0;
    gpsGroup->inUse.reset();
    snapshot = state.update(gps);
    QCOMPARE(snapshot.inView, std::optional<int>{2});
    QVERIFY(!snapshot.used);

    GPSNativeSatelliteReport usage;
    usage.fullSnapshot = false;
    auto* usageGroup = usage.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(usageGroup);
    usageGroup->inUseTimestampUs = 7'000'000;
    usageGroup->inUse = 1;
    GPSNativeData::SatelliteSnapshot usageOnly;
    const auto unavailableView = usageOnly.update(usage);
    QCOMPARE(unavailableView.timestampUs, uint64_t{0});
    QVERIFY(!unavailableView.inView);
    snapshot = state.update(usage);
    QCOMPARE(snapshot.used, std::optional<int>{2});
    gpsGroup->inViewTimestampUs = 7'500'000;
    snapshot = state.update(gps);
    QCOMPARE(snapshot.used, std::optional<int>{2});
    gpsGroup->inViewTimestampUs = 12'000'000;
    snapshot = state.update(gps);
    QCOMPARE(snapshot.inView, std::optional<int>{1});
    QVERIFY(!snapshot.used);
    gpsGroup->inView = 0;
    ++gpsGroup->inViewTimestampUs;
    QCOMPARE(state.update(gps).inView, std::optional<int>{0});
    QVERIFY(!state.expire(gpsGroup->inViewTimestampUs + 4'999'999));
    const auto expired = state.expire(gpsGroup->inViewTimestampUs + 5'000'000);
    QVERIFY(expired);
    QVERIFY(!expired->inView);
    QVERIFY(!state.expire(gpsGroup->inViewTimestampUs + 6'000'000));

    gpsGroup->inViewTimestampUs += 7'000'000;
    gpsGroup->inView = 1;
    state.update(gps);
    const auto gone = state.expire(gpsGroup->inViewTimestampUs + 5'000'000);
    QVERIFY(gone);
    QVERIFY(!gone->inView);
    QVERIFY(!state.expire(gpsGroup->inViewTimestampUs + 5'000'001));
}

void GPSNativeDataTest::_satelliteUsageCombination()
{
    GPSNativeData::SatelliteSnapshot snapshot;
    auto countOnly = snapshot.update(GPSNativeSatelliteUsageReport{.timestampUs = 100, .usedCount = 12});
    QVERIFY(!countOnly.inView);
    QCOMPARE(countOnly.used, std::optional<int>{12});

    GPSNativeSatelliteReport view;
    auto* gps = view.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(gps);
    gps->inViewTimestampUs = 200;
    gps->inView = 4;
    auto report = snapshot.update(view);
    QCOMPARE(report.inView, std::optional<int>{4});
    QCOMPARE(report.used, std::optional<int>{12});

    gps->inViewTimestampUs = 300;
    gps->inUseTimestampUs = 300;
    gps->inUse = 2;
    report = snapshot.update(view);
    QCOMPARE(report.inView, std::optional<int>{4});
    QCOMPARE(report.used, std::optional<int>{2});

    countOnly = snapshot.update(GPSNativeSatelliteUsageReport{.timestampUs = 400, .usedCount = 7});
    QCOMPARE(countOnly.inView, std::optional<int>{4});
    QCOMPARE(countOnly.used, std::optional<int>{7});

    countOnly = snapshot.update(GPSNativeSatelliteUsageReport{.timestampUs = 500});
    QCOMPARE(countOnly.inView, std::optional<int>{4});
    QVERIFY(!countOnly.used);

    gps->inViewTimestampUs = 600;
    gps->inView = 0;
    gps->inUseTimestampUs = 0;
    gps->inUse.reset();
    report = snapshot.update(view);
    QCOMPARE(report.inView, std::optional<int>{0});
    QCOMPARE(report.used, std::optional<int>{0});
}

void GPSNativeDataTest::_satelliteUsageExpiryFallback()
{
    GPSNativeData::SatelliteSnapshot snapshot;
    auto countOnly = snapshot.update(GPSNativeSatelliteUsageReport{.timestampUs = 500'000, .usedCount = 9});
    QVERIFY(!countOnly.inView);
    QCOMPARE(countOnly.used, std::optional<int>{9});

    GPSNativeSatelliteReport view;
    auto* gps = view.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(gps);
    gps->inViewTimestampUs = 1'000'000;
    gps->inView = 3;
    auto report = snapshot.update(view);
    QCOMPARE(report.inView, std::optional<int>{3});
    QCOMPARE(report.used, std::optional<int>{9});

    QVERIFY(!snapshot.expire(5'999'999));
    const auto expired = snapshot.expire(6'000'000);
    QVERIFY(expired);
    QVERIFY(!expired->inView);
    QCOMPARE(expired->used, std::optional<int>{9});
}

void GPSNativeDataTest::_surveyProjection_data()
{
    QTest::addColumn<bool>("altitudeKnown");
    QTest::addColumn<bool>("accuracyKnown");
    QTest::addColumn<double>("accuracyMeters");
    QTest::addColumn<uint32_t>("duration");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<bool>("active");
    const double unknown = std::numeric_limits<double>::quiet_NaN();
    QTest::newRow("position-without-altitude") << false << false << unknown << uint32_t{0} << false << false;
    QTest::newRow("zero-accuracy") << true << true << 0.0 << uint32_t{180} << false << true;
    QTest::newRow("millimeters-to-meters") << true << true << 1.25 << uint32_t{180} << true << true;
    QTest::newRow("full-width-values") << true << true << 4'294'967.295 << (std::numeric_limits<uint32_t>::max)()
                                       << true << false;
}

void GPSNativeDataTest::_surveyProjection()
{
    QFETCH(bool, altitudeKnown);
    QFETCH(bool, accuracyKnown);
    QFETCH(double, accuracyMeters);
    QFETCH(uint32_t, duration);
    QFETCH(bool, valid);
    QFETCH(bool, active);
    GPSNativeSurveyReport source;
    source.survey.position.latitudeDegrees = -47.123456789;
    source.survey.position.longitudeDegrees = 179.987654321;
    if (altitudeKnown) {
        source.survey.position.altitudeMeters = -25.5f;
    }
    if (accuracyKnown) {
        source.survey.meanAccuracyMeters = accuracyMeters;
    }
    source.survey.duration = std::chrono::seconds(duration);
    source.survey.valid = valid;
    source.survey.active = active;
    const auto& report = source.survey;
    QCOMPARE(report.position.latitudeDegrees, -47.123456789);
    QCOMPARE(report.position.longitudeDegrees, 179.987654321);
    if (altitudeKnown) {
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
