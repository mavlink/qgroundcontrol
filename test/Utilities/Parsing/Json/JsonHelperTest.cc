#include "JsonHelperTest.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtPositioning/QGeoCoordinate>

#include "JsonHelper.h"

void JsonHelperTest::_saveAndValidateExternalHeader_test()
{
    QJsonObject obj;
    JsonHelper::saveQGCJsonFileHeader(obj, "TestFile", 2);

    QCOMPARE(obj[JsonHelper::jsonFileTypeKey].toString(), QStringLiteral("TestFile"));
    QCOMPARE(obj[JsonHelper::jsonVersionKey].toInt(), 2);
    QVERIFY(obj.contains("groundStation"));

    int version = 0;
    QString errorString;
    QVERIFY(JsonHelper::validateExternalQGCJsonFile(obj, "TestFile", 1, 3, version, errorString));
    QCOMPARE(version, 2);
    QVERIFY(errorString.isEmpty());
}

void JsonHelperTest::_validateInternalQGCJsonFile_test()
{
    QJsonObject obj;
    obj[JsonHelper::jsonFileTypeKey] = "TestType";
    obj[JsonHelper::jsonVersionKey] = 2;

    int version = 0;
    QString errorString;
    QVERIFY(JsonHelper::validateInternalQGCJsonFile(obj, "TestType", 1, 3, version, errorString));
    QCOMPARE(version, 2);
    QVERIFY(errorString.isEmpty());
}

void JsonHelperTest::_validateInternalVersionTooOld_test()
{
    QJsonObject obj;
    obj[JsonHelper::jsonFileTypeKey] = "TestType";
    obj[JsonHelper::jsonVersionKey] = 1;

    int version = 0;
    QString errorString;
    QVERIFY(!JsonHelper::validateInternalQGCJsonFile(obj, "TestType", 2, 3, version, errorString));
    QVERIFY(!errorString.isEmpty());
}

void JsonHelperTest::_validateInternalVersionTooNew_test()
{
    QJsonObject obj;
    obj[JsonHelper::jsonFileTypeKey] = "TestType";
    obj[JsonHelper::jsonVersionKey] = 5;

    int version = 0;
    QString errorString;
    QVERIFY(!JsonHelper::validateInternalQGCJsonFile(obj, "TestType", 1, 3, version, errorString));
    QVERIFY(!errorString.isEmpty());
}

void JsonHelperTest::_validateInternalWrongFileType_test()
{
    QJsonObject obj;
    obj[JsonHelper::jsonFileTypeKey] = "WrongType";
    obj[JsonHelper::jsonVersionKey] = 2;

    int version = 0;
    QString errorString;
    QVERIFY(!JsonHelper::validateInternalQGCJsonFile(obj, "TestType", 1, 3, version, errorString));
    QVERIFY(errorString.contains("Incorrect file type"));
}

void JsonHelperTest::_validateInternalMissingKeys_test()
{
    QJsonObject obj;
    // Missing both fileType and version keys

    int version = 0;
    QString errorString;
    QVERIFY(!JsonHelper::validateInternalQGCJsonFile(obj, "TestType", 1, 3, version, errorString));
    QVERIFY(!errorString.isEmpty());
}

void JsonHelperTest::_validateKeysRequired_test()
{
    const QJsonObject obj = {
        {"name", "test"},
        {"count", 42},
    };

    const QList<JsonHelper::KeyValidateInfo> keyInfo = {
        {"name", QJsonValue::String, true},
        {"count", QJsonValue::Double, true},
    };

    QString errorString;
    QVERIFY(JsonHelper::validateKeys(obj, keyInfo, errorString));
    QVERIFY(errorString.isEmpty());
}

void JsonHelperTest::_validateKeysOptional_test()
{
    const QJsonObject obj = {
        {"name", "test"},
    };

    const QList<JsonHelper::KeyValidateInfo> keyInfo = {
        {"name", QJsonValue::String, true},
        {"optional", QJsonValue::Double, false},
    };

    QString errorString;
    QVERIFY(JsonHelper::validateKeys(obj, keyInfo, errorString));
    QVERIFY(errorString.isEmpty());
}

void JsonHelperTest::_validateKeysWrongType_test()
{
    const QJsonObject obj = {
        {"name", 123},
    };

    const QList<JsonHelper::KeyValidateInfo> keyInfo = {
        {"name", QJsonValue::String, true},
    };

    QString errorString;
    QVERIFY(!JsonHelper::validateKeys(obj, keyInfo, errorString));
    QVERIFY(!errorString.isEmpty());
}

