#include "GPSRTKFactGroupTest.h"
#include "GPSRTKFactGroup.h"
#include "UnitTest.h"

void GPSRTKFactGroupTest::testFactInitialValues()
{
    GPSRTKFactGroup fg;

    QCOMPARE(fg.connected()->rawValue().toBool(), false);
    QCOMPARE(fg.active()->rawValue().toBool(), false);
    QCOMPARE(fg.valid()->rawValue().toBool(), false);
    QCOMPARE(fg.numSatellites()->rawValue().toInt(), 0);
    QCOMPARE(fg.currentDuration()->rawValue().toDouble(), 0.0);
    // Facts with "default": null initialize to NaN
    QVERIFY(qIsNaN(fg.currentAccuracy()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.currentLatitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.currentLongitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.currentAltitude()->rawValue().toDouble()));
}

void GPSRTKFactGroupTest::testBasePositionFacts()
{
    GPSRTKFactGroup fg;

    fg.baseLatitude()->setRawValue(47.123456);
    fg.baseLongitude()->setRawValue(8.654321);
    fg.baseAltitude()->setRawValue(550.5);
    fg.baseFixType()->setRawValue(6); // RTK Fixed

    QCOMPARE(fg.baseLatitude()->rawValue().toDouble(), 47.123456);
    QCOMPARE(fg.baseLongitude()->rawValue().toDouble(), 8.654321);
    QCOMPARE(fg.baseAltitude()->rawValue().toDouble(), 550.5);
    QCOMPARE(fg.baseFixType()->rawValue().toInt(), 6);
}

void GPSRTKFactGroupTest::testGnssRelativeFacts()
{
    GPSRTKFactGroup fg;

    fg.heading()->setRawValue(1.5708);
    fg.headingValid()->setRawValue(true);
    fg.baselineLength()->setRawValue(2.35);
    fg.carrierFixed()->setRawValue(true);

    QCOMPARE(fg.heading()->rawValue().toDouble(), 1.5708);
    QCOMPARE(fg.headingValid()->rawValue().toBool(), true);
    QCOMPARE(fg.baselineLength()->rawValue().toDouble(), 2.35);
    QCOMPARE(fg.carrierFixed()->rawValue().toBool(), true);
}

void GPSRTKFactGroupTest::testSurveyInFacts()
{
    GPSRTKFactGroup fg;

    fg.currentDuration()->setRawValue(120.0);
    fg.currentAccuracy()->setRawValue(0.025);
    fg.currentLatitude()->setRawValue(37.7749);
    fg.currentLongitude()->setRawValue(-122.4194);
    fg.currentAltitude()->setRawValue(10.2);
    fg.valid()->setRawValue(true);
    fg.active()->setRawValue(false);
    fg.connected()->setRawValue(true);

    QCOMPARE(fg.currentDuration()->rawValue().toDouble(), 120.0);
    QVERIFY(qAbs(fg.currentAccuracy()->rawValue().toDouble() - 0.025) < 1e-9);
    QCOMPARE(fg.currentLatitude()->rawValue().toDouble(), 37.7749);
    QCOMPARE(fg.currentLongitude()->rawValue().toDouble(), -122.4194);
    QVERIFY(qAbs(fg.currentAltitude()->rawValue().toDouble() - 10.2) < 1e-6);
    QCOMPARE(fg.valid()->rawValue().toBool(), true);
    QCOMPARE(fg.active()->rawValue().toBool(), false);
    QCOMPARE(fg.connected()->rawValue().toBool(), true);
}

void GPSRTKFactGroupTest::testResetToDefaults()
{
    GPSRTKFactGroup fg;

    // Set non-default values
    fg.connected()->setRawValue(true);
    fg.active()->setRawValue(true);
    fg.valid()->setRawValue(true);
    fg.numSatellites()->setRawValue(12);
    fg.currentDuration()->setRawValue(300.0);
    fg.currentAccuracy()->setRawValue(0.01);
    fg.currentLatitude()->setRawValue(37.7749);
    fg.currentLongitude()->setRawValue(-122.4194);
    fg.currentAltitude()->setRawValue(10.5);
    fg.baseLatitude()->setRawValue(47.0);
    fg.baseLongitude()->setRawValue(8.0);
    fg.baseAltitude()->setRawValue(500.0);
    fg.baseFixType()->setRawValue(6);
    fg.heading()->setRawValue(90.0);
    fg.headingValid()->setRawValue(true);
    fg.baselineLength()->setRawValue(1.5);
    fg.carrierFixed()->setRawValue(true);

    fg.resetToDefaults();

    QCOMPARE(fg.connected()->rawValue().toBool(), false);
    QCOMPARE(fg.active()->rawValue().toBool(), false);
    QCOMPARE(fg.valid()->rawValue().toBool(), false);
    QCOMPARE(fg.numSatellites()->rawValue().toInt(), 0);
    QCOMPARE(fg.currentDuration()->rawValue().toDouble(), 0.0);
    QVERIFY(qIsNaN(fg.currentAccuracy()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.currentLatitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.currentLongitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.currentAltitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.baseLatitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.baseLongitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(fg.baseAltitude()->rawValue().toDouble()));
    QCOMPARE(fg.baseFixType()->rawValue().toInt(), 0);
    QVERIFY(qIsNaN(fg.heading()->rawValue().toDouble()));
    QCOMPARE(fg.headingValid()->rawValue().toBool(), false);
    QVERIFY(qIsNaN(fg.baselineLength()->rawValue().toDouble()));
    QCOMPARE(fg.carrierFixed()->rawValue().toBool(), false);
}

UT_REGISTER_TEST(GPSRTKFactGroupTest, TestLabel::Unit)
