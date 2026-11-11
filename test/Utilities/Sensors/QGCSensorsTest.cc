#include "QGCSensorsTest.h"

#include <limits>

#include "QGCSensors.h"

void QGCSensorsTest::_testAmbientTemperatureFilter_data()
{
    QTest::addColumn<qreal>("temperatureC");
    QTest::addColumn<bool>("accepted");

    QTest::newRow("room temperature") << 21.5 << true;
    QTest::newRow("coldest valid") << -40.0 << true;
    QTest::newRow("below coldest valid") << -40.01 << false;
    QTest::newRow("hottest valid") << 85.0 << true;
    QTest::newRow("above hottest valid") << 85.01 << false;
    QTest::newRow("not a number") << std::numeric_limits<qreal>::quiet_NaN() << false;
    QTest::newRow("infinite") << std::numeric_limits<qreal>::infinity() << false;
}

void QGCSensorsTest::_testAmbientTemperatureFilter()
{
    QFETCH(qreal, temperatureC);
    QFETCH(bool, accepted);

    QAmbientTemperatureReading reading;
    reading.setTemperature(temperatureC);

    QGCSensors::QGCAmbientTemperatureFilter filter;
    QCOMPARE(filter.filter(&reading), accepted);
}

void QGCSensorsTest::_testPressureFilter_data()
{
    QTest::addColumn<qreal>("pressurePa");
    QTest::addColumn<qreal>("temperatureC");
    QTest::addColumn<bool>("accepted");

    constexpr qreal seaLevelPa = 101325.0;
    constexpr qreal roomC = 21.5;

    QTest::newRow("sea level at room temperature") << seaLevelPa << roomC << true;
    QTest::newRow("lowest valid pressure") << 45000.0 << roomC << true;
    QTest::newRow("below lowest valid pressure") << 44999.9 << roomC << false;
    QTest::newRow("highest valid pressure") << 110000.0 << roomC << true;
    QTest::newRow("above highest valid pressure") << 110000.1 << roomC << false;
    QTest::newRow("infinite pressure") << std::numeric_limits<qreal>::infinity() << roomC << false;
    QTest::newRow("pressure not a number") << std::numeric_limits<qreal>::quiet_NaN() << roomC << false;
    QTest::newRow("coldest valid temperature") << seaLevelPa << -40.0 << true;
    QTest::newRow("below coldest valid temperature") << seaLevelPa << -40.01 << false;
    QTest::newRow("hottest valid temperature") << seaLevelPa << 85.0 << true;
    QTest::newRow("above hottest valid temperature") << seaLevelPa << 85.01 << false;
    QTest::newRow("temperature not a number") << seaLevelPa << std::numeric_limits<qreal>::quiet_NaN() << false;
}

void QGCSensorsTest::_testPressureFilter()
{
    QFETCH(qreal, pressurePa);
    QFETCH(qreal, temperatureC);
    QFETCH(bool, accepted);

    QPressureReading reading;
    reading.setPressure(pressurePa);
    reading.setTemperature(temperatureC);

    QGCSensors::QGCPressureFilter filter;
    QCOMPARE(filter.filter(&reading), accepted);
}

void QGCSensorsTest::_testCompassFilter_data()
{
    QTest::addColumn<qreal>("calibrationLevel");
    QTest::addColumn<bool>("accepted");

    QTest::newRow("fully calibrated") << 1.0 << true;
    QTest::newRow("lowest usable calibration") << 0.65 << true;
    QTest::newRow("below lowest usable calibration") << 0.64 << false;
    QTest::newRow("uncalibrated") << 0.0 << false;
    QTest::newRow("not a number") << std::numeric_limits<qreal>::quiet_NaN() << false;
}

void QGCSensorsTest::_testCompassFilter()
{
    QFETCH(qreal, calibrationLevel);
    QFETCH(bool, accepted);

    QCompassReading reading;
    reading.setAzimuth(123.0);
    reading.setCalibrationLevel(calibrationLevel);

    QGCSensors::QGCCompassFilter filter;
    QCOMPARE(filter.filter(&reading), accepted);
}

UT_REGISTER_TEST(QGCSensorsTest, TestLabel::Unit, TestLabel::Utilities)
