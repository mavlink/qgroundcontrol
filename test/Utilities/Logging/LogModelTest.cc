#include "LogModelTest.h"
#include "LogEntry.h"
#include "LogModel.h"

#include <QtTest/QSignalSpy>

UT_REGISTER_TEST(LogModelTest, TestLabel::Unit, TestLabel::Utilities)
#include <QtTest/QTest>

void LogModelTest::init()
{
    UnitTest::init();
    _model = new LogModel();
}

void LogModelTest::cleanup()
{
    delete _model;
    _model = nullptr;
    UnitTest::cleanup();
}

void LogModelTest::_testInitialState()
{
    QCOMPARE(_model->rowCount(), 0);
    QVERIFY(_model->allFormatted().isEmpty());
}

void LogModelTest::_testAppendEntry()
{
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.message = "Test message";
    entry.level = LogEntry::Info;

    QSignalSpy countSpy(_model, &LogModel::countChanged);

    _model->append(entry);

    // Process events to allow queued connection to complete
    QCoreApplication::processEvents();

    QCOMPARE(_model->rowCount(), 1);
    QCOMPARE(countSpy.count(), 1);
}

void LogModelTest::_testAppendMultiple()
{
    for (int i = 0; i < 10; ++i) {
        LogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.message = QString("Message %1").arg(i);
        _model->append(entry);
    }

    QCoreApplication::processEvents();

    QCOMPARE(_model->rowCount(), 10);
}

void LogModelTest::_testClear()
{
    // Add some entries
    for (int i = 0; i < 5; ++i) {
        LogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.message = QString("Message %1").arg(i);
        _model->append(entry);
    }

    QCoreApplication::processEvents();
    QCOMPARE(_model->rowCount(), 5);

    _model->clear();

    QCOMPARE(_model->rowCount(), 0);
}

void LogModelTest::_testRoleNames()
{
    const auto roles = _model->roleNames();

    QVERIFY(roles.contains(LogModel::TimestampRole));
    QVERIFY(roles.contains(LogModel::LevelRole));
    QVERIFY(roles.contains(LogModel::CategoryRole));
    QVERIFY(roles.contains(LogModel::MessageRole));
    QVERIFY(roles.contains(LogModel::FormattedRole));

    QCOMPARE(roles[LogModel::TimestampRole], QByteArray("timestamp"));
    QCOMPARE(roles[LogModel::LevelRole], QByteArray("level"));
    QCOMPARE(roles[LogModel::CategoryRole], QByteArray("category"));
    QCOMPARE(roles[LogModel::MessageRole], QByteArray("message"));
    QCOMPARE(roles[LogModel::FormattedRole], QByteArray("formatted"));
    QVERIFY(roles.contains(LogModel::FileRole));
    QVERIFY(roles.contains(LogModel::FunctionRole));
    QVERIFY(roles.contains(LogModel::LineRole));
    QCOMPARE(roles[LogModel::FileRole], QByteArray("file"));
    QCOMPARE(roles[LogModel::FunctionRole], QByteArray("function"));
    QCOMPARE(roles[LogModel::LineRole], QByteArray("line"));
}

void LogModelTest::_testDataAccess()
{
    LogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Test message";
    entry.category = "TestCategory";
    entry.level = LogEntry::Warning;

    _model->append(entry);
    QCoreApplication::processEvents();

    const QModelIndex index = _model->index(0);

    QCOMPARE(_model->data(index, LogModel::MessageRole).toString(), QString("Test message"));
    QCOMPARE(_model->data(index, LogModel::CategoryRole).toString(), QString("TestCategory"));
    QCOMPARE(_model->data(index, LogModel::LevelRole).toInt(), static_cast<int>(LogEntry::Warning));

    // Invalid index should return empty
    const QModelIndex invalidIndex = _model->index(-1);
    QVERIFY(!_model->data(invalidIndex, LogModel::MessageRole).isValid());
}

void LogModelTest::_testDataAccessSourceLocation()
{
    LogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Test message";
    entry.file = "main.cpp";
    entry.function = "doSomething";
    entry.line = 99;

    _model->append(entry);
    QCoreApplication::processEvents();

    const QModelIndex index = _model->index(0);

    QCOMPARE(_model->data(index, LogModel::FileRole).toString(), QString("main.cpp"));
    QCOMPARE(_model->data(index, LogModel::FunctionRole).toString(), QString("doSomething"));
    QCOMPARE(_model->data(index, LogModel::LineRole).toInt(), 99);
}

