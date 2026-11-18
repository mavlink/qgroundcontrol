#pragma once

#include <QtCore/QString>
#include <QtSql/QSqlQuery>

#include "QGCSqlHelper.h"

/// Runs \a sql on the tile cache file at \a path, as another process would
inline bool tileCacheExec(const QString& path, const QString& sql)
{
    const QGCSqlHelper::ScopedConnection connection(path, /*readOnly=*/false, QStringLiteral("TileCacheTestExec"));
    QSqlQuery query(connection.database());
    return query.exec(sql);
}

/// The unused pages of the tile cache file at \a path, read as another process would. -1 when they cannot be read.
inline qint64 tileCacheFreePages(const QString& path)
{
    const QGCSqlHelper::ScopedConnection connection(path, /*readOnly=*/true, QStringLiteral("TileCacheTestPages"));
    QSqlQuery query(connection.database());
    return (query.exec(QStringLiteral("PRAGMA freelist_count")) && query.next()) ? query.value(0).toLongLong() : -1;
}
