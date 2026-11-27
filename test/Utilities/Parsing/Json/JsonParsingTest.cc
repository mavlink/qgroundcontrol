#include "JsonParsingTest.h"

#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/qnumeric.h>

#include "Compression.h"
#include "FileFixtures.h"
#include "JsonParsing.h"
#include "QGCFormat.h"

namespace {

/// Limit of the content these tests parse, which is a few bytes
constexpr qint64 kJsonParsingTestMaxBytes = 1024;

/// A JSON file with the header of QGC's own JSON files
const QByteArray kInternalJsonFile =
    QByteArrayLiteral(R"({"fileType": "JsonParsingTest", "version": 1, "items": [1, 2]})");

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

void JsonParsingTest::_testPlainReadersRefuseCompressed_data()
{
    QTest::addColumn<QByteArray>("content");

    QTest::newRow("gzip") << TestFixtures::gzipMember(kInternalJsonFile);
    QTest::newRow("xz") << QByteArray(
                               "\xfd"
                               "7zXZ\0\0\x04",
                               8) +
                               kInternalJsonFile;
}

void JsonParsingTest::_testPlainReadersRefuseCompressed()
{
    QFETCH(QByteArray, content);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("compressed.json"));
    QVERIFY(TestFixtures::writeFile(path, content));

    QJsonDocument document;
    QString errorString;
    QVERIFY(!JsonParsing::parseJson(content, document, errorString));
    QVERIFY2(errorString.contains(QStringLiteral("compressed")), qPrintable(errorString));

    errorString.clear();
    QVERIFY(!JsonParsing::readJsonFile(path, document, errorString, kJsonParsingTestMaxBytes));
    QVERIFY(document.isNull());
    QVERIFY2(errorString.contains(QStringLiteral("compressed")), qPrintable(errorString));

    // QGC's own JSON files and the MAVLink actions file
    int version = 0;
    errorString.clear();
    QVERIFY(JsonParsing::openInternalQGCJsonFile(path, QStringLiteral("JsonParsingTest"), 1, 1, version, errorString)
                .isEmpty());
    QVERIFY2(errorString.contains(QStringLiteral("compressed")), qPrintable(errorString));
}

void JsonParsingTest::_testPlainReadersReadPlainFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("plain.json"));
    QVERIFY(TestFixtures::writeFile(path, kInternalJsonFile));

    QJsonDocument document;
    QString errorString;
    QVERIFY2(JsonParsing::readJsonFile(path, document, errorString, kJsonParsingTestMaxBytes), qPrintable(errorString));
    QVERIFY(document.isObject());

    int version = 0;
    const QJsonObject object =
        JsonParsing::openInternalQGCJsonFile(path, QStringLiteral("JsonParsingTest"), 1, 1, version, errorString);
    QVERIFY2(!object.isEmpty(), qPrintable(errorString));
    QCOMPARE(version, 1);
}

void JsonParsingTest::_testReadJsonFileSizeLimit()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("limit.json"));
    const QByteArray content = QByteArrayLiteral("{\"items\":[1,2,3]}");
    QVERIFY(TestFixtures::writeFile(path, content));

    QJsonDocument document;
    QString errorString;
    QVERIFY(JsonParsing::readJsonFile(path, document, errorString, content.size()));
    QVERIFY(document.isObject());

    QByteArray text;
    QVERIFY(JsonParsing::readJsonText(path, text, errorString, content.size()));
    QCOMPARE(text, content);

    // The size of the file decides before it is read. The caller names the file
    QVERIFY(!JsonParsing::readJsonFile(path, document, errorString, content.size() - 1));
    QVERIFY(document.isNull());
    QCOMPARE(errorString, QStringLiteral("The file is larger than %1").arg(QGC::bigSizeToString(content.size() - 1)));
    QVERIFY(!JsonParsing::readJsonText(path, text, errorString, content.size() - 1));
    QVERIFY(text.isEmpty());
}

