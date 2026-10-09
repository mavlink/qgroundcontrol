#include "GPSProtocolMathTest.h"

#include <cmath>
#include <cstdint>
#include <ctime>
#include <limits>

#include "GPSProtocolMath.h"
#include "GPSTime.h"

void GPSProtocolMathTest::_ecefConversion_data()
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

void GPSProtocolMathTest::_ecefConversion()
{
    QFETCH(GPSEllipsoidPosition, position);
    QFETCH(double, x);
    QFETCH(double, y);
    QFETCH(double, z);
    const auto ecef = GPSProtocolMath::toEcef(position);
    const auto restored = GPSProtocolMath::fromEcef(ecef);
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

void GPSProtocolMathTest::_utcMicroseconds_data()
{
    QTest::addColumn<QList<int>>("calendar");
    QTest::addColumn<int>("nanoseconds");
    QTest::addColumn<qint64>("epoch");
    const auto row = [](const char* name, QList<int> calendar, int nanoseconds, qint64 epoch) {
        QTest::newRow(name) << calendar << nanoseconds << epoch;
    };
    // 2026-07-12T23:00:00Z is 1783897200 s after the epoch.
    row("sub-second", {2026, 7, 12, 23, 0, 0}, 250000000, 1783897200);
    row("negative-sub-second", {2026, 7, 12, 23, 0, 0}, -1000, 1783897200);
    row("before-plausibility-floor", {2000, 7, 12, 23, 0, 0}, 0, 0);
    row("receiver-fix", {2026, 9, 8, 15, 58, 9}, 0, 1788883089);
    row("winter", {2026, 1, 8, 15, 58, 9}, 0, 1767887889);
    row("leap-day", {2024, 2, 29, 23, 59, 59}, 0, 1709251199);
    row("second-overflow", {2024, 2, 29, 23, 59, 60}, 0, 1709251200);
    row("sbf-gps-week", {1980, 1, 6 + 2435 * 7, 0, 0, 2 * 86400 + 15 * 3600 + 58 * 60 + 9}, 0, 1788883089);
    row("non-leap-century", {2100, 2, 29, 0, 0, 0}, 0, 4107542400LL);
    row("month-underflow", {2026, 0, 1, 0, 0, 0}, 0, 1764547200);
    row("month-overflow", {2026, 13, 1, 0, 0, 0}, 0, 1798761600);
    row("day-underflow", {2024, 3, 0, 0, 0, 0}, 0, 1709164800);
    row("second-underflow", {2024, 1, 1, 0, 0, -1}, 0, 1704067199);
    row("signed-32-bit-limit", {2038, 1, 19, 3, 14, 7}, 0, 2147483647);
    row("signed-32-bit-overflow", {2038, 1, 19, 3, 14, 8}, 0, 2147483648LL);
    row("signed-32-bit-next-day", {2038, 1, 20, 0, 0, 0}, 0, 2147558400LL);
    row("year-out-of-range", {65535, 12, 31, 0, 0, 0}, 0, 0);
}

void GPSProtocolMathTest::_utcMicroseconds()
{
    QFETCH(QList<int>, calendar);
    QFETCH(int, nanoseconds);
    QFETCH(qint64, epoch);
    // Dates the platform's time_t cannot hold are unknown.
    const bool representable =
        static_cast<uint64_t>(epoch) <= static_cast<uint64_t>((std::numeric_limits<time_t>::max)());
    const quint64 expected = epoch > 0 && representable ? quint64(epoch) * 1000000ULL + nanoseconds / 1000 : 0;
    QCOMPARE(GPSProtocolMath::utcMicroseconds(calendar[0], calendar[1], calendar[2], calendar[3], calendar[4],
                                              calendar[5], nanoseconds),
             expected);
}

void GPSProtocolMathTest::_towAdvances_data()
{
    QTest::addColumn<uint>("later");
    QTest::addColumn<uint>("earlier");
    QTest::addColumn<bool>("advances");
    constexpr uint WEEK = GPSTime::WEEK_MS;
    QTest::newRow("next-epoch") << 2000U << 1000U << true;
    QTest::newRow("same-epoch") << 1000U << 1000U << false;
    QTest::newRow("earlier-epoch") << 1000U << 2000U << false;
    QTest::newRow("across-rollover") << 500U << WEEK - 500 << true;
    QTest::newRow("late-before-rollover") << WEEK - 500 << 500U << false;
    QTest::newRow("just-under-half-week") << WEEK / 2 - 1 << 0U << true;
    QTest::newRow("half-week") << WEEK / 2 << 0U << false;
}

void GPSProtocolMathTest::_towAdvances()
{
    QFETCH(uint, later);
    QFETCH(uint, earlier);
    QFETCH(bool, advances);
    QCOMPARE(GPSTime::towAdvances(later, earlier), advances);
    QCOMPARE(GPSTime::epochMs(2435, later), quint64(2435) * GPSTime::WEEK_MS + later);
}

void GPSProtocolMathTest::_nearEarthSurface()
{
    QVERIFY(GPSProtocolMath::nearEarthSurface({6378137, 0, 0}));
    QVERIFY(GPSProtocolMath::nearEarthSurface({0, 0, -6356752.3}));
    QVERIFY(!GPSProtocolMath::nearEarthSurface({}));
    QVERIFY(!GPSProtocolMath::nearEarthSurface({5999999, 0, 0}));
    QVERIFY(!GPSProtocolMath::nearEarthSurface({7000001, 0, 0}));
    QVERIFY(!GPSProtocolMath::nearEarthSurface({qQNaN(), 0, 0}));
}

void GPSProtocolMathTest::_fixedDecimals_data()
{
    QTest::addColumn<double>("value");
    QTest::addColumn<int>("decimals");
    QTest::addColumn<QByteArray>("expected");
    // Command text keeps printf's "%.Nf" bytes: exact binary ties round to even, and negative zero keeps its sign.
    QTest::newRow("tie-down") << 0.125 << 2 << QByteArray("0.12");
    QTest::newRow("tie-up") << 0.375 << 2 << QByteArray("0.38");
    QTest::newRow("tie-even-integer") << 2.5 << 0 << QByteArray("2");
    QTest::newRow("tie-four-decimals") << 500.03125 << 4 << QByteArray("500.0312");
    QTest::newRow("negative-zero") << -0.0 << 4 << QByteArray("-0.0000");
    QTest::newRow("negative-rounds-to-zero") << -0.00001 << 4 << QByteArray("-0.0000");
    QTest::newRow("ecef-coordinate") << 4315616.67264 << 4 << QByteArray("4315616.6726");
    QTest::newRow("eight-decimals") << 47.001953125 << 8 << QByteArray("47.00195312");
    QTest::newRow("integer-nine-decimals") << 15.0 << 9 << QByteArray("15.000000000");
}

void GPSProtocolMathTest::_fixedDecimals()
{
    QFETCH(double, value);
    QFETCH(int, decimals);
    QFETCH(QByteArray, expected);
    QCOMPARE(gpsFixedDecimal(value, decimals), expected);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolMathTest, TestLabel::Unit)
