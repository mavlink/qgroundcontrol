#include "JsonParsingTest.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryFile>
#include <QtCore/qnumeric.h>

#include "JsonParsing.h"
#include "QGCFileHelper.h"

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
                                          {QJsonValue::Double, QJsonValue::String, QJsonValue::Bool, QJsonValue::Array},
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

void JsonParsingTest::_testIsJsonFileMissingPath()
{
    QJsonDocument document;
    QString errorString;

    QVERIFY(!JsonParsing::isJsonFile(QStringLiteral("/definitely/missing/file.json"), document, errorString));
    QVERIFY(!errorString.isEmpty());
}

void JsonParsingTest::_testIsJsonFileReplacesStaleError_data()
{
    QTest::addColumn<QByteArray>("content");
    QTest::addColumn<bool>("fromFile");
    QTest::addColumn<bool>("isJson");

    QTest::newRow("valid-bytes") << QByteArrayLiteral("{}") << false << true;
    QTest::newRow("valid-file") << QByteArrayLiteral("{}") << true << true;
    QTest::newRow("invalid-bytes") << QByteArrayLiteral("{") << false << false;
    QTest::newRow("invalid-file") << QByteArrayLiteral("{") << true << false;
    // No content and an error text: what a file that could not be read gives
    QTest::newRow("empty-file") << QByteArray() << true << false;
}

void JsonParsingTest::_testIsJsonFileReplacesStaleError()
{
    QFETCH(QByteArray, content);
    QFETCH(bool, fromFile);
    QFETCH(bool, isJson);

    QTemporaryFile file;
    QVERIFY(file.open());
    QCOMPARE(file.write(content), content.size());
    file.close();

    const auto parse = [&](QString& errorString) {
        QJsonDocument document;
        return fromFile ? JsonParsing::isJsonFile(file.fileName(), document, errorString)
                        : JsonParsing::isJsonFile(content, document, errorString);
    };

    QString freshError;
    QCOMPARE(parse(freshError), isJson);
    QCOMPARE(freshError.isEmpty(), isJson);

    QString staleError = QStringLiteral("error of an earlier call");
    QCOMPARE(parse(staleError), isJson);
    QCOMPARE(staleError, freshError);
}

UT_REGISTER_TEST(JsonParsingTest, TestLabel::Unit, TestLabel::Utilities)
