#pragma once

#include <QtCore/QString>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include <atomic>

/// Lightweight SQL utilities shared across QGC components.
namespace QGCSqlHelper {

// ── LIKE wildcard escaping ─────────────────────────────────────────────
/// Escapes SQL LIKE wildcards (%, _, \) so the string matches literally.
/// Use with "LIKE ? ESCAPE '\\'" in the query.
[[nodiscard]] QString escapeLikePattern(const QString& text);

// ── SQLite PRAGMA helpers ──────────────────────────────────────────────
/// Applies standard QGC pragmas: WAL journal mode + NORMAL synchronous.
void applySqlitePragmas(QSqlDatabase& db);

// ── Scoped connection RAII ─────────────────────────────────────────────
/// RAII wrapper around QSqlDatabase::addDatabase / removeDatabase.
/// Creates a uniquely-named connection on construction, removes it on
/// destruction. Avoids the common pitfall of reusing connection names
/// or forgetting to call removeDatabase.
class ScopedConnection
{
public:
    /// Opens a connection to \a dbPath using the QSQLITE driver.
    /// If \a readOnly is true, QSQLITE_OPEN_READONLY is set.
    explicit ScopedConnection(const QString& dbPath, bool readOnly = false);
    ~ScopedConnection();

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    [[nodiscard]] bool isValid() const { return _valid; }
    [[nodiscard]] QSqlDatabase database() const;

private:
    QString _connName;
    bool _valid = false;

    static std::atomic<int> s_connId;
};

} // namespace QGCSqlHelper
