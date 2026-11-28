#include <QtCore/QDateTime>
#include <QtCore/QSet>
#include <QtCore/QUuid>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "DownloadTile.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "QGCSqlHelper.h"
#include "TileCacheDatabase.h"
#include "TileRegion.h"

QList<TileSetRecord> TileCacheDatabase::getTileSets() const
{
    return _tileSets(QLatin1StringView("main"));
}

QList<TileSetRecord> TileCacheDatabase::_tileSets(QLatin1StringView schema) const
{
    QList<TileSetRecord> records;
    if (!_ensureConnected()) {
        return records;
    }

    QSqlQuery query(_database());
    query.setForwardOnly(true);
    if (!query.exec(
            QStringLiteral("SELECT setID, name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, "
                           "minZoom, maxZoom, type, numTiles, defaultSet, date "
                           "FROM %1.TileSets ORDER BY defaultSet DESC, name ASC")
                .arg(schema))) {
        return records;
    }

    while (query.next()) {
        TileSetRecord rec;
        rec.setID = query.value(0).toULongLong();
        rec.name = query.value(1).toString();
        rec.mapName = query.value(2).toString();
        rec.topleftLat = query.value(3).toDouble();
        rec.topleftLon = query.value(4).toDouble();
        rec.bottomRightLat = query.value(5).toDouble();
        rec.bottomRightLon = query.value(6).toDouble();
        rec.minZoom = query.value(7).toInt();
        rec.maxZoom = query.value(8).toInt();
        rec.mapId = query.value(9).toInt();
        rec.numTiles = query.value(10).toUInt();
        rec.defaultSet = (query.value(11).toInt() != 0);
        rec.date = query.value(12).toULongLong();
        records.append(rec);
    }

    return records;
}

