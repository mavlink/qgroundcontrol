/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Map Tile Cache Worker Thread
 *
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#include "QGCTileCacheWorker.h"
#include "QGCMapEngine.h"
#include "QGCCachedTileSet.h"
#include "QGCMapUrlEngine.h"
#include "QGCMapEngineData.h"
#include "QGCLoggingCategory.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QSettings>

#include "time.h"

static const QString kDefaultSet = QStringLiteral("Default Tile Set");
static const QString kSession = QStringLiteral("QGeoTileWorkerSession");
static const QString kExportSession = QStringLiteral("QGeoTileExportSession");

QGC_LOGGING_CATEGORY(QGCTileCacheLog, "qgc.qtlocationplugin.qgctilecacheworker")

#define LONG_TIMEOUT 5
#define SHORT_TIMEOUT 2

QGCCacheWorker::QGCCacheWorker(QObject* parent)
    : QThread(parent)
    , _db(std::make_unique<QSqlDatabase>())
    , _updateTimeout(SHORT_TIMEOUT)
{
    qCDebug(QGCTileCacheLog) << Q_FUNC_INFO << this;
}

QGCCacheWorker::~QGCCacheWorker()
{
    qCDebug(QGCTileCacheLog) << Q_FUNC_INFO << this;
}

void QGCCacheWorker::stopRunning()
{
    QMutexLocker lock(&_taskQueueMutex);
    while (_taskQueue.count()) {
        QGCMapTask* const task = _taskQueue.dequeue();
        delete task;
    }
    lock.unlock();

    if (isRunning()) {
        _waitc.wakeAll();
    }
}

bool QGCCacheWorker::enqueueTask(QGCMapTask* task)
{
    if (!_valid && (task->type() != QGCMapTask::taskInit)) {
        task->setError("Database Not Initialized");
        task->deleteLater();
        return false;
    }

    QMutexLocker lock(&_taskQueueMutex);
    _taskQueue.enqueue(task);
    lock.unlock();

    if (isRunning()) {
        _waitc.wakeAll();
    } else {
        start(QThread::HighPriority);
    }

    return true;
}

void QGCCacheWorker::run()
{
    if (!_valid && !_failed) {
        _init();
    }

    if (_valid) {
        _connectDB();
    }

    _deleteBingNoTileTiles();

    QMutexLocker lock(&_taskQueueMutex);
    while (true) {
        if (!_taskQueue.isEmpty()) {
            QGCMapTask* const task = _taskQueue.dequeue();
            lock.unlock();
            _runTask(task);
            lock.relock();
            task->deleteLater();

            const size_t count = static_cast<size_t>(_taskQueue.count());
            if (count > 100) {
                _updateTimeout = LONG_TIMEOUT;
            } else if (count < 25) {
                _updateTimeout = SHORT_TIMEOUT;
            }

            if (!count || ((time(nullptr) - _lastUpdate) > _updateTimeout)) {
                if (_valid) {
                    lock.unlock();
                    _updateTotals();
                    lock.relock();
                }
            }
        } else {
            const uint32_t timeoutMilliseconds = 5000;
            _waitc.wait(lock.mutex(), timeoutMilliseconds);
            if (_taskQueue.isEmpty()) {
                break;
            }
        }
    }

    lock.unlock();
    _disconnectDB();
}

void QGCCacheWorker::_runTask(QGCMapTask *task)
{
    switch (task->type()) {
        case QGCMapTask::taskInit:
            break;

        case QGCMapTask::taskCacheTile:
            _saveTile(task);
            break;

        case QGCMapTask::taskFetchTile:
            _getTile(task);
            break;

        case QGCMapTask::taskFetchTileSets:
            _getTileSets(task);
            break;

        case QGCMapTask::taskCreateTileSet:
            _createTileSet(task);
            break;

        case QGCMapTask::taskGetTileDownloadList:
            _getTileDownloadList(task);
            break;

        case QGCMapTask::taskUpdateTileDownloadState:
            _updateTileDownloadState(task);
            break;

        case QGCMapTask::taskDeleteTileSet:
            _deleteTileSet(task);
            break;

        case QGCMapTask::taskRenameTileSet:
            _renameTileSet(task);
            break;

        case QGCMapTask::taskPruneCache:
            _pruneCache(task);
            break;

        case QGCMapTask::taskReset:
            _resetCacheDatabase(task);
            break;

        case QGCMapTask::taskExport:
            _exportSets(task);
            break;

        case QGCMapTask::taskImport:
            _importSets(task);
            break;

        default:
            qCWarning(QGCTileCacheLog) << Q_FUNC_INFO << "given unhandled task type" << task->type();
            break;
    }
}

