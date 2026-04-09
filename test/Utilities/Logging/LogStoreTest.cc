#include "LogStoreTest.h"

#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "UnitTestList.h"

#include "LogEntry.h"
#include "LogFormatter.h"
#include "LogStore.h"

static LogEntry makeEntry(const QString& msg, LogEntry::Level level = LogEntry::Info,
                          const QString& cat = QStringLiteral("test.store"))
{
    LogEntry e;
    e.timestamp = QDateTime::currentDateTime();
    e.level = level;
    e.category = cat;
    e.message = msg;
    e.buildFormatted();
    return e;
}

static void waitForOpen(LogStore& store)
{
    QTRY_VERIFY_WITH_TIMEOUT(store.isOpen(), 3000);
}

static void appendAndWait(LogStore& store, const QList<LogEntry>& entries)
{
    const qint64 before = store.entryCount();
    for (const auto& e : entries) {
        store.append(e);
    }
    store.flush();
    QTRY_VERIFY_WITH_TIMEOUT(store.entryCount() >= before + entries.size(), 3000);
}

void LogStoreTest::_openAndClose()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    LogStore store;
    QVERIFY(!store.isOpen());

    store.open(dir.filePath(QStringLiteral("test.db")));
    waitForOpen(store);
    QVERIFY(!store.sessionId().isEmpty());

    store.close();
    QVERIFY(!store.isOpen());
}

void LogStoreTest::_appendAndQuery()
{
    QTemporaryDir dir;
    LogStore store;
    store.open(dir.filePath(QStringLiteral("test.db")));
    waitForOpen(store);

    appendAndWait(store, {
        makeEntry(QStringLiteral("first")),
        makeEntry(QStringLiteral("second")),
        makeEntry(QStringLiteral("third")),
    });

    LogStore::QueryParams params;
    params.sessionId = store.sessionId();
    const auto results = store.query(params);

    QCOMPARE(results.size(), 3);
    QCOMPARE(results[0].message, QStringLiteral("first"));
    QCOMPARE(results[1].message, QStringLiteral("second"));
    QCOMPARE(results[2].message, QStringLiteral("third"));
}

void LogStoreTest::_queryFilters()
{
    QTemporaryDir dir;
    LogStore store;
    store.open(dir.filePath(QStringLiteral("test.db")));
    waitForOpen(store);

    appendAndWait(store, {
        makeEntry(QStringLiteral("debug msg"), LogEntry::Debug, QStringLiteral("cat.a")),
        makeEntry(QStringLiteral("info msg"), LogEntry::Info, QStringLiteral("cat.b")),
        makeEntry(QStringLiteral("warn msg"), LogEntry::Warning, QStringLiteral("cat.a")),
    });

    LogStore::QueryParams params;
    params.sessionId = store.sessionId();
    params.minLevel = LogEntry::Warning;
    auto results = store.query(params);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].message, QStringLiteral("warn msg"));

    params.minLevel = LogEntry::Debug;
    params.category = QStringLiteral("cat.a");
    results = store.query(params);
    QCOMPARE(results.size(), 2);

    params.category.clear();
    params.textFilter = QStringLiteral("info");
    results = store.query(params);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].message, QStringLiteral("info msg"));
}

void LogStoreTest::_sessions()
{
    QTemporaryDir dir;
    LogStore store;
    store.open(dir.filePath(QStringLiteral("test.db")));
    waitForOpen(store);

    appendAndWait(store, {makeEntry(QStringLiteral("entry"))});

    const QStringList sessions = store.sessions();
    QVERIFY(sessions.contains(store.sessionId()));
}

void LogStoreTest::_deleteSession()
{
    QTemporaryDir dir;
    const QString dbPath = dir.filePath(QStringLiteral("test.db"));

    LogStore store;
    store.open(dbPath);
    waitForOpen(store);

    const QString sessionId = store.sessionId();
    appendAndWait(store, {makeEntry(QStringLiteral("to delete"))});

    QVERIFY(store.sessionEntryCount(sessionId) > 0);
    QVERIFY(store.deleteSession(sessionId));
    QCOMPARE(store.sessionEntryCount(sessionId), 0);
}

void LogStoreTest::_exportSession()
{
    QTemporaryDir dir;
    LogStore store;
    store.open(dir.filePath(QStringLiteral("test.db")));
    waitForOpen(store);

    appendAndWait(store, {makeEntry(QStringLiteral("export me"))});

    const QString exportPath = dir.filePath(QStringLiteral("export.txt"));

    QSignalSpy exportSpy(&store, &LogStore::exportFinished);
    store.exportSession(store.sessionId(), exportPath, LogFormatter::PlainText);
    QVERIFY(exportSpy.wait(5000));
    QCOMPARE(exportSpy.first().first().toBool(), true);

    QFile file(exportPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray content = file.readAll();
    QVERIFY(content.contains("export me"));
}

void LogStoreTest::_entryCount()
{
    QTemporaryDir dir;
    LogStore store;
    store.open(dir.filePath(QStringLiteral("test.db")));
    waitForOpen(store);

    QCOMPARE(store.entryCount(), 0);

    appendAndWait(store, {
        makeEntry(QStringLiteral("0")),
        makeEntry(QStringLiteral("1")),
        makeEntry(QStringLiteral("2")),
        makeEntry(QStringLiteral("3")),
        makeEntry(QStringLiteral("4")),
    });

    QCOMPARE(store.entryCount(), 5);
}

void LogStoreTest::_exportSessionEmpty()
{
    QTemporaryDir dir;
    LogStore store;
    store.open(dir.filePath(QStringLiteral("test.db")));
    waitForOpen(store);

    const QString exportPath = dir.filePath(QStringLiteral("empty_export.txt"));

    // Export a non-existent session should signal failure
    QSignalSpy exportSpy(&store, &LogStore::exportFinished);
    store.exportSession(QStringLiteral("nonexistent"), exportPath, 0);
    QVERIFY(exportSpy.wait(5000));
    QCOMPARE(exportSpy.first().first().toBool(), false);

    // File should not exist
    QVERIFY(!QFile::exists(exportPath));
}

UT_REGISTER_TEST(LogStoreTest, TestLabel::Unit, TestLabel::Utilities)
