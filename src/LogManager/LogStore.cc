#include "LogStore.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QMutexLocker>
#include <QtCore/QThread>
#include <QtCore/QUuid>

#include <optional>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "LogFormatter.h"
#include "QGCLoggingCategory.h"
#include "QGCSqlHelper.h"

QGC_LOGGING_CATEGORY(LogStoreLog, "Utilities.LogStore")

static std::atomic<int> s_instanceCounter{0};

LogStore::LogStore(QObject* parent)
    : QObject(parent), _sessionId(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8))
{
    const int id = s_instanceCounter.fetch_add(1);
    _writeConnName = QStringLiteral("LogStore_write_%1").arg(id);
}

LogStore::~LogStore()
{
    close();
    if (_exportFuture.isValid()) {
        _exportFuture.waitForFinished();
    }
}

QString LogStore::databasePath() const
{
    const QMutexLocker locker(&_mutex);
    return _dbPath;
}

void LogStore::open(const QString& dbPath)
{
    if (_isOpen.load(std::memory_order_relaxed)) {
        close();
    }

    {
        const QMutexLocker locker(&_mutex);
        _dbPath = dbPath;
    }

    _startWorker();
}

void LogStore::close()
{
    _stopWorker();

    const QMutexLocker locker(&_mutex);
    _dbPath.clear();
    _isOpen.store(false, std::memory_order_relaxed);
    emit isOpenChanged();
}

void LogStore::append(LogEntry entry)
{
    if (!_isOpen.load(std::memory_order_relaxed)) {
        return;
    }

    bool wakeWorker = false;
    {
        const QMutexLocker locker(&_mutex);
        _pendingWrites.push_back(std::move(entry));
        wakeWorker = (static_cast<int>(_pendingWrites.size()) >= kBatchSize);
    }

    if (wakeWorker) {
        _condition.wakeOne();
    }
}

void LogStore::flush()
{
    const QMutexLocker locker(&_mutex);
    _condition.wakeOne();
}

// ---------------------------------------------------------------------------
// Scoped read connection (delegates to QGCSqlHelper::ScopedConnection)
// ---------------------------------------------------------------------------

class LogStore::ScopedReadConnection
{
public:
    explicit ScopedReadConnection(const LogStore& store)
    {
        const QMutexLocker locker(&store._mutex);
        if (!store._dbPath.isEmpty()) {
            _conn.emplace(store._dbPath, true);
        }
    }

    [[nodiscard]] bool isValid() const { return _conn && _conn->isValid(); }
    [[nodiscard]] QSqlDatabase database() const { return _conn->database(); }

private:
    std::optional<QGCSqlHelper::ScopedConnection> _conn;
};

// ---------------------------------------------------------------------------
// Query methods (use persistent read connection, no mutex during SQL)
// ---------------------------------------------------------------------------

QList<LogEntry> LogStore::query(const QueryParams& params) const
{
    QList<LogEntry> result;

    if (!_isOpen.load(std::memory_order_relaxed)) {
        return result;
    }

    ScopedReadConnection conn(*this);
    if (!conn.isValid()) {
        return result;
    }

    QSqlDatabase db = conn.database();

    QString sql;
    sql.reserve(256);
    sql += QStringLiteral(
        "SELECT timestamp, level, category, message, file, function, line, formatted "
        "FROM log_entries WHERE 1=1");

    // Build parameterized WHERE clause; bind values added in order below
    QSqlQuery q(db);

    if (!params.sessionId.isEmpty()) {
        sql += QStringLiteral(" AND session_id = ?");
    }
    if (params.fromTime.isValid()) {
        sql += QStringLiteral(" AND timestamp >= ?");
    }
    if (params.toTime.isValid()) {
        sql += QStringLiteral(" AND timestamp <= ?");
    }
    if (params.minLevel > LogEntry::Debug) {
        sql += QStringLiteral(" AND level >= ?");
    }
    if (!params.category.isEmpty()) {
        sql += QStringLiteral(" AND category LIKE ? ESCAPE '\\'");
    }
    if (!params.textFilter.isEmpty()) {
        sql += QStringLiteral(" AND message LIKE ? ESCAPE '\\'");
    }

    sql += QStringLiteral(" ORDER BY id ASC LIMIT ? OFFSET ?");
    q.prepare(sql);

    // Bind values in same order as WHERE clauses above
    if (!params.sessionId.isEmpty()) {
        q.addBindValue(params.sessionId);
    }
    if (params.fromTime.isValid()) {
        q.addBindValue(params.fromTime.toMSecsSinceEpoch());
    }
    if (params.toTime.isValid()) {
        q.addBindValue(params.toTime.toMSecsSinceEpoch());
    }
    if (params.minLevel > LogEntry::Debug) {
        q.addBindValue(params.minLevel);
    }
    if (!params.category.isEmpty()) {
        q.addBindValue(QLatin1Char('%') + QGCSqlHelper::escapeLikePattern(params.category) + QLatin1Char('%'));
    }
    if (!params.textFilter.isEmpty()) {
        q.addBindValue(QLatin1Char('%') + QGCSqlHelper::escapeLikePattern(params.textFilter) + QLatin1Char('%'));
    }
    q.addBindValue(params.limit);
    q.addBindValue(params.offset);

    if (q.exec()) {
        while (q.next()) {
            LogEntry entry;
            entry.timestamp = QDateTime::fromMSecsSinceEpoch(q.value(0).toLongLong());
            entry.level = static_cast<LogEntry::Level>(q.value(1).toInt());
            entry.category = q.value(2).toString();
            entry.message = q.value(3).toString();
            entry.file = q.value(4).toString();
            entry.function = q.value(5).toString();
            entry.line = q.value(6).toInt();
            entry.formatted = q.value(7).toString();
            result.append(std::move(entry));
        }
    }

    return result;
}

