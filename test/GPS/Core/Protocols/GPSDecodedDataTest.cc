#include "GPSDecodedDataTest.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "GPSDecodedData.h"
#include "GPSReceiverReports.h"

void GPSDecodedDataTest::_unreportedPosition()
{
    const auto report = GPSDecodedData::position({}, {});
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
    QVERIFY(!report.navigation.satellitesUsed.has_value());
    QCOMPARE(report.integrity.timestampUs, uint64_t{0});
    QCOMPARE(report.integrity.jamming.state, GPSIntegrityReport::JammingState::Unknown);
    QCOMPARE(report.integrity.spoofing.state, GPSIntegrityReport::SpoofingState::Unknown);
}

void GPSDecodedDataTest::_positionValues_data()
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

void GPSDecodedDataTest::_positionValues()
{
    QFETCH(uint64_t, timestamp);
    QFETCH(uint64_t, utcTime);
    QFETCH(double, latitude);
    QFETCH(double, longitude);
    QFETCH(double, altitudeMsl);
    QFETCH(double, altitudeEllipsoid);
    QFETCH(float, quality);
    GPSDecodedPosition source;
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
    GPSIntegrityReport diagnostic;
    diagnostic.timestampUs = 4'294'967'299;
    const auto report = GPSDecodedData::position(source, diagnostic);
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
    QCOMPARE(report.integrity.timestampUs, diagnostic.timestampUs);
}

void GPSDecodedDataTest::_velocityValidity_data()
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

void GPSDecodedDataTest::_velocityValidity()
{
    QFETCH(GPSPositionReport::FixType, fix);
    QFETCH(bool, valid);
    QFETCH(float, speed);
    QFETCH(float, course);
    GPSDecodedPosition source;
    source.navigation.fixType = fix;
    source.velocityValid = valid;
    source.navigation.speedMetersPerSecond = speed;
    source.navigation.courseRadians = course;
    const auto report = GPSDecodedData::position(source, {});
    if (valid) {
        QCOMPARE(report.navigation.speedMetersPerSecond, speed);
        QCOMPARE(report.navigation.courseRadians, course);
    } else {
        QVERIFY(std::isnan(report.navigation.speedMetersPerSecond));
        QVERIFY(std::isnan(report.navigation.courseRadians));
    }
}

void GPSDecodedDataTest::_fixTypes_data()
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

void GPSDecodedDataTest::_fixTypes()
{
    QFETCH(int, native);
    QFETCH(GPSPositionReport::FixType, expected);
    GPSDecodedPosition source;
    QCOMPARE(gpsFixQualityFromValue(native), expected);
    source.navigation.fixType = static_cast<GPSPositionReport::FixType>(native);
    QCOMPARE(GPSDecodedData::position(source, {}).navigation.fixType, expected);
}

void GPSDecodedDataTest::_integrityStates_data()
{
    QTest::addColumn<int>("native");
    QTest::addColumn<GPSIntegrityReport::JammingState>("jamming");
    QTest::addColumn<GPSIntegrityReport::SpoofingState>("spoofing");
    using Jamming = GPSIntegrityReport::JammingState;
    using Spoofing = GPSIntegrityReport::SpoofingState;
    QTest::newRow("unknown") << 0 << Jamming::Unknown << Spoofing::Unknown;
    QTest::newRow("ok") << 1 << Jamming::Ok << Spoofing::None;
    QTest::newRow("warning") << 2 << Jamming::Warning << Spoofing::Indicated;
    QTest::newRow("critical") << 3 << Jamming::Critical << Spoofing::Multiple;
    QTest::newRow("reserved-4") << 4 << Jamming::Unknown << Spoofing::Unknown;
    QTest::newRow("reserved-255") << 255 << Jamming::Unknown << Spoofing::Unknown;
    QTest::newRow("negative") << -1 << Jamming::Unknown << Spoofing::Unknown;
    QTest::newRow("out-of-byte-range") << 256 << Jamming::Unknown << Spoofing::Unknown;
}

