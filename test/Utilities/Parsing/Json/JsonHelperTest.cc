#include "JsonHelperTest.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtPositioning/QGeoCoordinate>

#include "GeoJsonHelper.h"
#include "JsonParsing.h"

void JsonHelperTest::_saveAndValidateExternalHeader_test()
{
    QJsonObject obj;
    JsonParsing::saveQGCJsonFileHeader(obj, "TestFile", 2);

    QCOMPARE(obj[JsonParsing::jsonFileTypeKey].toString(), QStringLiteral("TestFile"));
    QCOMPARE(obj[JsonParsing::jsonVersionKey].toInt(), 2);
    QVERIFY(obj.contains("groundStation"));

    int version = 0;
    QString errorString;
    QVERIFY(JsonParsing::validateExternalQGCJsonFile(obj, "TestFile", 1, 3, version, errorString));
    QCOMPARE(version, 2);
    QVERIFY(errorString.isEmpty());
}

void JsonHelperTest::_validateInternalQGCJsonFile_test()
{
    QJsonObject obj;
    obj[JsonParsing::jsonFileTypeKey] = "TestType";
    obj[JsonParsing::jsonVersionKey] = 2;

    int version = 0;
    QString errorString;
    QVERIFY(JsonParsing::validateInternalQGCJsonFile(obj, "TestType", 1, 3, version, errorString));
    QCOMPARE(version, 2);
    QVERIFY(errorString.isEmpty());
}

void JsonHelperTest::_validateInternalVersionTooOld_test()
{
    QJsonObject obj;
    obj[JsonParsing::jsonFileTypeKey] = "TestType";
    obj[JsonParsing::jsonVersionKey] = 1;

    int version = 0;
    QString errorString;
    QVERIFY(!JsonParsing::validateInternalQGCJsonFile(obj, "TestType", 2, 3, version, errorString));
    QVERIFY(!errorString.isEmpty());
}

void JsonHelperTest::_validateInternalVersionTooNew_test()
{
    QJsonObject obj;
    obj[JsonParsing::jsonFileTypeKey] = "TestType";
    obj[JsonParsing::jsonVersionKey] = 5;

    int version = 0;
    QString errorString;
    QVERIFY(!JsonParsing::validateInternalQGCJsonFile(obj, "TestType", 1, 3, version, errorString));
    QVERIFY(!errorString.isEmpty());
}

void JsonHelperTest::_validateInternalWrongFileType_test()
{
    QJsonObject obj;
    obj[JsonParsing::jsonFileTypeKey] = "WrongType";
    obj[JsonParsing::jsonVersionKey] = 2;

    int version = 0;
    QString errorString;
    QVERIFY(!JsonParsing::validateInternalQGCJsonFile(obj, "TestType", 1, 3, version, errorString));
    QVERIFY(errorString.contains("Incorrect file type"));
}

void JsonHelperTest::_validateInternalMissingKeys_test()
{
    QJsonObject obj;
    // Missing both fileType and version keys

    int version = 0;
    QString errorString;
    QVERIFY(!JsonParsing::validateInternalQGCJsonFile(obj, "TestType", 1, 3, version, errorString));
    QVERIFY(!errorString.isEmpty());
}

void JsonHelperTest::_validateKeysRequired_test()
{
    const QJsonObject obj = {
        {"name", "test"},
        {"count", 42},
    };

    const QList<JsonParsing::KeyValidateInfo> keyInfo = {
        {"name", QJsonValue::String, true},
        {"count", QJsonValue::Double, true},
    };

    QString errorString;
    QVERIFY(JsonParsing::validateKeys(obj, keyInfo, errorString));
    QVERIFY(errorString.isEmpty());
}

void JsonHelperTest::_validateKeysOptional_test()
{
    const QJsonObject obj = {
        {"name", "test"},
    };

    const QList<JsonParsing::KeyValidateInfo> keyInfo = {
        {"name", QJsonValue::String, true},
        {"optional", QJsonValue::Double, false},
    };

    QString errorString;
    QVERIFY(JsonParsing::validateKeys(obj, keyInfo, errorString));
    QVERIFY(errorString.isEmpty());
}

void JsonHelperTest::_validateKeysWrongType_test()
{
    const QJsonObject obj = {
        {"name", 123},
    };

    const QList<JsonParsing::KeyValidateInfo> keyInfo = {
        {"name", QJsonValue::String, true},
    };

    QString errorString;
    QVERIFY(!JsonParsing::validateKeys(obj, keyInfo, errorString));
    QVERIFY(!errorString.isEmpty());
}