void LogModelTest::_testMaxEntries()
{
    _model->setMaxEntries(5);
    QCOMPARE(_model->maxEntries(), 5);

    // Add more than max entries
    for (int i = 0; i < 10; ++i) {
        LogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.message = QString("Message %1").arg(i);
        _model->append(entry);
    }

    QCoreApplication::processEvents();

    // Should be trimmed to max
    QCOMPARE(_model->rowCount(), 5);

    // First entry should be message 5 (oldest removed)
    const QModelIndex firstIndex = _model->index(0);
    QVERIFY(_model->data(firstIndex, LogModel::MessageRole).toString().contains("5"));
}

void LogModelTest::_testAllFormatted()
{
    for (int i = 0; i < 3; ++i) {
        LogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.message = QString("Message %1").arg(i);
        _model->append(entry);
    }

    QCoreApplication::processEvents();

    const QStringList formatted = _model->allFormatted();
    QCOMPARE(formatted.size(), 3);

    QVERIFY(formatted[0].contains("Message 0"));
    QVERIFY(formatted[1].contains("Message 1"));
    QVERIFY(formatted[2].contains("Message 2"));
}

void LogModelTest::_testCountChanged()
{
    QSignalSpy countSpy(_model, &LogModel::countChanged);

    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.message = "Test";

    _model->append(entry);
    QCoreApplication::processEvents();

    QCOMPARE(countSpy.count(), 1);

    _model->clear();
    QCOMPARE(countSpy.count(), 2);
}

void LogModelTest::_testFilterLevel()
{
    LogEntry debug;
    debug.timestamp = QDateTime::currentDateTime();
    debug.message = "Debug msg";
    debug.level = LogEntry::Debug;

    LogEntry warning;
    warning.timestamp = QDateTime::currentDateTime();
    warning.message = "Warning msg";
    warning.level = LogEntry::Warning;

    LogEntry critical;
    critical.timestamp = QDateTime::currentDateTime();
    critical.message = "Critical msg";
    critical.level = LogEntry::Critical;

    _model->append(debug);
    _model->append(warning);
    _model->append(critical);
    QCoreApplication::processEvents();

    QCOMPARE(_model->rowCount(), 3);
    QCOMPARE(_model->totalCount(), 3);

    // Filter to Warning and above
    _model->setFilterLevel(static_cast<int>(LogEntry::Warning));
    QCOMPARE(_model->rowCount(), 2);
    QCOMPARE(_model->totalCount(), 3);

    // Filter to Critical and above
    _model->setFilterLevel(static_cast<int>(LogEntry::Critical));
    QCOMPARE(_model->rowCount(), 1);

    // Reset filter
    _model->setFilterLevel(-1);
    QCOMPARE(_model->rowCount(), 3);
}

void LogModelTest::_testFilterCategory()
{
    LogEntry a;
    a.timestamp = QDateTime::currentDateTime();
    a.message = "A";
    a.category = "Network";

    LogEntry b;
    b.timestamp = QDateTime::currentDateTime();
    b.message = "B";
    b.category = "Vehicle";

    LogEntry c;
    c.timestamp = QDateTime::currentDateTime();
    c.message = "C";
    c.category = "Network";

    _model->append(a);
    _model->append(b);
    _model->append(c);
    QCoreApplication::processEvents();

    QCOMPARE(_model->rowCount(), 3);

    _model->setFilterCategory("Network");
    QCOMPARE(_model->rowCount(), 2);

    _model->setFilterCategory("Vehicle");
    QCOMPARE(_model->rowCount(), 1);

    _model->setFilterCategory("NonExistent");
    QCOMPARE(_model->rowCount(), 0);

    _model->setFilterCategory("");
    QCOMPARE(_model->rowCount(), 3);
}

void LogModelTest::_testFilterCategoryWildcard()
{
    LogEntry a;
    a.timestamp = QDateTime::currentDateTime();
    a.message = "A";
    a.category = "qgc.vehicle.gps";

    LogEntry b;
    b.timestamp = QDateTime::currentDateTime();
    b.message = "B";
    b.category = "qgc.vehicle.imu";

    LogEntry c;
    c.timestamp = QDateTime::currentDateTime();
    c.message = "C";
    c.category = "qgc.network";

    _model->append(a);
    _model->append(b);
    _model->append(c);
    QCoreApplication::processEvents();

    // Wildcard prefix match
    _model->setFilterCategory("qgc.vehicle*");
    QCOMPARE(_model->rowCount(), 2);

    _model->setFilterCategory("qgc*");
    QCOMPARE(_model->rowCount(), 3);

    _model->setFilterCategory("");
    QCOMPARE(_model->rowCount(), 3);
}

