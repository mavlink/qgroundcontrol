#include "LogModelTest.h"
#include "LogModel.h"
#include "UnitTest.h"

#include <QtCore/QCoreApplication>
#include <QtTest/QSignalSpy>

void LogModelTest::_appendAndRowCount()
{
    LogModel model;

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.totalCount(), 0);

    LogEntry entry;
    entry.level = LogEntry::Info;
    entry.category = QStringLiteral("test.cat");
    entry.message = QStringLiteral("hello");
    entry.buildFormatted();

    model.enqueue(entry);

    QTRY_COMPARE_WITH_TIMEOUT(model.totalCount(), 1, TestTimeout::shortMs());
    QCOMPARE(model.rowCount(), 1);

    const QModelIndex idx = model.index(0, 0);
    QCOMPARE(idx.data(static_cast<int>(LogEntry::MessageRole)).toString(), QStringLiteral("hello"));
    QCOMPARE(idx.data(static_cast<int>(LogEntry::LevelRole)).toInt(), static_cast<int>(LogEntry::Info));
    QCOMPARE(idx.data(static_cast<int>(LogEntry::CategoryRole)).toString(), QStringLiteral("test.cat"));
}

void LogModelTest::_batchFlush()
{
    LogModel model;
    QSignalSpy insertSpy(&model, &QAbstractItemModel::rowsInserted);

    for (int i = 0; i < 10; ++i) {
        LogEntry entry;
        entry.level = LogEntry::Debug;
        entry.message = QString::number(i);
        entry.buildFormatted();
        model.enqueue(entry);
    }

    QTRY_COMPARE_WITH_TIMEOUT(model.totalCount(), 10, TestTimeout::shortMs());
    QVERIFY(insertSpy.count() <= 2);
}

void LogModelTest::_trimExcessOnOverflow()
{
    LogModel model;
    model.setMaxEntries(1000);

    // Only need to exceed maxEntries to trigger the trim path; 1005 validates
    // the boundary without allocating ~100 extra LogEntry objects per run.
    for (int i = 0; i < 1005; ++i) {
        LogEntry entry;
        entry.level = LogEntry::Debug;
        entry.message = QString::number(i);
        entry.buildFormatted();
        model.enqueue(entry);
    }

    QTRY_VERIFY_WITH_TIMEOUT(model.totalCount() <= 1000, TestTimeout::shortMs());
}

void LogModelTest::_categories()
{
    LogModel model;

    LogEntry e1;
    e1.category = QStringLiteral("cat.a");
    e1.buildFormatted();
    model.enqueue(e1);

    LogEntry e2;
    e2.category = QStringLiteral("cat.b");
    e2.buildFormatted();
    model.enqueue(e2);

    LogEntry e3;
    e3.category = QStringLiteral("cat.a");
    e3.buildFormatted();
    model.enqueue(e3);

    QTRY_COMPARE_WITH_TIMEOUT(model.totalCount(), 3, TestTimeout::shortMs());

    const QStringList cats = model.categoriesList();
    QCOMPARE(cats.size(), 2);
    QVERIFY(cats.contains(QStringLiteral("cat.a")));
    QVERIFY(cats.contains(QStringLiteral("cat.b")));
}

void LogModelTest::_clear()
{
    LogModel model;

    for (int i = 0; i < 5; ++i) {
        LogEntry entry;
        entry.message = QString::number(i);
        entry.buildFormatted();
        model.enqueue(entry);
    }
    QTRY_COMPARE_WITH_TIMEOUT(model.totalCount(), 5, TestTimeout::shortMs());

    model.clear();
    QCOMPARE(model.totalCount(), 0);
    QCOMPARE(model.rowCount(), 0);
}

void LogModelTest::_maxEntries()
{
    LogModel model;
    model.setMaxEntries(500);
    QCOMPARE(model.maxEntries(), 1000);

    model.setMaxEntries(5000);
    QCOMPARE(model.maxEntries(), 5000);
}

