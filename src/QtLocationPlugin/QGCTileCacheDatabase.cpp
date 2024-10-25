#include "QGCTileCacheDatabase.h"
#include "QGCCacheTile.h"
#include "QGCCachedTileSet.h"
#include "QGCMapUrlEngine.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

QGC_LOGGING_CATEGORY(QGCTileCacheDBLog, "test.qtlocationplugin.qgctilecachedb")

/*===========================================================================*/

bool SetTilesTableModel::create(std::shared_ptr<QSqlDatabase> db)
{
    QSqlQuery query(*db);

    (void) query.prepare(
        "CREATE TABLE IF NOT EXISTS SetTiles ("
        "setID INTEGER NOT NULL, "
        "tileID INTEGER NOT NULL, "
        "PRIMARY KEY (setID, tileID)"
        ")"
    );

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool SetTilesTableModel::selectFromSetID(std::shared_ptr<QSqlDatabase> db, quint64 setID)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("SELECT * FROM SetTiles WHERE setID = %1").arg(setID));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool SetTilesTableModel::insertSetTiles(std::shared_ptr<QSqlDatabase> db, quint64 setID, quint64 tileID)
{
    QSqlQuery query(*db);

    (void) query.prepare("INSERT INTO SetTiles(setID, tileID) VALUES(?, ?)"); // INSERT OR IGNORE?
    query.addBindValue(setID);
    query.addBindValue(tileID);

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool SetTilesTableModel::deleteTileSet(std::shared_ptr<QSqlDatabase> db, quint64 setID)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("DELETE FROM SetTiles WHERE setID = %1").arg(setID));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool SetTilesTableModel::drop(std::shared_ptr<QSqlDatabase> db)
{
    QSqlQuery query(*db);
    (void) query.prepare("DROP TABLE IF EXISTS SetTiles");

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return query.exec();
}

/*===========================================================================*/

bool TileSetsTableModel::create(std::shared_ptr<QSqlDatabase> db)
{
    QSqlQuery query(*db);
    (void) query.prepare(
        "CREATE TABLE IF NOT EXISTS TileSets ("
        "setID INTEGER PRIMARY KEY NOT NULL, "
        "name TEXT NOT NULL UNIQUE, "
        "typeStr TEXT, "
        "topleftLat REAL DEFAULT 0.0, "
        "topleftLon REAL DEFAULT 0.0, "
        "bottomRightLat REAL DEFAULT 0.0, "
        "bottomRightLon REAL DEFAULT 0.0, "
        "minZoom INTEGER DEFAULT 3, "
        "maxZoom INTEGER DEFAULT 3, "
        "type INTEGER DEFAULT -1, "
        "numTiles INTEGER DEFAULT 0, "
        "defaultSet INTEGER DEFAULT 0, "
        "date INTEGER DEFAULT 0)"
    );

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool TileSetsTableModel::insertTileSet(std::shared_ptr<QSqlDatabase> db, const QGCCachedTileSet &tileSet, quint64 &setID)
{
    QSqlQuery query(*db);

    (void) query.prepare(
        "INSERT INTO TileSets"
        "(name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, maxZoom, type, numTiles, date) "
        "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
    );
    query.addBindValue(tileSet.name());
    query.addBindValue(tileSet.mapTypeStr());
    query.addBindValue(tileSet.topleftLat());
    query.addBindValue(tileSet.topleftLon());
    query.addBindValue(tileSet.bottomRightLat());
    query.addBindValue(tileSet.bottomRightLon());
    query.addBindValue(tileSet.minZoom());
    query.addBindValue(tileSet.maxZoom());
    query.addBindValue(UrlFactory::getQtMapIdFromProviderType(tileSet.type()));
    query.addBindValue(tileSet.totalTileCount());
    query.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    setID = query.lastInsertId().toULongLong();

    return true;
}

bool TileSetsTableModel::getTileSets(std::shared_ptr<QSqlDatabase> db, QList<QGCCachedTileSet*> &tileSets)
{
    QSqlQuery query(*db);

    (void) query.prepare("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC");

    if (!query.exec()) {
        qCDebug(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    while (query.next()) {
        const QString name = query.value("name").toString();
        QGCCachedTileSet* const set = new QGCCachedTileSet(name);
        set->setId(query.value("setID").toULongLong());
        set->setMapTypeStr(query.value("typeStr").toString());
        set->setTopleftLat(query.value("topleftLat").toDouble());
        set->setTopleftLon(query.value("topleftLon").toDouble());
        set->setBottomRightLat(query.value("bottomRightLat").toDouble());
        set->setBottomRightLon(query.value("bottomRightLon").toDouble());
        set->setMinZoom(query.value("minZoom").toInt());
        set->setMaxZoom(query.value("maxZoom").toInt());
        set->setType(UrlFactory::getProviderTypeFromQtMapId(query.value("type").toInt()));
        set->setTotalTileCount(query.value("numTiles").toUInt());
        set->setDefaultSet(query.value("defaultSet").toInt() != 0);
        set->setCreationDate(QDateTime::fromSecsSinceEpoch(query.value("date").toUInt()));
        (void) tileSets.append(set);
    }

    return true;
}

bool TileSetsTableModel::setName(std::shared_ptr<QSqlDatabase> db, quint64 setID, const QString &newName)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("UPDATE TileSets SET name = \"%1\" WHERE setID = %2").arg(newName).arg(setID));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool TileSetsTableModel::setNumTiles(std::shared_ptr<QSqlDatabase> db, quint64 setID, quint64 numTiles)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("UPDATE TileSets SET numTiles = %1 WHERE setID = %2").arg(numTiles).arg(setID));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool TileSetsTableModel::getTileSetID(std::shared_ptr<QSqlDatabase> db, quint64 &setID, const QString &name)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("SELECT setID FROM TileSets WHERE name = \"%1\"").arg(name));

    if (query.exec() && query.next()) {
        setID = query.value(0).toULongLong();
        return true;
    }

    qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << "TileSet not found for name" << name;
    return false;
}

