#include "QGCTileCacheDatabase.h"
#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

QGC_LOGGING_CATEGORY(QGCTileCacheDatabaseLog, "qgc.qtlocationplugin.qgctilecachedatabase")

static constexpr const char *kSession = "TileCacheSession";
static constexpr const char *kExportSession = "TileExportSession";
static constexpr int kShortTimeout = 2;
static constexpr int kLongTimeout = 5;

QGCTileCacheDatabase::QGCTileCacheDatabase(const QString &dbPath)
    : _databasePath(dbPath)
{
    // qCDebug(QGCTileCacheDatabaseLog) << Q_FUNC_INFO << this;
}

QGCTileCacheDatabase::~QGCTileCacheDatabase()
{
    disconnect();
    // qCDebug(QGCTileCacheDatabaseLog) << Q_FUNC_INFO << this;
}

bool QGCTileCacheDatabase::connect()
{
    _db = QSqlDatabase::addDatabase("QSQLITE", kSession);
    _db.setDatabaseName(_databasePath);
    _db.setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    const bool ok = _db.open();
    if (!ok) {
        qCCritical(QGCTileCacheDatabaseLog) << "QGCTileCacheDatabase: Failed to open database:" << _db.lastError().text();
    }
    return ok;
}

void QGCTileCacheDatabase::disconnect()
{
    if (_db.isOpen()) {
        _db.close();
        QSqlDatabase::removeDatabase(kSession);
    }
}

bool QGCTileCacheDatabase::createDatabase(bool createDefault) const
{
    return _createTables(createDefault);
}

bool QGCTileCacheDatabase::_createTables(bool createDefault) const
{
    QSqlQuery query(_db);
    if (!query.exec(
          "CREATE TABLE IF NOT EXISTS Tiles ("
          "tileID INTEGER PRIMARY KEY NOT NULL, "
          "hash TEXT NOT NULL UNIQUE, "
          "format TEXT NOT NULL, "
          "tile BLOB, "
          "size INTEGER, "
          "type INTEGER, "
          "date INTEGER DEFAULT 0)"))
    {
        qCWarning(QGCTileCacheDatabaseLog) << "QGCTileCacheDatabase: SQL error (Tiles):" << query.lastError().text();
        return false;
    }
    (void) query.exec("CREATE INDEX IF NOT EXISTS hash ON Tiles (hash, size, type)");

    if (!query.exec(
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
          "date INTEGER DEFAULT 0)")) {
        qCWarning(QGCTileCacheDatabaseLog) << "QGCTileCacheDatabase: SQL error (TileSets):" << query.lastError().text();
        return false;
    }

    if (!query.exec(
          "CREATE TABLE IF NOT EXISTS SetTiles ("
          "setID INTEGER, "
          "tileID INTEGER)")) {
        qCWarning(QGCTileCacheDatabaseLog) << "QGCTileCacheDatabase: SQL error (SetTiles):" << query.lastError().text();
        return false;
    }

    if (!query.exec(
          "CREATE TABLE IF NOT EXISTS TilesDownload ("
          "setID INTEGER, "
          "hash TEXT NOT NULL UNIQUE, "
          "type INTEGER, "
          "x INTEGER, "
          "y INTEGER, "
          "z INTEGER, "
          "state INTEGER DEFAULT 0)")) {
        qCWarning(QGCTileCacheDatabaseLog) << "QGCTileCacheDatabase: SQL error (TilesDownload):" << query.lastError().text();
        return false;
    }

    if (createDefault) {
        const QString s = QString("SELECT name FROM TileSets WHERE name = \"%1\"").arg("Default Tile Set");
        if (query.exec(s)) {
            if (!query.next()) {
                query.prepare("INSERT INTO TileSets(name, defaultSet, date) VALUES(?, ?, ?)");
                query.addBindValue("Default Tile Set");
                query.addBindValue(1);
                query.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
                if (!query.exec()) {
                    qCWarning(QGCTileCacheDatabaseLog) << "QGCTileCacheDatabase: SQL error (Creating default tile set):" << query.lastError().text();
                    return false;
                }
            }
        } else {
            qCWarning(QGCTileCacheDatabaseLog) << "QGCTileCacheDatabase: SQL error (Looking for default tile set):" << query.lastError().text();
            return false;
        }
    }
    return true;
}