void QGCCacheWorker::_deleteBingNoTileTiles()
{
    QSettings settings;
    static const QString alreadyDoneKey = QStringLiteral("_deleteBingNoTileTilesDone");

    if (settings.value(alreadyDoneKey, false).toBool()) {
        return;
    }
    settings.setValue(alreadyDoneKey, true);

    // Previously we would store these empty tile graphics in the cache. This prevented the ability to zoom beyond the level
    // of available tiles. So we need to remove only of these still hanging around to make higher zoom levels work.
    QFile file(":/res/BingNoTileBytes.dat");
    file.open(QFile::ReadOnly);
    const QByteArray noTileBytes = file.readAll();
    file.close();

    //-- Select tiles in default set only, sorted by oldest.
    QSqlQuery query(*_db);
    QString s = QString("SELECT tileID, tile, hash FROM Tiles WHERE LENGTH(tile) = %1").arg(noTileBytes.length());
    QList<quint64> idsToDelete;
    if (query.exec(s)) {
        while (query.next()) {
            if (query.value(1).toByteArray() == noTileBytes) {
                idsToDelete.append(query.value(0).toULongLong());
                qCDebug(QGCTileCacheLog) << Q_FUNC_INFO << "HASH:" << query.value(2).toString();
            }
        }

        for (const quint64 tileId: idsToDelete) {
            s = QString("DELETE FROM Tiles WHERE tileID = %1").arg(tileId);
            if (!query.exec(s)) {
                qCWarning(QGCTileCacheLog) << "Delete failed";
            }
        }
    } else {
        qCWarning(QGCTileCacheLog) << Q_FUNC_INFO << "query failed";
    }
}

bool QGCCacheWorker::_findTileSetID(const QString &name, quint64& setID)
{
    QSqlQuery query(*_db);
    const QString s = QString("SELECT setID FROM TileSets WHERE name = \"%1\"").arg(name);
    if (query.exec(s)) {
        if (query.next()) {
            setID = query.value(0).toULongLong();
            return true;
        }
    }

    return false;
}

quint64 QGCCacheWorker::_getDefaultTileSet()
{
    if (_defaultSet != UINT64_MAX) {
        return _defaultSet;
    }

    QSqlQuery query(*_db);
    const QString s = QString("SELECT setID FROM TileSets WHERE defaultSet = 1");
    if (query.exec(s)) {
        if (query.next()) {
            _defaultSet = query.value(0).toULongLong();
            return _defaultSet;
        }
    }

    return 1L;
}

void QGCCacheWorker::_saveTile(QGCMapTask *mtask)
{
    if (!_valid) {
        qCWarning(QGCTileCacheLog) << "Map Cache SQL error (saveTile() open db):" << _db->lastError();
        return;
    }

    QGCSaveTileTask* const task = static_cast<QGCSaveTileTask*>(mtask);

    QSqlQuery query(*_db);
    (void) query.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)");
    query.addBindValue(task->tile()->hash());
    query.addBindValue(task->tile()->format());
    query.addBindValue(task->tile()->img());
    query.addBindValue(task->tile()->img().size());
    query.addBindValue(task->tile()->type());
    query.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
    if (query.exec()) {
        const quint64 tileID = query.lastInsertId().toULongLong();
        const quint64 setID = task->tile()->set() == UINT64_MAX ? _getDefaultTileSet() : task->tile()->set();
        const QString s = QString("INSERT INTO SetTiles(tileID, setID) VALUES(%1, %2)").arg(tileID).arg(setID);
        (void) query.prepare(s);
        if (!query.exec()) {
            qCWarning(QGCTileCacheLog) << "Map Cache SQL error (add tile into SetTiles):" << query.lastError().text();
        }
        qCDebug(QGCTileCacheLog) << Q_FUNC_INFO << "HASH:" << task->tile()->hash();
    }
}