bool TileSetsTableModel::getDefaultTileSet(std::shared_ptr<QSqlDatabase> db, quint64 &setID)
{
    static quint64 defaultSet = UINT64_MAX;

    if (defaultSet != UINT64_MAX) {
        setID = defaultSet;
        return true;
    }

    QSqlQuery query(*db);

    (void) query.prepare("SELECT setID FROM TileSets WHERE defaultSet = 1");

    if (!query.exec() || !query.next()) {
        setID = 1; // Default fallback if no set found.
        return false;
    }

    defaultSet = query.value(0).toULongLong();
    setID = defaultSet;

    return true;
}

bool TileSetsTableModel::deleteTileSet(std::shared_ptr<QSqlDatabase> db, quint64 setID)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("DELETE FROM TileSets WHERE setID = %1").arg(setID));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool TileSetsTableModel::drop(std::shared_ptr<QSqlDatabase> db)
{
    QSqlQuery query(*db);

    (void) query.prepare("DROP TABLE IF EXISTS TileSets");

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool TileSetsTableModel::createDefaultTileSet(std::shared_ptr<QSqlDatabase> db)
{
    static const QString kDefaultSet = QStringLiteral("Default Tile Set");

    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("SELECT name FROM TileSets WHERE name = \"%1\"").arg(kDefaultSet));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    if (query.next()) {
        return true;
    }

    (void) query.prepare("INSERT INTO TileSets(name, defaultSet, date) VALUES(?, ?, ?)");
    query.addBindValue(kDefaultSet);
    query.addBindValue(1);
    query.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

/*===========================================================================*/

bool TilesTableModel::create(std::shared_ptr<QSqlDatabase> db)
{
    QSqlQuery query(*db);
    (void) query.prepare(
        "CREATE TABLE IF NOT EXISTS Tiles ("
        "tileID INTEGER PRIMARY KEY NOT NULL, "
        "hash TEXT NOT NULL UNIQUE, "
        "format TEXT NOT NULL, "
        "tile BLOB, "
        "size INTEGER, "
        "type INTEGER, "
        "date INTEGER DEFAULT 0)"
    );

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    (void) query.prepare(
        "CREATE INDEX IF NOT EXISTS hash ON Tiles ("
        "hash, size, type)"
    );

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool TilesTableModel::getTile(std::shared_ptr<QSqlDatabase> db, QGCCacheTile *tile, const QString &hash)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("SELECT tile, format, type FROM Tiles WHERE hash = \"%1\"").arg(hash));

    if (!query.exec() || !query.next()) {
        qCDebug(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    const QByteArray array = query.value(0).toByteArray();
    const QString format = query.value(1).toString();
    const QString type = query.value(2).toString();
    tile = new QGCCacheTile(hash, array, format, type);

    return true;
}

bool TilesTableModel::getTileCount(std::shared_ptr<QSqlDatabase> db, quint64 &tileCount)
{
    QSqlQuery query(*db);

    (void) query.prepare("SELECT COUNT(tileID) FROM Tiles");
    if (!query.exec() || !query.next()) {
        qCDebug(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();;
        return false;
    }

    tileCount = query.value(0).toULongLong();

    return true;
}

bool TilesTableModel::getTileID(std::shared_ptr<QSqlDatabase> db, quint64 &tileID, const QString &hash)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("SELECT tileID FROM Tiles WHERE hash = \"%1\"").arg(hash));

    if (!query.exec() || !query.next()) {
        qCDebug(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();;
        return false;
    }

    tileID = query.value(0).toULongLong();

    return true;
}

bool TilesTableModel::selectFromTileID(std::shared_ptr<QSqlDatabase> db, quint64 tileID)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("SELECT * FROM Tiles WHERE tileID = %1").arg(tileID));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    if (!query.next()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << "No rows found for tileID" << tileID;
        return false;
    }

    return true;
}

