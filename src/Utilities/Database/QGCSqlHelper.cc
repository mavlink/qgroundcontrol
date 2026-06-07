#include "QGCSqlHelper.h"

#include <QtCore/QLatin1Char>
#include <QtCore/QStringLiteral>
#include <QtSql/QSqlQuery>

namespace QGCSqlHelper {

QString escapeLikePattern(const QString& text)
{
    QString escaped = text;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('%'), QStringLiteral("\\%"));
    escaped.replace(QLatin1Char('_'), QStringLiteral("\\_"));
    return escaped;
}

QString placeholders(int n)
{
    if (n <= 0) {
        return {};
    }
    QString out;
    out.reserve((n * 2) - 1);
    out += QLatin1Char('?');
    for (int i = 1; i < n; ++i) {
        out += QLatin1Char(',');
        out += QLatin1Char('?');
    }
    return out;
}

void applySqlitePragmas(QSqlDatabase& db, qint64 mmapSize, qint64 cacheSizeKb, int pageSize)
{
    QSqlQuery q(db);
    // page_size only binds on a not-yet-created DB and must precede journal_mode=WAL;
    // SQLite silently ignores it on an existing DB (would need VACUUM to change).
    if (pageSize > 0) {
        q.exec(QStringLiteral("PRAGMA page_size=%1").arg(pageSize));
    }
    // auto_vacuum must be set before journal_mode=WAL to take effect on a fresh DB;
    // on an existing DB SQLite silently ignores it unless followed by a full VACUUM.
    q.exec(QStringLiteral("PRAGMA auto_vacuum=INCREMENTAL"));
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    q.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    q.exec(QStringLiteral("PRAGMA cache_size=%1").arg(cacheSizeKb));
    q.exec(QStringLiteral("PRAGMA temp_store=MEMORY"));
    // mmap_size reserves address space per connection; keep it modest by default and
    // raise it only for large read-heavy DBs (e.g. the tile cache) via the argument.
    if (mmapSize > 0) {
        q.exec(QStringLiteral("PRAGMA mmap_size=%1").arg(mmapSize));
    }
}

bool walCheckpointTruncate(QSqlDatabase& db)
{
    QSqlQuery q(db);
    return q.exec(QStringLiteral("PRAGMA wal_checkpoint(TRUNCATE)"));
}

bool runOptimize(QSqlDatabase& db)
{
    QSqlQuery q(db);
    return q.exec(QStringLiteral("PRAGMA optimize"));
}

bool incrementalVacuum(QSqlDatabase& db, int pages)
{
    QSqlQuery q(db);
    const QString sql = (pages > 0)
        ? QStringLiteral("PRAGMA incremental_vacuum(%1)").arg(pages)
        : QStringLiteral("PRAGMA incremental_vacuum");
    return q.exec(sql);
}

std::optional<int> userVersion(QSqlDatabase& db)
{
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("PRAGMA user_version")) || !q.next()) {
        return std::nullopt;
    }
    return q.value(0).toInt();
}

bool setUserVersion(QSqlDatabase& db, int v)
{
    QSqlQuery q(db);
    return q.exec(QStringLiteral("PRAGMA user_version = %1").arg(v));
}

// ── ScopedConnection ───────────────────────────────────────────────────

std::atomic<int> ScopedConnection::s_connId{0};

ScopedConnection::ScopedConnection(const QString& dbPath, bool readOnly, const QString& prefix, qint64 mmapSize)
{
    if (dbPath.isEmpty()) {
        return;
    }

    _connName = QStringLiteral("%1_%2").arg(prefix).arg(s_connId.fetch_add(1));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), _connName);
    db.setDatabaseName(dbPath);
    // Wait out a contended WAL writer lock instead of failing with SQLITE_BUSY.
    QString connectOptions = QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000");
    if (readOnly) {
        connectOptions += QStringLiteral(";QSQLITE_OPEN_READONLY");
    }
    db.setConnectOptions(connectOptions);

    if (db.open()) {
        applySqlitePragmas(db, mmapSize);
        _valid = true;
    }
}

ScopedConnection::~ScopedConnection()
{
    if (!_connName.isEmpty()) {
        {
            QSqlDatabase db = QSqlDatabase::database(_connName, false);
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(_connName);
    }
}

QSqlDatabase ScopedConnection::database() const
{
    return QSqlDatabase::database(_connName, false);
}

// ── Transaction ────────────────────────────────────────────────────────

// QSqlDatabase is a shallow handle; pass by value so the caller's connection
// remains valid for the transaction's lifetime regardless of caller scope.
Transaction::Transaction(QSqlDatabase db)  // NOLINT(performance-unnecessary-value-param)
    : _db(db)
    , _active(_db.transaction())
{
}

Transaction::~Transaction()
{
    if (_active && !_committed) {
        _db.rollback();
    }
}

bool Transaction::commit()
{
    if (!_active) {
        return false;
    }
    _committed = _db.commit();
    return _committed;
}

} // namespace QGCSqlHelper