void QGCCacheWorker::_getTile(QGCMapTask* mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCFetchTileTask* const task = static_cast<QGCFetchTileTask*>(mtask);
    bool found = false;

    QSqlQuery query(*_db);
    const QString s = QString("SELECT tile, format, type FROM Tiles WHERE hash = \"%1\"").arg(task->hash());
    if (query.exec(s)) {
        if (query.next()) {
            const QByteArray array = query.value(0).toByteArray();
            const QString format = query.value(1).toString();
            const QString type = query.value(2).toString();
            QGCCacheTile* tile = new QGCCacheTile(task->hash(), array, format, type);
            task->setTileFetched(tile);
            found = true;
        }
    }

    if (found) {
        qCDebug(QGCTileCacheLog) << Q_FUNC_INFO << "(Found in DB) HASH:" << task->hash();
    } else {
        qCDebug(QGCTileCacheLog) << Q_FUNC_INFO << "(NOT in DB) HASH:" << task->hash();
        task->setError("Tile not in cache database");
    }
}

void QGCCacheWorker::_getTileSets(QGCMapTask* mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCFetchTileSetTask* const task = static_cast<QGCFetchTileSetTask*>(mtask);

    QSqlQuery query(*_db);
    const QString s = QString("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC");
    qCDebug(QGCTileCacheLog) << Q_FUNC_INFO << s;
    if (query.exec(s)) {
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
            _updateSetTotals(set);
            set->moveToThread(QCoreApplication::instance()->thread());
            task->setTileSetFetched(set);
        }
    } else {
        task->setError("No tile set in database");
    }
}

void QGCCacheWorker::_updateSetTotals(QGCCachedTileSet* set)
{
    if (set->defaultSet()) {
        _updateTotals();
        set->setSavedTileCount(_totalCount);
        set->setSavedTileSize(_totalSize);
        set->setTotalTileCount(_defaultCount);
        set->setTotalTileSize(_defaultSize);
        return;
    }

    QSqlQuery subquery(*_db);
    QString sq = QString("SELECT COUNT(size), SUM(size) FROM Tiles A INNER JOIN SetTiles B on A.tileID = B.tileID WHERE B.setID = %1").arg(set->id());
    qCDebug(QGCTileCacheLog) << Q_FUNC_INFO << sq;
    if (subquery.exec(sq)) {
        if (subquery.next()) {
            set->setSavedTileCount(subquery.value(0).toUInt());
            set->setSavedTileSize(subquery.value(1).toULongLong());
            qCDebug(QGCTileCacheLog) << "Set" << set->id() << "Totals:" << set->savedTileCount() << " " << set->savedTileSize() << "Expected: " << set->totalTileCount() << " " << set->totalTilesSize();

            quint64 avg = UrlFactory::averageSizeForType(set->type());
            if (set->totalTileCount() <= set->savedTileCount()) {
                set->setTotalTileSize(set->savedTileSize());
            } else {
                if(set->savedTileCount() > 10 && set->savedTileSize()) {
                    avg = set->savedTileSize() / set->savedTileCount();
                }
                set->setTotalTileSize(avg * set->totalTileCount());
            }

            quint32 ucount = 0;
            quint64 usize  = 0;

            sq = QString("SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A join SetTiles B on A.tileID = B.tileID WHERE B.setID = %1 GROUP by A.tileID HAVING COUNT(A.tileID) = 1)").arg(set->id());
            if (subquery.exec(sq)) {
                if (subquery.next()) {
                    //-- This is only accurate when all tiles are downloaded
                    ucount = subquery.value(0).toUInt();
                    usize = subquery.value(1).toULongLong();
                }
            }

            //-- If we haven't downloaded it all, estimate size of unique tiles
            quint32 expectedUcount = set->totalTileCount() - set->savedTileCount();
            if (!ucount) {
                usize = expectedUcount * avg;
            } else {
                expectedUcount = ucount;
            }
            set->setUniqueTileCount(expectedUcount);
            set->setUniqueTileSize(usize);
        }
    }
}