bool QGCTileCacheDatabase::deleteBingNoTileTiles() const
{
    QSettings settings;
    static const char* alreadyDoneKey = "_deleteBingNoTileTilesDone";
    if (settings.value(alreadyDoneKey, false).toBool()) {
        return true;
    }
    settings.setValue(alreadyDoneKey, true);

    QFile file(":/res/BingNoTileBytes.dat");
    if (!file.open(QFile::ReadOnly)) {
        return false;
    }
    const QByteArray noTileBytes = file.readAll();
    file.close();

    QSqlQuery query(_db);
    QString s = QStringLiteral("SELECT tileID, tile, hash FROM Tiles WHERE LENGTH(tile) = %1").arg(noTileBytes.length());
    QList<quint64> idsToDelete;
    if (query.exec(s)) {
        while (query.next()) {
            if (query.value(1).toByteArray() == noTileBytes) {
                idsToDelete.append(query.value(0).toULongLong());
                qCDebug(QGCTileCacheDatabaseLog) << "deleteBingNoTileTiles HASH:" << query.value(2).toString();
            }
        }
        for (const quint64 tileId : idsToDelete) {
            s = QStringLiteral("DELETE FROM Tiles WHERE tileID = %1").arg(tileId);
            if (!query.exec(s)) {
                qCWarning(QGCTileCacheDatabaseLog) << "deleteBingNoTileTiles: Delete failed for tileID:" << tileId;
            }
        }
        return true;
    } else {
        qCWarning(QGCTileCacheDatabaseLog) << "deleteBingNoTileTiles: query failed:" << query.lastError().text();
        return false;
    }
}

bool QGCTileCacheDatabase::saveTile(QGCCacheTile* tile, quint64 &tileID, quint64 tileSetID) const
{
    if (!_db.isOpen()) {
        return false;
    }
    QSqlQuery query(_db);
    (void) query.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)");
    query.addBindValue(tile->hash());
    query.addBindValue(tile->format());
    query.addBindValue(tile->img());
    query.addBindValue(tile->img().size());
    query.addBindValue(tile->type());
    query.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
    if (query.exec()) {
        tileID = query.lastInsertId().toULongLong();
        if (tileSetID == UINT64_MAX) {
            tileSetID = getDefaultTileSet();
        }
        const QString s = QStringLiteral("INSERT INTO SetTiles(tileID, setID) VALUES(%1, %2)").arg(tileID).arg(tileSetID);
        (void) query.prepare(s);
        if (!query.exec()) {
            qCWarning(QGCTileCacheDatabaseLog) << "saveTile: Error inserting into SetTiles:" << query.lastError().text();
        }
        qCDebug(QGCTileCacheDatabaseLog) << "saveTile: Saved tile HASH:" << tile->hash();
        return true;
    } else {
        // Tile might already be in the database.
        return false;
    }
}

QGCCacheTile* QGCTileCacheDatabase::fetchTile(const QString &hash) const
{
    QSqlQuery query(_db);
    const QString s = QStringLiteral("SELECT tile, format, type FROM Tiles WHERE hash = \"%1\"").arg(hash);
    if (query.exec(s)) {
        if (query.next()) {
            const QByteArray tileData = query.value(0).toByteArray();
            const QString format = query.value(1).toString();
            const QString type = query.value(2).toString();
            qCDebug(QGCTileCacheDatabaseLog) << "fetchTile: Found tile HASH:" << hash;
            return new QGCCacheTile(hash, tileData, format, type);
        }
    }
    qCDebug(QGCTileCacheDatabaseLog) << "fetchTile: Tile not found HASH:" << hash;
    return nullptr;
}

