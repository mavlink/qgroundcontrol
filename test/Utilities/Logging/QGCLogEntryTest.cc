#include "QGCLogEntryTest.h"
#include "QGCLogEntry.h"

#include <QtTest/QTest>

void QGCLogEntryTest::init()
{
    UnitTest::init();
}

void QGCLogEntryTest::cleanup()
{
    UnitTest::cleanup();
}

void QGCLogEntryTest::_testDefaultConstruction()
{
    QGCLogEntry entry;

    QCOMPARE(entry.level, QGCLogEntry::Debug);
    QVERIFY(entry.category.isEmpty());
    QVERIFY(entry.message.isEmpty());
    QVERIFY(entry.function.isEmpty());
    QCOMPARE(entry.line, 0);
}

void QGCLogEntryTest::_testToStringBasic()
{
    QGCLogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Test message";

    const QString result = entry.toString();

    QVERIFY(result.contains("2024-01-15"));
    QVERIFY(result.contains("Test message"));
}

void QGCLogEntryTest::_testToStringWithCategory()
{
    QGCLogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Test message";
    entry.category = "TestCategory";

    const QString result = entry.toString();

    QVERIFY(result.contains("TestCategory"));
    QVERIFY(result.contains("Test message"));
}

void QGCLogEntryTest::_testToStringWithFunction()
{
    QGCLogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Test message";
    entry.function = "myFunction";
    entry.line = 42;

    const QString result = entry.toString();

    QVERIFY(result.contains("myFunction"));
    QVERIFY(result.contains("42"));
}

void QGCLogEntryTest::_testToStringWithLevel()
{
    QGCLogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Test message";
    entry.level = QGCLogEntry::Warning;

    const QString result = entry.toString();

    QVERIFY(result.contains("W:"));
}

void QGCLogEntryTest::_testFromQtMsgType()
{
    QCOMPARE(QGCLogEntry::fromQtMsgType(QtDebugMsg), QGCLogEntry::Debug);
    QCOMPARE(QGCLogEntry::fromQtMsgType(QtInfoMsg), QGCLogEntry::Info);
    QCOMPARE(QGCLogEntry::fromQtMsgType(QtWarningMsg), QGCLogEntry::Warning);
    QCOMPARE(QGCLogEntry::fromQtMsgType(QtCriticalMsg), QGCLogEntry::Critical);
    QCOMPARE(QGCLogEntry::fromQtMsgType(QtFatalMsg), QGCLogEntry::Fatal);
}

void QGCLogEntryTest::_testLevelLabel()
{
    QCOMPARE(QGCLogEntry::levelLabel(QGCLogEntry::Debug), QString("D"));
    QCOMPARE(QGCLogEntry::levelLabel(QGCLogEntry::Info), QString("I"));
    QCOMPARE(QGCLogEntry::levelLabel(QGCLogEntry::Warning), QString("W"));
    QCOMPARE(QGCLogEntry::levelLabel(QGCLogEntry::Critical), QString("C"));
    QCOMPARE(QGCLogEntry::levelLabel(QGCLogEntry::Fatal), QString("F"));
}