void QGCCacheWorker::_updateTotals()
{
    QSqlQuery query(*_db);
    QString s = QString("SELECT COUNT(size), SUM(size) FROM Tiles");
    qCDebug(QGCTileCacheLog) << Q_FUNC_INFO << s;
    if (query.exec(s)) {
        if (query.next()) {
            _totalCount = query.value(0).toUInt();
            _totalSize = query.value(1).toULongLong();
        }
    }

    s = QString("SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A join SetTiles B on A.tileID = B.tileID WHERE B.setID = %1 GROUP by A.tileID HAVING COUNT(A.tileID) = 1)").arg(_getDefaultTileSet());
    qCDebug(QGCTileCacheLog) << Q_FUNC_INFO << s;
    if (query.exec(s)) {
        if (query.next()) {
            _defaultCount = query.value(0).toUInt();
            _defaultSize  = query.value(1).toULongLong();
        }
    }

    emit updateTotals(_totalCount, _totalSize, _defaultCount, _defaultSize);
    _lastUpdate = time(nullptr);
}

quint64 QGCCacheWorker::_findTile(const QString &hash)
{
    quint64 tileID = 0;

    QSqlQuery query(*_db);
    const QString s = QString("SELECT tileID FROM Tiles WHERE hash = \"%1\"").arg(hash);
    if (query.exec(s)) {
        if (query.next()) {
            tileID = query.value(0).toULongLong();
        }
    }

    return tileID;
}

void QGCCacheWorker::_createTileSet(QGCMapTask *mtask)
{
    QGCCreateTileSetTask* const task = static_cast<QGCCreateTileSetTask*>(mtask);

    if (_valid) {
        QGCCachedTileSet* tileSet = task->tileSet();

        QSqlQuery query(*_db);
        (void) query.prepare("INSERT INTO TileSets("
            "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, maxZoom, type, numTiles, date"
            ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
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
            qCWarning(QGCTileCacheLog) << "Map Cache SQL error (add tileSet into TileSets):" << query.lastError().text();
        } else {
            const quint64 setID = query.lastInsertId().toULongLong();
            tileSet->setId(setID);
            _db->transaction();

            for (int z = tileSet->minZoom(); z <= tileSet->maxZoom(); z++) {
                const QGCTileSet set = UrlFactory::getTileCount(z,
                    tileSet->topleftLon(), tileSet->topleftLat(),
                    tileSet->bottomRightLon(), tileSet->bottomRightLat(), tileSet->type());
                const QString type = tileSet->type();
                for (int x = set.tileX0; x <= set.tileX1; x++) {
                    for (int y = set.tileY0; y <= set.tileY1; y++) {
                        const QString hash = UrlFactory::getTileHash(type, x, y, z);
                        const quint64 tileID = _findTile(hash);
                        if (!tileID) {
                            (void) query.prepare("INSERT OR IGNORE INTO TilesDownload(setID, hash, type, x, y, z, state) VALUES(?, ?, ?, ?, ? ,? ,?)");
                            query.addBindValue(setID);
                            query.addBindValue(hash);
                            query.addBindValue(UrlFactory::getQtMapIdFromProviderType(type));
                            query.addBindValue(x);
                            query.addBindValue(y);
                            query.addBindValue(z);
                            query.addBindValue(0);
                            if (!query.exec()) {
                                qCWarning(QGCTileCacheLog) << "Map Cache SQL error (add tile into TilesDownload):" << query.lastError().text();
                                mtask->setError("Error creating tile set download list");
                                return;
                            }
                        } else {
                            const QString s = QString("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(%1, %2)").arg(tileID).arg(setID);
                            (void) query.prepare(s);
                            if (!query.exec()) {
                                qCWarning(QGCTileCacheLog) << "Map Cache SQL error (add tile into SetTiles):" << query.lastError().text();
                            }
                            qCDebug(QGCTileCacheLog) << Q_FUNC_INFO << "Already Cached HASH:" << hash;
                        }
                    }
                }
            }

            _db->commit();
            _updateSetTotals(tileSet);
            task->setTileSetSaved();
            return;
        }
    }

    task->setError("Error saving tile set");
}