std::optional<quint64> TileCacheDatabase::createTileSet(const TileSetRecord& set, const QString& cacheVariant)
{
    if (!_ensureConnected()) {
        return std::nullopt;
    }

    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromMapId(set.mapId);
    if (!provider) {
        qCWarning(TileCacheDatabaseLog) << "Cannot create a tile set of no map provider:" << set.mapId;
        return std::nullopt;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(TileCacheDatabaseLog) << "Failed to start transaction for createTileSet";
        return std::nullopt;
    }

    QSqlQuery query(_database());
    TileSetRecord record = set;
    record.defaultSet = false;
    record.date = static_cast<quint64>(QDateTime::currentSecsSinceEpoch());
    if (!_insertTileCacheSet(query, record)) {
        qCWarning(TileCacheDatabaseLog) << "Map Cache SQL error (add tileSet into TileSets):"
                                        << query.lastError().text();
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
                                  {setID, tc.hash, set.mapId, tc.x, tc.y, z,
                                   static_cast<int>(DownloadTile::TileState::Pending)},
                                  "add tile into TilesDownload");
            if (!added) {
                return false;
            }
        }
        return true;
    };

    for (int z = set.minZoom; z <= set.maxZoom; z++) {
        const TileRegion region =
            provider->getTileCount(z, set.topleftLon, set.topleftLat, set.bottomRightLon, set.bottomRightLat);
        const int tileColumns = provider->tileColumns(z);

        QList<TileCoord> batch;
        batch.reserve(kHashBatchSize);

        for (int column = region.tileX0; column <= region.tileX1; column++) {
            // The columns of a set that crosses the antimeridian run on past the last one
            const int x = column % tileColumns;
            for (int y = region.tileY0; y <= region.tileY1; y++) {
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
        qCWarning(TileCacheDatabaseLog) << "Failed to commit createTileSet transaction";
        return std::nullopt;
    }

    return setID;
}

bool TileCacheDatabase::deleteTileSet(quint64 id)
{
    if (!_ensureConnected()) {
        return false;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(TileCacheDatabaseLog) << "Failed to start transaction for deleteTileSet";
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

bool TileCacheDatabase::deleteCachedTiles()
{
    if (!_ensureConnected()) {
        return false;
    }

    // In one statement: the cache can have more tiles than a statement can have parameters. The links of the tiles
    // to the default set go with them.
    QSqlQuery query(_database());
    return _execOrWarn(query, QStringLiteral("DELETE FROM Tiles WHERE tileID IN (%1)").arg(kUniqueTilesSubquery),
                       {_getDefaultTileSet()}, "delete cached tiles");
}

bool TileCacheDatabase::renameTileSet(quint64 setID, const QString& newName)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    return _execBound(query, QStringLiteral("UPDATE TileSets SET name = ? WHERE setID = ?"), {newName, setID});
}

std::optional<quint64> TileCacheDatabase::findTileSetID(const QString& name) const
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

std::optional<QList<DownloadTile>> TileCacheDatabase::getTileDownloadList(quint64 setID, int count)
{
    if (!_ensureConnected()) {
        return std::nullopt;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(TileCacheDatabaseLog) << "Failed to start transaction for getTileDownloadList";
        return std::nullopt;
    }

    QSqlQuery query(_database());
    if (!_execOrWarn(
            query,
            QStringLiteral("SELECT hash, type, x, y, z FROM TilesDownload WHERE setID = ? AND state = ? LIMIT ?"),
            {setID, static_cast<int>(DownloadTile::TileState::Pending), count}, "list tiles to download")) {
        return std::nullopt;
    }

    QList<DownloadTile> tiles;
    QVariantList downloading = {static_cast<int>(DownloadTile::TileState::Downloading), setID};
    while (query.next()) {
        DownloadTile tile;
        tile.hash = query.value(0).toString();
        tile.mapId = query.value(1).toInt();
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
        return std::nullopt;
    }

    if (!txn.commit()) {
        qCWarning(TileCacheDatabaseLog) << "Failed to commit getTileDownloadList transaction";
        return std::nullopt;
    }

    return tiles;
}

bool TileCacheDatabase::updateTileDownloadState(quint64 setID, DownloadTile::TileState state, const QString& hash)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    if (state == DownloadTile::TileState::Complete) {
        return _execOrWarn(query, QStringLiteral("DELETE FROM TilesDownload WHERE setID = ? AND hash = ?"),
                           {setID, hash}, "remove downloaded tile");
    }
    return _execOrWarn(query, QStringLiteral("UPDATE TilesDownload SET state = ? WHERE setID = ? AND hash = ?"),
                       {static_cast<int>(state), setID, hash}, "set TilesDownload state");
}

bool TileCacheDatabase::updateTileDownloadStates(quint64 setID, const QList<DownloadTileState>& states)
{
    if (!_ensureConnected()) {
        return false;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(TileCacheDatabaseLog) << "Failed to start transaction for updateTileDownloadStates";
        return false;
    }
    for (const DownloadTileState& state : states) {
        if (!updateTileDownloadState(setID, state.state, state.hash)) {
            return false;
        }
    }
    if (!txn.commit()) {
        qCWarning(TileCacheDatabaseLog) << "Failed to commit updateTileDownloadStates transaction";
        return false;
    }
    return true;
}

bool TileCacheDatabase::updateAllTileDownloadStates(quint64 setID, DownloadTile::TileState state)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    return _execOrWarn(query, QStringLiteral("UPDATE TilesDownload SET state = ? WHERE setID = ?"),
                       {static_cast<int>(state), setID}, "set TilesDownload states of set");
}

TileSetTotals TileCacheDatabase::computeSetTotals(quint64 setID, bool isDefault, quint32 totalTileCount, int mapId)
{
    TileSetTotals result;
    result.totalTileCount = totalTileCount;

    if (isDefault) {
        TileCacheTotals totals = computeTotals();
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

    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromMapId(mapId);
    quint64 avg = provider ? provider->averageSize() : QGC_AVERAGE_TILE_SIZE;
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

QString TileCacheDatabase::_deduplicateSetName(const QString& name)
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