QList<QGCCachedTileSet*> QGCTileCacheDatabase::fetchTileSets() const
{
    QList<QGCCachedTileSet*> sets;
    QSqlQuery query(_db);
    const QString s = QStringLiteral("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC");
    qCDebug(QGCTileCacheDatabaseLog) << "fetchTileSets:" << s;
    if (query.exec(s)) {
        while (query.next()) {
            const QString name = query.value("name").toString();
            QGCCachedTileSet *const set = new QGCCachedTileSet(name);
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
            (void) set->moveToThread(QCoreApplication::instance()->thread());
            sets.append(set);
        }
    }
    return sets;
}

bool QGCTileCacheDatabase::createTileSet(QGCCachedTileSet *tileSet)
{
    QSqlQuery query(_db);
    (void) query.prepare("INSERT INTO TileSets("
                         "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, "
                         "minZoom, maxZoom, type, numTiles, date) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(tileSet->name());
    query.addBindValue(tileSet->mapTypeStr());
    query.addBindValue(tileSet->topleftLat());
    query.addBindValue(tileSet->topleftLon());
    query.addBindValue(tileSet->bottomRightLat());
    query.addBindValue(tileSet->bottomRightLon());
    query.addBindValue(tileSet->minZoom());
    query.addBindValue(tileSet->maxZoom());
    query.addBindValue(UrlFactory::getQtMapIdFromProviderType(tileSet->type()));
    query.addBindValue(tileSet->totalTileCount());
    query.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "createTileSet: Error inserting tile set:" << query.lastError().text();
        return false;
    } else {
        const quint64 setID = query.lastInsertId().toULongLong();
        tileSet->setId(setID);
        (void) _db.transaction();
        int actualCount = 0;
        for (int z = tileSet->minZoom(); z <= tileSet->maxZoom(); z++) {
            const QGCTileSet set = UrlFactory::getTileCount(z,
                tileSet->topleftLon(), tileSet->topleftLat(),
                tileSet->bottomRightLon(), tileSet->bottomRightLat(),
                tileSet->type());
            const QString type = tileSet->type();
            for (int x = set.tileX0; x <= set.tileX1; x++) {
                for (int y = set.tileY0; y <= set.tileY1; y++) {
                    const QString hash = UrlFactory::getTileHash(type, x, y, z);
                    const quint64 tileID = _findTile(hash);
                    if (!tileID) {
                        (void) query.prepare("INSERT OR IGNORE INTO TilesDownload(setID, hash, type, x, y, z, state) VALUES(?, ?, ?, ?, ?, ?, ?)");
                        query.addBindValue(setID);
                        query.addBindValue(hash);
                        query.addBindValue(UrlFactory::getQtMapIdFromProviderType(type));
                        query.addBindValue(x);
                        query.addBindValue(y);
                        query.addBindValue(z);
                        query.addBindValue(0);
                        if (!query.exec()) {
                            qCWarning(QGCTileCacheDatabaseLog) << "createTileSet: Error adding tile to TilesDownload:" << query.lastError().text();
                        } else {
                            actualCount++;
                        }
                    } else {
                        const QString s = QStringLiteral("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(%1, %2)").arg(tileID).arg(setID);
                        (void) query.prepare(s);
                        if (!query.exec()) {
                            qCWarning(QGCTileCacheDatabaseLog) << "createTileSet: Error adding tile into SetTiles:" << query.lastError().text();
                        }
                        qCDebug(QGCTileCacheDatabaseLog) << "createTileSet: Already cached tile HASH:" << hash;
                    }
                }
            }
        }
        (void) _db.commit();
        (void) updateTotals(*(new quint32()), *(new quint64()), *(new quint32()), *(new quint64()));
        return true;
    }
}