void QGCCacheWorker::_getTileDownloadList(QGCMapTask* mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCGetTileDownloadListTask* const task = static_cast<QGCGetTileDownloadListTask*>(mtask);

    QList<QGCTile*> tiles;
    QSqlQuery query(*_db);
    QString s = QString("SELECT hash, type, x, y, z FROM TilesDownload WHERE setID = %1 AND state = 0 LIMIT %2").arg(task->setID()).arg(task->count());
    if (query.exec(s)) {
        while (query.next()) {
            QGCTile* tile = new QGCTile();
            tile->setHash(query.value("hash").toString());
            tile->setType(UrlFactory::getProviderTypeFromQtMapId(query.value("type").toInt()));
            tile->setX(query.value("x").toInt());
            tile->setY(query.value("y").toInt());
            tile->setZ(query.value("z").toInt());
            tiles.append(tile);
        }

        for (size_t i = 0; i < tiles.size(); i++) {
            s = QString("UPDATE TilesDownload SET state = %1 WHERE setID = %2 and hash = \"%3\"").arg(static_cast<int>(QGCTile::StateDownloading)).arg(task->setID()).arg(tiles[i]->hash());
            if (!query.exec(s)) {
                qCWarning(QGCTileCacheLog) << "Map Cache SQL error (set TilesDownload state):" << query.lastError().text();
            }
        }
    }

    task->setTileListFetched(tiles);
}

void QGCCacheWorker::_updateTileDownloadState(QGCMapTask* mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCUpdateTileDownloadStateTask* const task = static_cast<QGCUpdateTileDownloadStateTask*>(mtask);

    QSqlQuery query(*_db);
    QString s;
    if (task->state() == QGCTile::StateComplete) {
        s = QString("DELETE FROM TilesDownload WHERE setID = %1 AND hash = \"%2\"").arg(task->setID()).arg(task->hash());
    } else if (task->hash() == "*") {
        s = QString("UPDATE TilesDownload SET state = %1 WHERE setID = %2").arg(static_cast<int>(task->state())).arg(task->setID());
    } else {
        s = QString("UPDATE TilesDownload SET state = %1 WHERE setID = %2 AND hash = \"%3\"").arg(static_cast<int>(task->state())).arg(task->setID()).arg(task->hash());
    }

    if (!query.exec(s)) {
        qCWarning(QGCTileCacheLog) << Q_FUNC_INFO << "Error:" << query.lastError().text();
    }
}

void QGCCacheWorker::_pruneCache(QGCMapTask* mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCPruneCacheTask* const task = static_cast<QGCPruneCacheTask*>(mtask);

    QSqlQuery query(*_db);
    QString s = QString("SELECT tileID, size, hash FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A join SetTiles B on A.tileID = B.tileID WHERE B.setID = %1 GROUP by A.tileID HAVING COUNT(A.tileID) = 1) ORDER BY DATE ASC LIMIT 128").arg(_getDefaultTileSet());
    qint64 amount = (qint64)task->amount();
    QList<quint64> tlist;
    if (query.exec(s)) {
        while (query.next() && (amount >= 0)) {
            tlist << query.value(0).toULongLong();
            amount -= query.value(1).toULongLong();
            qCDebug(QGCTileCacheLog) << Q_FUNC_INFO << "HASH:" << query.value(2).toString();
        }

        while (tlist.count()) {
            s = QString("DELETE FROM Tiles WHERE tileID = %1").arg(tlist.first());
            tlist.removeFirst();
            if (!query.exec(s)) {
                break;
            }
        }

        task->setPruned();
    }
}

void QGCCacheWorker::_deleteTileSet(QGCMapTask* mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCDeleteTileSetTask* const task = static_cast<QGCDeleteTileSetTask*>(mtask);
    _deleteTileSet(task->setID());
    task->setTileSetDeleted();
}

void QGCCacheWorker::_deleteTileSet(qulonglong id)
{
    QSqlQuery query(*_db);
    QString s = QString("DELETE FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID WHERE B.setID = %1 GROUP BY A.tileID HAVING COUNT(A.tileID) = 1)").arg(id);
    (void) query.exec(s);
    s = QString("DELETE FROM TilesDownload WHERE setID = %1").arg(id);
    (void) query.exec(s);
    s = QString("DELETE FROM TileSets WHERE setID = %1").arg(id);
    (void) query.exec(s);
    s = QString("DELETE FROM SetTiles WHERE setID = %1").arg(id);
    (void) query.exec(s);
    _updateTotals();
}

void QGCCacheWorker::_renameTileSet(QGCMapTask* mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCRenameTileSetTask* const task = static_cast<QGCRenameTileSetTask*>(mtask);

    QSqlQuery query(*_db);
    const QString s = QString("UPDATE TileSets SET name = \"%1\" WHERE setID = %2").arg(task->newName()).arg(task->setID());
    if (!query.exec(s)) {
        task->setError("Error renaming tile set");
    }
}