bool TilesTableModel::insertTile(std::shared_ptr<QSqlDatabase> db, quint64 &tileID, const QGCCacheTile &tile)
{
    return insertTile(db, tileID, tile.hash(), tile.format(), tile.img(), tile.type());
}

bool TilesTableModel::insertTile(std::shared_ptr<QSqlDatabase> db, quint64 &tileID, const QString &hash, const QString &format, const QByteArray &img, const QString &type)
{
    QSqlQuery query(*db);

    (void) query.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)");
    query.addBindValue(hash);
    query.addBindValue(format);
    query.addBindValue(img);
    query.addBindValue(img.size());
    query.addBindValue(type);
    query.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    tileID = query.lastInsertId().toULongLong();

    return true;
}

bool TilesTableModel::deleteTileSet(std::shared_ptr<QSqlDatabase> db, quint64 setID)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral(
        "DELETE FROM Tiles WHERE tileID IN "
        "(SELECT A.tileID FROM SetTiles A "
        "JOIN SetTiles B ON A.tileID = B.tileID WHERE B.setID = %1 "
        "GROUP BY A.tileID HAVING COUNT(A.tileID) = 1)"
    ).arg(setID));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool TilesTableModel::getSetTotal(std::shared_ptr<QSqlDatabase> db, quint64 &count, quint64 setID)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral(
        "SELECT COUNT(size) FROM Tiles A "
        "INNER JOIN SetTiles B on A.tileID = B.tileID "
        "WHERE B.setID = %1"
    ).arg(setID));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << "Failed to execute COUNT query:" << query.lastError().text();
        return false;
    }

    if (!query.next()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << "No result from COUNT query.";
        return false;
    }

    count = query.value(0).toULongLong();

    return true;
}

