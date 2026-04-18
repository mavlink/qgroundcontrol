#include "LogEntryTest.h"


#include "LogEntry.h"
#include "UnitTestList.h"

void LogEntryTest::_levelLabel()
{
    LogEntry e;

    e.level = LogEntry::Debug;
    QCOMPARE(e.levelLabel(), QStringLiteral("D"));

    e.level = LogEntry::Info;
    QCOMPARE(e.levelLabel(), QStringLiteral("I"));

    e.level = LogEntry::Warning;
    QCOMPARE(e.levelLabel(), QStringLiteral("W"));

    e.level = LogEntry::Critical;
    QCOMPARE(e.levelLabel(), QStringLiteral("C"));

    e.level = LogEntry::Fatal;
    QCOMPARE(e.levelLabel(), QStringLiteral("F"));
}

void LogEntryTest::_fromQtMsgType()
{
    QCOMPARE(LogEntry::fromQtMsgType(QtDebugMsg), LogEntry::Debug);
    QCOMPARE(LogEntry::fromQtMsgType(QtInfoMsg), LogEntry::Info);
    QCOMPARE(LogEntry::fromQtMsgType(QtWarningMsg), LogEntry::Warning);
    QCOMPARE(LogEntry::fromQtMsgType(QtCriticalMsg), LogEntry::Critical);
    QCOMPARE(LogEntry::fromQtMsgType(QtFatalMsg), LogEntry::Fatal);
}

void LogEntryTest::_buildFormatted()
{
    LogEntry e;
    e.timestamp = QDateTime(QDate(2024, 6, 15), QTime(12, 30, 45, 123), QTimeZone::UTC);
    e.level = LogEntry::Warning;
    e.category = QStringLiteral("test.cat");
    e.message = QStringLiteral("something broke");
    e.buildFormatted();

    QVERIFY(!e.formatted.isEmpty());
    QVERIFY(e.formatted.contains(QStringLiteral("W")));
    QVERIFY(e.formatted.contains(QStringLiteral("test.cat")));
    QVERIFY(e.formatted.contains(QStringLiteral("something broke")));
    QVERIFY(e.formatted.contains(QStringLiteral("2024-06-15")));
}

void LogEntryTest::_moveSemantics()
{
    LogEntry e;
    e.level = LogEntry::Info;
    e.category = QStringLiteral("move.test");
    e.message = QStringLiteral("original");
    e.buildFormatted();

    LogEntry moved(std::move(e));
    QCOMPARE(moved.message, QStringLiteral("original"));
    QCOMPARE(moved.category, QStringLiteral("move.test"));
    QCOMPARE(moved.level, LogEntry::Info);
    QVERIFY(!moved.formatted.isEmpty());
}

void LogEntryTest::_defaultValues()
{
    LogEntry e;
    QCOMPARE(e.level, LogEntry::Debug);
    QCOMPARE(e.line, 0);
    QVERIFY(e.category.isEmpty());
    QVERIFY(e.message.isEmpty());
    QVERIFY(e.file.isEmpty());
    QVERIFY(e.function.isEmpty());
    QVERIFY(e.formatted.isEmpty());
    QVERIFY(!e.timestamp.isValid());
}

void LogEntryTest::_roleNames()
{
    const auto roles = LogEntry::roleNames();
    QVERIFY(roles.contains(Qt::DisplayRole));
    QVERIFY(roles.contains(LogEntry::TimestampRole));
    QVERIFY(roles.contains(LogEntry::LevelRole));
    QVERIFY(roles.contains(LogEntry::CategoryRole));
    QVERIFY(roles.contains(LogEntry::MessageRole));
    QVERIFY(roles.contains(LogEntry::FormattedRole));
    QVERIFY(roles.contains(LogEntry::FileRole));
    QVERIFY(roles.contains(LogEntry::FunctionRole));
    QVERIFY(roles.contains(LogEntry::LineRole));
    QVERIFY(roles.contains(LogEntry::ThreadIdRole));
    QVERIFY(roles.contains(LogEntry::LevelLabelRole));

    // Verify static-const optimization returns same data each call
    const auto roles2 = LogEntry::roleNames();
    QCOMPARE(roles, roles2);
}

