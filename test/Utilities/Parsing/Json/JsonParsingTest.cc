#include "JsonParsingTest.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryFile>
#include <QtCore/qnumeric.h>

#include "Compression.h"
#include "JsonParsing.h"
#include "QGCFileHelper.h"

namespace {

/// Limit of the content these tests parse, which is a few bytes
constexpr qint64 kJsonParsingTestMaxBytes = 1024;

}  // namespace

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

    QVERIFY(JsonParsing::isJsonFile(data, document, errorString, kJsonParsingTestMaxBytes));
    QVERIFY(document.isObject());
    QCOMPARE(document.object().value(QStringLiteral("version")).toInt(), 1);
}

void JsonParsingTest::_testIsJsonFileCompressedResourceBytes()
{
    QString fileError;
    const QByteArray data = QGCFileHelper::readFile(QStringLiteral(":/unittest/manifest.json.gz"), &fileError);
    QVERIFY(fileError.isEmpty());
    QVERIFY(!data.isEmpty());

    QJsonDocument document;
    QString errorString;
    QVERIFY(JsonParsing::isJsonFile(data, document, errorString, kJsonParsingTestMaxBytes));
    QVERIFY(document.isObject());

    // The limit is on the decompressed content, as for a file
    QByteArray content;
    QVERIFY(Compression::decompressData(data, content, kJsonParsingTestMaxBytes));
    QVERIFY(JsonParsing::isJsonFile(data, document, errorString, content.size()));
    QVERIFY(!JsonParsing::isJsonFile(data, document, errorString, content.size() - 1));
    QVERIFY2(errorString.contains(QStringLiteral("size limit exceeded")), qPrintable(errorString));

    // Damaged compression is reported as that, not as a syntax error in bytes that are no text
    QVERIFY(!JsonParsing::isJsonFile(data.first(data.size() / 2), document, errorString, kJsonParsingTestMaxBytes));
    QVERIFY(document.isNull());
    QVERIFY2(errorString.startsWith("Failed to decompress data"), qPrintable(errorString));
    QVERIFY2(errorString.contains("truncated"), qPrintable(errorString));
}

void JsonParsingTest::_testIsJsonFileCompressedResourcePath()
{
    QJsonDocument document;
    QString errorString;

    const QString path = QStringLiteral(":/unittest/manifest.json.gz");
    QVERIFY(JsonParsing::isJsonFile(path, document, errorString, kJsonParsingTestMaxBytes));
    QVERIFY(document.isObject());

    // The limit is on the decompressed content: a file of exactly the limit is parsed, a larger one is not
    QByteArray content;
    QVERIFY(Compression::readFile(path, content, kJsonParsingTestMaxBytes));
    QVERIFY(JsonParsing::isJsonFile(path, document, errorString, content.size()));
    QVERIFY(!JsonParsing::isJsonFile(path, document, errorString, content.size() - 1));
    QVERIFY(document.isNull());
    QVERIFY2(errorString.contains(QStringLiteral("size limit exceeded")), qPrintable(errorString));
}

void JsonParsingTest::_testIsJsonFileMissingPath()
{
    QJsonDocument document;
    QString errorString;

    QVERIFY(!JsonParsing::isJsonFile(QStringLiteral("/definitely/missing/file.json"), document, errorString,
                                     kJsonParsingTestMaxBytes));
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
        return fromFile ? JsonParsing::isJsonFile(file.fileName(), document, errorString, kJsonParsingTestMaxBytes)
                        : JsonParsing::isJsonFile(content, document, errorString, kJsonParsingTestMaxBytes);
    };

    QString freshError;
    QCOMPARE(parse(freshError), isJson);
    QCOMPARE(freshError.isEmpty(), isJson);

    QString staleError = QStringLiteral("error of an earlier call");
    QCOMPARE(parse(staleError), isJson);
    QCOMPARE(staleError, freshError);
}

UT_REGISTER_TEST(JsonParsingTest, TestLabel::Unit, TestLabel::Utilities)