QList<QGCTile*> QGCTileCacheDatabase::getTileDownloadList(quint64 setID, int count) const
{
    QList<QGCTile*> tileList;
    QSqlQuery query(_db);
    const QString s = QStringLiteral("SELECT hash, type, x, y, z FROM TilesDownload WHERE setID = %1 AND state = 0 LIMIT %2")
                      .arg(setID).arg(count);
    if (query.exec(s)) {
        while (query.next()) {
            QGCTile *const tile = new QGCTile;
            tile->setHash(query.value("hash").toString());
            tile->setType(UrlFactory::getProviderTypeFromQtMapId(query.value("type").toInt()));
            tile->setX(query.value("x").toInt());
            tile->setY(query.value("y").toInt());
            tile->setZ(query.value("z").toInt());
            tileList.append(tile);
        }
        for (const auto &tile : tileList) {
            const QString updateStr = QStringLiteral("UPDATE TilesDownload SET state = %1 WHERE setID = %2 and hash = \"%3\"")
                                      .arg(static_cast<int>(QGCTile::StateDownloading)).arg(setID).arg(tile->hash());
            if (!query.exec(updateStr)) {
                qCWarning(QGCTileCacheDatabaseLog) << "getTileDownloadList: Error updating state:" << query.lastError().text();
            }
        }
    }
    return tileList;
}

bool QGCTileCacheDatabase::updateTileDownloadState(quint64 setID, const QString &hash, int state) const
{
    QSqlQuery query(_db);
    QString s;
    if (state == QGCTile::StateComplete) {
        s = QStringLiteral("DELETE FROM TilesDownload WHERE setID = %1 AND hash = \"%2\"").arg(setID).arg(hash);
    } else {
        if (hash == "*") {
            s = QStringLiteral("UPDATE TilesDownload SET state = %1 WHERE setID = %2").arg(state).arg(setID);
        } else {
            s = QStringLiteral("UPDATE TilesDownload SET state = %1 WHERE setID = %2 AND hash = \"%3\"").arg(state).arg(setID).arg(hash);
        }
    }
    if (!query.exec(s)) {
        qCWarning(QGCTileCacheDatabaseLog) << "updateTileDownloadState: Error updating state:" << query.lastError().text();
        return false;
    }
    return true;
}

bool QGCTileCacheDatabase::pruneCache(qint64 amount) const
{
    QSqlQuery query(_db);
    QString s = QStringLiteral("SELECT tileID, size, hash FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID WHERE B.setID = %1 GROUP BY A.tileID HAVING COUNT(A.tileID) = 1) ORDER BY date ASC LIMIT 128")
                .arg(getDefaultTileSet());
    QList<quint64> tileIds;
    if (query.exec(s)) {
        while (query.next() && amount >= 0) {
            tileIds.append(query.value(0).toULongLong());
            amount -= query.value(1).toLongLong();
            qCDebug(QGCTileCacheDatabaseLog) << "pruneCache: Removing tile HASH:" << query.value(2).toString();
        }
        for (const quint64 &tileId : tileIds) {
            s = QStringLiteral("DELETE FROM Tiles WHERE tileID = %1").arg(tileId);
            if (!query.exec(s)) {
                break;
            }
        }
        return true;
    }
    return false;
}

bool QGCTileCacheDatabase::deleteTileSet(quint64 setID) const
{
    QSqlQuery query(_db);
    QString s;
    s = QStringLiteral("DELETE FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID WHERE B.setID = %1 GROUP BY A.tileID HAVING COUNT(A.tileID) = 1)").arg(setID);
    (void) query.exec(s);
    s = QStringLiteral("DELETE FROM TilesDownload WHERE setID = %1").arg(setID);
    (void) query.exec(s);
    s = QStringLiteral("DELETE FROM TileSets WHERE setID = %1").arg(setID);
    (void) query.exec(s);
    s = QStringLiteral("DELETE FROM SetTiles WHERE setID = %1").arg(setID);
    (void) query.exec(s);
    (void) updateTotals(*(new quint32()), *(new quint64()), *(new quint32()), *(new quint64()));
    return true;
}