void QGCCacheWorker::_resetCacheDatabase(QGCMapTask* mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCResetTask* const task = static_cast<QGCResetTask*>(mtask);

    QSqlQuery query(*_db);
    QString s = QString("DROP TABLE Tiles");
    (void) query.exec(s);
    s = QString("DROP TABLE TileSets");
    (void) query.exec(s);
    s = QString("DROP TABLE SetTiles");
    (void) query.exec(s);
    s = QString("DROP TABLE TilesDownload");
    (void) query.exec(s);

    _valid = _createDB(*_db);
    task->setResetCompleted();
}

void QGCCacheWorker::_importSets(QGCMapTask* mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCImportTileTask* const task = static_cast<QGCImportTileTask*>(mtask);
    if (task->replace()) {
        _disconnectDB();
        QFile file(_databasePath);
        (void) file.remove();
        (void) QFile::copy(task->path(), _databasePath);
        task->setProgress(25);
        _init();
        if (_valid) {
            task->setProgress(50);
            _connectDB();
        }
        task->setProgress(100);
    } else {
        QSqlDatabase* const dbImport = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", kExportSession));
        dbImport->setDatabaseName(task->path());
        dbImport->setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
        if (dbImport->open()) {
            quint64 tileCount = 0;
            quint64 currentCount = 0;
            int lastProgress = -1;
            QSqlQuery query(*dbImport);
            QString s = QString("SELECT COUNT(tileID) FROM Tiles");
            if (query.exec(s)) {
                if (query.next()) {
                    tileCount = query.value(0).toULongLong();
                }
            }

            if (tileCount) {
                s = QString("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC");
                if (query.exec(s)) {
                    while (query.next()) {
                        QString name                  = query.value("name").toString();
                        const quint64 setID           = query.value("setID").toULongLong();
                        const QString mapType         = query.value("typeStr").toString();
                        const double  topleftLat      = query.value("topleftLat").toDouble();
                        const double  topleftLon      = query.value("topleftLon").toDouble();
                        const double  bottomRightLat  = query.value("bottomRightLat").toDouble();
                        const double  bottomRightLon  = query.value("bottomRightLon").toDouble();
                        const int     minZoom         = query.value("minZoom").toInt();
                        const int     maxZoom         = query.value("maxZoom").toInt();
                        const int     type            = query.value("type").toInt();
                        const quint32 numTiles        = query.value("numTiles").toUInt();
                        const int     defaultSet      = query.value("defaultSet").toInt();
                        quint64 insertSetID           = _getDefaultTileSet();
                        if (!defaultSet) {
                            if (_findTileSetID(name, insertSetID)) {
                                int testCount = 0;
                                while (true) {
                                    const QString testName = QString::asprintf("%s %02d", name.toLatin1().data(), ++testCount);
                                    if (!_findTileSetID(testName, insertSetID) || testCount > 99) {
                                        name = testName;
                                        break;
                                    }
                                }
                            }

                            QSqlQuery cQuery(*_db);
                            (void) cQuery.prepare("INSERT INTO TileSets("
                                "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, maxZoom, type, numTiles, defaultSet, date"
                                ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
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
                                task->setError("Error adding imported tile set to database");
                                break;
                            } else {
                                insertSetID = cQuery.lastInsertId().toULongLong();
                            }
                        }

                        QSqlQuery cQuery(*_db);
                        QSqlQuery subQuery(*dbImport);
                        const QString sb = QString("SELECT * FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID WHERE B.setID = %1 GROUP BY A.tileID HAVING COUNT(A.tileID) = 1)").arg(setID);
                        if (subQuery.exec(sb)) {
                            quint64 tilesFound = 0;
                            quint64 tilesSaved = 0;
                            _db->transaction();
                            while (subQuery.next()) {
                                tilesFound++;
                                const QString hash    = subQuery.value("hash").toString();
                                const QString format  = subQuery.value("format").toString();
                                const QByteArray img  = subQuery.value("tile").toByteArray();
                                const int type        = subQuery.value("type").toInt();
                                (void) cQuery.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)");
                                cQuery.addBindValue(hash);
                                cQuery.addBindValue(format);
                                cQuery.addBindValue(img);
                                cQuery.addBindValue(img.size());
                                cQuery.addBindValue(type);
                                cQuery.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
                                if (cQuery.exec()) {
                                    tilesSaved++;
                                    const quint64 importTileID = cQuery.lastInsertId().toULongLong();
                                    const QString s = QString("INSERT INTO SetTiles(tileID, setID) VALUES(%1, %2)").arg(importTileID).arg(insertSetID);
                                    (void) cQuery.prepare(s);
                                    (void) cQuery.exec();
                                    currentCount++;
                                    if (tileCount) {
                                        const int progress = (int)((double)currentCount / (double)tileCount * 100.0);
                                        if (lastProgress != progress) {
                                            lastProgress = progress;
                                            task->setProgress(progress);
                                        }
                                    }
                                }
                            }

                            _db->commit();
                            if (tilesSaved) {
                                s = QString("SELECT COUNT(size) FROM Tiles A INNER JOIN SetTiles B on A.tileID = B.tileID WHERE B.setID = %1").arg(insertSetID);
                                if (cQuery.exec(s)) {
                                    if (cQuery.next()) {
                                        const quint64 count = cQuery.value(0).toULongLong();
                                        s = QString("UPDATE TileSets SET numTiles = %1 WHERE setID = %2").arg(count).arg(insertSetID);
                                        (void) cQuery.exec(s);
                                    }
                                }
                            }

                            const qint64 uniqueTiles = tilesFound - tilesSaved;
                            if ((quint64)uniqueTiles < tileCount) {
                                tileCount -= uniqueTiles;
                            } else {
                                tileCount = 0;
                            }

                            if (!tilesSaved && !defaultSet) {
                                _deleteTileSet(insertSetID);
                                qCDebug(QGCTileCacheLog) << "No unique tiles in" << name << "Removing it.";
                            }
                        }
                    }
                } else {
                    task->setError("No tile set in database");
                }
            }

            delete dbImport;
            QSqlDatabase::removeDatabase(kExportSession);
            if (!tileCount) {
                task->setError("No unique tiles in imported database");
            }
        } else {
            task->setError("Error opening import database");
        }
    }

    task->setImportCompleted();
}