static void populateWithMixedEntries(LogModel &model)
{
    const struct { LogEntry::Level level; QString category; QString message; } data[] = {
        {LogEntry::Debug,    QStringLiteral("cat.a"), QStringLiteral("debug msg")},
        {LogEntry::Info,     QStringLiteral("cat.b"), QStringLiteral("info msg")},
        {LogEntry::Warning,  QStringLiteral("cat.a"), QStringLiteral("warning msg")},
        {LogEntry::Critical, QStringLiteral("cat.c"), QStringLiteral("error 42")},
        {LogEntry::Info,     QStringLiteral("cat.b"), QStringLiteral("another info")},
    };
    for (const auto &d : data) {
        LogEntry entry;
        entry.level = d.level;
        entry.category = d.category;
        entry.message = d.message;
        entry.buildFormatted();
        model.enqueue(entry);
    }
    QTRY_COMPARE_WITH_TIMEOUT(model.totalCount(), 5, TestTimeout::shortMs());
}

void LogModelTest::_filterByLevel()
{
    LogModel model;
    populateWithMixedEntries(model);

    QCOMPARE(model.rowCount(), 5);

    model.setFilterLevel(LogEntry::Warning);
    QCOMPARE(model.rowCount(), 2);

    model.setFilterLevel(LogEntry::Critical);
    QCOMPARE(model.rowCount(), 1);
}

void LogModelTest::_filterByCategory()
{
    LogModel model;
    populateWithMixedEntries(model);

    model.setFilterCategory(QStringLiteral("cat.b"));
    QCOMPARE(model.rowCount(), 2);

    model.setFilterCategory(QStringLiteral("cat.c"));
    QCOMPARE(model.rowCount(), 1);
}

void LogModelTest::_filterByText()
{
    LogModel model;
    populateWithMixedEntries(model);

    model.setFilterText(QStringLiteral("info"));
    QCOMPARE(model.rowCount(), 2);

    model.setFilterText(QStringLiteral("42"));
    QCOMPARE(model.rowCount(), 1);
}

void LogModelTest::_filterByRegex()
{
    LogModel model;
    populateWithMixedEntries(model);

    model.setFilterRegex(true);
    model.setFilterText(QStringLiteral("info|warning"));
    QCOMPARE(model.rowCount(), 3);

    model.setFilterText(QStringLiteral("error \\d+"));
    QCOMPARE(model.rowCount(), 1);
}

void LogModelTest::_clearFilters()
{
    LogModel model;
    populateWithMixedEntries(model);

    model.setFilterLevel(LogEntry::Critical);
    QCOMPARE(model.rowCount(), 1);

    model.clearFilters();
    QCOMPARE(model.rowCount(), 5);
    QCOMPARE(model.filterLevel(), static_cast<int>(LogEntry::Debug));
    QVERIFY(model.filterCategory().isEmpty());
    QVERIFY(model.filterText().isEmpty());
    QVERIFY(!model.filterRegex());
}

void LogModelTest::_filteredEntries()
{
    LogModel model;
    populateWithMixedEntries(model);

    model.setFilterCategory(QStringLiteral("cat.a"));
    const auto entries = model.filteredEntries();
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries[0].category, QStringLiteral("cat.a"));
    QCOMPARE(entries[1].category, QStringLiteral("cat.a"));
}

void LogModelTest::_categoriesComboList()
{
    LogModel model;

    QStringList cats = model.categoriesList();
    QCOMPARE(cats.size(), 0);

    LogEntry e1;
    e1.level = LogEntry::Info;
    e1.category = QStringLiteral("cat.b");
    e1.message = QStringLiteral("msg1");
    e1.buildFormatted();
    model.enqueue(e1);

    LogEntry e2;
    e2.level = LogEntry::Info;
    e2.category = QStringLiteral("cat.a");
    e2.message = QStringLiteral("msg2");
    e2.buildFormatted();
    model.enqueue(e2);

    QTRY_COMPARE_WITH_TIMEOUT(model.totalCount(), 2, TestTimeout::shortMs());

    cats = model.categoriesList();
    QCOMPARE(cats.size(), 2);
    // Should be sorted
    QCOMPARE(cats[0], QStringLiteral("cat.a"));
    QCOMPARE(cats[1], QStringLiteral("cat.b"));
}

