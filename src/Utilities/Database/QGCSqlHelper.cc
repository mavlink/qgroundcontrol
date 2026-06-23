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

void applySqlitePragmas(QSqlDatabase& db)
{
    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    q.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
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

ScopedConnection::ScopedConnection(const QString& dbPath, bool readOnly, const QString& prefix)
{
    if (dbPath.isEmpty()) {
        return;
    }

    _connName = QStringLiteral("%1_%2").arg(prefix).arg(s_connId.fetch_add(1));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), _connName);
    db.setDatabaseName(dbPath);
    if (readOnly) {
        db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    }

    if (db.open()) {
        applySqlitePragmas(db);
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
