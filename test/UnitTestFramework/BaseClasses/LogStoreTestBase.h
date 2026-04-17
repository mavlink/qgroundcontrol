#pragma once

#include <QtCore/QDateTime>

#include <memory>
#include <vector>

#include "LogEntry.h"
#include "LogStore.h"
#include "TempDirectoryTest.h"

/// Test fixture for LogStore-based tests.
///
/// Provides helpers for opening a temporary database, creating LogEntry
/// objects, and populating stores. Inherits TempDirectoryTest for automatic
/// temp directory management.
///
/// Example:
/// @code
/// class MyLogStoreTest : public LogStoreTestBase
/// {
///     Q_OBJECT
/// private slots:
///     void _testInsert() {
///         LogStore* store = openStore();
///         QVERIFY(store);
///         store->insert(makeEntry("hello"));
///         // ...
///     }
/// };
/// @endcode
class LogStoreTestBase : public TempDirectoryTest
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(LogStoreTestBase)

public:
    explicit LogStoreTestBase(QObject* parent = nullptr) : TempDirectoryTest(parent)
    {
    }

protected:
    /// Opens a LogStore backed by a temp database file.
    /// Waits for the async open to complete before returning.
    /// @param dbName Database filename within the temp directory
    /// @return Pointer to the open store, or nullptr on failure
    LogStore* openStore(const QString& dbName = QStringLiteral("test.db"))
    {
        auto store = std::make_unique<LogStore>(this);
        store->open(tempPath(dbName));
        if (!waitForStoreOpen(store.get())) {
            return nullptr;
        }
        LogStore* ptr = store.get();
        _stores.push_back(std::move(store));
        return ptr;
    }

    /// Waits for a LogStore to finish its async open.
    static bool waitForStoreOpen(LogStore* store, int timeoutMs = TestTimeout::mediumMs())
    {
        return QTest::qWaitFor([store]() { return store->isOpen(); }, timeoutMs);
    }

    /// Factory for LogEntry with sensible defaults.
    static LogEntry makeEntry(const QString& message = QStringLiteral("msg"),
                              LogEntry::Level level = LogEntry::Info,
                              const QString& category = QStringLiteral("Test"))
    {
        LogEntry e;
        e.timestamp = QDateTime::currentDateTime();
        e.level = level;
        e.category = category;
        e.message = message;
        e.buildFormatted();
        return e;
    }

    /// Inserts @a count entries into the store.
    static void populateStore(LogStore* store, int count,
                              LogEntry::Level level = LogEntry::Info,
                              const QString& category = QStringLiteral("Test"))
    {
        for (int i = 0; i < count; ++i) {
            store->append(makeEntry(QStringLiteral("msg_%1").arg(i), level, category));
        }
    }

protected slots:
    void cleanup() override
    {
        _stores.clear();
        TempDirectoryTest::cleanup();
    }

private:
    std::vector<std::unique_ptr<LogStore>> _stores;
};
