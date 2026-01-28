#include "JsonHelperTest.h"
#include "JsonHelper.h"
#include "QtTestExtensions.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QTemporaryFile>
#include <QtTest/QTest>

// ============================================================================
// JSON Parsing Tests
// ============================================================================

void JsonHelperTest::_isJsonFileValidTest()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write(R"({"key": "value", "number": 42})");
    tempFile.close();

    QJsonDocument jsonDoc;
    QString errorString;
    QVERIFY(JsonHelper::isJsonFile(tempFile.fileName(), jsonDoc, errorString));
    QGC_VERIFY_EMPTY(errorString);
    QVERIFY(!jsonDoc.isEmpty());
    QCOMPARE_EQ(jsonDoc.object()["key"].toString(), QStringLiteral("value"));
    QCOMPARE_EQ(jsonDoc.object()["number"].toInt(), 42);
}

void JsonHelperTest::_isJsonFileInvalidTest()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write("not valid json {{{");
    tempFile.close();

    QJsonDocument jsonDoc;
    QString errorString;
    QVERIFY(!JsonHelper::isJsonFile(tempFile.fileName(), jsonDoc, errorString));
    QGC_VERIFY_NOT_EMPTY(errorString);
}

void JsonHelperTest::_isJsonFileBytesTest()
{
    const QByteArray validJson = R"({"test": true})";
    QJsonDocument jsonDoc;
    QString errorString;

    QVERIFY(JsonHelper::isJsonFile(validJson, jsonDoc, errorString));
    QGC_VERIFY_EMPTY(errorString);
    QVERIFY(jsonDoc.object()["test"].toBool());

    const QByteArray invalidJson = "not json";
    QVERIFY(!JsonHelper::isJsonFile(invalidJson, jsonDoc, errorString));
    QGC_VERIFY_NOT_EMPTY(errorString);
}

// ============================================================================
// Header Validation Tests
// ============================================================================

void JsonHelperTest::_saveAndValidateHeaderTest()
{
    QJsonObject jsonObject;
    JsonHelper::saveQGCJsonFileHeader(jsonObject, "TestFileType", 2);

    QVERIFY(jsonObject.contains(JsonHelper::jsonFileTypeKey));
    QVERIFY(jsonObject.contains(JsonHelper::jsonVersionKey));
    QCOMPARE_EQ(jsonObject[JsonHelper::jsonFileTypeKey].toString(), QStringLiteral("TestFileType"));
    QCOMPARE_EQ(jsonObject[JsonHelper::jsonVersionKey].toInt(), 2);
}

void JsonHelperTest::_validateExternalQGCJsonFileTest()
{
    QJsonObject jsonObject;
    JsonHelper::saveQGCJsonFileHeader(jsonObject, "TestType", 1);
    jsonObject["groundStation"] = "QGroundControl";

    int version = 0;
    QString errorString;

    // Valid case
    QVERIFY(JsonHelper::validateExternalQGCJsonFile(jsonObject, "TestType", 1, 2, version, errorString));
    QCOMPARE_EQ(version, 1);
    QGC_VERIFY_EMPTY(errorString);

    // Wrong file type
    QVERIFY(!JsonHelper::validateExternalQGCJsonFile(jsonObject, "WrongType", 1, 2, version, errorString));
    QGC_VERIFY_NOT_EMPTY(errorString);
}

void JsonHelperTest::_validateInternalQGCJsonFileTest()
{
    QJsonObject jsonObject;
    JsonHelper::saveQGCJsonFileHeader(jsonObject, "InternalType", 3);

    int version = 0;
    QString errorString;

    // Valid case
    QVERIFY(JsonHelper::validateInternalQGCJsonFile(jsonObject, "InternalType", 1, 5, version, errorString));
    QCOMPARE_EQ(version, 3);
    QGC_VERIFY_EMPTY(errorString);

    // Version too high
    QVERIFY(!JsonHelper::validateInternalQGCJsonFile(jsonObject, "InternalType", 1, 2, version, errorString));
    QGC_VERIFY_NOT_EMPTY(errorString);
}

