#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "QGCSqlHelper.h"
#include "QGCTileCacheDatabase.h"

/// Removes a database file and the files SQLite keeps next to it in WAL mode
static void removeDatabaseFiles(const QString& path)
{
    for (const char* suffix : {"", "-wal", "-shm"}) {
        (void) QFile::remove(path + QLatin1String(suffix));
    }
}

/// What copyTileToTileCacheSet() did. Where a statement failed, QSqlQuery::lastError() of its query says why.
enum class TileCacheCopy
{
    Linked,             ///< The set has the tile, and had not before
    NotLinked,          ///< The set had the tile, or the statement that links it could not be prepared
    LinkFailed,         ///< The statement that links the tile failed
    NoTile,             ///< The tile was neither added nor found
    InsertNotPrepared,  ///< The statement that adds the tile could not be prepared
};

/// Copies a tile to the database of \a query and links it to the set \a setID there. A tile that database has
/// under \a hash stays as it is, and is the one linked.
static TileCacheCopy copyTileToTileCacheSet(QSqlQuery& query, quint64 setID, const QString& hash, const QString& format,
                                            const QByteArray& img, int type, quint64 date)
{
    if (!query.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)")) {
        return TileCacheCopy::InsertNotPrepared;
    }
    query.addBindValue(hash);
    query.addBindValue(format);
    query.addBindValue(img);
    query.addBindValue(img.size());
    query.addBindValue(type);
    query.addBindValue(date);
    quint64 tileID = 0;
    if (query.exec()) {
        tileID = query.lastInsertId().toULongLong();
    } else if (query.prepare("SELECT tileID FROM Tiles WHERE hash = ?")) {
        query.addBindValue(hash);
        if (query.exec() && query.next()) {
            tileID = query.value(0).toULongLong();
        }
    }
    if (tileID == 0) {
        return TileCacheCopy::NoTile;
    }

    if (!query.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
        return TileCacheCopy::NotLinked;
    }
    query.addBindValue(tileID);
    query.addBindValue(setID);
    if (!query.exec()) {
        return TileCacheCopy::LinkFailed;
    }
    return (query.numRowsAffected() > 0) ? TileCacheCopy::Linked : TileCacheCopy::NotLinked;
}

std::optional<int> QGCTileCacheDatabase::_importableVersion(const QString& path, QString& errorString)
{
    std::optional<int> version;
    const QGCSqlHelper::ScopedConnection versionDB(path, /*readOnly=*/true, QStringLiteral("QGCTileCacheImport"));
    if (versionDB.isValid()) {
        QSqlDatabase db = versionDB.database();
        version = QGCSqlHelper::userVersion(db);
    }
    if (!version) {
        errorString = "Error opening import database";
        return std::nullopt;
    }
    // Version 0 cannot be migrated: see _checkSchemaVersion(), which discards a cache of that version
    if (*version < 1) {
        errorString = "Import database was written by a version of QGroundControl that is too old";
        return std::nullopt;
    }
    if (*version > kSchemaVersion) {
        errorString = "Import database was written by a newer version of QGroundControl";
        return std::nullopt;
    }
    return version;
}

DatabaseResult QGCTileCacheDatabase::importSetsReplace(const QString& path, ProgressCallback progressCb)
{
    DatabaseResult result;
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(_databasePath).canonicalFilePath()) {
        result.errorString = "Import path must differ from the active database";
        return result;
    }
    if (!_importableVersion(path, result.errorString)) {
        return result;
    }
    _defaultSet = kInvalidTileSet;
    disconnectDB();
    const QString backupPath = _databasePath + QStringLiteral(".bak");
    (void) QFile::remove(backupPath);
    const bool hasBackup = QFile::rename(_databasePath, backupPath);
    if (!hasBackup) {
        (void) QFile::remove(_databasePath);
    }
    const auto restoreCache = [this, hasBackup, &backupPath] {
        if (hasBackup) {
            (void) QFile::rename(backupPath, _databasePath);
        }
        if (init()) {
            connectDB();
        }
    };
    if (!QFile::copy(path, _databasePath)) {
        result.errorString = "Failed to copy import database";
        restoreCache();
        return result;
    }
    if (progressCb)
        progressCb(25);
    // The file is not the cache yet: one that cannot be migrated is not reset, and the cache is put back
    _resetOnMigrationFailure = false;
    const bool imported = init();
    _resetOnMigrationFailure = true;
    if (!imported) {
        result.errorString = QStringLiteral("Failed to initialize tile cache database after import");
        removeDatabaseFiles(_databasePath);
        restoreCache();
    } else {
        (void) QFile::remove(backupPath);
        if (progressCb)
            progressCb(50);
        connectDB();
        if (!_valid) {
            result.errorString = QStringLiteral("Failed to connect to tile cache database after import");
        }
    }
    if (progressCb)
        progressCb(100);
    result.success = result.errorString.isEmpty();
    return result;
}

