#pragma once

#include <functional>
#include <memory>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>

#include "CacheTask.h"
#include "CacheTasks.h"
#include "CacheTile.h"
#include "CacheWorker.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "QGCSqlHelper.h"
#include "TileCache.h"
#include "TileCacheDatabase.h"
#include "UnitTest.h"

/// The provider of the tiles that the tests of the tile cache store, and its id
inline const QString kTileCacheTestProviderType = QStringLiteral("Bing Road");
inline constexpr int kTileCacheTestMapId = 6;

/// The provider that fetches from the URL template the user sets
inline const QString kTileCacheCustomUrlType = QStringLiteral("CustomURL Custom");
inline const QString kTileCacheTestSourceA = QStringLiteral("https://a.example/{z}/{x}/{y}.png");
inline const QString kTileCacheTestSourceB = QStringLiteral("https://b.example/{z}/{x}/{y}.png");

inline constexpr int kTileCacheLargeTileSize = 64 * 1024;

/// Queues a task for the cache under test: for its worker, or for the cache itself
using TileCacheAddTask = std::function<bool(CacheTask*)>;

/// Queues \a task for the tile cache of the process
inline bool tileCacheProcessAddTask(CacheTask* task)
{
    return TileCache::instance()->addTask(task);
}

/// Runs \a sql on the tile cache file at \a path, as another process would
inline bool tileCacheExec(const QString& path, const QString& sql)
{
    const QGCSqlHelper::ScopedConnection connection(path, /*readOnly=*/false, QStringLiteral("TileCacheTestExec"));
    QSqlQuery query(connection.database());
    return query.exec(sql);
}

/// The rows that \a sql gives on the tile cache file at \a path, read as another process would: the values of a row
/// are joined with '|'. The error as the only row when the statement fails.
inline QStringList tileCacheRows(const QString& path, const QString& sql)
{
    QStringList rows;
    const QGCSqlHelper::ScopedConnection connection(path, /*readOnly=*/true, QStringLiteral("TileCacheTestRows"));
    QSqlQuery query(connection.database());
    if (!query.exec(sql)) {
        return {query.lastError().text()};
    }
    while (query.next()) {
        QStringList values;
        for (int i = 0; i < query.record().count(); i++) {
            values.append(query.value(i).toString());
        }
        rows.append(values.join(QLatin1Char('|')));
    }
    return rows;
}

/// Makes an MBTiles file at \a path, as other programs write them. \a metadataRows are "('name', 'value'), ..." or
/// empty. \a tileRows are "(zoom, column, row, x'picture in hex'), ...", whose rows count from the south.
inline bool tileCacheCreateMBTiles(const QString& path, const QString& metadataRows, const QString& tileRows)
{
    return tileCacheExec(path, QStringLiteral("CREATE TABLE metadata (name TEXT, value TEXT)")) &&
           (metadataRows.isEmpty() ||
            tileCacheExec(path, QStringLiteral("INSERT INTO metadata VALUES ") + metadataRows)) &&
           tileCacheExec(path, QStringLiteral("CREATE TABLE tiles (zoom_level INTEGER, tile_column INTEGER, "
                                              "tile_row INTEGER, tile_data BLOB)")) &&
           tileCacheExec(path, QStringLiteral("INSERT INTO tiles VALUES ") + tileRows);
}

/// The unused pages of the tile cache file at \a path, read as another process would. -1 when they cannot be read.
inline qint64 tileCacheFreePages(const QString& path)
{
    const QGCSqlHelper::ScopedConnection connection(path, /*readOnly=*/true, QStringLiteral("TileCacheTestPages"));
    QSqlQuery query(connection.database());
    return (query.exec(QStringLiteral("PRAGMA freelist_count")) && query.next()) ? query.value(0).toLongLong() : -1;
}

inline SharedMapProvider tileCacheCustomUrlProvider()
{
    return MapProviderRegistry::getMapProviderFromProviderType(kTileCacheCustomUrlType);
}

/// A cache of schema version 2, where the key of a tile did not tell its source, with a tile of the custom URL
/// provider. Version 2 has the tables of today.
inline bool createVersion2TileCacheWithCustomUrlTile(const QString& path, int x, int y, int z, const QByteArray& image)
{
    TileCacheDatabase db(path);
    if (!db.init() || !db.connectDB() ||
        !db.saveTile(tileCacheCustomUrlProvider()->getTileHash(x, y, z, QString()), QStringLiteral("png"), image,
                     tileCacheCustomUrlProvider()->mapId(), TileCacheDatabase::kInvalidTileSet)) {
        return false;
    }
    QSqlQuery query(db.database());
    return query.exec(QStringLiteral("PRAGMA user_version = 2"));
}