void JsonHelperTest::_validateVersionRangeTest()
{
    QJsonObject jsonObject;
    JsonHelper::saveQGCJsonFileHeader(jsonObject, "VersionTest", 5);

    int version = 0;
    QString errorString;

    // Version within range
    QVERIFY(JsonHelper::validateInternalQGCJsonFile(jsonObject, "VersionTest", 3, 7, version, errorString));

    // Version below minimum
    QVERIFY(!JsonHelper::validateInternalQGCJsonFile(jsonObject, "VersionTest", 6, 10, version, errorString));
    QGC_VERIFY_NOT_EMPTY(errorString);

    // Version above maximum
    errorString.clear();
    QVERIFY(!JsonHelper::validateInternalQGCJsonFile(jsonObject, "VersionTest", 1, 4, version, errorString));
    QGC_VERIFY_NOT_EMPTY(errorString);
}

// ============================================================================
// Key Validation Tests
// ============================================================================

void JsonHelperTest::_validateRequiredKeysTest()
{
    QJsonObject jsonObject;
    jsonObject["key1"] = "value1";
    jsonObject["key2"] = 42;
    jsonObject["key3"] = true;

    QString errorString;

    // All keys present
    QVERIFY(JsonHelper::validateRequiredKeys(jsonObject, {"key1", "key2"}, errorString));
    QGC_VERIFY_EMPTY(errorString);

    // Missing key
    QVERIFY(!JsonHelper::validateRequiredKeys(jsonObject, {"key1", "missingKey"}, errorString));
    QGC_VERIFY_NOT_EMPTY(errorString);
}

void JsonHelperTest::_validateKeyTypesTest()
{
    QJsonObject jsonObject;
    jsonObject["stringKey"] = "hello";
    jsonObject["numberKey"] = 3.14;
    jsonObject["boolKey"] = true;
    jsonObject["arrayKey"] = QJsonArray{1, 2, 3};

    QString errorString;

    // Correct types
    QVERIFY(JsonHelper::validateKeyTypes(
        jsonObject,
        {"stringKey", "numberKey", "boolKey", "arrayKey"},
        {QJsonValue::String, QJsonValue::Double, QJsonValue::Bool, QJsonValue::Array},
        errorString));
    QGC_VERIFY_EMPTY(errorString);

    // Wrong type
    QVERIFY(!JsonHelper::validateKeyTypes(
        jsonObject,
        {"stringKey"},
        {QJsonValue::Double},  // stringKey is not a double
        errorString));
    QGC_VERIFY_NOT_EMPTY(errorString);
}

void JsonHelperTest::_validateKeysTest()
{
    QJsonObject jsonObject;
    jsonObject["required1"] = "value";
    jsonObject["required2"] = 100;
    jsonObject["optional1"] = true;

    QString errorString;

    QList<JsonHelper::KeyValidateInfo> keyInfo = {
        {"required1", QJsonValue::String, true},
        {"required2", QJsonValue::Double, true},
        {"optional1", QJsonValue::Bool, false},
        {"optional2", QJsonValue::String, false},  // not present but optional
    };

    QVERIFY(JsonHelper::validateKeys(jsonObject, keyInfo, errorString));
    QGC_VERIFY_EMPTY(errorString);

    // Missing required key
    QList<JsonHelper::KeyValidateInfo> keyInfoMissing = {
        {"required1", QJsonValue::String, true},
        {"missingRequired", QJsonValue::String, true},
    };

    QVERIFY(!JsonHelper::validateKeys(jsonObject, keyInfoMissing, errorString));
    QGC_VERIFY_NOT_EMPTY(errorString);
}

// ============================================================================
// GeoCoordinate Tests
// ============================================================================

void JsonHelperTest::_loadGeoCoordinateTest()
{
    // Standard format: [lat, lon, alt]
    QJsonArray coordArray;
    coordArray.append(47.3977);
    coordArray.append(8.5456);
    coordArray.append(500.0);

    QGeoCoordinate coord;
    QString errorString;

    QVERIFY(JsonHelper::loadGeoCoordinate(QJsonValue(coordArray), true, coord, errorString));
    QGC_VERIFY_EMPTY(errorString);
    QCOMPARE_EQ(coord.latitude(), 47.3977);
    QCOMPARE_EQ(coord.longitude(), 8.5456);
    QCOMPARE_EQ(coord.altitude(), 500.0);
}

void JsonHelperTest::_loadGeoCoordinateNoAltitudeTest()
{
    // No altitude: [lat, lon]
    QJsonArray coordArray;
    coordArray.append(51.5074);
    coordArray.append(-0.1278);

    QGeoCoordinate coord;
    QString errorString;

    // Altitude not required
    QVERIFY(JsonHelper::loadGeoCoordinate(QJsonValue(coordArray), false, coord, errorString));
    QGC_VERIFY_EMPTY(errorString);
    QCOMPARE_EQ(coord.latitude(), 51.5074);
    QCOMPARE_EQ(coord.longitude(), -0.1278);

    // Altitude required but not present
    QVERIFY(!JsonHelper::loadGeoCoordinate(QJsonValue(coordArray), true, coord, errorString));
    QGC_VERIFY_NOT_EMPTY(errorString);
}

