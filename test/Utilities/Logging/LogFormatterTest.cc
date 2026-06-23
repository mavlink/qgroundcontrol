#include "LogFormatterTest.h"
#include "LogEntry.h"
#include "LogFormatter.h"
#include "LogTestHelpers.h"
#include "UnitTest.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

using LogTestHelpers::makeEntry;

void LogFormatterTest::_formatAsText()
{
    QList<LogEntry> entries = {makeEntry(QStringLiteral("hello")), makeEntry(QStringLiteral("world"))};
    const QByteArray result = LogFormatter::formatAsText(entries);

    QVERIFY(result.contains("hello"));
    QVERIFY(result.contains("world"));
    QVERIFY(result.endsWith('\n'));
    QCOMPARE(result.count('\n'), 2);
}

void LogFormatterTest::_formatAsJson()
{
    QList<LogEntry> entries = {makeEntry(QStringLiteral("test msg"))};
    const QByteArray result = LogFormatter::formatAsJson(entries);

    const QJsonDocument doc = QJsonDocument::fromJson(result);
    QVERIFY(doc.isArray());
    QCOMPARE(doc.array().size(), 1);
    QCOMPARE(doc.array().first().toObject()["message"].toString(), QStringLiteral("test msg"));
    QCOMPARE(doc.array().first().toObject()["level"].toString(), QStringLiteral("I"));
}

void LogFormatterTest::_formatAsCsv()
{
    QList<LogEntry> entries = {makeEntry(QStringLiteral("csv test"))};
    const QByteArray result = LogFormatter::formatAsCsv(entries);

    const QList<QByteArray> lines = result.split('\n');
    QVERIFY(lines.size() >= 2);
    QVERIFY(lines[0].contains("timestamp"));
    QVERIFY(lines[1].contains("csv test"));
}

void LogFormatterTest::_formatAsJsonLines()
{
    QList<LogEntry> entries = {makeEntry(QStringLiteral("line1")), makeEntry(QStringLiteral("line2"))};
    const QByteArray result = LogFormatter::formatAsJsonLines(entries);

    const QList<QByteArray> lines = result.split('\n');
    QVERIFY(lines.size() >= 2);

    const QJsonDocument doc1 = QJsonDocument::fromJson(lines[0]);
    QVERIFY(doc1.isObject());
    QCOMPARE(doc1.object()["message"].toString(), QStringLiteral("line1"));
}

void LogFormatterTest::_csvEscaping()
{
    QList<LogEntry> entries = {makeEntry(QStringLiteral("has, comma and \"quotes\""))};
    const QByteArray result = LogFormatter::formatAsCsv(entries);

    QVERIFY(result.contains("\"has, comma and \"\"quotes\"\"\""));
}

void LogFormatterTest::_entryToJsonExportSchema()
{
    const auto entry = makeEntry(QStringLiteral("export test"), LogEntry::Warning);
    const QJsonObject obj = LogFormatter::entryToJson(entry, LogFormatter::ExportSchema);

    QVERIFY(obj.contains("timestamp"));
    QVERIFY(obj.contains("level"));
    QVERIFY(obj.contains("category"));
    QVERIFY(obj.contains("message"));
    QCOMPARE(obj["level"].toString(), QStringLiteral("W"));
    QCOMPARE(obj["message"].toString(), QStringLiteral("export test"));
    // Export schema uses ISO string timestamps
    QVERIFY(obj["timestamp"].isString());
}

void LogFormatterTest::_entryToJsonRemoteSchema()
{
    const auto entry = makeEntry(QStringLiteral("remote test"), LogEntry::Critical);
    const QJsonObject obj = LogFormatter::entryToJson(entry, LogFormatter::RemoteCompactSchema);

    // Remote schema uses short keys and numeric timestamp
    QVERIFY(obj.contains("t"));
    QVERIFY(obj.contains("l"));
    QVERIFY(obj.contains("c"));
    QVERIFY(obj.contains("m"));
    QCOMPARE(obj["l"].toInt(), static_cast<int>(LogEntry::Critical));
    QCOMPARE(obj["m"].toString(), QStringLiteral("remote test"));
    QVERIFY(obj["t"].isDouble()); // msecs since epoch
}

void LogFormatterTest::_formatDispatch()
{
    QList<LogEntry> entries = {makeEntry(QStringLiteral("dispatch"))};

    // PlainText
    const QByteArray text = LogFormatter::format(entries, LogFormatter::PlainText);
    QVERIFY(text.contains("dispatch"));

    // JSON
    const QByteArray json = LogFormatter::format(entries, LogFormatter::JSON);
    QVERIFY(QJsonDocument::fromJson(json).isArray());

    // CSV
    const QByteArray csv = LogFormatter::format(entries, LogFormatter::CSV);
    QVERIFY(csv.contains("timestamp"));

    // JSONLines
    const QByteArray jsonl = LogFormatter::format(entries, LogFormatter::JSONLines);
    QVERIFY(QJsonDocument::fromJson(jsonl.split('\n').first()).isObject());
}

UT_REGISTER_TEST(LogFormatterTest, TestLabel::Unit, TestLabel::Utilities)