/// Adds a tile set with tiles that take hundreds of pages together to the cache at \a path. Its tiles go to the
/// default set instead when \a setID is null. With \a deleteSet the set is deleted again, which leaves its pages
/// unused in the file.
inline bool tileCacheAddLargeTileSet(const QString& path, const QString& name, int tileCount, quint64* setID,
                                     bool deleteSet = false)
{
    TileCacheDatabase db(path);
    if (!db.init() || !db.connectDB()) {
        return false;
    }

    quint64 id = TileCacheDatabase::kInvalidTileSet;
    if (setID) {
        QSqlQuery query(db.database());
        if (!query.prepare(QStringLiteral("INSERT INTO TileSets(name) VALUES(?)"))) {
            return false;
        }
        query.addBindValue(name);
        if (!query.exec()) {
            return false;
        }
        id = query.lastInsertId().toULongLong();
        *setID = id;
    }

    const QByteArray image(kTileCacheLargeTileSize, 'T');
    for (int i = 0; i < tileCount; i++) {
        if (!db.saveTile(QStringLiteral("%1_%2").arg(name).arg(i), QStringLiteral("png"), image, kTileCacheTestMapId,
                         id)) {
            return false;
        }
    }
    return !deleteSet || db.deleteTileSet(id);
}

/// A task that the worker has nothing to do for
inline CacheTask* tileCacheIdleTask()
{
    return new CacheTask(QStringLiteral("Idle"), [](CacheWorker&, CacheTask&) { return QString(); });
}

/// The tile the cache has for \a hash, asked through \a addTask. None for a tile the cache lacks, and for a question
/// that got no answer. The answer arrives on the thread of \a context.
inline std::optional<CacheTile> tileCacheFetchTile(const QObject* context, const TileCacheAddTask& addTask,
                                                   const QString& hash)
{
    const auto answer = std::make_shared<std::optional<std::optional<CacheTile>>>();
    CacheTask* const task = CacheTasks::fetchTile(hash, context, [answer](const CacheTile* tile) {
        *answer = tile ? std::optional<CacheTile>(*tile) : std::nullopt;
    });
    if (!addTask(task)) {
        return std::nullopt;
    }

    (void) UnitTest::waitForCondition([answer] { return answer->has_value(); }, TestTimeout::mediumMs(),
                                      QStringLiteral("answer of the tile cache"));
    return answer->value_or(std::nullopt);
}

/// Queues \a task through \a addTask and waits for its end, which arrives on the thread of \a context. The error of
/// the task, or none for a task that got no end.
inline std::optional<QString> tileCacheRunTask(const QObject* context, const TileCacheAddTask& addTask, CacheTask* task)
{
    const auto error = std::make_shared<std::optional<QString>>();
    (void) QObject::connect(task, &CacheTask::finished, context,
                            [error](const QString& taskError) { *error = taskError; });
    if (!addTask(task)) {
        return std::nullopt;
    }

    (void) UnitTest::waitForCondition([error] { return error->has_value(); }, TestTimeout::mediumMs(),
                                      QStringLiteral("end of the task"));
    return *error;
}

/// The rows that \a sql gives on the database of the cache behind \a addTask, read by its worker: the columns of a
/// row are joined with '|'. None when the statement fails.
inline std::optional<QStringList> tileCacheWorkerRows(const QObject* context, const TileCacheAddTask& addTask,
                                                      const QString& sql)
{
    const auto rows = std::make_shared<QStringList>();
    CacheTask* const task = new CacheTask(QStringLiteral("Test Query"), [sql, rows](CacheWorker& worker, CacheTask&) {
        QSqlQuery query(worker.database().database());
        if (!query.exec(sql)) {
            return QStringLiteral("query failed");
        }
        while (query.next()) {
            QStringList columns;
            for (int i = 0; i < query.record().count(); i++) {
                columns.append(query.value(i).toString());
            }
            rows->append(columns.join(QLatin1Char('|')));
        }
        return QString();
    });
    return (tileCacheRunTask(context, addTask, task) == QString()) ? std::optional<QStringList>(*rows) : std::nullopt;
}