void JsonHelperTest::_loadGeoCoordinateGeoJsonFormatTest()
{
    // GeoJSON format: [lon, lat, alt]
    QJsonArray coordArray;
    coordArray.append(-122.4194);  // lon first in GeoJSON
    coordArray.append(37.7749);    // lat second
    coordArray.append(100.0);

    QGeoCoordinate coord;
    QString errorString;

    QVERIFY(JsonHelper::loadGeoCoordinate(QJsonValue(coordArray), true, coord, errorString, true));
    QGC_VERIFY_EMPTY(errorString);
    QCOMPARE_EQ(coord.latitude(), 37.7749);
    QCOMPARE_EQ(coord.longitude(), -122.4194);
    QCOMPARE_EQ(coord.altitude(), 100.0);
}

void JsonHelperTest::_saveGeoCoordinateTest()
{
    QGeoCoordinate coord(48.8566, 2.3522, 35.0);
    QJsonValue jsonValue;

    // With altitude
    JsonHelper::saveGeoCoordinate(coord, true, jsonValue);
    QVERIFY(jsonValue.isArray());
    QJsonArray arr = jsonValue.toArray();
    QCOMPARE_EQ(arr.size(), 3);
    QCOMPARE_EQ(arr[0].toDouble(), 48.8566);
    QCOMPARE_EQ(arr[1].toDouble(), 2.3522);
    QCOMPARE_EQ(arr[2].toDouble(), 35.0);

    // Without altitude
    JsonHelper::saveGeoCoordinate(coord, false, jsonValue);
    arr = jsonValue.toArray();
    QCOMPARE_EQ(arr.size(), 2);
}

void JsonHelperTest::_loadGeoCoordinateArrayTest()
{
    QJsonArray pointsArray;

    QJsonArray point1;
    point1.append(10.0);
    point1.append(20.0);
    point1.append(100.0);
    pointsArray.append(point1);

    QJsonArray point2;
    point2.append(11.0);
    point2.append(21.0);
    point2.append(200.0);
    pointsArray.append(point2);

    QList<QGeoCoordinate> coords;
    QString errorString;

    QVERIFY(JsonHelper::loadGeoCoordinateArray(QJsonValue(pointsArray), true, coords, errorString));
    QGC_VERIFY_EMPTY(errorString);
    QCOMPARE_EQ(coords.size(), 2);
    QCOMPARE_EQ(coords[0].latitude(), 10.0);
    QCOMPARE_EQ(coords[0].longitude(), 20.0);
    QCOMPARE_EQ(coords[1].latitude(), 11.0);
    QCOMPARE_EQ(coords[1].altitude(), 200.0);
}

void JsonHelperTest::_saveGeoCoordinateArrayTest()
{
    QList<QGeoCoordinate> coords;
    coords.append(QGeoCoordinate(1.0, 2.0, 3.0));
    coords.append(QGeoCoordinate(4.0, 5.0, 6.0));

    QJsonValue jsonValue;
    JsonHelper::saveGeoCoordinateArray(coords, true, jsonValue);

    QVERIFY(jsonValue.isArray());
    QJsonArray arr = jsonValue.toArray();
    QCOMPARE_EQ(arr.size(), 2);
    QCOMPARE_EQ(arr[0].toArray()[0].toDouble(), 1.0);
    QCOMPARE_EQ(arr[1].toArray()[2].toDouble(), 6.0);
}

// ============================================================================
// Utility Tests
// ============================================================================

void JsonHelperTest::_possibleNaNJsonValueTest()
{
    // Null value should return NaN
    QJsonValue nullValue;
    QVERIFY(qIsNaN(JsonHelper::possibleNaNJsonValue(nullValue)));

    // Regular double should return value
    QJsonValue doubleValue(42.5);
    QCOMPARE_EQ(JsonHelper::possibleNaNJsonValue(doubleValue), 42.5);

    // Zero should return zero
    QJsonValue zeroValue(0.0);
    QCOMPARE_EQ(JsonHelper::possibleNaNJsonValue(zeroValue), 0.0);
}