bool QGCTileCacheDatabase::renameTileSet(quint64 setID, const QString &newName) const
{
    QSqlQuery query(_db);
    const QString s = QStringLiteral("UPDATE TileSets SET name = \"%1\" WHERE setID = %2").arg(newName).arg(setID);
    if (!query.exec(s)) {
        qCWarning(QGCTileCacheDatabaseLog) << "renameTileSet: Error renaming:" << query.lastError().text();
        return false;
    }
    return true;
}

bool QGCTileCacheDatabase::resetCacheDatabase() const
{
    QSqlQuery query(_db);
    (void) query.exec("DROP TABLE Tiles");
    (void) query.exec("DROP TABLE TileSets");
    (void) query.exec("DROP TABLE SetTiles");
    (void) query.exec("DROP TABLE TilesDownload");
    const bool res = _createTables(true);
    return res;
}

bool QGCTileCacheDatabase::importSets(const QString &path, bool replace)
{
    if (replace) {
        disconnect();
        QFile file(_databasePath);
        (void) file.remove();
        (void) QFile::copy(path, _databasePath);
        if (!connect()) {
            return false;
        }
        if (!_createTables(true)) {
            return false;
        }
        // In a real implementation, progress reporting would be added here.
        return true;
    } else {
        QSqlDatabase dbImport = QSqlDatabase::addDatabase("QSQLITE", kExportSession);
        dbImport.setDatabaseName(path);
        dbImport.setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
        if (!dbImport.open()) {
            return false;
        }
        QSqlQuery queryImport(dbImport);
        quint64 tileCount = 0;
        if (queryImport.exec("SELECT COUNT(tileID) FROM Tiles")) {
            if (queryImport.next()) {
                tileCount = queryImport.value(0).toULongLong();
            }
        }
        if (tileCount == 0) {
            tileCount = 1;
        }
        if (queryImport.exec("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC")) {
            while (queryImport.next()) {
                QString name = queryImport.value("name").toString();
                const quint64 setID = queryImport.value("setID").toULongLong();
                const QString mapType = queryImport.value("typeStr").toString();
                const double topleftLat = queryImport.value("topleftLat").toDouble();
                const double topleftLon = queryImport.value("topleftLon").toDouble();
                const double bottomRightLat = queryImport.value("bottomRightLat").toDouble();
                const double bottomRightLon = queryImport.value("bottomRightLon").toDouble();
                const int minZoom = queryImport.value("minZoom").toInt();
                const int maxZoom = queryImport.value("maxZoom").toInt();
                const int type = queryImport.value("type").toInt();
                const quint32 numTiles = queryImport.value("numTiles").toUInt();
                const int defaultSet = queryImport.value("defaultSet").toInt();
                quint64 insertSetID = getDefaultTileSet();
                if (!defaultSet) {
                    if (findTileSetID(name, insertSetID)) {
                        int testCount = 0;
                        while (true) {
                            const QString testName = QString("%1 %2").arg(name).arg(++testCount, 2, 10, QChar('0'));
                            if (!findTileSetID(testName, insertSetID) || testCount > 99) {
                                name = testName;
                                break;
                            }
                        }
                    }
                    QSqlQuery cQuery(_db);
                    (void) cQuery.prepare("INSERT INTO TileSets("
                                          "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, "
                                          "minZoom, maxZoom, type, numTiles, defaultSet, date) "
                                          "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
                    cQuery.addBindValue(name);
                    cQuery.addBindValue(mapType);
                    cQuery.addBindValue(topleftLat);
                    cQuery.addBindValue(topleftLon);
                    cQuery.addBindValue(bottomRightLat);
                    cQuery.addBindValue(bottomRightLon);
                    cQuery.addBindValue(minZoom);
                    cQuery.addBindValue(maxZoom);
                    cQuery.addBindValue(type);
                    cQuery.addBindValue(numTiles);
                    cQuery.addBindValue(defaultSet);
                    cQuery.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
                    if (!cQuery.exec()) {
                        qCWarning(QGCTileCacheDatabaseLog) << "importSets: Error adding imported tile set:" << cQuery.lastError().text();
                        return false;
                    } else {
                        insertSetID = cQuery.lastInsertId().toULongLong();
                    }
                }
                QSqlQuery cQuery(_db);
                QSqlQuery subQuery(dbImport);
                const QString sb = QStringLiteral("SELECT * FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID WHERE B.setID = %1 GROUP BY A.tileID HAVING COUNT(A.tileID) = 1)").arg(setID);
                if (subQuery.exec(sb)) {
                    (void) _db.transaction();
                    while (subQuery.next()) {
                        const QString hash = subQuery.value("hash").toString();
                        const QString format = subQuery.value("format").toString();
                        const QByteArray img = subQuery.value("tile").toByteArray();
                        const int typeVal = subQuery.value("type").toInt();
                        (void) cQuery.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)");
                        cQuery.addBindValue(hash);
                        cQuery.addBindValue(format);
                        cQuery.addBindValue(img);
                        cQuery.addBindValue(img.size());
                        cQuery.addBindValue(typeVal);
                        cQuery.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
                        if (cQuery.exec()) {
                            const quint64 importTileID = cQuery.lastInsertId().toULongLong();
                            const QString s = QStringLiteral("INSERT INTO SetTiles(tileID, setID) VALUES(%1, %2)").arg(importTileID).arg(insertSetID);
                            (void) cQuery.prepare(s);
                            (void) cQuery.exec();
                        }
                    }
                    (void) _db.commit();
                }
            }
        }
        dbImport.close();
        QSqlDatabase::removeDatabase(kExportSession);
        return true;
    }
}

