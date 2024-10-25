#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QQueue>

#include "QGCTile.h"

class QGCCacheTile;
class QGCCachedTileSet;
class QSqlDatabase;

Q_DECLARE_LOGGING_CATEGORY(QGCTileCacheDBLog)

/*===========================================================================*/

namespace SetTilesTableModel
{
    bool create(std::shared_ptr<QSqlDatabase> db);
    bool selectFromSetID(std::shared_ptr<QSqlDatabase> db, quint64 setID);
    bool insertSetTiles(std::shared_ptr<QSqlDatabase> db, quint64 setID, quint64 tileID);
    bool deleteTileSet(std::shared_ptr<QSqlDatabase> db, quint64 setID);
    bool drop(std::shared_ptr<QSqlDatabase> db);
};

/*===========================================================================*/

namespace TileSetsTableModel
{
    bool create(std::shared_ptr<QSqlDatabase> db);
    bool insertTileSet(std::shared_ptr<QSqlDatabase> db, const QGCCachedTileSet &tileSet, quint64 &setID);
    bool getTileSets(std::shared_ptr<QSqlDatabase> db, QList<QGCCachedTileSet*> &tileSets);
    bool setName(std::shared_ptr<QSqlDatabase> db, quint64 setID, const QString &newName);
    bool setNumTiles(std::shared_ptr<QSqlDatabase> db, quint64 setID, quint64 numTiles);
    bool getTileSetID(std::shared_ptr<QSqlDatabase> db, quint64 &setID, const QString &name);
    bool getDefaultTileSet(std::shared_ptr<QSqlDatabase> db, quint64 &setID);
    bool deleteTileSet(std::shared_ptr<QSqlDatabase> db, quint64 setID);
    bool drop(std::shared_ptr<QSqlDatabase> db);
    bool createDefaultTileSet(std::shared_ptr<QSqlDatabase> db);
};

/*===========================================================================*/

namespace TilesTableModel
{
    bool create(std::shared_ptr<QSqlDatabase> db);
    bool getTile(std::shared_ptr<QSqlDatabase> db, QGCCacheTile *tile, const QString &hash);
    bool getTileCount(std::shared_ptr<QSqlDatabase> db, quint64 &tileCount);
    bool getTileID(std::shared_ptr<QSqlDatabase> db, quint64 &tileID, const QString &hash);
    bool selectFromTileID(std::shared_ptr<QSqlDatabase> db, quint64 tileID);
    bool insertTile(std::shared_ptr<QSqlDatabase> db, quint64 &tileID, const QGCCacheTile &tile);
    bool insertTile(std::shared_ptr<QSqlDatabase> db, quint64 &tileID, const QString &hash, const QString &format, const QByteArray &img, const QString &type);
    bool deleteTileSet(std::shared_ptr<QSqlDatabase> db, quint64 setID);
    bool getSetTotal(std::shared_ptr<QSqlDatabase> db, quint64 &count, quint64 setID);
    bool updateSetTotals(std::shared_ptr<QSqlDatabase> db, QGCCachedTileSet &set);
    bool updateTotals(std::shared_ptr<QSqlDatabase> db, quint32 &totalCount, quint64 &totalSize, quint32 &defaultCount, quint64 &defaultSize, quint64 defaultTileSetID);
    bool prune(std::shared_ptr<QSqlDatabase> db, quint64 defaultTileSetID, qint64 amount);
    bool deleteBingNoTileImageTiles(std::shared_ptr<QSqlDatabase> db);
    bool drop(std::shared_ptr<QSqlDatabase> db);
};

/*===========================================================================*/

namespace TilesDownloadTableModel
{
    bool create(std::shared_ptr<QSqlDatabase> db);
    bool insertTilesDownload(std::shared_ptr<QSqlDatabase> db, const QGCCachedTileSet *tileSet);
    bool setState(std::shared_ptr<QSqlDatabase> db, quint64 setID, const QString &hash, int state);
    bool setState(std::shared_ptr<QSqlDatabase> db, quint64 setID, int state);
    bool deleteTileSet(std::shared_ptr<QSqlDatabase> db, quint64 setID);
    bool getTileDownloadList(std::shared_ptr<QSqlDatabase> db, QQueue<QGCTile*> &tiles, quint64 setID, int count);
    bool updateTilesDownloadSet(std::shared_ptr<QSqlDatabase> db, QGCTile::TileState state, quint64 setID, const QString &hash);
    bool drop(std::shared_ptr<QSqlDatabase> db);
};

/*===========================================================================*/
