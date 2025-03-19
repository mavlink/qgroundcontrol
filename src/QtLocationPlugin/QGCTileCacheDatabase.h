#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtSql/QSqlDatabase>
#include "QGCCachedTileSet.h"
#include "QGCCacheTile.h"
#include "QGCTile.h"

Q_DECLARE_LOGGING_CATEGORY(QGCTileCacheDatabaseLog)

class QGCTileCacheDatabase
{
public:
    explicit QGCTileCacheDatabase(const QString &dbPath);
    ~QGCTileCacheDatabase();

    bool connect();
    void disconnect();
    bool createDatabase(bool createDefault = true) const;

    bool deleteBingNoTileTiles() const;
    bool saveTile(QGCCacheTile *tile, quint64 &tileID, quint64 tileSetID) const;
    QGCCacheTile* fetchTile(const QString &hash) const;
    QList<QGCCachedTileSet*> fetchTileSets() const;
    bool createTileSet(QGCCachedTileSet *tileSet);
    QList<QGCTile*> getTileDownloadList(quint64 setID, int count) const;
    bool updateTileDownloadState(quint64 setID, const QString &hash, int state) const;
    bool pruneCache(qint64 amount) const;
    bool deleteTileSet(quint64 setID) const;
    bool renameTileSet(quint64 setID, const QString &newName) const;
    bool resetCacheDatabase() const;
    bool importSets(const QString &path, bool replace);
    bool exportSets(const QString &path, const QList<QGCCachedTileSet*> &sets) const;

    bool findTileSetID(const QString &name, quint64 &setID) const;
    quint64 getDefaultTileSet() const;
    bool updateTotals(quint32 &totalTiles, quint64 &totalSize, quint32 &defaultTiles, quint64 &defaultSize) const;

private:
    QSqlDatabase _db;
    const QString _databasePath;

    quint64 _findTile(const QString &hash) const;
    bool _createTables(bool createDefault) const;
};
