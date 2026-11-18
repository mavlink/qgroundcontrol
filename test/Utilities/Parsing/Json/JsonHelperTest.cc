#include "JsonHelperTest.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtPositioning/QGeoCoordinate>

#include "GeoJsonHelper.h"
#include "JsonParsing.h"
#include "UnitTestMath.h"

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

void JsonHelperTest::_validateInternalQGCJsonFile_test_data()
{
    QTest::addColumn<QJsonObject>("object");
    QTest::addColumn<int>("minVersion");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QString>("errorPart");

    const auto header = [](const char* fileType, int version) {
        return QJsonObject{{JsonParsing::jsonFileTypeKey, fileType}, {JsonParsing::jsonVersionKey, version}};
    };
    QTest::newRow("supported-version") << header("TestType", 2) << 1 << true << QString();
    QTest::newRow("version-too-old") << header("TestType", 1) << 2 << false << QString();
    QTest::newRow("version-too-new") << header("TestType", 5) << 1 << false << QString();
    QTest::newRow("wrong-file-type") << header("WrongType", 2) << 1 << false << QStringLiteral("Incorrect file type");
    // Neither the file type nor the version
    QTest::newRow("missing-keys") << QJsonObject() << 1 << false << QString();
}

void JsonHelperTest::_validateInternalQGCJsonFile_test()
{
    QFETCH(QJsonObject, object);
    QFETCH(int, minVersion);
    QFETCH(bool, valid);
    QFETCH(QString, errorPart);

    int version = 0;
    QString errorString;
    QCOMPARE(JsonParsing::validateInternalQGCJsonFile(object, "TestType", minVersion, 3, version, errorString), valid);
    QCOMPARE(errorString.isEmpty(), valid);
    QVERIFY2(errorString.contains(errorPart), qPrintable(errorString));
    if (valid) {
        QCOMPARE(version, 2);
    }
}

void JsonHelperTest::_validateKeys_test_data()
{
    using KeyInfo = QList<JsonParsing::KeyValidateInfo>;

    QTest::addColumn<QJsonObject>("object");
    QTest::addColumn<KeyInfo>("keyInfo");
    QTest::addColumn<bool>("strict");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QString>("errorPart");

    const QJsonObject name{{"name", "test"}};
    const QJsonObject nameAndCount{{"name", "test"}, {"count", 42}};
    const KeyInfo requiredNameAndCount{{"name", QJsonValue::String, true}, {"count", QJsonValue::Double, true}};

    QTest::newRow("required-keys") << nameAndCount << requiredNameAndCount << false << true << QString();
    QTest::newRow("optional-key-absent") << name
                                         << KeyInfo{{"name", QJsonValue::String, true},
                                                    {"optional", QJsonValue::Double, false}}
                                         << false << true << QString();
    QTest::newRow("wrong-type") << QJsonObject{{"name", 123}} << KeyInfo{{"name", QJsonValue::String, true}} << false
                                << false << QString();
    QTest::newRow("strict-required-keys") << nameAndCount << requiredNameAndCount << true << true << QString();
    QTest::newRow("strict-unknown-key") << QJsonObject{{"name", "test"}, {"count", 42}, {"extra", true}}
                                        << requiredNameAndCount << true << false
                                        << QStringLiteral("Unknown key: extra");
    QTest::newRow("strict-required-key-absent") << name << requiredNameAndCount << true << false << QString();
}

void JsonHelperTest::_validateKeys_test()
{
    QFETCH(QJsonObject, object);
    QFETCH(QList<JsonParsing::KeyValidateInfo>, keyInfo);
    QFETCH(bool, strict);
    QFETCH(bool, valid);
    QFETCH(QString, errorPart);

    QString errorString;
    QCOMPARE(strict ? JsonParsing::validateKeysStrict(object, keyInfo, errorString)
                    : JsonParsing::validateKeys(object, keyInfo, errorString),
             valid);
    QCOMPARE(errorString.isEmpty(), valid);
    QVERIFY2(errorString.contains(errorPart), qPrintable(errorString));
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

QGC_REGISTER_PORTABLE_TEST(JsonHelperTest, TestLabel::Unit, TestLabel::Utilities)