void QGCCacheWorker::_exportSets(QGCMapTask* mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCExportTileTask* const task = static_cast<QGCExportTileTask*>(mtask);

    QFile file(task->path());
    file.remove();

    std::unique_ptr<QSqlDatabase> dbExport = std::make_unique<QSqlDatabase>(QSqlDatabase::addDatabase("QSQLITE", kExportSession));
    dbExport->setDatabaseName(task->path());
    dbExport->setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    if (dbExport->open()) {
        if (_createDB(*dbExport, false)) {
            quint64 tileCount = 0;
            quint64 currentCount = 0;
            for (size_t i = 0; i < task->sets().count(); i++) {
                const QGCCachedTileSet* const set = task->sets().at(i);
                if (set->defaultSet()) {
                    tileCount += set->totalTileCount();
                } else {
                    tileCount += set->uniqueTileCount();
                }
            }

            if (!tileCount) {
                tileCount = 1;
            }

            for (size_t i = 0; i < task->sets().count(); i++) {
                const QGCCachedTileSet* const set = task->sets().at(i);

                QSqlQuery exportQuery(*dbExport);
                (void) exportQuery.prepare("INSERT INTO TileSets("
                    "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, maxZoom, type, numTiles, defaultSet, date"
                    ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
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
                    task->setError("Error adding tile set to exported database");
                    break;
                } else {
                    const quint64 exportSetID = exportQuery.lastInsertId().toULongLong();

                    QString s = QString("SELECT * FROM SetTiles WHERE setID = %1").arg(set->id());
                    QSqlQuery query(*_db);
                    if (query.exec(s)) {
                        dbExport->transaction();
                        while (query.next()) {
                            const quint64 tileID = query.value("tileID").toULongLong();

                            s = QString("SELECT * FROM Tiles WHERE tileID = \"%1\"").arg(tileID);
                            QSqlQuery subQuery(*_db);
                            if (subQuery.exec(s)) {
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
                                        s = QString("INSERT INTO SetTiles(tileID, setID) VALUES(%1, %2)").arg(exportTileID).arg(exportSetID);
                                        (void) exportQuery.prepare(s);
                                        (void) exportQuery.exec();
                                        currentCount++;
                                        task->setProgress((int)((double)currentCount / (double)tileCount * 100.0));
                                    }
                                }
                            }
                        }
                    }
                    (void) dbExport->commit();
                }
            }
        } else {
            task->setError("Error creating export database");
        }
    } else {
        qCCritical(QGCTileCacheLog) << "Map Cache SQL error (create export database):" << dbExport->lastError();
        task->setError("Error opening export database");
    }

    dbExport.reset();
    QSqlDatabase::removeDatabase(kExportSession);
    task->setExportCompleted();
}