bool QGCTileCacheDatabase::exportSets(const QString &path, const QList<QGCCachedTileSet*> &sets) const
{
    QFile file(path);
    if (file.exists()) {
        (void) file.remove();
    }
    QSqlDatabase dbExport = QSqlDatabase::addDatabase("QSQLITE", kExportSession);
    dbExport.setDatabaseName(path);
    dbExport.setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    if (!dbExport.open()) {
        qCCritical(QGCTileCacheDatabaseLog) << "exportSets: Failed to open export database:" << dbExport.lastError().text();
        return false;
    }
    QSqlQuery exportQuery(dbExport);
    if (!_createTables(false)) {
        return false;
    }
    quint64 tileCount = 0;
    quint64 currentCount = 0;
    for (const auto set : sets) {
        if (set->defaultSet()) {
            tileCount += set->totalTileCount();
        } else {
            tileCount += set->uniqueTileCount();
        }
    }
    if (tileCount == 0) {
        tileCount = 1;
    }
    for (auto set : sets) {
        (void) exportQuery.prepare("INSERT INTO TileSets("
                                   "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, "
                                   "minZoom, maxZoom, type, numTiles, defaultSet, date) "
                                   "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        exportQuery.addBindValue(set->name());
        exportQuery.addBindValue(set->mapTypeStr());
        exportQuery.addBindValue(set->topleftLat());
        exportQuery.addBindValue(set->topleftLon());
        exportQuery.addBindValue(set->bottomRightLat());
        exportQuery.addBindValue(set->bottomRightLon());
        exportQuery.addBindValue(set->minZoom());
        exportQuery.addBindValue(set->maxZoom());
        exportQuery.addBindValue(UrlFactory::getQtMapIdFromProviderType(set->type()));
        exportQuery.addBindValue(set->totalTileCount());
        exportQuery.addBindValue(set->defaultSet());
        exportQuery.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
        if (!exportQuery.exec()) {
            qCWarning(QGCTileCacheDatabaseLog) << "exportSets: Error adding tile set to export db:" << exportQuery.lastError().text();
            return false;
        } else {
            const quint64 exportSetID = exportQuery.lastInsertId().toULongLong();
            const QString s = QStringLiteral("SELECT * FROM SetTiles WHERE setID = %1").arg(set->id());
            QSqlQuery query(_db);
            if (query.exec(s)) {
                (void) dbExport.transaction();
                while (query.next()) {
                    const quint64 tileID = query.value("tileID").toULongLong();
                    const QString s1 = QStringLiteral("SELECT * FROM Tiles WHERE tileID = \"%1\"").arg(tileID);
                    QSqlQuery subQuery(_db);
                    if (subQuery.exec(s1)) {
                        if (subQuery.next()) {
                            const QString hash = subQuery.value("hash").toString();
                            const QString format = subQuery.value("format").toString();
                            const QByteArray img = subQuery.value("tile").toByteArray();
                            const int type = subQuery.value("type").toInt();
                            (void) exportQuery.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)");
                            exportQuery.addBindValue(hash);
                            exportQuery.addBindValue(format);
                            exportQuery.addBindValue(img);
                            exportQuery.addBindValue(img.size());
                            exportQuery.addBindValue(type);
                            exportQuery.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
                            if (exportQuery.exec()) {
                                const quint64 exportTileID = exportQuery.lastInsertId().toULongLong();
                                const QString s2 = QStringLiteral("INSERT INTO SetTiles(tileID, setID) VALUES(%1, %2)").arg(exportTileID).arg(exportSetID);
                                (void) exportQuery.prepare(s2);
                                (void) exportQuery.exec();
                                currentCount++;
                                // (A progress callback can be invoked here.)
                            }
                        }
                    }
                }
                (void) dbExport.commit();
            }
        }
    }
    dbExport.close();
    QSqlDatabase::removeDatabase(kExportSession);
    return true;
}

