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

// ── SQLite PRAGMA helpers ──────────────────────────────────────────────
/// Applies standard QGC pragmas: WAL journal mode, NORMAL synchronous,
/// foreign_keys = ON.
void applySqlitePragmas(QSqlDatabase& db);

/// PRAGMA user_version — read/write the SQLite schema version integer.
/// Returns std::nullopt on failure.
[[nodiscard]] std::optional<int> userVersion(QSqlDatabase& db);
[[nodiscard]] bool setUserVersion(QSqlDatabase& db, int v);

// ── Scoped connection RAII ─────────────────────────────────────────────
/// RAII wrapper around QSqlDatabase::addDatabase / removeDatabase.
/// Creates a uniquely-named connection on construction, removes it on
/// destruction. Avoids the common pitfall of reusing connection names
/// or forgetting to call removeDatabase.
class ScopedConnection
{
public:
    /// Opens a connection to \a dbPath using the QSQLITE driver.
    /// \a prefix is prepended to the auto-generated connection name; useful
    /// for distinguishing concurrent connections in logs / Qt's connection
    /// registry. If \a readOnly is true, QSQLITE_OPEN_READONLY is set.
    explicit ScopedConnection(const QString& dbPath, bool readOnly = false,
                              const QString& prefix = QStringLiteral("QGCSql"));
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
/// RAII wrapper around QSqlDatabase::transaction()/commit()/rollback().
/// Begins a transaction on construction; rolls back in destructor unless
/// commit() was called. Check ok() before issuing queries — begin can fail
/// (e.g. driver doesn't support transactions, or one is already open).
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
