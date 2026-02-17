#include "JsonParsingTest.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/qnumeric.h>

#include "QGCFileHelper.h"
#include "JsonParsing.h"

void JsonParsingTest::_testValidateRequiredKeysSuccess()
{
    const QJsonObject object = {
        {"version", 1},
        {"name", "test"},
    };

    QString errorString;
    QVERIFY(JsonParsing::validateRequiredKeys(object, {"version", "name"}, errorString));
    QVERIFY(errorString.isEmpty());
}

void JsonParsingTest::_testValidateRequiredKeysMissing()
{
    const QJsonObject object = {
        {"version", 1},
    };

    QString errorString;
    QVERIFY(!JsonParsing::validateRequiredKeys(object, {"version", "name", "type"}, errorString));
    QVERIFY(errorString.contains(QStringLiteral("name")));
    QVERIFY(errorString.contains(QStringLiteral("type")));
}

void JsonParsingTest::_testValidateKeyTypesSuccess()
{
    const QJsonObject object = {
        {"count", 10},
        {"label", "abc"},
        {"enabled", true},
    };

    QString errorString;
    QVERIFY(JsonParsing::validateKeyTypes(object, {"count", "label", "enabled", "optional"},
                                             {QJsonValue::Double, QJsonValue::String, QJsonValue::Bool,
                                              QJsonValue::Array},
                                             errorString));
    QVERIFY(errorString.isEmpty());
}

void JsonParsingTest::_testValidateKeyTypesTypeMismatch()
{
    const QJsonObject object = {
        {"count", "ten"},
    };

    QString errorString;
    QVERIFY(!JsonParsing::validateKeyTypes(object, {"count"}, {QJsonValue::Double}, errorString));
    QVERIFY(errorString.contains(QStringLiteral("count")));
}

void JsonParsingTest::_testValidateKeyTypesListSizeMismatch()
{
    const QJsonObject object = {
        {"count", 10},
    };

    QString errorString;
    QVERIFY(!JsonParsing::validateKeyTypes(object, {"count", "extra"}, {QJsonValue::Double}, errorString));
    QVERIFY(errorString.contains(QStringLiteral("Mismatched key and type list sizes")));
}

void JsonParsingTest::_testValidateKeyTypesNullAcceptsDouble()
{
    const QJsonObject object = {
        {"possiblyNaN", 12.5},
    };

    QString errorString;
    QVERIFY(JsonParsing::validateKeyTypes(object, {"possiblyNaN"}, {QJsonValue::Null}, errorString));
    QVERIFY(errorString.isEmpty());
}

void JsonParsingTest::_testPossibleNaNJsonValue()
{
    const double nullValue = JsonParsing::possibleNaNJsonValue(QJsonValue::Null);
    QVERIFY(qIsNaN(nullValue));

    QCOMPARE(JsonParsing::possibleNaNJsonValue(QJsonValue(42.75)), 42.75);
}

void JsonParsingTest::_testIsJsonFileBytes()
{
    const QByteArray data = QByteArrayLiteral("{\"version\":1,\"items\":[1,2,3]}");
    QJsonDocument document;
    QString errorString;

    QVERIFY(JsonParsing::isJsonFile(data, document, errorString));
    QVERIFY(document.isObject());
    QCOMPARE(document.object().value(QStringLiteral("version")).toInt(), 1);
}

void JsonParsingTest::_testIsJsonFileInvalidBytes()
{
    const QByteArray data = QByteArrayLiteral("{invalid-json");
    QJsonDocument document;
    QString errorString;

    QVERIFY(!JsonParsing::isJsonFile(data, document, errorString));
    QVERIFY(!errorString.isEmpty());
}

void JsonParsingTest::_testIsJsonFileCompressedResourceBytes()
{
    QString fileError;
    const QByteArray data = QGCFileHelper::readFile(QStringLiteral(":/unittest/manifest.json.gz"), &fileError);
    QVERIFY(fileError.isEmpty());
    QVERIFY(!data.isEmpty());

    QJsonDocument document;
    QString errorString;
    QVERIFY(JsonParsing::isJsonFile(data, document, errorString));
    QVERIFY(document.isObject());
}

void JsonParsingTest::_testIsJsonFileCompressedResourcePath()
{
    QJsonDocument document;
    QString errorString;

    QVERIFY(JsonParsing::isJsonFile(QStringLiteral(":/unittest/manifest.json.gz"), document, errorString));
    QVERIFY(document.isObject());
}

UT_REGISTER_TEST(JsonParsingTest, TestLabel::Unit, TestLabel::Utilities)
