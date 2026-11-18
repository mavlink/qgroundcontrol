#include <QtCore/QDateTime>
#include <QtCore/QSet>
#include <QtCore/QUuid>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "MapProvider.h"
#include "QGCSqlHelper.h"
#include "QGCTile.h"
#include "QGCTileCacheDatabase.h"
#include "QGCTileSet.h"
#include "UrlFactory.h"

QList<TileSetRecord> QGCTileCacheDatabase::getTileSets()
{
    QList<TileSetRecord> records;
    if (!_ensureConnected()) {
        return records;
    }

    QSqlQuery query(_database());
    query.setForwardOnly(true);
    if (!query.exec("SELECT setID, name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, "
                    "minZoom, maxZoom, type, numTiles, defaultSet, date "
                    "FROM TileSets ORDER BY defaultSet DESC, name ASC")) {
        return records;
    }

    while (query.next()) {
        TileSetRecord rec;
        rec.setID = query.value(0).toULongLong();
        rec.name = query.value(1).toString();
        rec.mapTypeStr = query.value(2).toString();
        rec.topleftLat = query.value(3).toDouble();
        rec.topleftLon = query.value(4).toDouble();
        rec.bottomRightLat = query.value(5).toDouble();
        rec.bottomRightLon = query.value(6).toDouble();
        rec.minZoom = query.value(7).toInt();
        rec.maxZoom = query.value(8).toInt();
        rec.type = query.value(9).toInt();
        rec.numTiles = query.value(10).toUInt();
        rec.defaultSet = (query.value(11).toInt() != 0);
        rec.date = query.value(12).toULongLong();
        records.append(rec);
    }

    return records;
}

std::optional<quint64> QGCTileCacheDatabase::createTileSet(const QString& name, const QString& mapTypeStr,
                                                           double topleftLat, double topleftLon, double bottomRightLat,
                                                           double bottomRightLon, int minZoom, int maxZoom,
                                                           const QString& type, quint32 numTiles,
                                                           const QString& cacheVariant)
{
    if (!_ensureConnected()) {
        return std::nullopt;
    }

    const SharedMapProvider provider = UrlFactory::getMapProviderFromProviderType(type);
    if (!provider) {
        qCWarning(QGCTileCacheDatabaseLog) << "Cannot create a tile set of no map provider:" << type;
        return std::nullopt;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for createTileSet";
        return std::nullopt;
    }

    QSqlQuery query(_database());
    const int mapTypeId = provider->getMapId();
    const TileSetRecord record{.name = name,
                               .mapTypeStr = mapTypeStr,
                               .topleftLat = topleftLat,
                               .topleftLon = topleftLon,
                               .bottomRightLat = bottomRightLat,
                               .bottomRightLon = bottomRightLon,
                               .minZoom = minZoom,
                               .maxZoom = maxZoom,
                               .type = mapTypeId,
                               .numTiles = numTiles,
                               .date = static_cast<quint64>(QDateTime::currentSecsSinceEpoch())};
    if (!_insertTileCacheSet(query, record)) {
        qCWarning(QGCTileCacheDatabaseLog)
            << "Map Cache SQL error (add tileSet into TileSets):" << query.lastError().text();
        return std::nullopt;
    }

    const quint64 setID = query.lastInsertId().toULongLong();

    // Process tiles in streaming batches to avoid holding all coordinates in memory
    constexpr int kHashBatchSize = 500;

    struct TileCoord
    {
        int x, y;
        QString hash;
    };

    auto processBatch = [&](const QList<TileCoord>& tiles, int z) -> bool {
        QVariantList hashes;
        hashes.reserve(tiles.size());
        for (const auto& tc : tiles) {
            hashes.append(tc.hash);
        }
        QSqlQuery lookup(_database());
        lookup.setForwardOnly(true);
        if (!_execOrWarn(lookup,
                         QStringLiteral("SELECT hash, tileID FROM Tiles WHERE hash IN (%1)")
                             .arg(QGCSqlHelper::placeholders(tiles.size())),
                         hashes, "find cached tiles of set")) {
            return false;
        }
        QHash<QString, quint64> existingTiles;
        while (lookup.next()) {
            existingTiles.insert(lookup.value(0).toString(), lookup.value(1).toULongLong());
        }

        for (const auto& tc : tiles) {
            auto it = existingTiles.find(tc.hash);
            const bool added =
                (it != existingTiles.end())
                    ? _execOrWarn(query, QStringLiteral("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)"),
                                  {it.value(), setID}, "add tile into SetTiles")
                    : _execOrWarn(query,
                                  QStringLiteral("INSERT OR IGNORE INTO TilesDownload(setID, hash, type, x, y, z, "
                                                 "state) VALUES(?, ?, ?, ?, ?, ?, ?)"),
                                  {setID, tc.hash, mapTypeId, tc.x, tc.y, z, static_cast<int>(QGCTile::StatePending)},
                                  "add tile into TilesDownload");
            if (!added) {
                return false;
            }
        }
        return true;
    };

    for (int z = minZoom; z <= maxZoom; z++) {
        const QGCTileSet set = provider->getTileCount(z, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
        const int tileColumns = provider->tileColumns(z);

        QList<TileCoord> batch;
        batch.reserve(kHashBatchSize);

        for (int column = set.tileX0; column <= set.tileX1; column++) {
            // The columns of a set that crosses the antimeridian run on past the last one
            const int x = column % tileColumns;
            for (int y = set.tileY0; y <= set.tileY1; y++) {
                batch.append({x, y, provider->getTileHash(x, y, z, cacheVariant)});

                if (batch.size() >= kHashBatchSize) {
                    if (!processBatch(batch, z))
                        return std::nullopt;
                    batch.clear();
                }
            }
        }

        if (!batch.isEmpty()) {
            if (!processBatch(batch, z))
                return std::nullopt;
        }
    }

    if (!txn.commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit createTileSet transaction";
        return std::nullopt;
    }

    return setID;
}

bool QGCTileCacheDatabase::deleteTileSet(quint64 id)
{
    if (!_ensureConnected()) {
        return false;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for deleteTileSet";
        return false;
    }

    // The tiles that no other set has go with the set. Before its links to its tiles, which tell which those are,
    // and in one statement: a set can have more tiles than a statement can have parameters.
    QSqlQuery query(_database());
    if (!_execOrWarn(query, QStringLiteral("DELETE FROM TilesDownload WHERE setID = ?"), {id},
                     "delete downloads of set") ||
        !_execOrWarn(query, QStringLiteral("DELETE FROM Tiles WHERE tileID IN (%1)").arg(kUniqueTilesSubquery), {id},
                     "delete tiles of set") ||
        !_execOrWarn(query, QStringLiteral("DELETE FROM SetTiles WHERE setID = ?"), {id}, "delete tile links of set") ||
        !_execOrWarn(query, QStringLiteral("DELETE FROM TileSets WHERE setID = ?"), {id}, "delete set")) {
        return false;
    }

    if (id == _defaultSet) {
        _defaultSet = kInvalidTileSet;
    }

    return txn.commit();
}

bool QGCTileCacheDatabase::renameTileSet(quint64 setID, const QString& newName)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    return _execBound(query, QStringLiteral("UPDATE TileSets SET name = ? WHERE setID = ?"), {newName, setID});
}

