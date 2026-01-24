#include "GeoJsonHelperTest.h"
#include "GeoJsonHelper.h"
#include "QtTestExtensions.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>
#include <QtTest/QTest>

void GeoJsonHelperTest::_loadGeoJsonCoordinateWithAltitudeTest()
{
    // GeoJSON format: [lon, lat, alt]
    QJsonArray coordArray;
    coordArray.append(-122.4194);  // longitude
    coordArray.append(37.7749);    // latitude
    coordArray.append(100.0);      // altitude

    QGeoCoordinate coord;
    QString errorString;

    QVERIFY(GeoJsonHelper::loadGeoJsonCoordinate(QJsonValue(coordArray), true, coord, errorString));
    QGC_VERIFY_EMPTY(errorString);
    QCOMPARE_EQ(coord.longitude(), -122.4194);
    QCOMPARE_EQ(coord.latitude(), 37.7749);
    QCOMPARE_EQ(coord.altitude(), 100.0);
}

void GeoJsonHelperTest::_loadGeoJsonCoordinateNoAltitudeTest()
{
    // GeoJSON format without altitude: [lon, lat]
    QJsonArray coordArray;
    coordArray.append(8.5456);   // longitude
    coordArray.append(47.3977);  // latitude

    QGeoCoordinate coord;
    QString errorString;

    // Altitude not required
    QVERIFY(GeoJsonHelper::loadGeoJsonCoordinate(QJsonValue(coordArray), false, coord, errorString));
    QGC_VERIFY_EMPTY(errorString);
    QCOMPARE_EQ(coord.longitude(), 8.5456);
    QCOMPARE_EQ(coord.latitude(), 47.3977);
}

void GeoJsonHelperTest::_loadGeoJsonCoordinateAltitudeRequiredTest()
{
    // GeoJSON format without altitude
    QJsonArray coordArray;
    coordArray.append(8.5456);
    coordArray.append(47.3977);

    QGeoCoordinate coord;
    QString errorString;

    // Altitude required but not present
    QVERIFY(!GeoJsonHelper::loadGeoJsonCoordinate(QJsonValue(coordArray), true, coord, errorString));
    QGC_VERIFY_NOT_EMPTY(errorString);
}

void GeoJsonHelperTest::_loadGeoJsonCoordinateInvalidTest()
{
    QGeoCoordinate coord;
    QString errorString;

    // Not an array
    QVERIFY(!GeoJsonHelper::loadGeoJsonCoordinate(QJsonValue("not an array"), false, coord, errorString));
    QGC_VERIFY_NOT_EMPTY(errorString);

    // Array with wrong number of elements
    errorString.clear();
    QJsonArray singleElement;
    singleElement.append(1.0);
    QVERIFY(!GeoJsonHelper::loadGeoJsonCoordinate(QJsonValue(singleElement), false, coord, errorString));
    QGC_VERIFY_NOT_EMPTY(errorString);
}

void GeoJsonHelperTest::_saveGeoJsonCoordinateWithAltitudeTest()
{
    QGeoCoordinate coord(48.8566, 2.3522, 35.0);  // Paris
    QJsonValue jsonValue;

    GeoJsonHelper::saveGeoJsonCoordinate(coord, true, jsonValue);

    QVERIFY(jsonValue.isArray());
    QJsonArray arr = jsonValue.toArray();
    QCOMPARE(arr.size(), 3);
    // GeoJSON: [lon, lat, alt]
    QCOMPARE_EQ(arr[0].toDouble(), 2.3522);   // longitude first
    QCOMPARE_EQ(arr[1].toDouble(), 48.8566);  // latitude second
    QCOMPARE_EQ(arr[2].toDouble(), 35.0);     // altitude third
}

void GeoJsonHelperTest::_saveGeoJsonCoordinateNoAltitudeTest()
{
    QGeoCoordinate coord(51.5074, -0.1278, 100.0);  // London
    QJsonValue jsonValue;

    GeoJsonHelper::saveGeoJsonCoordinate(coord, false, jsonValue);

    QVERIFY(jsonValue.isArray());
    QJsonArray arr = jsonValue.toArray();
    QCOMPARE(arr.size(), 2);
    // GeoJSON: [lon, lat]
    QCOMPARE_EQ(arr[0].toDouble(), -0.1278);  // longitude first
    QCOMPARE_EQ(arr[1].toDouble(), 51.5074);  // latitude second
}