bool QGCTileCacheDatabase::findTileSetID(const QString &name, quint64 &setID) const
{
    QSqlQuery query(_db);
    const QString s = QStringLiteral("SELECT setID FROM TileSets WHERE name = \"%1\"").arg(name);
    if (query.exec(s)) {
        if (query.next()) {
            setID = query.value(0).toULongLong();
            return true;
        }
    }
    return false;
}

quint64 QGCTileCacheDatabase::getDefaultTileSet() const
{
    static quint64 defaultSet = UINT64_MAX;
    if (defaultSet != UINT64_MAX) {
        return defaultSet;
    }
    QSqlQuery query(_db);
    const QString s = QStringLiteral("SELECT setID FROM TileSets WHERE defaultSet = 1");
    if (query.exec(s)) {
        if (query.next()) {
            defaultSet = query.value(0).toULongLong();
            return defaultSet;
        }
    }
    return 1;
}

bool QGCTileCacheDatabase::updateTotals(quint32 &totalTiles, quint64 &totalSize, quint32 &defaultTiles, quint64 &defaultSize) const
{
    QSqlQuery query(_db);
    QString s = "SELECT COUNT(size), SUM(size) FROM Tiles";
    if (query.exec(s)) {
        if (query.next()) {
            totalTiles = query.value(0).toUInt();
            totalSize = query.value(1).toULongLong();
        }
    }
    s = QStringLiteral("SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID WHERE B.setID = %1 GROUP BY A.tileID HAVING COUNT(A.tileID) = 1)").arg(getDefaultTileSet());
    if (query.exec(s)) {
        if (query.next()) {
            defaultTiles = query.value(0).toUInt();
            defaultSize = query.value(1).toULongLong();
        }
    }
    return true;
}

quint64 QGCTileCacheDatabase::_findTile(const QString &hash) const
{
    quint64 tileID = 0;
    QSqlQuery query(_db);
    const QString s = QStringLiteral("SELECT tileID FROM Tiles WHERE hash = \"%1\"").arg(hash);
    if (query.exec(s)) {
        if (query.next()) {
            tileID = query.value(0).toULongLong();
        }
    }
    return tileID;
}