void JsonHelperTest::_validateKeysStrictValid_test()
{
    const QJsonObject obj = {
        {"name", "test"},
        {"count", 42},
    };

    const QList<JsonParsing::KeyValidateInfo> keyInfo = {
        {"name", QJsonValue::String, true},
        {"count", QJsonValue::Double, true},
    };

    QString errorString;
    QVERIFY(JsonParsing::validateKeysStrict(obj, keyInfo, errorString));
    QVERIFY(errorString.isEmpty());
}

void JsonHelperTest::_validateKeysStrictUnknownKey_test()
{
    const QJsonObject obj = {
        {"name", "test"},
        {"count", 42},
        {"extra", true},
    };

    const QList<JsonParsing::KeyValidateInfo> keyInfo = {
        {"name", QJsonValue::String, true},
        {"count", QJsonValue::Double, true},
    };

    QString errorString;
    QVERIFY(!JsonParsing::validateKeysStrict(obj, keyInfo, errorString));
    QVERIFY(errorString.contains("Unknown key"));
    QVERIFY(errorString.contains("extra"));
}

void JsonHelperTest::_validateKeysStrictMissingRequired_test()
{
    const QJsonObject obj = {
        {"name", "test"},
    };

    const QList<JsonParsing::KeyValidateInfo> keyInfo = {
        {"name", QJsonValue::String, true},
        {"count", QJsonValue::Double, true},
    };

    QString errorString;
    QVERIFY(!JsonParsing::validateKeysStrict(obj, keyInfo, errorString));
    QVERIFY(!errorString.isEmpty());
}

void JsonHelperTest::_loadSaveGeoCoordinate_test()
{
    const QGeoCoordinate original(47.3764, 8.5481);

    QJsonValue jsonValue;
    GeoJsonHelper::saveGeoCoordinate(original, false, jsonValue);
    QVERIFY(jsonValue.isArray());

    QGeoCoordinate loaded;
    QString errorString;
    QVERIFY(GeoJsonHelper::loadGeoCoordinate(jsonValue, false, loaded, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE_FUZZY(loaded.latitude(), original.latitude(), 1e-7);
    QCOMPARE_FUZZY(loaded.longitude(), original.longitude(), 1e-7);
}

void JsonHelperTest::_loadSaveGeoCoordinateWithAltitude_test()
{
    const QGeoCoordinate original(47.3764, 8.5481, 500.0);

    QJsonValue jsonValue;
    GeoJsonHelper::saveGeoCoordinate(original, true, jsonValue);

    QGeoCoordinate loaded;
    QString errorString;
    QVERIFY(GeoJsonHelper::loadGeoCoordinate(jsonValue, true, loaded, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE_FUZZY(loaded.latitude(), original.latitude(), 1e-7);
    QCOMPARE_FUZZY(loaded.longitude(), original.longitude(), 1e-7);
    QCOMPARE_FUZZY(loaded.altitude(), original.altitude(), 1e-7);
}

void JsonHelperTest::_loadGeoCoordinateInvalidArray_test()
{
    QGeoCoordinate loaded;
    QString errorString;

    // Not an array
    QVERIFY(!GeoJsonHelper::loadGeoCoordinate(QJsonValue("not array"), false, loaded, errorString));
    QVERIFY(!errorString.isEmpty());

    // Wrong count (3 values when altitude not required = expects 2)
    errorString.clear();
    QJsonArray arr;
    arr << 1.0 << 2.0 << 3.0;
    QVERIFY(!GeoJsonHelper::loadGeoCoordinate(QJsonValue(arr), false, loaded, errorString));
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
    GeoJsonHelper::saveGeoCoordinateArray(varPoints, true, jsonValue);
    QVERIFY(jsonValue.isArray());

    QVariantList loadedVarPoints;
    QString errorString;
    QVERIFY(GeoJsonHelper::loadGeoCoordinateArray(jsonValue, true, loadedVarPoints, errorString));
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
    GeoJsonHelper::saveGeoCoordinateArray(originals, false, jsonValue);
    QVERIFY(jsonValue.isArray());

    QList<QGeoCoordinate> loadedPoints;
    QString errorString;
    QVERIFY(GeoJsonHelper::loadGeoCoordinateArray(jsonValue, false, loadedPoints, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(loadedPoints.count(), originals.count());

    for (int i = 0; i < originals.count(); ++i) {
        QCOMPARE_FUZZY(loadedPoints[i].latitude(), originals[i].latitude(), 1e-7);
        QCOMPARE_FUZZY(loadedPoints[i].longitude(), originals[i].longitude(), 1e-7);
    }
}

UT_REGISTER_TEST(JsonHelperTest, TestLabel::Unit, TestLabel::Utilities)