bool TilesTableModel::updateSetTotals(std::shared_ptr<QSqlDatabase> db, QGCCachedTileSet &set)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral(
        "SELECT COUNT(size), SUM(size) FROM Tiles A "
        "INNER JOIN SetTiles B on A.tileID = B.tileID WHERE B.setID = %1"
    ).arg(set.id()));

    if (!query.exec() || !query.next()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << "query failed";
        return false;
    }

    set.setSavedTileCount(query.value(0).toUInt());
    set.setSavedTileSize(query.value(1).toULongLong());
    qCDebug(QGCTileCacheDBLog) << "Set" << set.id() << "Totals:" << set.savedTileCount() << set.savedTileSize() << "Expected:" << set.totalTileCount() << set.totalTilesSize();

    quint64 avg = UrlFactory::averageSizeForType(set.type());
    if (set.totalTileCount() <= set.savedTileCount()) {
        set.setTotalTileSize(set.savedTileSize());
    } else {
        if ((set.savedTileCount() > 10) && (set.savedTileSize() > 0)) {
            avg = (set.savedTileSize() / set.savedTileCount());
        }
        set.setTotalTileSize(avg * set.totalTileCount());
    }

    quint32 ucount = 0;
    quint64 usize = 0;
    (void) query.prepare(QStringLiteral(
        "SELECT COUNT(size), SUM(size) FROM Tiles "
        "WHERE tileID IN (SELECT A.tileID FROM SetTiles A "
        "JOIN SetTiles B on A.tileID = B.tileID WHERE B.setID = %1 "
        "GROUP by A.tileID HAVING COUNT(A.tileID) = 1)"
    ).arg(set.id()));

    if (query.exec() && query.next()) {
        ucount = query.value(0).toUInt();
        usize = query.value(1).toULongLong();
    }

    quint32 expectedUcount = set.totalTileCount() - set.savedTileCount();
    if (ucount == 0) {
        usize = expectedUcount * avg;
    } else {
        expectedUcount = ucount;
    }
    set.setUniqueTileCount(expectedUcount);
    set.setUniqueTileSize(usize);

    return true;
}

bool TilesTableModel::updateTotals(std::shared_ptr<QSqlDatabase> db, quint32 &totalCount, quint64 &totalSize, quint32 &defaultCount, quint64 &defaultSize, quint64 defaultTileSetID)
{
    QSqlQuery query(*db);

    (void) query.prepare("SELECT COUNT(size), SUM(size) FROM Tiles");
    if (!query.exec() || !query.next()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    totalCount = query.value(0).toUInt();
    totalSize = query.value(1).toULongLong();

    (void) query.prepare(QStringLiteral(
        "SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN "
        "(SELECT A.tileID FROM SetTiles A "
        "JOIN SetTiles B on A.tileID = B.tileID WHERE B.setID = %1 "
        "GROUP by A.tileID HAVING COUNT(A.tileID) = 1)"
    ).arg(defaultTileSetID));

    if (!query.exec() || !query.next()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    defaultCount = query.value(0).toUInt();
    defaultSize = query.value(1).toULongLong();

    return true;
}

bool TilesTableModel::prune(std::shared_ptr<QSqlDatabase> db, quint64 defaultTileSetID, qint64 amount)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral(
        "SELECT tileID, size, hash FROM Tiles WHERE tileID IN "
        "(SELECT A.tileID FROM SetTiles A JOIN SetTiles B on A.tileID = B.tileID "
        "WHERE B.setID = %1 GROUP by A.tileID HAVING COUNT(A.tileID) = 1) "
        "ORDER BY DATE ASC LIMIT 128"
    ).arg(defaultTileSetID));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    QQueue<quint64> tilelist;
    while (query.next() && (amount >= 0)) {
        tilelist.enqueue(query.value(0).toULongLong());
        amount -= query.value(1).toULongLong();
    }

    while (!tilelist.isEmpty()) {
        const quint64 tileID = tilelist.dequeue();

        (void) query.prepare(QStringLiteral("DELETE FROM Tiles WHERE tileID = %1").arg(tileID));

        if (!query.exec()) {
            qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
            return false;
        }
    }

    return true;
}