void JsonHelperTest::_loadSaveGeoCoordinate_test()
{
    const QGeoCoordinate original(47.3764, 8.5481);

    QJsonValue jsonValue;
    JsonHelper::saveGeoCoordinate(original, false, jsonValue);
    QVERIFY(jsonValue.isArray());

    QGeoCoordinate loaded;
    QString errorString;
    QVERIFY(JsonHelper::loadGeoCoordinate(jsonValue, false, loaded, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE_FUZZY(loaded.latitude(), original.latitude(), 1e-7);
    QCOMPARE_FUZZY(loaded.longitude(), original.longitude(), 1e-7);
}

void JsonHelperTest::_loadSaveGeoCoordinateWithAltitude_test()
{
    const QGeoCoordinate original(47.3764, 8.5481, 500.0);

    QJsonValue jsonValue;
    JsonHelper::saveGeoCoordinate(original, true, jsonValue);

    QGeoCoordinate loaded;
    QString errorString;
    QVERIFY(JsonHelper::loadGeoCoordinate(jsonValue, true, loaded, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE_FUZZY(loaded.latitude(), original.latitude(), 1e-7);
    QCOMPARE_FUZZY(loaded.longitude(), original.longitude(), 1e-7);
    QCOMPARE_FUZZY(loaded.altitude(), original.altitude(), 1e-7);
}

void JsonHelperTest::_loadSaveGeoCoordinateGeoJson_test()
{
    const QGeoCoordinate original(47.3764, 8.5481);

    QJsonValue jsonValue;
    JsonHelper::saveGeoCoordinate(original, false, jsonValue, true);
    QVERIFY(jsonValue.isArray());

    // GeoJSON format: [lon, lat]
    const QJsonArray arr = jsonValue.toArray();
    QCOMPARE(arr.count(), 2);
    QCOMPARE_FUZZY(arr[0].toDouble(), original.longitude(), 1e-7);
    QCOMPARE_FUZZY(arr[1].toDouble(), original.latitude(), 1e-7);

    QGeoCoordinate loaded;
    QString errorString;
    QVERIFY(JsonHelper::loadGeoCoordinate(jsonValue, false, loaded, errorString, true));
    QVERIFY(errorString.isEmpty());
    QCOMPARE_FUZZY(loaded.latitude(), original.latitude(), 1e-7);
    QCOMPARE_FUZZY(loaded.longitude(), original.longitude(), 1e-7);
}

void JsonHelperTest::_loadGeoCoordinateInvalidArray_test()
{
    QGeoCoordinate loaded;
    QString errorString;

    // Not an array
    QVERIFY(!JsonHelper::loadGeoCoordinate(QJsonValue("not array"), false, loaded, errorString));
    QVERIFY(!errorString.isEmpty());

    // Wrong count (3 values when altitude not required = expects 2)
    errorString.clear();
    QJsonArray arr;
    arr << 1.0 << 2.0 << 3.0;
    QVERIFY(!JsonHelper::loadGeoCoordinate(QJsonValue(arr), false, loaded, errorString));
    QVERIFY(!errorString.isEmpty());
}

void JsonHelperTest::_loadSaveGeoCoordinateArray_test()
{
    const QList<QGeoCoordinate> originals = {
        QGeoCoordinate(47.3764, 8.5481, 100.0),
        QGeoCoordinate(47.3800, 8.5500, 200.0),
        QGeoCoordinate(47.3850, 8.5550, 300.0),
    };

    QVariantList varPoints;
    for (const auto &coord : originals) {
        varPoints.append(QVariant::fromValue(coord));
    }

    QJsonValue jsonValue;
    JsonHelper::saveGeoCoordinateArray(varPoints, true, jsonValue);
    QVERIFY(jsonValue.isArray());

    QVariantList loadedVarPoints;
    QString errorString;
    QVERIFY(JsonHelper::loadGeoCoordinateArray(jsonValue, true, loadedVarPoints, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(loadedVarPoints.count(), originals.count());

    for (int i = 0; i < originals.count(); ++i) {
        const auto loaded = loadedVarPoints[i].value<QGeoCoordinate>();
        QCOMPARE_FUZZY(loaded.latitude(), originals[i].latitude(), 1e-7);
        QCOMPARE_FUZZY(loaded.longitude(), originals[i].longitude(), 1e-7);
        QCOMPARE_FUZZY(loaded.altitude(), originals[i].altitude(), 1e-7);
    }
}

void JsonHelperTest::_loadSaveGeoCoordinateArrayQList_test()
{
    const QList<QGeoCoordinate> originals = {
        QGeoCoordinate(47.3764, 8.5481),
        QGeoCoordinate(47.3800, 8.5500),
    };

    QJsonValue jsonValue;
    JsonHelper::saveGeoCoordinateArray(originals, false, jsonValue);
    QVERIFY(jsonValue.isArray());

    QList<QGeoCoordinate> loadedPoints;
    QString errorString;
    QVERIFY(JsonHelper::loadGeoCoordinateArray(jsonValue, false, loadedPoints, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(loadedPoints.count(), originals.count());

    for (int i = 0; i < originals.count(); ++i) {
        QCOMPARE_FUZZY(loadedPoints[i].latitude(), originals[i].latitude(), 1e-7);
        QCOMPARE_FUZZY(loadedPoints[i].longitude(), originals[i].longitude(), 1e-7);
    }
}

UT_REGISTER_TEST(JsonHelperTest, TestLabel::Unit, TestLabel::Utilities)