void LogModelTest::_setFilterTextDeferred()
{
    LogModel model;

    LogEntry e1;
    e1.level = LogEntry::Info;
    e1.category = QStringLiteral("test");
    e1.message = QStringLiteral("hello world");
    e1.buildFormatted();
    model.enqueue(e1);

    LogEntry e2;
    e2.level = LogEntry::Info;
    e2.category = QStringLiteral("test");
    e2.message = QStringLiteral("goodbye");
    e2.buildFormatted();
    model.enqueue(e2);

    QTRY_COMPARE_WITH_TIMEOUT(model.rowCount(), 2, TestTimeout::shortMs());

    // Deferred filter should not apply immediately
    QSignalSpy filterSpy(&model, &LogModel::filterTextChanged);
    model.setFilterTextDeferred(QStringLiteral("hello"));
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(filterSpy.count(), 0);

    // After debounce interval it should apply
    QTRY_COMPARE_WITH_TIMEOUT(filterSpy.count(), 1, TestTimeout::shortMs());
    QCOMPARE(model.filterText(), QStringLiteral("hello"));
    QCOMPARE(model.rowCount(), 1);

    // Immediate setFilterText should override pending deferred
    model.setFilterTextDeferred(QStringLiteral("xyz"));
    model.setFilterText(QStringLiteral("goodbye"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.filterText(), QStringLiteral("goodbye"));

    // Deferred should not overwrite since immediate already set. Flush pending
    // events so any buffered deferred-filter timer fires in-line, rather than
    // burning a fixed 300 ms QTRY spin.
    QCoreApplication::processEvents();
    QCOMPARE(model.filterText(), QStringLiteral("goodbye"));
}

void LogModelTest::_allEntriesSnapshot()
{
    LogModel model;
    populateWithMixedEntries(model);

    const auto snapshot = model.allEntriesSnapshot();
    QCOMPARE(snapshot.size(), 5);

    // Snapshot is a copy — clearing model doesn't affect it
    model.clear();
    QCOMPARE(model.totalCount(), 0);
    QCOMPARE(snapshot.size(), 5);
}

void LogModelTest::_invalidRegex()
{
    LogModel model;
    populateWithMixedEntries(model);

    model.setFilterRegex(true);
    QVERIFY(model.filterRegexValid());

    model.setFilterText(QStringLiteral("[invalid"));
    QVERIFY(!model.filterRegexValid());
    // Invalid regex matches nothing
    QCOMPARE(model.rowCount(), 0);

    // Switching back to non-regex should show results again
    model.setFilterRegex(false);
    QVERIFY(model.filterRegexValid());
}

void LogModelTest::_columnCount()
{
    LogModel model;
    QCOMPARE(model.columnCount(), static_cast<int>(LogEntry::ColumnCount));
    QVERIFY(model.columnCount() > 1);
}

void LogModelTest::_multiData()
{
    LogModel model;

    LogEntry entry;
    entry.level = LogEntry::Warning;
    entry.category = QStringLiteral("multi.test");
    entry.message = QStringLiteral("multi data");
    entry.timestamp = QDateTime::currentDateTime();
    entry.buildFormatted();
    model.enqueue(entry);

    QTRY_COMPARE_WITH_TIMEOUT(model.totalCount(), 1, TestTimeout::shortMs());

    const QModelIndex idx = model.index(0, 0);

    // Test multiData by fetching multiple roles at once
    QModelRoleData roleData[] = {
        QModelRoleData(LogEntry::LevelRole),
        QModelRoleData(LogEntry::MessageRole),
        QModelRoleData(LogEntry::CategoryRole),
    };
    QModelRoleDataSpan span(roleData);
    model.multiData(idx, span);

    QCOMPARE(roleData[0].data().toInt(), static_cast<int>(LogEntry::Warning));
    QCOMPARE(roleData[1].data().toString(), QStringLiteral("multi data"));
    QCOMPARE(roleData[2].data().toString(), QStringLiteral("multi.test"));
}

UT_REGISTER_TEST(LogModelTest, TestLabel::Unit, TestLabel::Utilities)