// A file that reports no size is limited by what is read from it: the size checked before the read says nothing.
// The file has an end, so a read without a limit returns its content here instead of filling the memory
void JsonParsingTest::_testFileWithoutSizeIsLimited()
{
#ifndef Q_OS_LINUX
    QSKIP("Needs a file of /proc, which reports no size and has content: Linux only");
#else
    const QString path = QStringLiteral("/proc/self/status");
    QCOMPARE(QFileInfo(path).size(), 0);
    constexpr qint64 maxBytes = 16;
    QVERIFY(TestFixtures::readFile(path).size() > maxBytes);

    QByteArray text("content of an earlier read");
    QString errorString;
    QVERIFY(!JsonParsing::readJsonText(path, text, errorString, maxBytes));
    QVERIFY(text.isEmpty());
    QCOMPARE(errorString, QStringLiteral("The file is larger than %1").arg(QGC::bigSizeToString(maxBytes)));

    QJsonDocument document;
    QVERIFY(!JsonParsing::readJsonFile(path, document, errorString, maxBytes));
    QVERIFY(document.isNull());
#endif
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
    QVERIFY(Compression::decompressFileToMemory(path, content, kJsonParsingTestMaxBytes));
    QVERIFY(JsonParsing::isJsonFile(path, document, errorString, content.size()));
    QVERIFY(!JsonParsing::isJsonFile(path, document, errorString, content.size() - 1));
    QVERIFY(document.isNull());
    QVERIFY2(errorString.contains(QStringLiteral("larger than the size limit")), qPrintable(errorString));

    // Damaged compression is reported as that, not as a syntax error in bytes that are no text
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QByteArray compressed = TestFixtures::readFile(path);
    const QString truncatedPath = tempDir.filePath(QStringLiteral("truncated.json.gz"));
    QVERIFY(TestFixtures::writeFile(truncatedPath, compressed.first(compressed.size() / 2)));
    QVERIFY(!JsonParsing::isJsonFile(truncatedPath, document, errorString, kJsonParsingTestMaxBytes));
    QVERIFY(document.isNull());
    QVERIFY2(errorString.contains("truncated"), qPrintable(errorString));

    // Plain content is read too
    const QString plainPath = tempDir.filePath(QStringLiteral("plain.json"));
    QVERIFY(TestFixtures::writeFile(plainPath, content));
    QVERIFY2(JsonParsing::isJsonFile(plainPath, document, errorString, content.size()), qPrintable(errorString));
    QVERIFY(document.isObject());
}

void JsonParsingTest::_testReadersMissingPath()
{
    QJsonDocument document;
    QString errorString;

    // What looks like a placeholder in the name stays as it is in the message
    const QString path = QStringLiteral("/definitely/missing/file%1.json");
    QVERIFY(!JsonParsing::isJsonFile(path, document, errorString, kJsonParsingTestMaxBytes));
    QVERIFY(!errorString.isEmpty());

    errorString.clear();
    QVERIFY(!JsonParsing::readJsonFile(path, document, errorString, kJsonParsingTestMaxBytes));
    QVERIFY2(errorString.contains(path), qPrintable(errorString));
}

void JsonParsingTest::_testReadersReplaceStaleError_data()
{
    QTest::addColumn<QByteArray>("content");
    QTest::addColumn<QString>("reader");
    QTest::addColumn<bool>("isJson");

    for (const QString& reader :
         {QStringLiteral("parseJson"), QStringLiteral("readJsonFile"), QStringLiteral("isJsonFile")}) {
        QTest::addRow("valid-%s", qPrintable(reader)) << QByteArrayLiteral("{\"version\":1}") << reader << true;
        QTest::addRow("invalid-%s", qPrintable(reader)) << QByteArrayLiteral("{") << reader << false;
        QTest::addRow("empty-%s", qPrintable(reader)) << QByteArray() << reader << false;
    }
}

void JsonParsingTest::_testReadersReplaceStaleError()
{
    QFETCH(QByteArray, content);
    QFETCH(QString, reader);
    QFETCH(bool, isJson);

    QTemporaryFile file;
    QVERIFY(file.open());
    QCOMPARE(file.write(content), content.size());
    file.close();

    QJsonDocument document;
    const auto parse = [&](QString& errorString) {
        if (reader == QStringLiteral("parseJson")) {
            return JsonParsing::parseJson(content, document, errorString);
        }
        if (reader == QStringLiteral("readJsonFile")) {
            return JsonParsing::readJsonFile(file.fileName(), document, errorString, kJsonParsingTestMaxBytes);
        }
        return JsonParsing::isJsonFile(file.fileName(), document, errorString, kJsonParsingTestMaxBytes);
    };

    QString freshError;
    QCOMPARE(parse(freshError), isJson);
    QCOMPARE(freshError.isEmpty(), isJson);
    // The document is the content, and nothing when it could not be parsed
    QCOMPARE(document.isObject(), isJson);
    QCOMPARE(document.object().value(QStringLiteral("version")).toInt(), isJson ? 1 : 0);

    QString staleError = QStringLiteral("error of an earlier call");
    QCOMPARE(parse(staleError), isJson);
    QCOMPARE(staleError, freshError);
}

UT_REGISTER_TEST(JsonParsingTest, TestLabel::Unit, TestLabel::Utilities)
