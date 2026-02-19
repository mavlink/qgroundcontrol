#include "GeoTagDataTest.h"

#include <QtTest/QTest>

#include "GeoTagData.h"

void GeoTagDataTest::_captureResultEnumTest()
{
    // Verify enum values match PX4 camera_capture result field
    QCOMPARE(static_cast<int8_t>(GeoTagData::CaptureResult::NoFeedback), static_cast<int8_t>(-1));
    QCOMPARE(static_cast<int8_t>(GeoTagData::CaptureResult::Failure), static_cast<int8_t>(0));
    QCOMPARE(static_cast<int8_t>(GeoTagData::CaptureResult::Success), static_cast<int8_t>(1));
}

void GeoTagDataTest::_isValidTest()
{
    GeoTagData data;

    // Default state: invalid coordinate, no feedback
    QVERIFY(!data.isValid());

    // Valid coordinate but no success result
    data.coordinate = QGeoCoordinate(37.0, -122.0, 100.0);
    data.captureResult = GeoTagData::CaptureResult::NoFeedback;
    QVERIFY(!data.isValid());

    // Valid coordinate with failure result
    data.captureResult = GeoTagData::CaptureResult::Failure;
    QVERIFY(!data.isValid());

    // Valid coordinate with success result
    data.captureResult = GeoTagData::CaptureResult::Success;
    QVERIFY(data.isValid());

    // Invalid coordinate with success result
    data.coordinate = QGeoCoordinate();
    QVERIFY(!data.isValid());
}

void GeoTagDataTest::_defaultValuesTest()
{
    GeoTagData data;

    QCOMPARE(data.timestamp, 0.);
    QCOMPARE(data.timestampUTC, 0.);
    QCOMPARE(data.imageSequence, 0u);
    QVERIFY(!data.coordinate.isValid());
    QCOMPARE(data.groundDistance, 0.f);
    QCOMPARE(data.attitude, QQuaternion());
    QCOMPARE(data.captureResult, GeoTagData::CaptureResult::NoFeedback);
}

UT_REGISTER_TEST(GeoTagDataTest, TestLabel::Unit, TestLabel::AnalyzeView)