void LogModelTest::_testFilterText()
{
    LogEntry a;
    a.timestamp = QDateTime::currentDateTime();
    a.message = "Connection established";

    LogEntry b;
    b.timestamp = QDateTime::currentDateTime();
    b.message = "Parameter loaded";

    LogEntry c;
    c.timestamp = QDateTime::currentDateTime();
    c.message = "Connection lost";

    _model->append(a);
    _model->append(b);
    _model->append(c);
    QCoreApplication::processEvents();

    _model->setFilterText("connection");
    QCOMPARE(_model->rowCount(), 2);

    _model->setFilterText("PARAMETER");  // case-insensitive
    QCOMPARE(_model->rowCount(), 1);

    _model->setFilterText("");
    QCOMPARE(_model->rowCount(), 3);
}

void LogModelTest::_testClearFilters()
{
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.message = "Test";
    entry.level = LogEntry::Debug;
    entry.category = "Cat";

    _model->append(entry);
    QCoreApplication::processEvents();

    _model->setFilterLevel(static_cast<int>(LogEntry::Warning));
    QCOMPARE(_model->rowCount(), 0);

    _model->clearFilters();
    QCOMPARE(_model->rowCount(), 1);
    QCOMPARE(_model->filterLevel(), -1);
    QVERIFY(_model->filterCategory().isEmpty());
    QVERIFY(_model->filterText().isEmpty());
}

void LogModelTest::_testFilteredFormatted()
{
    LogEntry debug;
    debug.timestamp = QDateTime::currentDateTime();
    debug.message = "Debug msg";
    debug.level = LogEntry::Debug;

    LogEntry warning;
    warning.timestamp = QDateTime::currentDateTime();
    warning.message = "Warning msg";
    warning.level = LogEntry::Warning;

    _model->append(debug);
    _model->append(warning);
    QCoreApplication::processEvents();

    _model->setFilterLevel(static_cast<int>(LogEntry::Warning));

    const QStringList all = _model->allFormatted();
    const QStringList filtered = _model->filteredFormatted();

    QCOMPARE(all.size(), 2);
    QCOMPARE(filtered.size(), 1);
    QVERIFY(filtered[0].contains("Warning msg"));
}

void LogModelTest::_testFilteredEntries()
{
    LogEntry a;
    a.timestamp = QDateTime::currentDateTime();
    a.message = "A";
    a.level = LogEntry::Debug;

    LogEntry b;
    b.timestamp = QDateTime::currentDateTime();
    b.message = "B";
    b.level = LogEntry::Warning;

    _model->append(a);
    _model->append(b);
    QCoreApplication::processEvents();

    _model->setFilterLevel(static_cast<int>(LogEntry::Warning));

    const QList<LogEntry> all = _model->allEntries();
    const QList<LogEntry> filtered = _model->filteredEntries();

    QCOMPARE(all.size(), 2);
    QCOMPARE(filtered.size(), 1);
    QCOMPARE(filtered[0].message, QString("B"));
}

void LogModelTest::_testCategories()
{
    LogEntry a;
    a.timestamp = QDateTime::currentDateTime();
    a.message = "A";
    a.category = "Network";

    LogEntry b;
    b.timestamp = QDateTime::currentDateTime();
    b.message = "B";
    b.category = "Vehicle";

    LogEntry c;
    c.timestamp = QDateTime::currentDateTime();
    c.message = "C";
    c.category = "Network";

    _model->append(a);
    _model->append(b);
    _model->append(c);
    QCoreApplication::processEvents();

    QStringList cats = _model->categories();
    cats.sort();

    QCOMPARE(cats.size(), 2);
    QCOMPARE(cats[0], QString("Network"));
    QCOMPARE(cats[1], QString("Vehicle"));
}

void LogModelTest::_testTotalCountVsCount()
{
    LogEntry debug;
    debug.timestamp = QDateTime::currentDateTime();
    debug.message = "Debug";
    debug.level = LogEntry::Debug;

    LogEntry warning;
    warning.timestamp = QDateTime::currentDateTime();
    warning.message = "Warning";
    warning.level = LogEntry::Warning;

    _model->append(debug);
    _model->append(warning);
    QCoreApplication::processEvents();

    QCOMPARE(_model->rowCount(), 2);
    QCOMPARE(_model->totalCount(), 2);

    _model->setFilterLevel(static_cast<int>(LogEntry::Warning));

    QCOMPARE(_model->rowCount(), 1);
    QCOMPARE(_model->totalCount(), 2);
}