DatabaseResult QGCTileCacheDatabase::importSetsMerge(const QString& path, ProgressCallback progressCb)
{
    DatabaseResult result;
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(_databasePath).canonicalFilePath()) {
        result.errorString = "Import path must differ from the active database";
        return result;
    }
    if (!_ensureConnected()) {
        result.errorString = "Database not connected";
        return result;
    }

    const std::optional<int> importVersion = _importableVersion(path, result.errorString);
    if (!importVersion) {
        return result;
    }

    // An older file is migrated in a copy next to the cache: the file to import stays as it is
    QString importPath = path;
    const QString migratedPath = _databasePath + QStringLiteral(".import");
    const auto removeMigrated = qScopeGuard([&migratedPath] { removeDatabaseFiles(migratedPath); });
    if (*importVersion < kSchemaVersion) {
        // An import that died can have left the write-ahead log of its copy, which SQLite would apply to this one
        removeDatabaseFiles(migratedPath);
        if (!QFile::copy(path, migratedPath)) {
            result.errorString = "Failed to copy import database";
            return result;
        }
        // The copy has the permissions of the file, which can be read-only
        (void) QFile::setPermissions(migratedPath, QFile::ReadOwner | QFile::WriteOwner);

        QGCTileCacheDatabase migrated(migratedPath);
        migrated.setLegacyMapIds(_legacyMapIds);
        migrated.setCacheVariants(_cacheVariants);
        migrated._resetOnMigrationFailure = false;
        if (!migrated.init()) {
            result.errorString = "Failed to migrate import database";
            return result;
        }
        importPath = migratedPath;
    }

    QGCSqlHelper::ScopedConnection importDB(importPath, /*readOnly=*/true, QStringLiteral("QGCTileCacheImport"));
    if (!importDB.isValid()) {
        result.errorString = "Error opening import database";
        return result;
    }

    QSqlQuery query(importDB.database());
    quint64 tileCount = 0;
    int lastProgress = -1;
    if (query.exec("SELECT COUNT(tileID) FROM Tiles") && query.next()) {
        tileCount = query.value(0).toULongLong();
    }

    bool tilesImported = false;

    if (tileCount > 0) {
        if (query.exec("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC")) {
            quint64 currentCount = 0;
            while (query.next()) {
                TileSetRecord set;
                set.name = query.value("name").toString();
                const quint64 setID = query.value("setID").toULongLong();
                set.mapTypeStr = query.value("typeStr").toString();
                set.topleftLat = query.value("topleftLat").toDouble();
                set.topleftLon = query.value("topleftLon").toDouble();
                set.bottomRightLat = query.value("bottomRightLat").toDouble();
                set.bottomRightLon = query.value("bottomRightLon").toDouble();
                set.minZoom = query.value("minZoom").toInt();
                set.maxZoom = query.value("maxZoom").toInt();
                set.type = query.value("type").toInt();
                set.numTiles = query.value("numTiles").toUInt();
                const int defaultSet = query.value("defaultSet").toInt();
                quint64 insertSetID = _getDefaultTileSet();

                // Wrap each set creation + tile copy in a single transaction
                QGCSqlHelper::Transaction txn(_database());
                if (!txn.ok()) {
                    result.errorString = "Failed to start transaction for import set";
                    break;
                }

                if (defaultSet == 0) {
                    set.name = _deduplicateSetName(set.name);
                    set.date = static_cast<quint64>(QDateTime::currentSecsSinceEpoch());
                    QSqlQuery cQuery(_database());
                    if (!_insertTileCacheSet(cQuery, set)) {
                        result.errorString = "Error adding imported tile set to database";
                        break;
                    }
                    insertSetID = cQuery.lastInsertId().toULongLong();
                }

                quint64 tilesIterated = 0;
                const quint64 tilesSaved = _copyTilesForSet(importDB.database(), setID, insertSetID, currentCount,
                                                            tileCount, lastProgress, progressCb, &tilesIterated);
                if (tilesSaved > 0) {
                    QSqlQuery cQuery(_database());
                    if (_execBound(cQuery,
                                   QStringLiteral("SELECT COUNT(size) FROM Tiles A INNER JOIN SetTiles B ON A.tileID = "
                                                  "B.tileID WHERE B.setID = ?"),
                                   {insertSetID}) &&
                        cQuery.next()) {
                        const quint64 count = cQuery.value(0).toULongLong();
                        (void) _execOrWarn(cQuery, QStringLiteral("UPDATE TileSets SET numTiles = ? WHERE setID = ?"),
                                           {count, insertSetID}, "set tile count of imported set");
                    }
                }

                // Without the commit the set and its tiles are not in the cache
                if (!txn.commit()) {
                    qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit import transaction for set:" << set.name;
                    result.errorString = "Error saving imported tile set to database";
                    break;
                }
                if (tilesSaved > 0) {
                    tilesImported = true;
                }

                if (tilesIterated > tilesSaved) {
                    const quint64 alreadyExisting = tilesIterated - tilesSaved;
                    tileCount = (alreadyExisting < tileCount) ? tileCount - alreadyExisting : 0;
                }

                if ((tilesSaved == 0) && (defaultSet == 0)) {
                    qCDebug(QGCTileCacheDatabaseLog) << "No unique tiles in" << set.name << "Removing it.";
                    deleteTileSet(insertSetID);
                }
            }
        } else {
            result.errorString = "No tile set in database";
        }
    }

    if (!tilesImported && result.errorString.isEmpty()) {
        result.errorString = "No unique tiles in imported database";
    }
    result.success = result.errorString.isEmpty();
    return result;
}

