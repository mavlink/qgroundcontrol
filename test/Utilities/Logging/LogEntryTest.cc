#include "LogEntryTest.h"
#include "LogEntry.h"

#include <QtTest/QTest>

UT_REGISTER_TEST(LogEntryTest, TestLabel::Unit, TestLabel::Utilities)

void LogEntryTest::init()
{
    UnitTest::init();
}

void LogEntryTest::cleanup()
{
    UnitTest::cleanup();
}

void LogEntryTest::_testDefaultConstruction()
{
    LogEntry entry;

    QCOMPARE(entry.level, LogEntry::Debug);
    QVERIFY(entry.category.isEmpty());
    QVERIFY(entry.message.isEmpty());
    QVERIFY(entry.file.isEmpty());
    QVERIFY(entry.function.isEmpty());
    QCOMPARE(entry.line, 0);
}

void LogEntryTest::_testToStringBasic()
{
    LogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Test message";

    const QString result = entry.toString();

    QVERIFY(result.contains("2024-01-15"));
    QVERIFY(result.contains("Test message"));
}

void LogEntryTest::_testToStringWithCategory()
{
    LogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Test message";
    entry.category = "TestCategory";

    const QString result = entry.toString();

    QVERIFY(result.contains("TestCategory"));
    QVERIFY(result.contains("Test message"));
}

void LogEntryTest::_testToStringWithFunction()
{
    LogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Test message";
    entry.function = "myFunction";
    entry.line = 42;

    const QString result = entry.toString();

    QVERIFY(result.contains("myFunction"));
    QVERIFY(result.contains("42"));
}

void LogEntryTest::_testToStringWithLevel()
{
    LogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Test message";
    entry.level = LogEntry::Warning;

    const QString result = entry.toString();

    QVERIFY(result.contains("W:"));
}

void LogEntryTest::_testFromQtMsgType()
{
    QCOMPARE(LogEntry::fromQtMsgType(QtDebugMsg), LogEntry::Debug);
    QCOMPARE(LogEntry::fromQtMsgType(QtInfoMsg), LogEntry::Info);
    QCOMPARE(LogEntry::fromQtMsgType(QtWarningMsg), LogEntry::Warning);
    QCOMPARE(LogEntry::fromQtMsgType(QtCriticalMsg), LogEntry::Critical);
    QCOMPARE(LogEntry::fromQtMsgType(QtFatalMsg), LogEntry::Fatal);
}

void LogEntryTest::_testLevelLabel()
{
    QCOMPARE(LogEntry::levelLabel(LogEntry::Debug), QString("D"));
    QCOMPARE(LogEntry::levelLabel(LogEntry::Info), QString("I"));
    QCOMPARE(LogEntry::levelLabel(LogEntry::Warning), QString("W"));
    QCOMPARE(LogEntry::levelLabel(LogEntry::Critical), QString("C"));
    QCOMPARE(LogEntry::levelLabel(LogEntry::Fatal), QString("F"));
}