std::optional<quint64> QGCTileCacheDatabase::findTileSetID(const QString& name)
{
    if (!_ensureConnected()) {
        return std::nullopt;
    }

    QSqlQuery query(_database());
    if (_execBound(query, QStringLiteral("SELECT setID FROM TileSets WHERE name = ?"), {name}) && query.next()) {
        return query.value(0).toULongLong();
    }

    return std::nullopt;
}

QList<QGCTile> QGCTileCacheDatabase::getTileDownloadList(quint64 setID, int count)
{
    QList<QGCTile> tiles;
    if (!_ensureConnected()) {
        return tiles;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for getTileDownloadList";
        return tiles;
    }

    QSqlQuery query(_database());
    if (!_execOrWarn(
            query,
            QStringLiteral("SELECT hash, type, x, y, z FROM TilesDownload WHERE setID = ? AND state = ? LIMIT ?"),
            {setID, static_cast<int>(QGCTile::StatePending), count}, "list tiles to download")) {
        return tiles;
    }

    QVariantList downloading = {static_cast<int>(QGCTile::StateDownloading), setID};
    while (query.next()) {
        QGCTile tile;
        tile.hash = query.value(0).toString();
        tile.type = query.value(1).toInt();
        tile.x = query.value(2).toInt();
        tile.y = query.value(3).toInt();
        tile.z = query.value(4).toInt();
        downloading.append(tile.hash);
        tiles.append(std::move(tile));
    }

    if (!tiles.isEmpty() &&
        !_execOrWarn(query,
                     QStringLiteral("UPDATE TilesDownload SET state = ? WHERE setID = ? AND hash IN (%1)")
                         .arg(QGCSqlHelper::placeholders(tiles.size())),
                     downloading, "batch set TilesDownload state")) {
        tiles.clear();
        return tiles;
    }

    if (!txn.commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit getTileDownloadList transaction";
        tiles.clear();
    }

    return tiles;
}