DatabaseResult QGCTileCacheDatabase::exportSets(const QList<TileSetRecord>& sets, const QString& path,
                                                ProgressCallback progressCb)
{
    DatabaseResult result;
    if (!_ensureConnected()) {
        result.errorString = "Database not connected";
        return result;
    }
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(_databasePath).canonicalFilePath()) {
        result.errorString = "Export path must differ from the active database";
        return result;
    }

    (void) QFile::remove(path);
    QGCSqlHelper::ScopedConnection exportDB(path, /*readOnly=*/false, QStringLiteral("QGCTileCacheExport"));
    if (!exportDB.isValid()) {
        qCCritical(QGCTileCacheDatabaseLog)
            << "Map Cache SQL error (create export database):" << exportDB.database().lastError();
        result.errorString = "Error opening export database";
        return result;
    }

    if (!_createDB(exportDB.database(), false)) {
        result.errorString = "Error creating export database";
        return result;
    }

    quint64 tileCount = 0;
    quint64 currentCount = 0;
    int lastProgress = -1;
    for (const auto& set : sets) {
        QSqlQuery countQuery(_database());
        quint64 actualCount = 0;
        if (_execBound(countQuery,
                       QStringLiteral("SELECT COUNT(*) FROM Tiles T INNER JOIN SetTiles S ON T.tileID = S.tileID WHERE "
                                      "S.setID = ?"),
                       {set.setID}) &&
            countQuery.next()) {
            actualCount = countQuery.value(0).toULongLong();
        }
        tileCount += (actualCount > 0) ? actualCount : set.numTiles;
    }

    if (tileCount == 0) {
        tileCount = 1;
    }

    for (const auto& set : sets) {
        // A set that is not exported whole fails the export: the file would look complete
        QSqlQuery query(_database());
        query.setForwardOnly(true);
        if (!_execOrWarn(query,
                         QStringLiteral("SELECT T.hash, T.format, T.tile, T.type, T.date FROM Tiles T "
                                        "INNER JOIN SetTiles S ON T.tileID = S.tileID WHERE S.setID = ?"),
                         {set.setID}, "read tiles of set to export")) {
            result.errorString = "Error reading tile set to export";
            break;
        }

        QGCSqlHelper::Transaction txn(exportDB.database());
        if (!txn.ok()) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for export set" << set.name;
            result.errorString = "Failed to start export transaction";
            break;
        }

        QSqlQuery exportQuery(exportDB.database());
        if (!_insertTileCacheSet(exportQuery, set)) {
            result.errorString = "Error adding tile set to exported database";
            break;
        }

        const quint64 exportSetID = exportQuery.lastInsertId().toULongLong();

        quint64 skippedTiles = 0;
        while (query.next()) {
            const QString hash = query.value(0).toString();
            const QString format = query.value(1).toString();
            const QByteArray img = query.value(2).toByteArray();
            const int tileType = query.value(3).toInt();
            const quint64 tileDate = query.value(4).toULongLong();

            const TileCacheCopy copied =
                copyTileToTileCacheSet(exportQuery, exportSetID, hash, format, img, tileType, tileDate);
            if (copied == TileCacheCopy::InsertNotPrepared) {
                qCWarning(QGCTileCacheDatabaseLog)
                    << "Failed to prepare tile INSERT for export:" << exportQuery.lastError().text();
                skippedTiles++;
                continue;
            }
            if (copied == TileCacheCopy::NoTile) {
                skippedTiles++;
            } else if (copied == TileCacheCopy::LinkFailed) {
                qCWarning(QGCTileCacheDatabaseLog)
                    << "Failed to link tile to set in export:" << exportQuery.lastError().text();
            }
            currentCount++;
            if (progressCb) {
                const int progress = qMin(
                    100,
                    static_cast<int>((static_cast<double>(currentCount) / static_cast<double>(tileCount)) * 100.0));
                if (lastProgress != progress) {
                    lastProgress = progress;
                    progressCb(progress);
                }
            }
        }
        if (skippedTiles > 0) {
            qCWarning(QGCTileCacheDatabaseLog) << "Skipped" << skippedTiles << "tiles during export of" << set.name;
        }
        if (!txn.commit()) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit export transaction for" << set.name;
            result.errorString = "Error saving tile set to exported database";
            break;
        }
    }

    result.success = result.errorString.isEmpty();
    return result;
}

