#include "LogStoreQueryModelTest.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "LogEntry.h"
#include "LogModel.h"
#include "LogStore.h"
#include "LogStoreQueryModel.h"
#include "UnitTestList.h"

void LogStoreQueryModelTest::populateStore(LogStore* store, int count, const QString& prefix,
                                           LogEntry::Level level, const QString& category)
{
    const qint64 before = store->entryCount();
    for (int i = 0; i < count; ++i) {
        store->append(makeEntry(QStringLiteral("%1-%2").arg(prefix).arg(i), level, category));
    }
    store->flush();
    QTRY_VERIFY_WITH_TIMEOUT(store->entryCount() >= before + count, TestTimeout::mediumMs());
}

void LogStoreQueryModelTest::_emptyModel()
{
    LogStoreQueryModel model(nullptr);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.totalResults(), 0);
    QCOMPARE(model.loading(), false);
    QVERIFY(model.sessionFilter().isEmpty());
    QCOMPARE(model.filterLevel(), static_cast<int>(LogEntry::Debug));
}

void LogStoreQueryModelTest::_refreshPopulates()
{
    LogStore* store = openStore();
    QVERIFY(store);

    populateStore(store, 5);

    LogStoreQueryModel model(store);
    model.setSessionFilter(store->sessionId());

    QTRY_VERIFY_WITH_TIMEOUT(model.rowCount() > 0, TestTimeout::shortMs());
    QCOMPARE(model.rowCount(), 5);

    // Verify data access
    const QModelIndex idx = model.index(0, 0);
    QVERIFY(model.data(idx, static_cast<int>(LogEntry::MessageRole)).toString().startsWith(QStringLiteral("msg-")));
    QCOMPARE(model.data(idx, static_cast<int>(LogEntry::LevelRole)).toInt(), static_cast<int>(LogEntry::Info));
}

void LogStoreQueryModelTest::_sessionFilter()
{
    LogStore* store = openStore();
    QVERIFY(store);

    populateStore(store, 3);
    const QString sessionId = store->sessionId();

    LogStoreQueryModel model(store);
    QSignalSpy sessionSpy(&model, &LogStoreQueryModel::sessionFilterChanged);

    model.setSessionFilter(sessionId);
    QCOMPARE(sessionSpy.count(), 1);

    QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), TestTimeout::shortMs());
    QCOMPARE(model.rowCount(), 3);

    // Filter to a non-existent session
    model.setSessionFilter(QStringLiteral("nonexistent"));
    QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), TestTimeout::shortMs());
    QCOMPARE(model.rowCount(), 0);
}

void LogStoreQueryModelTest::_levelFilter()
{
    LogStore* store = openStore();
    QVERIFY(store);

    store->append(makeEntry(QStringLiteral("debug msg"), LogEntry::Debug));
    store->append(makeEntry(QStringLiteral("info msg"), LogEntry::Info));
    store->append(makeEntry(QStringLiteral("warn msg"), LogEntry::Warning));
    store->flush();
    QTRY_COMPARE_WITH_TIMEOUT(store->entryCount(), 3, TestTimeout::shortMs());

    LogStoreQueryModel model(store);
    model.setSessionFilter(store->sessionId());
    QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), TestTimeout::shortMs());
    QCOMPARE(model.rowCount(), 3);

    model.setFilterLevel(LogEntry::Warning);
    QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), TestTimeout::shortMs());
    QCOMPARE(model.rowCount(), 1);
}

void LogStoreQueryModelTest::_categoryFilter()
{
    LogStore* store = openStore();
    QVERIFY(store);

    store->append(makeEntry(QStringLiteral("a"), LogEntry::Info, QStringLiteral("Alpha")));
    store->append(makeEntry(QStringLiteral("b"), LogEntry::Info, QStringLiteral("Beta")));
    store->append(makeEntry(QStringLiteral("c"), LogEntry::Info, QStringLiteral("Alpha")));
    store->flush();
    QTRY_COMPARE_WITH_TIMEOUT(store->entryCount(), 3, TestTimeout::shortMs());

    LogStoreQueryModel model(store);
    model.setSessionFilter(store->sessionId());
    QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), TestTimeout::shortMs());
    QCOMPARE(model.rowCount(), 3);

    model.setFilterCategory(QStringLiteral("Alpha"));
    QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), TestTimeout::shortMs());
    QCOMPARE(model.rowCount(), 2);
}

void LogStoreQueryModelTest::_textFilter()
{
    LogStore* store = openStore();
    QVERIFY(store);

    store->append(makeEntry(QStringLiteral("hello world")));
    store->append(makeEntry(QStringLiteral("goodbye world")));
    store->append(makeEntry(QStringLiteral("hello again")));
    store->flush();
    QTRY_COMPARE_WITH_TIMEOUT(store->entryCount(), 3, TestTimeout::shortMs());

    LogStoreQueryModel model(store);
    model.setSessionFilter(store->sessionId());
    QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), TestTimeout::shortMs());
    QCOMPARE(model.rowCount(), 3);

    model.setFilterText(QStringLiteral("hello"));
    QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), TestTimeout::shortMs());
    QCOMPARE(model.rowCount(), 2);
}

void LogStoreQueryModelTest::_availableSessions()
{
    LogStore* store = openStore();
    QVERIFY(store);

    populateStore(store, 1);

    LogStoreQueryModel model(store);
    const QStringList sessions = model.availableSessions();
    QVERIFY(sessions.contains(store->sessionId()));
}

void LogStoreQueryModelTest::_loadMore()
{
    LogStore* store = openStore();
    QVERIFY(store);

    // LogStoreQueryModel::kPageSize is 1000, so insert more than that
    populateStore(store, 1050);

    LogStoreQueryModel model(store);
    model.setSessionFilter(store->sessionId());
    QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), TestTimeout::shortMs());

    const int firstPage = model.rowCount();
    QVERIFY(firstPage > 0);

    // If pagination is active, loadMore should fetch additional entries
    if (model.totalResults() > firstPage) {
        model.loadMore();
        QTRY_VERIFY_WITH_TIMEOUT(!model.loading(), TestTimeout::shortMs());
        QVERIFY(model.rowCount() > firstPage);
    }

    // All entries should eventually be accessible
    QVERIFY(model.rowCount() >= 1000);
}

UT_REGISTER_TEST(LogStoreQueryModelTest, TestLabel::Unit, TestLabel::Utilities)