void GPSDecodedDataTest::_integrityStates()
{
    QFETCH(int, native);
    QFETCH(GPSIntegrityReport::JammingState, jamming);
    QFETCH(GPSIntegrityReport::SpoofingState, spoofing);
    GPSIntegrityReport diagnostic;
    QCOMPARE(GPSIntegrityReport::jammingStateFromValue(native), jamming);
    QCOMPARE(GPSIntegrityReport::spoofingStateFromValue(native), spoofing);
    diagnostic.jamming.state = static_cast<GPSIntegrityReport::JammingState>(native);
    diagnostic.spoofing.state = static_cast<GPSIntegrityReport::SpoofingState>(native);
    diagnostic.jamming.timestampUs = 100;
    diagnostic.spoofing.timestampUs = 100;
    const auto integrity = GPSDecodedData::position({.navigation = {.timestampUs = 100}}, diagnostic).integrity;
    QCOMPARE(integrity.jamming.state, jamming);
    QCOMPARE(integrity.spoofing.state, spoofing);
}

void GPSDecodedDataTest::_integrityReceipts()
{
    GPSIntegrityReport diagnostic;
    diagnostic.timestampUs = 7'000'000;
    diagnostic.jamming.state = GPSIntegrityReport::JammingState::Critical;
    diagnostic.jamming.timestampUs = 1'000'000;
    diagnostic.spoofing.state = GPSIntegrityReport::SpoofingState::Indicated;
    diagnostic.spoofing.timestampUs = 7'000'000;
    const auto report = GPSDecodedData::position({.navigation = {.timestampUs = 7'000'000}}, diagnostic);
    QCOMPARE(report.integrity.jamming.state, GPSIntegrityReport::JammingState::Unknown);
    QCOMPARE(report.integrity.jamming.timestampUs, uint64_t{1'000'000});
    QCOMPARE(report.integrity.spoofing.state, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(report.integrity.spoofing.timestampUs, uint64_t{7'000'000});
    const auto earlierEpoch = GPSDecodedData::position({.navigation = {.timestampUs = 6'500'000}}, diagnostic);
    QCOMPARE(earlierEpoch.integrity.spoofing.state, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(earlierEpoch.integrity.jamming.state, GPSIntegrityReport::JammingState::Unknown);
    const auto delayedPublication =
        GPSDecodedData::position({.navigation = {.timestampUs = 6'500'000}}, diagnostic, 8'000'000);
    QCOMPARE(delayedPublication.integrity.spoofing.state, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(delayedPublication.integrity.jamming.timestampUs, uint64_t{1'000'000});

    QCOMPARE(report.integrity.freshAt(8'000'000).spoofing.state, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(report.integrity.freshAt(12'000'000).spoofing.state, GPSIntegrityReport::SpoofingState::Unknown);
    const auto future = report.integrity.freshAt(6'500'000);
    QCOMPARE(future.spoofing.state, GPSIntegrityReport::SpoofingState::Unknown);
}

void GPSDecodedDataTest::_optionalValues_data()
{
    QTest::addColumn<uint8_t>("satellites");
    QTest::newRow("unreported") << uint8_t{255};
    QTest::newRow("explicit-zero") << uint8_t{0};
    QTest::newRow("reported") << uint8_t{18};
    QTest::newRow("largest-reported") << uint8_t{254};
}

void GPSDecodedDataTest::_optionalValues()
{
    QFETCH(uint8_t, satellites);
    const auto used = gpsSatellitesUsed(satellites);
    QCOMPARE(used.has_value(), satellites != 255);
    if (satellites != 255) {
        QCOMPARE(*used, satellites);
    }
}

void GPSDecodedDataTest::_satelliteCounts_data()
{
    QTest::addColumn<int>("nativeCount");
    QTest::addColumn<int>("expectedCount");
    constexpr int LIMIT = GPSDecodedSatellites::SAT_INFO_MAX_SATELLITES;
    QTest::newRow("empty") << 0 << 0;
    QTest::newRow("one") << 1 << 1;
    QTest::newRow("limit") << LIMIT << LIMIT;
    QTest::newRow("above-limit") << (LIMIT + 1) << LIMIT;
    QTest::newRow("negative") << -1 << 0;
}

void GPSDecodedDataTest::_satelliteCounts()
{
    QFETCH(int, nativeCount);
    QFETCH(int, expectedCount);
    GPSDecodedSatellites source;
    auto* system = source.ensureConstellation(GPSConstellation::Unknown);
    QVERIFY(system);
    system->inViewTimestampUs = 4'294'967'297;
    system->inView = nativeCount;
    system->inUseTimestampUs = system->inViewTimestampUs;
    system->inUse = nativeCount;
    GPSDecodedData::SatelliteCounts counts;
    const auto report = counts.update(source, system->inViewTimestampUs);
    QCOMPARE(report.timestampUs, system->inViewTimestampUs);
    QCOMPARE(report.inView, std::optional<int>{expectedCount});
    QCOMPARE(report.used, std::optional<int>{expectedCount});
}

void GPSDecodedDataTest::_satelliteSnapshotScopes()
{
    GPSDecodedData::SatelliteCounts counts;
    GPSDecodedSatellites whole;
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
    auto report = counts.update(whole, 100);
    QCOMPARE(report.timestampUs, uint64_t{100});
    QCOMPARE(report.inView, std::optional<int>{3});
    QVERIFY(!report.used);

    GPSDecodedSatellites scoped;
    scoped.fullSnapshot = false;
    glonass = scoped.ensureConstellation(GPSConstellation::GLONASS);
    QVERIFY(glonass);
    glonass->inViewTimestampUs = 200;
    glonass->inView = 1;
    glonass->inUseTimestampUs = 200;
    glonass->inUse = 1;
    report = counts.update(scoped, 200);
    QCOMPARE(report.timestampUs, uint64_t{200});
    QCOMPARE(report.inView, std::optional<int>{3});
    QVERIFY(!report.used);

    scoped = {};
    scoped.fullSnapshot = false;
    auto* galileo = scoped.ensureConstellation(GPSConstellation::Galileo);
    QVERIFY(galileo);
    galileo->inViewTimestampUs = 300;
    galileo->inView = 0;
    report = counts.update(scoped, 300);
    QCOMPARE(report.timestampUs, uint64_t{300});
    QCOMPARE(report.inView, std::optional<int>{3});
    scoped.constellations[0].constellation = GPSConstellation::GLONASS;
    report = counts.update(scoped, 300);
    QCOMPARE(report.inView, std::optional<int>{2});

    whole = {};
    galileo = whole.ensureConstellation(GPSConstellation::Galileo);
    QVERIFY(galileo);
    galileo->inViewTimestampUs = 400;
    galileo->inView = 1;
    galileo->inUseTimestampUs = 400;
    galileo->inUse = 0;
    report = counts.update(whole, 400);
    QCOMPARE(report.inView, std::optional<int>{1});
    QCOMPARE(report.used, std::optional<int>{0});

    whole = {};
    auto* empty = whole.ensureConstellation(GPSConstellation::Unknown);
    QVERIFY(empty);
    empty->inViewTimestampUs = 600;
    empty->inView = 0;
    report = counts.update(whole, 600);
    QCOMPARE(report.inView, std::optional<int>{0});
    QCOMPARE(report.used, std::optional<int>{0});
}

void GPSDecodedDataTest::_satelliteSnapshotBounds()
{
    GPSDecodedData::SatelliteCounts counts;
    GPSDecodedSatellites full;
    auto* gps = full.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(gps);
    gps->inViewTimestampUs = 4'294'967'296;
    gps->inView = GPSDecodedSatellites::SAT_INFO_MAX_SATELLITES;
    gps->inUseTimestampUs = gps->inViewTimestampUs;
    gps->inUse = GPSDecodedSatellites::SAT_INFO_MAX_SATELLITES;
    QCOMPARE(counts.update(full, gps->inViewTimestampUs).inView,
             std::optional<int>{int(GPSDecodedSatellites::SAT_INFO_MAX_SATELLITES)});
    GPSDecodedSatellites extra;
    extra.fullSnapshot = false;
    auto* galileo = extra.ensureConstellation(GPSConstellation::Galileo);
    QVERIFY(galileo);
    galileo->inViewTimestampUs = 4'294'967'297;
    galileo->inView = 1;
    galileo->inUseTimestampUs = galileo->inViewTimestampUs;
    galileo->inUse = 1;
    const auto report = counts.update(extra, galileo->inViewTimestampUs);
    QCOMPARE(report.inView, std::optional<int>{int(GPSDecodedSatellites::SAT_INFO_MAX_SATELLITES) + 1});
    QCOMPARE(report.timestampUs, galileo->inViewTimestampUs);
    full = {};
    auto* emptyGps = full.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(emptyGps);
    emptyGps->inViewTimestampUs = galileo->inViewTimestampUs + 1;
    QCOMPARE(counts.update(full, emptyGps->inViewTimestampUs).inView, std::optional<int>{0});
}

void GPSDecodedDataTest::_satelliteSnapshotExpiry()
{
    GPSDecodedData::SatelliteCounts state;
    GPSDecodedSatellites gps;
    gps.fullSnapshot = false;
    auto* gpsGroup = gps.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(gpsGroup);
    gpsGroup->inViewTimestampUs = 1'000'000;
    gpsGroup->inView = 1;
    gpsGroup->inUseTimestampUs = 1'000'000;
    gpsGroup->inUse = 0;
    state.update(gps, 1'000'000);
    GPSDecodedSatellites glonass;
    glonass.fullSnapshot = false;
    auto* glonassGroup = glonass.ensureConstellation(GPSConstellation::GLONASS);
    QVERIFY(glonassGroup);
    glonassGroup->inViewTimestampUs = 4'000'000;
    glonassGroup->inView = 1;
    glonassGroup->inUseTimestampUs = 4'000'000;
    glonassGroup->inUse = 1;
    auto snapshot = state.update(glonass, 4'000'000);
    QCOMPARE(snapshot.inView, std::optional<int>{2});
    QCOMPARE(snapshot.used, std::optional<int>{1});

    glonassGroup->inViewTimestampUs = 6'000'000;
    glonassGroup->inUseTimestampUs = 6'000'000;
    snapshot = state.update(glonass, 6'000'000);
    QCOMPARE(snapshot.inView, std::optional<int>{1});
    QCOMPARE(snapshot.used, std::optional<int>{1});
    QCOMPARE(state.update(gps, 6'000'000).inView, std::optional<int>{1});  // Expired receipts cannot resurrect a view.
    gpsGroup->inViewTimestampUs = 6'000'001;
    gpsGroup->inUseTimestampUs = 0;
    gpsGroup->inUse.reset();
    snapshot = state.update(gps, 6'000'001);
    QCOMPARE(snapshot.inView, std::optional<int>{2});
    QVERIFY(!snapshot.used);

    GPSDecodedSatellites usage;
    usage.fullSnapshot = false;
    auto* usageGroup = usage.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(usageGroup);
    usageGroup->inUseTimestampUs = 7'000'000;
    usageGroup->inUse = 1;
    GPSDecodedData::SatelliteCounts usageOnly;
    const auto unavailableView = usageOnly.update(usage, 7'000'000);
    QCOMPARE(unavailableView.timestampUs, uint64_t{0});
    QVERIFY(!unavailableView.inView);
    snapshot = state.update(usage, 7'000'000);
    QCOMPARE(snapshot.used, std::optional<int>{2});
    gpsGroup->inViewTimestampUs = 7'500'000;
    snapshot = state.update(gps, 7'500'000);
    QCOMPARE(snapshot.used, std::optional<int>{2});
    gpsGroup->inViewTimestampUs = 12'000'000;
    snapshot = state.update(gps, 12'000'000);
    QCOMPARE(snapshot.inView, std::optional<int>{1});
    QVERIFY(!snapshot.used);
    gpsGroup->inView = 0;
    ++gpsGroup->inViewTimestampUs;
    QCOMPARE(state.update(gps, gpsGroup->inViewTimestampUs).inView, std::optional<int>{0});
    QVERIFY(!state.expire(gpsGroup->inViewTimestampUs + 4'999'999));
    const auto expired = state.expire(gpsGroup->inViewTimestampUs + 5'000'000);
    QVERIFY(expired);
    QVERIFY(!expired->inView);
    QVERIFY(!state.expire(gpsGroup->inViewTimestampUs + 6'000'000));

    gpsGroup->inViewTimestampUs += 7'000'000;
    gpsGroup->inView = 1;
    state.update(gps, gpsGroup->inViewTimestampUs);
    const auto gone = state.expire(gpsGroup->inViewTimestampUs + 5'000'000);
    QVERIFY(gone);
    QVERIFY(!gone->inView);
    QVERIFY(!state.expire(gpsGroup->inViewTimestampUs + 5'000'001));
}

void GPSDecodedDataTest::_satelliteReceipts()
{
    constexpr uint64_t NOW = 10'000'000;
    GPSDecodedData::SatelliteCounts counts;
    GPSDecodedSatellites gps;
    gps.fullSnapshot = false;
    auto* group = gps.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(group);
    group->inViewTimestampUs = NOW;
    group->inView = 2;
    group->inUseTimestampUs = NOW;
    group->inUse = 1;
    QCOMPARE(counts.update(gps, NOW).used, std::optional<int>{1});

    // An older count does not replace a newer one, and a count received in the future is ignored.
    group->inViewTimestampUs = NOW - 1;
    group->inView = 3;
    group->inUseTimestampUs = NOW + 1;
    group->inUse = 2;
    auto report = counts.update(gps, NOW);
    QCOMPARE(report.timestampUs, NOW);
    QCOMPARE(report.inView, std::optional<int>{2});
    QCOMPARE(report.used, std::optional<int>{1});

    // A used receipt without a count makes the used count unknown.
    group->inViewTimestampUs = 0;
    group->inUseTimestampUs = NOW + 1;
    group->inUse.reset();
    report = counts.update(gps, NOW + 1);
    QCOMPARE(report.inView, std::optional<int>{2});
    QVERIFY(!report.used);
}

void GPSDecodedDataTest::_satelliteUsageCombination()
{
    GPSDecodedData::SatelliteCounts counts;
    auto countOnly = counts.update(GPSDecodedSatelliteUsage{.timestampUs = 100, .usedCount = 12}, 100);
    QVERIFY(!countOnly.inView);
    QCOMPARE(countOnly.used, std::optional<int>{12});

    GPSDecodedSatellites view;
    auto* gps = view.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(gps);
    gps->inViewTimestampUs = 200;
    gps->inView = 4;
    auto report = counts.update(view, 200);
    QCOMPARE(report.inView, std::optional<int>{4});
    QCOMPARE(report.used, std::optional<int>{12});

    gps->inViewTimestampUs = 300;
    gps->inUseTimestampUs = 300;
    gps->inUse = 2;
    report = counts.update(view, 300);
    QCOMPARE(report.inView, std::optional<int>{4});
    QCOMPARE(report.used, std::optional<int>{2});

    countOnly = counts.update(GPSDecodedSatelliteUsage{.timestampUs = 400, .usedCount = 7}, 400);
    QCOMPARE(countOnly.inView, std::optional<int>{4});
    QCOMPARE(countOnly.used, std::optional<int>{7});

    countOnly = counts.update(GPSDecodedSatelliteUsage{.timestampUs = 500}, 500);
    QCOMPARE(countOnly.inView, std::optional<int>{4});
    QVERIFY(!countOnly.used);

    gps->inViewTimestampUs = 600;
    gps->inView = 0;
    gps->inUseTimestampUs = 0;
    gps->inUse.reset();
    report = counts.update(view, 600);
    QCOMPARE(report.inView, std::optional<int>{0});
    QCOMPARE(report.used, std::optional<int>{0});
}

void GPSDecodedDataTest::_satelliteUsageExpiryFallback()
{
    GPSDecodedData::SatelliteCounts counts;
    auto countOnly = counts.update(GPSDecodedSatelliteUsage{.timestampUs = 500'000, .usedCount = 9}, 500'000);
    QVERIFY(!countOnly.inView);
    QCOMPARE(countOnly.used, std::optional<int>{9});

    GPSDecodedSatellites view;
    auto* gps = view.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(gps);
    gps->inViewTimestampUs = 1'000'000;
    gps->inView = 3;
    auto report = counts.update(view, 1'000'000);
    QCOMPARE(report.inView, std::optional<int>{3});
    QCOMPARE(report.used, std::optional<int>{9});

    QVERIFY(!counts.expire(5'999'999));
    const auto expired = counts.expire(6'000'000);
    QVERIFY(expired);
    QVERIFY(!expired->inView);
    QCOMPARE(expired->used, std::optional<int>{9});
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSDecodedDataTest, TestLabel::Unit)
