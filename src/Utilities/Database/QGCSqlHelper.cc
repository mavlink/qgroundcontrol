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

void applySqlitePragmas(QSqlDatabase& db)
{
    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
}

// ── ScopedConnection ───────────────────────────────────────────────────

std::atomic<int> ScopedConnection::s_connId{0};

ScopedConnection::ScopedConnection(const QString& dbPath, bool readOnly)
{
    if (dbPath.isEmpty()) {
        return;
    }

    _connName = QStringLiteral("QGCSql_%1").arg(s_connId.fetch_add(1));

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

} // namespace QGCSqlHelper