QStringList LogStore::sessions() const
{
    QStringList result;
    if (!_isOpen.load(std::memory_order_relaxed)) {
        return result;
    }

    ScopedReadConnection conn(*this);
    if (!conn.isValid()) {
        return result;
    }

    QSqlQuery q(conn.database());
    q.exec(QStringLiteral("SELECT DISTINCT session_id FROM log_entries ORDER BY id ASC"));
    while (q.next()) {
        result << q.value(0).toString();
    }

    return result;
}

qint64 LogStore::sessionEntryCount(const QString& sessionId) const
{
    if (!_isOpen.load(std::memory_order_relaxed)) {
        return 0;
    }

    ScopedReadConnection conn(*this);
    if (!conn.isValid()) {
        return 0;
    }

    QSqlQuery q(conn.database());
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM log_entries WHERE session_id = ?"));
    q.addBindValue(sessionId);
    if (q.exec() && q.next()) {
        return q.value(0).toLongLong();
    }

    return 0;
}

bool LogStore::deleteSession(const QString& sessionId)
{
    if (!_isOpen.load(std::memory_order_relaxed)) {
        return false;
    }

    QString dbPath;
    {
        const QMutexLocker locker(&_mutex);
        dbPath = _dbPath;
    }

    QGCSqlHelper::ScopedConnection conn(dbPath);
    if (!conn.isValid()) {
        return false;
    }

    QSqlQuery q(conn.database());
    q.prepare(QStringLiteral("DELETE FROM log_entries WHERE session_id = ?"));
    q.addBindValue(sessionId);
    return q.exec();
}

void LogStore::exportSession(const QString& sessionId, const QString& destFile, int format)
{
    if (!_isOpen.load(std::memory_order_relaxed)) {
        emit exportFinished(false);
        return;
    }

    QPointer<LogStore> guard(this);
    _exportFuture = QtConcurrent::run([guard, sessionId, destFile, format]() {
        if (!guard) {
            return;
        }

        LogStore::QueryParams params;
        params.sessionId = sessionId;
        params.limit = 1000000;
        const auto entries = guard->query(params);

        bool success = false;
        if (!entries.isEmpty()) {
            QFile file(destFile);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                file.write(LogFormatter::format(entries, format));
                success = true;
            }
        }

        if (guard) {
            QMetaObject::invokeMethod(
                guard.data(), [guard, success]() {
                    if (guard) {
                        emit guard->exportFinished(success);
                    }
                },
                Qt::QueuedConnection);
        }
    });
}

// ---------------------------------------------------------------------------
// Worker thread
// ---------------------------------------------------------------------------

void LogStore::_bindAndExec(QSqlQuery& q, const QString& sessionId, const LogEntry& entry)
{
    q.addBindValue(sessionId);
    q.addBindValue(entry.timestamp.toMSecsSinceEpoch());
    q.addBindValue(static_cast<int>(entry.level));
    q.addBindValue(entry.category);
    q.addBindValue(entry.message);
    q.addBindValue(entry.file);
    q.addBindValue(entry.function);
    q.addBindValue(entry.line);
    q.addBindValue(entry.formatted);
    q.exec();
}