bool QGCCacheWorker::_testTask(QGCMapTask* mtask)
{
    if (!_valid) {
        mtask->setError("No Cache Database");
        return false;
    }

    return true;
}

bool QGCCacheWorker::_init()
{
    _failed = false;

    if (!_databasePath.isEmpty()) {
        qCDebug(QGCTileCacheLog) << "Mapping cache directory:" << _databasePath;
        if (_connectDB()) {
            _valid = _createDB(*_db);
            if(!_valid) {
                _failed = true;
            }
        } else {
            qCCritical(QGCTileCacheLog) << "Map Cache SQL error (init() open db):" << _db->lastError();
            _failed = true;
        }

        _disconnectDB();
    } else {
        qCCritical(QGCTileCacheLog) << "Could not find suitable cache directory.";
        _failed = true;
    }

    return _failed;
}

bool QGCCacheWorker::_connectDB()
{
    _db.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", kSession)));
    _db->setDatabaseName(_databasePath);
    _db->setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    _valid = _db->open();

    return _valid;
}

bool QGCCacheWorker::_createDB(QSqlDatabase& db, bool createDefault)
{
    bool res = false;
    QSqlQuery query(db);
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS Tiles ("
        "tileID INTEGER PRIMARY KEY NOT NULL, "
        "hash TEXT NOT NULL UNIQUE, "
        "format TEXT NOT NULL, "
        "tile BLOB NULL, "
        "size INTEGER, "
        "type INTEGER, "
        "date INTEGER DEFAULT 0)")) {
        qCWarning(QGCTileCacheLog) << "Map Cache SQL error (create Tiles db):" << query.lastError().text();
    } else {
        (void) query.exec("CREATE INDEX IF NOT EXISTS hash ON Tiles ( hash, size, type ) ");

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
            qCWarning(QGCTileCacheLog) << "Map Cache SQL error (create TileSets db):" << query.lastError().text();
        } else {
            if (!query.exec(
                "CREATE TABLE IF NOT EXISTS SetTiles ("
                "setID INTEGER, "
                "tileID INTEGER)")) {
                qCWarning(QGCTileCacheLog) << "Map Cache SQL error (create SetTiles db):" << query.lastError().text();
            } else {
                if (!query.exec(
                    "CREATE TABLE IF NOT EXISTS TilesDownload ("
                    "setID INTEGER, "
                    "hash TEXT NOT NULL UNIQUE, "
                    "type INTEGER, "
                    "x INTEGER, "
                    "y INTEGER, "
                    "z INTEGER, "
                    "state INTEGER DEFAULT 0)")) {
                    qCWarning(QGCTileCacheLog) << "Map Cache SQL error (create TilesDownload db):" << query.lastError().text();
                } else {
                    res = true;
                }
            }
        }
    }

    if (res && createDefault) {
        const QString s = QString("SELECT name FROM TileSets WHERE name = \"%1\"").arg(kDefaultSet);
        if (query.exec(s)) {
            if (!query.next()) {
                (void) query.prepare("INSERT INTO TileSets(name, defaultSet, date) VALUES(?, ?, ?)");
                query.addBindValue(kDefaultSet);
                query.addBindValue(1);
                query.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
                if (!query.exec()) {
                    qCWarning(QGCTileCacheLog) << "Map Cache SQL error (Creating default tile set):" << db.lastError();
                    res = false;
                }
            }
        } else {
            qCWarning(QGCTileCacheLog) << "Map Cache SQL error (Looking for default tile set):" << db.lastError();
        }
    }

    if (!res) {
        QFile file(_databasePath);
        (void) file.remove();
    }

    return res;
}

void QGCCacheWorker::_disconnectDB()
{
    if (_db) {
        _db.reset();
        QSqlDatabase::removeDatabase(kSession);
    }
}