bool QGCTileCacheDatabase::updateTileDownloadState(quint64 setID, int state, const QString& hash)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    if (state == QGCTile::StateComplete) {
        return _execOrWarn(query, QStringLiteral("DELETE FROM TilesDownload WHERE setID = ? AND hash = ?"),
                           {setID, hash}, "remove downloaded tile");
    }
    return _execOrWarn(query, QStringLiteral("UPDATE TilesDownload SET state = ? WHERE setID = ? AND hash = ?"),
                       {state, setID, hash}, "set TilesDownload state");
}

bool QGCTileCacheDatabase::updateAllTileDownloadStates(quint64 setID, int state)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    return _execOrWarn(query, QStringLiteral("UPDATE TilesDownload SET state = ? WHERE setID = ?"), {state, setID},
                       "set TilesDownload states of set");
}

SetTotalsResult QGCTileCacheDatabase::computeSetTotals(quint64 setID, bool isDefault, quint32 totalTileCount,
                                                       const QString& type)
{
    SetTotalsResult result;
    result.totalTileCount = totalTileCount;

    if (isDefault) {
        TotalsResult totals = computeTotals();
        result.savedTileCount = totals.totalCount;
        result.savedTileSize = totals.totalSize;
        result.totalTileSize = totals.totalSize;
        result.uniqueTileCount = totals.defaultCount;
        result.uniqueTileSize = totals.defaultSize;
        return result;
    }

    if (!_ensureConnected()) {
        return result;
    }

    QSqlQuery subquery(_database());
    if (!_execBound(subquery,
                    QStringLiteral("SELECT COUNT(size), SUM(size) FROM Tiles A INNER JOIN SetTiles B ON A.tileID = "
                                   "B.tileID WHERE B.setID = ?"),
                    {setID}) ||
        !subquery.next()) {
        return result;
    }

    result.savedTileCount = subquery.value(0).toUInt();
    result.savedTileSize = subquery.value(1).toULongLong();

    // A provider does not have every tile of a region. A set is complete with the tiles it saved once it has
    // tried them all, or it would look unfinished for good.
    if ((totalTileCount > result.savedTileCount) &&
        _execBound(subquery, QStringLiteral("SELECT COUNT(*) FROM TilesDownload WHERE setID = ?"), {setID}) &&
        subquery.next() && (subquery.value(0).toUInt() == 0)) {
        totalTileCount = result.savedTileCount;
        result.totalTileCount = totalTileCount;
    }

    const SharedMapProvider provider = UrlFactory::getMapProviderFromProviderType(type);
    quint64 avg = provider ? provider->getAverageSize() : QGC_AVERAGE_TILE_SIZE;
    if (avg == 0) {
        avg = 4096;
    }
    if (totalTileCount <= result.savedTileCount) {
        result.totalTileSize = result.savedTileSize;
    } else {
        if ((result.savedTileCount > 10) && result.savedTileSize) {
            avg = result.savedTileSize / result.savedTileCount;
        }
        result.totalTileSize = avg * totalTileCount;
    }

    quint32 dbUniqueCount = 0;
    quint64 dbUniqueSize = 0;
    if (_execOrWarn(
            subquery,
            QStringLiteral("SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN (%1)").arg(kUniqueTilesSubquery),
            {setID}, "count the tiles only the set has") &&
        subquery.next()) {
        dbUniqueCount = subquery.value(0).toUInt();
        dbUniqueSize = subquery.value(1).toULongLong();
    }

    if (dbUniqueCount > 0) {
        result.uniqueTileCount = dbUniqueCount;
        result.uniqueTileSize = dbUniqueSize;
    } else {
        const quint32 estimatedCount =
            (totalTileCount > result.savedTileCount) ? (totalTileCount - result.savedTileCount) : 0;
        result.uniqueTileCount = estimatedCount;
        result.uniqueTileSize = estimatedCount * avg;
    }

    return result;
}

QString QGCTileCacheDatabase::_deduplicateSetName(const QString& name)
{
    if (!findTileSetID(name).has_value()) {
        return name;
    }

    QSet<QString> existing;
    existing.insert(name);
    QSqlQuery query(_database());
    if (_execBound(query, QStringLiteral("SELECT name FROM TileSets WHERE name LIKE ? || ' %' ESCAPE '\\'"),
                   {QGCSqlHelper::escapeLikePattern(name)})) {
        while (query.next()) {
            existing.insert(query.value(0).toString());
        }
    }

    for (int i = 1; i <= 9999; i++) {
        const QString candidate = QStringLiteral("%1 %2").arg(name).arg(i, 4, 10, QChar('0'));
        if (!existing.contains(candidate)) {
            return candidate;
        }
    }

    return QStringLiteral("%1 %2").arg(name, QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
}
