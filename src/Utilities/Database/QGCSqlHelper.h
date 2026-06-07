#pragma once

#include <QtCore/QString>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include <atomic>
#include <optional>

/// Lightweight SQL utilities shared across QGC components.
namespace QGCSqlHelper {

// ── LIKE wildcard escaping ─────────────────────────────────────────────
/// Escapes SQL LIKE wildcards (%, _, \) so the string matches literally.
/// Use with "LIKE ? ESCAPE '\\'" in the query.
[[nodiscard]] QString escapeLikePattern(const QString& text);

// ── Bound-parameter placeholders ───────────────────────────────────────
/// Returns "?,?,?" with \a n placeholders for use in IN-clauses.
/// E.g. `"DELETE FROM T WHERE id IN (" + placeholders(n) + ")"`.
[[nodiscard]] QString placeholders(int n);

// ── Prepare + bind + exec ──────────────────────────────────────────────
/// Prepares \a sql on \a query, positionally binds \a binds in order, and
/// execs. Returns false if any step fails; on failure \a query retains the
/// failing error (query.lastError()). On success the caller may still use
/// \a query for result iteration / lastInsertId() / numRowsAffected().
/// Not for prepared-once-reused loops — re-preparing per call defeats that.
template <typename... Args>
[[nodiscard]] bool execPrepared(QSqlQuery& query, const QString& sql, const Args&... binds)
{
    if (!query.prepare(sql)) {
        return false;
    }
    (query.addBindValue(binds), ...);
    return query.exec();
}

// ── SQLite PRAGMA helpers ──────────────────────────────────────────────
/// Applies standard QGC pragmas: page_size (empty DBs only), auto_vacuum=INCREMENTAL
/// (before WAL so it binds on fresh DBs), WAL journal mode, NORMAL synchronous,
/// foreign_keys=ON, cache_size, temp_store=MEMORY and mmap_size. auto_vacuum only
/// takes effect on DBs created with it (or after a full VACUUM). \a mmapSize bytes of
/// address space are reserved per connection; the default (0) skips the mmap pragma —
/// opt in only for large read-heavy DBs (e.g. the tile cache). \a cacheSizeKb sets
/// cache_size (negative = KiB of RAM, the SQLite convention); pass a larger magnitude
/// for write-heavy connections. \a pageSize, when > 0, is issued first; SQLite applies
/// it only to a not-yet-created DB and ignores it otherwise.
void applySqlitePragmas(QSqlDatabase& db, qint64 mmapSize = 0, qint64 cacheSizeKb = -10000, int pageSize = 0);

/// Runs PRAGMA wal_checkpoint(TRUNCATE) to flush the WAL into the main DB and
/// shrink the -wal file back to zero. Returns the query success.
bool walCheckpointTruncate(QSqlDatabase& db);

/// Runs PRAGMA optimize — lets SQLite refresh stale query-planner statistics.
/// Cheap; intended to run on connection close. Returns the query success.
bool runOptimize(QSqlDatabase& db);

/// Runs PRAGMA incremental_vacuum(\a pages) to reclaim freed pages from an
/// auto_vacuum=INCREMENTAL database. Pass pages <= 0 (default) to reclaim all
/// free pages. No-op (returns true) on a DB without incremental auto_vacuum.
/// Returns the query success.
bool incrementalVacuum(QSqlDatabase& db, int pages = 0);

/// PRAGMA user_version — read/write the SQLite schema version integer.
/// Returns std::nullopt on failure.
[[nodiscard]] std::optional<int> userVersion(QSqlDatabase& db);
[[nodiscard]] bool setUserVersion(QSqlDatabase& db, int v);

// ── Scoped connection RAII ─────────────────────────────────────────────
/// \brief RAII wrapper around QSqlDatabase::addDatabase / removeDatabase.
///
/// Creates a uniquely-named connection on construction, removes it on
/// destruction. Avoids the common pitfall of reusing connection names
/// or forgetting to call removeDatabase.
///
class ScopedConnection
{
public:
    /// Opens a connection to \a dbPath using the QSQLITE driver.
    /// \a prefix is prepended to the auto-generated connection name; useful
    /// for distinguishing concurrent connections in logs / Qt's connection
    /// registry. If \a readOnly is true, QSQLITE_OPEN_READONLY is set.
    explicit ScopedConnection(const QString& dbPath, bool readOnly = false,
                              const QString& prefix = QStringLiteral("QGCSql"), qint64 mmapSize = 0);
    ~ScopedConnection();

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;
    ScopedConnection(ScopedConnection&&) = delete;
    ScopedConnection& operator=(ScopedConnection&&) = delete;

    [[nodiscard]] bool isValid() const { return _valid; }
    [[nodiscard]] QSqlDatabase database() const;

private:
    QString _connName;
    bool _valid = false;

    static std::atomic<int> s_connId;
};

// ── Transaction RAII ───────────────────────────────────────────────────
/// \brief RAII wrapper around QSqlDatabase::transaction()/commit()/rollback().
///
/// Begins a transaction on construction; rolls back in destructor unless
/// commit() was called. Check ok() before issuing queries — begin can fail
/// (e.g. driver doesn't support transactions, or one is already open).
///
class Transaction
{
public:
    explicit Transaction(QSqlDatabase db);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction(Transaction&&) = delete;
    Transaction& operator=(Transaction&&) = delete;

    [[nodiscard]] bool ok() const { return _active; }
    bool commit();

private:
    QSqlDatabase _db;
    bool _active = false;
    bool _committed = false;
};

} // namespace QGCSqlHelper