quint64 QGCTileCacheDatabase::_copyTilesForSet(QSqlDatabase srcDB, quint64 srcSetID, quint64 dstSetID,
                                               quint64& currentCount, quint64 tileCount, int& lastProgress,
                                               ProgressCallback progressCb, quint64* tilesIteratedOut)
{
    QSqlQuery subQuery(srcDB);
    subQuery.setForwardOnly(true);
    if (!_execOrWarn(subQuery,
                     QStringLiteral("SELECT T.hash, T.format, T.tile, T.type, T.date FROM Tiles T "
                                    "INNER JOIN SetTiles S ON T.tileID = S.tileID WHERE S.setID = ?"),
                     {srcSetID}, "read tiles of set to import")) {
        if (tilesIteratedOut) {
            *tilesIteratedOut = 0;
        }
        return 0;
    }

    quint64 tilesFound = 0;
    quint64 tilesLinked = 0;

    QSqlQuery cQuery(_database());
    while (subQuery.next()) {
        tilesFound++;
        const QString hash = subQuery.value(0).toString();
        const QString format = subQuery.value(1).toString();
        const QByteArray img = subQuery.value(2).toByteArray();
        const int tileType = subQuery.value(3).toInt();
        const quint64 tileDate = subQuery.value(4).toULongLong();

        if (copyTileToTileCacheSet(cQuery, dstSetID, hash, format, img, tileType, tileDate) == TileCacheCopy::Linked) {
            tilesLinked++;
        }

        currentCount++;
        if (tileCount > 0 && progressCb) {
            const int progress = qMin(
                100, static_cast<int>((static_cast<double>(currentCount) / static_cast<double>(tileCount)) * 100.0));
            if (lastProgress != progress) {
                lastProgress = progress;
                progressCb(progress);
            }
        }
    }

    if (tilesIteratedOut)
        *tilesIteratedOut = tilesFound;
    return tilesLinked;
}
