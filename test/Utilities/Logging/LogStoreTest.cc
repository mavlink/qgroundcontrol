#include "LogStoreTest.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "UnitTestList.h"

#include "LogEntry.h"
#include "LogFormatter.h"
#include "LogStore.h"

static void appendAndWait(LogStore* store, const QList<LogEntry>& entries)
{
    const qint64 before = store->entryCount();
    for (const auto& e : entries) {
        store->append(e);
    }
    store->flush();
    QTRY_VERIFY_WITH_TIMEOUT(store->entryCount() >= before + entries.size(), TestTimeout::shortMs());
}

void LogStoreTest::_openAndClose()
{
    LogStore* store = openStore();
    QVERIFY(store);
    QVERIFY(!store->sessionId().isEmpty());

    store->close();
    QVERIFY(!store->isOpen());
}

void LogStoreTest::_appendAndQuery()
{
    LogStore* store = openStore();
    QVERIFY(store);

    appendAndWait(store, {
        makeEntry(QStringLiteral("first")),
        makeEntry(QStringLiteral("second")),
        makeEntry(QStringLiteral("third")),
    });

    LogStore::QueryParams params;
    params.sessionId = store->sessionId();
    const auto results = store->query(params);

    QCOMPARE(results.size(), 3);
    QCOMPARE(results[0].message, QStringLiteral("first"));
    QCOMPARE(results[1].message, QStringLiteral("second"));
    QCOMPARE(results[2].message, QStringLiteral("third"));
}

void LogStoreTest::_queryFilters()
{
    LogStore* store = openStore();
    QVERIFY(store);

    appendAndWait(store, {
        makeEntry(QStringLiteral("debug msg"), LogEntry::Debug, QStringLiteral("cat.a")),
        makeEntry(QStringLiteral("info msg"), LogEntry::Info, QStringLiteral("cat.b")),
        makeEntry(QStringLiteral("warn msg"), LogEntry::Warning, QStringLiteral("cat.a")),
    });

    LogStore::QueryParams params;
    params.sessionId = store->sessionId();
    params.minLevel = LogEntry::Warning;
    auto results = store->query(params);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].message, QStringLiteral("warn msg"));

    params.minLevel = LogEntry::Debug;
    params.category = QStringLiteral("cat.a");
    results = store->query(params);
    QCOMPARE(results.size(), 2);

    params.category.clear();
    params.textFilter = QStringLiteral("info");
    results = store->query(params);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].message, QStringLiteral("info msg"));
}

void LogStoreTest::_sessions()
{
    LogStore* store = openStore();
    QVERIFY(store);

    appendAndWait(store, {makeEntry(QStringLiteral("entry"))});

    const QStringList sessions = store->sessions();
    QVERIFY(sessions.contains(store->sessionId()));
}

void LogStoreTest::_deleteSession()
{
    LogStore* store = openStore();
    QVERIFY(store);

    const QString sessionId = store->sessionId();
    appendAndWait(store, {makeEntry(QStringLiteral("to delete"))});

    QVERIFY(store->sessionEntryCount(sessionId) > 0);
    QVERIFY(store->deleteSession(sessionId));
    QCOMPARE(store->sessionEntryCount(sessionId), 0);
}

void LogStoreTest::_exportSession()
{
    LogStore* store = openStore();
    QVERIFY(store);

    appendAndWait(store, {makeEntry(QStringLiteral("export me"))});

    const QString exportPath = tempPath(QStringLiteral("export.txt"));

    QSignalSpy exportSpy(store, &LogStore::exportFinished);
    store->exportSession(store->sessionId(), exportPath, LogFormatter::PlainText);
    QVERIFY(exportSpy.wait(TestTimeout::mediumMs()));
    QCOMPARE(exportSpy.first().first().toBool(), true);

    QFile file(exportPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray content = file.readAll();
    QVERIFY(content.contains("export me"));
}

void LogStoreTest::_entryCount()
{
    LogStore* store = openStore();
    QVERIFY(store);

    QCOMPARE(store->entryCount(), 0);

    appendAndWait(store, {
        makeEntry(QStringLiteral("0")),
        makeEntry(QStringLiteral("1")),
        makeEntry(QStringLiteral("2")),
        makeEntry(QStringLiteral("3")),
        makeEntry(QStringLiteral("4")),
    });

    QCOMPARE(store->entryCount(), 5);
}

void LogStoreTest::_exportSessionEmpty()
{
    LogStore* store = openStore();
    QVERIFY(store);

    const QString exportPath = tempPath(QStringLiteral("empty_export.txt"));

    // Export a non-existent session should signal failure
    QSignalSpy exportSpy(store, &LogStore::exportFinished);
    store->exportSession(QStringLiteral("nonexistent"), exportPath, 0);
    QVERIFY(exportSpy.wait(TestTimeout::mediumMs()));
    QCOMPARE(exportSpy.first().first().toBool(), false);

    // File should not exist
    QVERIFY(!QFile::exists(exportPath));
}

UT_REGISTER_TEST(LogStoreTest, TestLabel::Unit, TestLabel::Utilities)