static bool _initDataFromResources(QByteArray &bingNoTileImage)
{
    if (bingNoTileImage.isEmpty()) {
        QFile file("://res/BingNoTileBytes.dat");
        if (file.open(QFile::ReadOnly)) {
            bingNoTileImage = file.readAll();
            file.close();
        }

        if (bingNoTileImage.isEmpty()) {
            qCWarning(QGCTileCacheDBLog) << "Unable to read BingNoTileBytes";
            return false;
        }
    }

    return true;
}

bool TilesTableModel::deleteBingNoTileImageTiles(std::shared_ptr<QSqlDatabase> db)
{
    static const QString alreadyDoneKey = QStringLiteral("_deleteBingNoTileTilesDone");

    QSettings settings;
    if (settings.value(alreadyDoneKey, false).toBool()) {
        return true;
    }
    settings.setValue(alreadyDoneKey, true);

    static QByteArray bingNoTileImage;
    if (!_initDataFromResources(bingNoTileImage)) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << "Error getting BingNoTileBytes.dat";
        return false;
    }

    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("SELECT tileID, tile FROM Tiles WHERE LENGTH(tile) = %1").arg(bingNoTileImage.length()));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    QList<quint64> idsToDelete;
    while (query.next()) {
        if (query.value(1).toByteArray() == bingNoTileImage) {
            (void) idsToDelete.append(query.value(0).toULongLong());
        }
    }

    for (const quint64 tileId: idsToDelete) {
        (void) query.prepare(QStringLiteral("DELETE FROM Tiles WHERE tileID = %1").arg(tileId));

        if (!query.exec()) {
            qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool TilesTableModel::drop(std::shared_ptr<QSqlDatabase> db)
{
    QSqlQuery query(*db);

    (void) query.prepare("DROP TABLE IF EXISTS Tiles");

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

/*===========================================================================*/

bool TilesDownloadTableModel::create(std::shared_ptr<QSqlDatabase> db)
{
    QSqlQuery query(*db);

    (void) query.prepare(
        "CREATE TABLE IF NOT EXISTS TilesDownload ("
        "setID INTEGER PRIMARY KEY NOT NULL, "
        "hash TEXT NOT NULL UNIQUE, "
        "type INTEGER, "
        "x INTEGER, "
        "y INTEGER, "
        "z INTEGER, "
        "state INTEGER DEFAULT 0)"
    );

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool TilesDownloadTableModel::insertTilesDownload(std::shared_ptr<QSqlDatabase> db, const QGCCachedTileSet *tileSet)
{
    QSqlQuery query(*db);

    if (!db->transaction()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << "Transaction start failed";
        return false;
    }

    for (int z = tileSet->minZoom(); z <= tileSet->maxZoom(); z++) {
        const QGCTileSet set = UrlFactory::getTileCount(
            z,
            tileSet->topleftLon(),
            tileSet->topleftLat(),
            tileSet->bottomRightLon(),
            tileSet->bottomRightLat(),
            tileSet->type()
        );
        const QString type = tileSet->type();

        for (int x = set.tileX0; x <= set.tileX1; x++) {
            for (int y = set.tileY0; y <= set.tileY1; y++) {
                const QString hash = UrlFactory::getTileHash(type, x, y, z);
                const quint64 setID = tileSet->id();
                quint64 tileID;
                (void) TilesTableModel::getTileID(db, tileID, hash);
                if (!tileID) {
                    (void) query.prepare(QStringLiteral(
                        "INSERT OR IGNORE INTO TilesDownload(setID, hash, type, x, y, z, state) "
                        "VALUES(?, ?, ?, ?, ? ,? ,?)"
                    ));
                    query.addBindValue(setID);
                    query.addBindValue(hash);
                    query.addBindValue(UrlFactory::getQtMapIdFromProviderType(type));
                    query.addBindValue(x);
                    query.addBindValue(y);
                    query.addBindValue(z);
                    query.addBindValue(0);

                    if (!query.exec()) {
                        qCWarning(QGCTileCacheDBLog) << "Map Cache SQL error (add tile into TilesDownload):"
                                                     << query.lastError().text();
                        if (!db->rollback()) {
                            qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << "Transaction rollback failed";
                        }
                        return false;
                    }
                } else {
                    (void) query.prepare("INSERT OR IGNORE INTO SetTiles(setID, tileID) VALUES(?, ?)");
                    query.addBindValue(setID);
                    query.addBindValue(tileID);

                    if (!query.exec()) {
                        qCWarning(QGCTileCacheDBLog) << "Map Cache SQL error (add tile into SetTiles):"
                                                     << query.lastError().text();
                    }

                    qCDebug(QGCTileCacheDBLog) << Q_FUNC_INFO << "Already Cached HASH:" << hash;
                }
            }
        }
    }

    if (!db->commit()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << "Transaction commit failed";
        if (!db->rollback()) {
            qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << "Transaction rollback failed";
        }
    }

    return true;
}

bool TilesDownloadTableModel::setState(std::shared_ptr<QSqlDatabase> db, quint64 setID, const QString &hash, int state)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("UPDATE TilesDownload SET state = %1 WHERE setID = %2 AND hash = \"%3\"").arg(state).arg(setID).arg(hash));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool TilesDownloadTableModel::setState(std::shared_ptr<QSqlDatabase> db, quint64 setID, int state)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("UPDATE TilesDownload SET state = %1 WHERE setID = %2").arg(state, setID));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool TilesDownloadTableModel::deleteTileSet(std::shared_ptr<QSqlDatabase> db, quint64 setID)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("DELETE FROM TilesDownload WHERE setID = %1").arg(setID));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool TilesDownloadTableModel::getTileDownloadList(std::shared_ptr<QSqlDatabase> db, QQueue<QGCTile*> &tiles, quint64 setID, int count)
{
    QSqlQuery query(*db);

    (void) query.prepare(QStringLiteral("SELECT hash, type, x, y, z FROM TilesDownload WHERE setID = %1 AND state = 0 LIMIT %2").arg(setID, count));

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    while (query.next()) {
        QGCTile* const tile = new QGCTile();
        // tile->setTileSet(task->setID());
        tile->setHash(query.value("hash").toString());
        tile->setType(UrlFactory::getProviderTypeFromQtMapId(query.value("type").toInt()));
        tile->setX(query.value("x").toInt());
        tile->setY(query.value("y").toInt());
        tile->setZ(query.value("z").toInt());
        tiles.enqueue(tile);
    }

    for (qsizetype i = 0; i < tiles.size(); i++) {
        (void) query.prepare(QStringLiteral("UPDATE TilesDownload SET state = %1 WHERE setID = %2 AND hash = \"%3\"").arg(static_cast<int>(QGCTile::StateDownloading)).arg(setID).arg(tiles[i]->hash()));

        if (!query.exec()) {
            qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool TilesDownloadTableModel::updateTilesDownloadSet(std::shared_ptr<QSqlDatabase> db, QGCTile::TileState state, quint64 setID, const QString &hash)
{
    QSqlQuery query(*db);

    if (state == QGCTile::StateComplete) {
        (void) query.prepare(QStringLiteral("DELETE FROM TilesDownload WHERE setID = %1 AND hash = \"%2\"").arg(setID).arg(hash));
    } else if (hash == "*") {
        (void) query.prepare(QStringLiteral("UPDATE TilesDownload SET state = %1 WHERE setID = %2").arg(state).arg(setID));
    } else {
        (void) query.prepare(QStringLiteral("UPDATE TilesDownload SET state = %1 WHERE setID = %2 AND hash = \"%3\"").arg(state).arg(setID).arg(hash));
    }

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

bool TilesDownloadTableModel::drop(std::shared_ptr<QSqlDatabase> db)
{
    QSqlQuery query(*db);

    (void) query.prepare("DROP TABLE IF EXISTS TilesDownload");

    if (!query.exec()) {
        qCWarning(QGCTileCacheDBLog) << Q_FUNC_INFO << query.lastError().text();
        return false;
    }

    return true;
}

/*===========================================================================*/