void LogEntryTest::_roleData()
{
    LogEntry e;
    e.timestamp = QDateTime(QDate(2024, 6, 15), QTime(12, 30, 45, 123), QTimeZone::UTC);
    e.level = LogEntry::Warning;
    e.category = QStringLiteral("test.role");
    e.message = QStringLiteral("role data");
    e.file = QStringLiteral("Foo.cc");
    e.function = QStringLiteral("bar()");
    e.line = 42;
    e.threadId = reinterpret_cast<Qt::HANDLE>(static_cast<quintptr>(0xABCD));
    e.buildFormatted();

    QCOMPARE(e.roleData(LogEntry::LevelRole).toInt(), static_cast<int>(LogEntry::Warning));
    QCOMPARE(e.roleData(LogEntry::LevelLabelRole).toString(), QStringLiteral("W"));
    QCOMPARE(e.roleData(LogEntry::CategoryRole).toString(), QStringLiteral("test.role"));
    QCOMPARE(e.roleData(LogEntry::MessageRole).toString(), QStringLiteral("role data"));
    QCOMPARE(e.roleData(LogEntry::FileRole).toString(), QStringLiteral("Foo.cc"));
    QCOMPARE(e.roleData(LogEntry::FunctionRole).toString(), QStringLiteral("bar()"));
    QCOMPARE(e.roleData(LogEntry::LineRole).toInt(), 42);
    QVERIFY(!e.roleData(LogEntry::FormattedRole).toString().isEmpty());
    QVERIFY(!e.roleData(LogEntry::ThreadIdRole).toString().isEmpty());

    // Unknown role returns invalid QVariant
    QVERIFY(!e.roleData(Qt::UserRole + 999).isValid());
}

void LogEntryTest::_columnDisplayData()
{
    LogEntry e;
    e.timestamp = QDateTime(QDate(2024, 6, 15), QTime(12, 30, 45, 123), QTimeZone::UTC);
    e.level = LogEntry::Info;
    e.category = QStringLiteral("col.test");
    e.message = QStringLiteral("column data");
    e.file = QStringLiteral("Bar.cc");
    e.line = 99;

    QCOMPARE(e.columnDisplayData(LogEntry::TimestampColumn).toString(), QStringLiteral("12:30:45.123"));
    QCOMPARE(e.columnDisplayData(LogEntry::LevelColumn).toString(), QStringLiteral("I"));
    QCOMPARE(e.columnDisplayData(LogEntry::CategoryColumn).toString(), QStringLiteral("col.test"));
    QCOMPARE(e.columnDisplayData(LogEntry::MessageColumn).toString(), QStringLiteral("column data"));
    QCOMPARE(e.columnDisplayData(LogEntry::SourceColumn).toString(), QStringLiteral("Bar.cc:99"));

    // Source with no file
    LogEntry e2;
    QCOMPARE(e2.columnDisplayData(LogEntry::SourceColumn).toString(), QString());

    // Source with file but no line
    LogEntry e3;
    e3.file = QStringLiteral("Baz.cc");
    QCOMPARE(e3.columnDisplayData(LogEntry::SourceColumn).toString(), QStringLiteral("Baz.cc"));
}

void LogEntryTest::_columnHeaderData()
{
    QCOMPARE(LogEntry::columnHeaderData(LogEntry::TimestampColumn).toString(), QStringLiteral("Time"));
    QCOMPARE(LogEntry::columnHeaderData(LogEntry::LevelColumn).toString(), QStringLiteral("Level"));
    QCOMPARE(LogEntry::columnHeaderData(LogEntry::CategoryColumn).toString(), QStringLiteral("Category"));
    QCOMPARE(LogEntry::columnHeaderData(LogEntry::MessageColumn).toString(), QStringLiteral("Message"));
    QCOMPARE(LogEntry::columnHeaderData(LogEntry::SourceColumn).toString(), QStringLiteral("Source"));
}

UT_REGISTER_TEST(LogEntryTest, TestLabel::Unit, TestLabel::Utilities)
