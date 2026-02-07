#include "LogFormatterTest.h"
#include "LogEntry.h"
#include "LogFormatter.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QTest>

UT_REGISTER_TEST(LogFormatterTest, TestLabel::Unit, TestLabel::Utilities)

void LogFormatterTest::_testFormatText()
{
    LogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Test message";
    entry.level = LogEntry::Warning;

    const QString result = LogFormatter::formatText(entry);

    QVERIFY(result.contains("Test message"));
    QVERIFY(result.contains("W:"));
}

void LogFormatterTest::_testFormatJson()
{
    LogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "JSON message";
    entry.level = LogEntry::Info;
    entry.category = "TestCat";

    const QString result = LogFormatter::formatJson(entry);
    const QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
    QVERIFY(!doc.isNull());

    const QJsonObject json = doc.object();
    QCOMPARE(json["message"].toString(), QString("JSON message"));
    QCOMPARE(json["level"].toString(), QString("I"));
    QCOMPARE(json["category"].toString(), QString("TestCat"));
    QVERIFY(json["timestamp"].toString().contains("2024-01-15"));
}

void LogFormatterTest::_testFormatJsonSourceLocation()
{
    LogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "With location";
    entry.file = "main.cpp";
    entry.function = "doStuff";
    entry.line = 42;

    const QString result = LogFormatter::formatJson(entry);
    const QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
    const QJsonObject json = doc.object();

    QCOMPARE(json["file"].toString(), QString("main.cpp"));
    QCOMPARE(json["function"].toString(), QString("doStuff"));
    QCOMPARE(json["line"].toInt(), 42);
}

void LogFormatterTest::_testFormatCsvRow()
{
    LogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "CSV message";
    entry.level = LogEntry::Debug;
    entry.category = "Cat";
    entry.file = "test.cpp";
    entry.function = "func";
    entry.line = 10;

    const QString row = LogFormatter::formatCsvRow(entry);
    const QStringList fields = row.split(',');

    QVERIFY(fields.size() >= 7);
    QVERIFY(fields[0].contains("2024-01-15"));
    QCOMPARE(fields[1], QString("D"));
    QCOMPARE(fields[2], QString("Cat"));
    QCOMPARE(fields[3], QString("CSV message"));
    QCOMPARE(fields[4], QString("test.cpp"));
    QCOMPARE(fields[5], QString("func"));
    QCOMPARE(fields[6], QString("10"));
}

void LogFormatterTest::_testCsvHeader()
{
    const QString header = LogFormatter::csvHeader();

    QVERIFY(header.contains("timestamp"));
    QVERIFY(header.contains("level"));
    QVERIFY(header.contains("category"));
    QVERIFY(header.contains("message"));
    QVERIFY(header.contains("file"));
    QVERIFY(header.contains("function"));
    QVERIFY(header.contains("line"));
}

void LogFormatterTest::_testFormatAsText()
{
    QList<LogEntry> entries;
    for (int i = 0; i < 3; ++i) {
        LogEntry entry;
        entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
        entry.message = QString("Message %1").arg(i);
        entries.append(entry);
    }

    const QString result = LogFormatter::formatAsText(entries);
    QVERIFY(result.contains("Message 0"));
    QVERIFY(result.contains("Message 1"));
    QVERIFY(result.contains("Message 2"));
    QVERIFY(result.endsWith('\n'));
}

void LogFormatterTest::_testFormatAsJson()
{
    QList<LogEntry> entries;
    for (int i = 0; i < 2; ++i) {
        LogEntry entry;
        entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
        entry.message = QString("Entry %1").arg(i);
        entries.append(entry);
    }

    const QString result = LogFormatter::formatAsJson(entries);
    const QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
    QVERIFY(doc.isArray());

    const QJsonArray array = doc.array();
    QCOMPARE(array.size(), 2);
    QCOMPARE(array[0].toObject()["message"].toString(), QString("Entry 0"));
    QCOMPARE(array[1].toObject()["message"].toString(), QString("Entry 1"));
}