void LogStore::_startWorker()
{
    const QMutexLocker locker(&_mutex);
    if (_thread) {
        return;
    }

    _quit.store(false, std::memory_order_relaxed);
    _thread = QThread::create([this]() { _workerLoop(); });
    _thread->setObjectName(QStringLiteral("LogStore"));
    _thread->start();
}

void LogStore::_stopWorker()
{
    {
        const QMutexLocker locker(&_mutex);
        if (!_thread) {
            return;
        }
        _quit.store(true, std::memory_order_relaxed);
    }
    _condition.wakeOne();
    _thread->wait();
    _thread->deleteLater();

    const QMutexLocker locker(&_mutex);
    _thread = nullptr;
}

void LogStore::_workerLoop()
{
    QString dbPath;
    {
        const QMutexLocker locker(&_mutex);
        dbPath = _dbPath;
    }

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), _writeConnName);
        db.setDatabaseName(dbPath);

        if (!db.open()) {
            QMetaObject::invokeMethod(
                this, [this, err = db.lastError().text()]() { emit errorOccurred(err); }, Qt::QueuedConnection);
            QSqlDatabase::removeDatabase(_writeConnName);
            return;
        }

        {
            QSqlQuery q(db);
            q.exec(
                QStringLiteral("CREATE TABLE IF NOT EXISTS log_entries ("
                               "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                               "  session_id TEXT NOT NULL,"
                               "  timestamp INTEGER NOT NULL,"
                               "  level INTEGER NOT NULL,"
                               "  category TEXT,"
                               "  message TEXT,"
                               "  file TEXT,"
                               "  function TEXT,"
                               "  line INTEGER DEFAULT 0,"
                               "  formatted TEXT"
                               ")"));
            q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_session ON log_entries(session_id)"));
            q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_timestamp ON log_entries(timestamp)"));
            q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_level ON log_entries(level)"));
            q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_category ON log_entries(category)"));
        }
        QGCSqlHelper::applySqlitePragmas(db);

        _isOpen.store(true, std::memory_order_relaxed);
        QMetaObject::invokeMethod(
            this,
            [this]() {
                emit isOpenChanged();
                emit databasePathChanged();
            },
            Qt::QueuedConnection);

        QSqlQuery insertQuery(db);
        insertQuery.prepare(QStringLiteral(
            "INSERT INTO log_entries (session_id, timestamp, level, category, message, file, function, line, formatted) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));

        while (!_quit.load(std::memory_order_relaxed)) {
            std::vector<LogEntry> batch;

            {
                QMutexLocker locker(&_mutex);
                while (_pendingWrites.empty() && !_quit.load(std::memory_order_relaxed)) {
                    _condition.wait(&_mutex, kFlushIntervalMs);
                    if (!_pendingWrites.empty()) {
                        break;
                    }
                }
                batch.swap(_pendingWrites);
            }

            if (batch.empty()) {
                continue;
            }

            QGCSqlHelper::Transaction txn(db);
            bool batchOk = txn.ok();
            for (const auto& entry : batch) {
                if (!batchOk) {
                    break;
                }
                _bindAndExec(insertQuery, _sessionId, entry);
                if (insertQuery.lastError().isValid()) {
                    batchOk = false;
                }
            }
            if (batchOk) {
                txn.commit();
            } else {
                // Transaction destructor rolls back if commit() wasn't called.
                QMetaObject::invokeMethod(
                    this,
                    [this, err = insertQuery.lastError().text()]() { emit errorOccurred(err); },
                    Qt::QueuedConnection);
            }

            _entryCount.fetch_add(static_cast<qint64>(batch.size()), std::memory_order_release);
            QMetaObject::invokeMethod(this, [this]() { emit entryCountChanged(); }, Qt::QueuedConnection);
        }

        // Drain remaining
        {
            const QMutexLocker locker(&_mutex);
            if (!_pendingWrites.empty()) {
                QGCSqlHelper::Transaction txn(db);
                for (const auto& entry : _pendingWrites) {
                    _bindAndExec(insertQuery, _sessionId, entry);
                }
                txn.commit();
                _pendingWrites.clear();
            }
        }

        db.close();
    }
    // All QSqlQuery objects are destroyed — safe to remove the connection.
    QSqlDatabase::removeDatabase(_writeConnName);
    _isOpen.store(false, std::memory_order_relaxed);
}