void LogFormatterTest::_testFormatAsCsv()
{
    QList<LogEntry> entries;
    LogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "CSV entry";
    entries.append(entry);

    const QString result = LogFormatter::formatAsCsv(entries);
    const QStringList lines = result.split('\n', Qt::SkipEmptyParts);

    QVERIFY(lines.size() >= 2);
    QVERIFY(lines[0].startsWith("timestamp"));  // header
    QVERIFY(lines[1].contains("CSV entry"));
}

void LogFormatterTest::_testFormatAsJsonLines()
{
    QList<LogEntry> entries;
    for (int i = 0; i < 2; ++i) {
        LogEntry entry;
        entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
        entry.message = QString("Line %1").arg(i);
        entries.append(entry);
    }

    const QString result = LogFormatter::formatAsJsonLines(entries);
    const QStringList lines = result.split('\n', Qt::SkipEmptyParts);

    QCOMPARE(lines.size(), 2);

    // Each line should be valid JSON
    for (const QString& line : lines) {
        const QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        QVERIFY2(!doc.isNull(), qPrintable(QString("Invalid JSON line: %1").arg(line)));
    }
}

void LogFormatterTest::_testFormatDispatch()
{
    QList<LogEntry> entries;
    LogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Dispatch test";
    entries.append(entry);

    // Each format should produce non-empty output
    QVERIFY(!LogFormatter::format(entries, LogFormatter::PlainText).isEmpty());
    QVERIFY(!LogFormatter::format(entries, LogFormatter::JSON).isEmpty());
    QVERIFY(!LogFormatter::format(entries, LogFormatter::CSV).isEmpty());
    QVERIFY(!LogFormatter::format(entries, LogFormatter::JSONLines).isEmpty());
}

void LogFormatterTest::_testFileExtension()
{
    QCOMPARE(LogFormatter::fileExtension(LogFormatter::PlainText), QString("txt"));
    QCOMPARE(LogFormatter::fileExtension(LogFormatter::JSON), QString("json"));
    QCOMPARE(LogFormatter::fileExtension(LogFormatter::CSV), QString("csv"));
    QCOMPARE(LogFormatter::fileExtension(LogFormatter::JSONLines), QString("jsonl"));
}

void LogFormatterTest::_testMimeType()
{
    QCOMPARE(LogFormatter::mimeType(LogFormatter::PlainText), QString("text/plain"));
    QCOMPARE(LogFormatter::mimeType(LogFormatter::JSON), QString("application/json"));
    QCOMPARE(LogFormatter::mimeType(LogFormatter::CSV), QString("text/csv"));
    QCOMPARE(LogFormatter::mimeType(LogFormatter::JSONLines), QString("application/x-ndjson"));
}

void LogFormatterTest::_testCsvEscaping()
{
    LogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Message with, comma";
    entry.category = "Has \"quotes\"";

    const QString row = LogFormatter::formatCsvRow(entry);

    // Fields with commas or quotes should be quoted/escaped
    QVERIFY(row.contains("\"Message with, comma\""));
    QVERIFY(row.contains("\"Has \"\"quotes\"\"\""));
}

void LogFormatterTest::_testSourceLocationGating()
{
    // Entry WITHOUT source location — JSON should not include file/function/line
    LogEntry noLoc;
    noLoc.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    noLoc.message = "No location";

    const QString noLocJson = LogFormatter::formatJson(noLoc);
    const QJsonObject noLocObj = QJsonDocument::fromJson(noLocJson.toUtf8()).object();

    QVERIFY(!noLocObj.contains("file"));
    QVERIFY(!noLocObj.contains("function"));
    QVERIFY(!noLocObj.contains("line"));

    // Entry WITH source location — JSON should include them
    LogEntry withLoc;
    withLoc.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    withLoc.message = "With location";
    withLoc.file = "src.cpp";
    withLoc.function = "run";
    withLoc.line = 5;

    const QString withLocJson = LogFormatter::formatJson(withLoc);
    const QJsonObject withLocObj = QJsonDocument::fromJson(withLocJson.toUtf8()).object();

    QVERIFY(withLocObj.contains("file"));
    QVERIFY(withLocObj.contains("function"));
    QVERIFY(withLocObj.contains("line"));
}
