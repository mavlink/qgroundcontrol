/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "QGCCachedTileSet.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

QByteArray QGCCacheWorker::_bingNoTileImage;

QGC_LOGGING_CATEGORY(QGCTileCacheWorkerLog, "qgc.qtlocationplugin.qgctilecacheworker")

QGCCacheWorker::QGCCacheWorker(QObject* parent)
    : QThread(parent)
{
    // qCDebug(QGCTileCacheWorkerLog) << Q_FUNC_INFO << this;
}

QGCCacheWorker::~QGCCacheWorker()
{
    // qCDebug(QGCTileCacheWorkerLog) << Q_FUNC_INFO << this;
}

void QGCCacheWorker::stop()
{
    QMutexLocker lock(&_taskQueueMutex);
    qDeleteAll(_taskQueue);
    lock.unlock();

    if(this->isRunning()) {
        _waitc.wakeAll();
    }
}

bool QGCCacheWorker::enqueueTask(QGCMapTask *task)
{
    if (!_valid && (task->type() != QGCMapTask::taskInit)) {
        task->setError(tr("Database Not Initialized"));
        task->deleteLater();
        return false;
    }

    // TODO: Prepend Stop Task Instead?
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

//-----------------------------------------------------------------------------
void
QGCCacheWorker::run()
{
    if (!_valid && !_failed) {
        if (!_init()) {
            qCWarning(QGCTileCacheWorkerLog) << Q_FUNC_INFO << "Failed To Init Database";
            return;
        }
    }

    if (_valid) {
        if (_connectDB()) {
            _deleteBingNoTileTiles();
        }
    }

    QMutexLocker lock(&_taskQueueMutex);
    while (true) {
        if (!_taskQueue.isEmpty()) {
            QGCMapTask* const task = _taskQueue.dequeue();
            lock.unlock();
            _runTask(task);
            lock.relock();
            task->deleteLater();

            const qsizetype count = _taskQueue.count();
            if (count > 100) {
                _updateTimeout = kLongTimeout;
            } else if (count < 25) {
                _updateTimeout = kShortTimeout;
            }

            if ((count == 0) || _updateTimer.hasExpired(_updateTimeout)) {
                if (_valid) {
                    lock.unlock();
                    _updateTotals();
                    lock.relock();
                }
            }
        } else {
            (void) _waitc.wait(lock.mutex(), 5000);
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
        qCWarning(QGCTileCacheWorkerLog) << Q_FUNC_INFO << "given unhandled task type" << task->type();
        break;
    }
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_deleteBingNoTileTiles()
{
    QSettings settings;
    static const char* alreadyDoneKey = "_deleteBingNoTileTilesDone";

    if (settings.value(alreadyDoneKey, false).toBool()) {
        return;
    }
    settings.setValue(alreadyDoneKey, true);

    // Previously we would store these empty tile graphics in the cache. This prevented the ability to zoom beyong the level
    // of available tiles. So we need to remove only of these still hanging around to make higher zoom levels work.
    QFile file(":/res/BingNoTileBytes.dat");
    file.open(QFile::ReadOnly);
    QByteArray noTileBytes = file.readAll();
    file.close();

    QSqlQuery query(*_db);
    QString s;
    //-- Select tiles in default set only, sorted by oldest.
    s = QString("SELECT tileID, tile, hash FROM Tiles WHERE LENGTH(tile) = %1").arg(noTileBytes.length());
    QList<quint64> idsToDelete;
    if (query.exec(s)) {
        while(query.next()) {
            if (query.value(1).toByteArray() == noTileBytes) {
                idsToDelete.append(query.value(0).toULongLong());
                qCDebug(QGCTileCacheWorkerLog) << "_deleteBingNoTileTiles HASH:" << query.value(2).toString();
            }
        }
        for (const quint64 tileId: idsToDelete) {
            s = QString("DELETE FROM Tiles WHERE tileID = %1").arg(tileId);
            if (!query.exec(s)) {
                qCWarning(QGCTileCacheWorkerLog) << "Delete failed";
            }
        }
    } else {
        qCWarning(QGCTileCacheWorkerLog) << "_deleteBingNoTileTiles query failed";
    }
}

//-----------------------------------------------------------------------------
bool
QGCCacheWorker::_findTileSetID(const QString &name, quint64& setID)
{
    QSqlQuery query(*_db);
    QString s = QString("SELECT setID FROM TileSets WHERE name = \"%1\"").arg(name);
    if(query.exec(s)) {
        if(query.next()) {
            setID = query.value(0).toULongLong();
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
quint64
QGCCacheWorker::_getDefaultTileSet()
{
    if(_defaultSet != UINT64_MAX)
        return _defaultSet;
    QSqlQuery query(*_db);
    QString s = QString("SELECT setID FROM TileSets WHERE defaultSet = 1");
    if(query.exec(s)) {
        if(query.next()) {
            _defaultSet = query.value(0).toULongLong();
            return _defaultSet;
        }
    }
    return 1L;
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_saveTile(QGCMapTask *mtask)
{
    if(_valid) {
        QGCSaveTileTask* task = static_cast<QGCSaveTileTask*>(mtask);
        QSqlQuery query(*_db);
        query.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)");
        query.addBindValue(task->tile()->hash());
        query.addBindValue(task->tile()->format());
        query.addBindValue(task->tile()->img());
        query.addBindValue(task->tile()->img().size());
        query.addBindValue(task->tile()->type());
        query.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
        if(query.exec()) {
            quint64 tileID = query.lastInsertId().toULongLong();
            quint64 setID = task->tile()->tileSet() == UINT64_MAX ? _getDefaultTileSet() : task->tile()->tileSet();
            QString s = QString("INSERT INTO SetTiles(tileID, setID) VALUES(%1, %2)").arg(tileID).arg(setID);
            query.prepare(s);
            if(!query.exec()) {
                qWarning() << "Map Cache SQL error (add tile into SetTiles):" << query.lastError().text();
            }
            qCDebug(QGCTileCacheWorkerLog) << "_saveTile() HASH:" << task->tile()->hash();
        } else {
            //-- Tile was already there.
            //   QtLocation some times requests the same tile twice in a row. The first is saved, the second is already there.
        }
    } else {
        qWarning() << "Map Cache SQL error (saveTile() open db):" << _db->lastError();
    }
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_getTile(QGCMapTask* mtask)
{
    if(!_testTask(mtask)) {
        return;
    }
    bool found = false;
    QGCFetchTileTask* task = static_cast<QGCFetchTileTask*>(mtask);
    QSqlQuery query(*_db);
    QString s = QString("SELECT tile, format, type FROM Tiles WHERE hash = \"%1\"").arg(task->hash());
    if(query.exec(s)) {
        if(query.next()) {
            const QByteArray& arrray   = query.value(0).toByteArray();
            const QString& format  = query.value(1).toString();
            const QString& type = query.value(2).toString();
            qCDebug(QGCTileCacheWorkerLog) << "_getTile() (Found in DB) HASH:" << task->hash();
            QGCCacheTile* tile = new QGCCacheTile(task->hash(), arrray, format, type);
            task->setTileFetched(tile);
            found = true;
        }
    }
    if(!found) {
        qCDebug(QGCTileCacheWorkerLog) << "_getTile() (NOT in DB) HASH:" << task->hash();
        task->setError("Tile not in cache database");
    }
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_getTileSets(QGCMapTask* mtask)
{
    if(!_testTask(mtask)) {
        return;
    }
    QGCFetchTileSetTask* task = static_cast<QGCFetchTileSetTask*>(mtask);
    QSqlQuery query(*_db);
    QString s = QString("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC");
    qCDebug(QGCTileCacheWorkerLog) << "_getTileSets(): " << s;
    if(query.exec(s)) {
        while(query.next()) {
            QString name = query.value("name").toString();
            QGCCachedTileSet* set = new QGCCachedTileSet(name);
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
            //-- Object created here must be moved to app thread to be used there
            set->moveToThread(QCoreApplication::instance()->thread());
            task->tileSetFetched(set);
        }
    } else {
        task->setError("No tile set in database");
    }
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_updateSetTotals(QGCCachedTileSet* set)
{
    if(set->defaultSet()) {
        _updateTotals();
        set->setSavedTileCount(_totalCount);
        set->setSavedTileSize(_totalSize);
        set->setTotalTileCount(_defaultCount);
        set->setTotalTileSize(_defaultSize);
        return;
    }
    QSqlQuery subquery(*_db);
    QString sq = QString("SELECT COUNT(size), SUM(size) FROM Tiles A INNER JOIN SetTiles B on A.tileID = B.tileID WHERE B.setID = %1").arg(set->id());
    qCDebug(QGCTileCacheWorkerLog) << "_updateSetTotals(): " << sq;
    if(subquery.exec(sq)) {
        if(subquery.next()) {
            set->setSavedTileCount(subquery.value(0).toUInt());
            set->setSavedTileSize(subquery.value(1).toULongLong());
            qCDebug(QGCTileCacheWorkerLog) << "Set" << set->id() << "Totals:" << set->savedTileCount() << " " << set->savedTileSize() << "Expected: " << set->totalTileCount() << " " << set->totalTilesSize();
            //-- Update (estimated) size
            quint64 avg = UrlFactory::averageSizeForType(set->type());
            if(set->totalTileCount() <= set->savedTileCount()) {
                //-- We're done so the saved size is the total size
                set->setTotalTileSize(set->savedTileSize());
            } else {
                //-- Otherwise we need to estimate it.
                if(set->savedTileCount() > 10 && set->savedTileSize()) {
                    avg = set->savedTileSize() / set->savedTileCount();
                }
                set->setTotalTileSize(avg * set->totalTileCount());
            }
            //-- Now figure out the count for tiles unique to this set
            quint32 ucount = 0;
            quint64 usize  = 0;
            sq = QString("SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A join SetTiles B on A.tileID = B.tileID WHERE B.setID = %1 GROUP by A.tileID HAVING COUNT(A.tileID) = 1)").arg(set->id());
            if(subquery.exec(sq)) {
                if(subquery.next()) {
                    //-- This is only accurate when all tiles are downloaded
                    ucount = subquery.value(0).toUInt();
                    usize  = subquery.value(1).toULongLong();
                }
            }
            //-- If we haven't downloaded it all, estimate size of unique tiles
            quint32 expectedUcount = set->totalTileCount() - set->savedTileCount();
            if(!ucount) {
                usize = expectedUcount * avg;
            } else {
                expectedUcount = ucount;
            }
            set->setUniqueTileCount(expectedUcount);
            set->setUniqueTileSize(usize);
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_updateTotals()
{
    QSqlQuery query(*_db);
    QString s;
    s = QString("SELECT COUNT(size), SUM(size) FROM Tiles");
    qCDebug(QGCTileCacheWorkerLog) << "_updateTotals(): " << s;
    if(query.exec(s)) {
        if(query.next()) {
            _totalCount = query.value(0).toUInt();
            _totalSize  = query.value(1).toULongLong();
        }
    }
    s = QString("SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A join SetTiles B on A.tileID = B.tileID WHERE B.setID = %1 GROUP by A.tileID HAVING COUNT(A.tileID) = 1)").arg(_getDefaultTileSet());
    qCDebug(QGCTileCacheWorkerLog) << "_updateTotals(): " << s;
    if(query.exec(s)) {
        if(query.next()) {
            _defaultCount = query.value(0).toUInt();
            _defaultSize  = query.value(1).toULongLong();
        }
    }
    emit updateTotals(_totalCount, _totalSize, _defaultCount, _defaultSize);
    if (!_updateTimer.isValid()) {
        _updateTimer.start();
    } else {
        (void) _updateTimer.restart();
    }
}

//-----------------------------------------------------------------------------
quint64 QGCCacheWorker::_findTile(const QString &hash)
{
    quint64 tileID = 0;
    QSqlQuery query(*_db);
    QString s = QString("SELECT tileID FROM Tiles WHERE hash = \"%1\"").arg(hash);
    if(query.exec(s)) {
        if(query.next()) {
            tileID = query.value(0).toULongLong();
        }
    }
    return tileID;
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_createTileSet(QGCMapTask *mtask)
{
    if(_valid) {
        //-- Create Tile Set
        quint32 actual_count = 0;
        QGCCreateTileSetTask* task = static_cast<QGCCreateTileSetTask*>(mtask);
        QSqlQuery query(*_db);
        query.prepare("INSERT INTO TileSets("
            "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, maxZoom, type, numTiles, date"
            ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        query.addBindValue(task->tileSet()->name());
        query.addBindValue(task->tileSet()->mapTypeStr());
        query.addBindValue(task->tileSet()->topleftLat());
        query.addBindValue(task->tileSet()->topleftLon());
        query.addBindValue(task->tileSet()->bottomRightLat());
        query.addBindValue(task->tileSet()->bottomRightLon());
        query.addBindValue(task->tileSet()->minZoom());
        query.addBindValue(task->tileSet()->maxZoom());
        query.addBindValue(UrlFactory::getQtMapIdFromProviderType(task->tileSet()->type()));
        query.addBindValue(task->tileSet()->totalTileCount());
        query.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
        if(!query.exec()) {
            qWarning() << "Map Cache SQL error (add tileSet into TileSets):" << query.lastError().text();
        } else {
            //-- Get just created (auto-incremented) setID
            quint64 setID = query.lastInsertId().toULongLong();
            task->tileSet()->setId(setID);
            //-- Prepare Download List
            _db->transaction();
            for(int z = task->tileSet()->minZoom(); z <= task->tileSet()->maxZoom(); z++) {
                QGCTileSet set = UrlFactory::getTileCount(z,
                    task->tileSet()->topleftLon(), task->tileSet()->topleftLat(),
                    task->tileSet()->bottomRightLon(), task->tileSet()->bottomRightLat(), task->tileSet()->type());
                QString type = task->tileSet()->type();
                for(int x = set.tileX0; x <= set.tileX1; x++) {
                    for(int y = set.tileY0; y <= set.tileY1; y++) {
                        //-- See if tile is already downloaded
                        QString hash = UrlFactory::getTileHash(type, x, y, z);
                        quint64 tileID = _findTile(hash);
                        if(!tileID) {
                            //-- Set to download
                            query.prepare("INSERT OR IGNORE INTO TilesDownload(setID, hash, type, x, y, z, state) VALUES(?, ?, ?, ?, ? ,? ,?)");
                            query.addBindValue(setID);
                            query.addBindValue(hash);
                            query.addBindValue(UrlFactory::getQtMapIdFromProviderType(type));
                            query.addBindValue(x);
                            query.addBindValue(y);
                            query.addBindValue(z);
                            query.addBindValue(0);
                            if(!query.exec()) {
                                qWarning() << "Map Cache SQL error (add tile into TilesDownload):" << query.lastError().text();
                                mtask->setError("Error creating tile set download list");
                                return;
                            } else
                                actual_count++;
                        } else {
                            //-- Tile already in the database. No need to dowload.
                            QString s = QString("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(%1, %2)").arg(tileID).arg(setID);
                            query.prepare(s);
                            if(!query.exec()) {
                                qWarning() << "Map Cache SQL error (add tile into SetTiles):" << query.lastError().text();
                            }
                            qCDebug(QGCTileCacheWorkerLog) << "_createTileSet() Already Cached HASH:" << hash;
                        }
                    }
                }
            }
            _db->commit();
            //-- Done
            _updateSetTotals(task->tileSet());
            task->setTileSetSaved();
            return;
        }
    }
    mtask->setError("Error saving tile set");
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_getTileDownloadList(QGCMapTask* mtask)
{
    if(!_testTask(mtask)) {
        return;
    }
    QQueue<QGCTile*> tiles;
    QGCGetTileDownloadListTask* task = static_cast<QGCGetTileDownloadListTask*>(mtask);
    QSqlQuery query(*_db);
    QString s = QString("SELECT hash, type, x, y, z FROM TilesDownload WHERE setID = %1 AND state = 0 LIMIT %2").arg(task->setID()).arg(task->count());
    if(query.exec(s)) {
        while(query.next()) {
            QGCTile* tile = new QGCTile;
            // tile->setTileSet(task->setID());
            tile->setHash(query.value("hash").toString());
            tile->setType(UrlFactory::getProviderTypeFromQtMapId(query.value("type").toInt()));
            tile->setX(query.value("x").toInt());
            tile->setY(query.value("y").toInt());
            tile->setZ(query.value("z").toInt());
            tiles.enqueue(tile);
        }
        for(int i = 0; i < tiles.size(); i++) {
            s = QString("UPDATE TilesDownload SET state = %1 WHERE setID = %2 and hash = \"%3\"").arg(static_cast<int>(QGCTile::StateDownloading)).arg(task->setID()).arg(tiles[i]->hash());
            if(!query.exec(s)) {
                qWarning() << "Map Cache SQL error (set TilesDownload state):" << query.lastError().text();
            }
        }
    }
    task->setTileListFetched(tiles);
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_updateTileDownloadState(QGCMapTask* mtask)
{
    if(!_testTask(mtask)) {
        return;
    }
    QGCUpdateTileDownloadStateTask* task = static_cast<QGCUpdateTileDownloadStateTask*>(mtask);
    QSqlQuery query(*_db);
    QString s;
    if(task->state() == QGCTile::StateComplete) {
        s = QString("DELETE FROM TilesDownload WHERE setID = %1 AND hash = \"%2\"").arg(task->setID()).arg(task->hash());
    } else {
        if(task->hash() == "*") {
            s = QString("UPDATE TilesDownload SET state = %1 WHERE setID = %2").arg(static_cast<int>(task->state())).arg(task->setID());
        } else {
            s = QString("UPDATE TilesDownload SET state = %1 WHERE setID = %2 AND hash = \"%3\"").arg(static_cast<int>(task->state())).arg(task->setID()).arg(task->hash());
        }
    }
    if(!query.exec(s)) {
        qWarning() << "QGCCacheWorker::_updateTileDownloadState() Error:" << query.lastError().text();
    }
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_pruneCache(QGCMapTask* mtask)
{
    if(!_testTask(mtask)) {
        return;
    }
    QGCPruneCacheTask* task = static_cast<QGCPruneCacheTask*>(mtask);
    QSqlQuery query(*_db);
    QString s;
    //-- Select tiles in default set only, sorted by oldest.
    s = QString("SELECT tileID, size, hash FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A join SetTiles B on A.tileID = B.tileID WHERE B.setID = %1 GROUP by A.tileID HAVING COUNT(A.tileID) = 1) ORDER BY DATE ASC LIMIT 128").arg(_getDefaultTileSet());
    qint64 amount = (qint64)task->amount();
    QList<quint64> tlist;
    if(query.exec(s)) {
        while(query.next() && amount >= 0) {
            tlist << query.value(0).toULongLong();
            amount -= query.value(1).toULongLong();
            qCDebug(QGCTileCacheWorkerLog) << "_pruneCache() HASH:" << query.value(2).toString();
        }
        while(tlist.count()) {
            s = QString("DELETE FROM Tiles WHERE tileID = %1").arg(tlist[0]);
            tlist.removeFirst();
            if(!query.exec(s))
                break;
        }
        task->setPruned();
    }
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_deleteTileSet(QGCMapTask* mtask)
{
    if(!_testTask(mtask)) {
        return;
    }
    QGCDeleteTileSetTask* task = static_cast<QGCDeleteTileSetTask*>(mtask);
    _deleteTileSet(task->setID());
    task->setTileSetDeleted();
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_deleteTileSet(qulonglong id)
{
    QSqlQuery query(*_db);
    QString s;
    //-- Only delete tiles unique to this set
    s = QString("DELETE FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID WHERE B.setID = %1 GROUP BY A.tileID HAVING COUNT(A.tileID) = 1)").arg(id);
    query.exec(s);
    s = QString("DELETE FROM TilesDownload WHERE setID = %1").arg(id);
    query.exec(s);
    s = QString("DELETE FROM TileSets WHERE setID = %1").arg(id);
    query.exec(s);
    s = QString("DELETE FROM SetTiles WHERE setID = %1").arg(id);
    query.exec(s);
    _updateTotals();
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_renameTileSet(QGCMapTask* mtask)
{
    if(!_testTask(mtask)) {
        return;
    }
    QGCRenameTileSetTask* task = static_cast<QGCRenameTileSetTask*>(mtask);
    QSqlQuery query(*_db);
    QString s;
    s = QString("UPDATE TileSets SET name = \"%1\" WHERE setID = %2").arg(task->newName()).arg(task->setID());
    if(!query.exec(s)) {
        task->setError("Error renaming tile set");
    }
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_resetCacheDatabase(QGCMapTask* mtask)
{
    if(!_testTask(mtask)) {
        return;
    }
    QGCResetTask* task = static_cast<QGCResetTask*>(mtask);
    QSqlQuery query(*_db);
    QString s;
    s = QString("DROP TABLE Tiles");
    query.exec(s);
    s = QString("DROP TABLE TileSets");
    query.exec(s);
    s = QString("DROP TABLE SetTiles");
    query.exec(s);
    s = QString("DROP TABLE TilesDownload");
    query.exec(s);
    _valid = _createDB(*_db);
    task->setResetCompleted();
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_importSets(QGCMapTask* mtask)
{
    if(!_testTask(mtask)) {
        return;
    }
    QGCImportTileTask* task = static_cast<QGCImportTileTask*>(mtask);
    //-- If replacing, simply copy over it
    if(task->replace()) {
        //-- Close and delete old database
        _disconnectDB();
        QFile file(_databasePath);
        file.remove();
        //-- Copy given database
        QFile::copy(task->path(), _databasePath);
        task->setProgress(25);
        _init();
        if(_valid) {
            task->setProgress(50);
            _connectDB();
        }
        task->setProgress(100);
    } else {
        //-- Open imported set
        QSqlDatabase* dbImport = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", kExportSession));
        dbImport->setDatabaseName(task->path());
        dbImport->setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
        if (dbImport->open()) {
            QSqlQuery query(*dbImport);
            //-- Prepare progress report
            quint64 tileCount = 0;
            quint64 currentCount = 0;
            int lastProgress = -1;
            QString s;
            s = QString("SELECT COUNT(tileID) FROM Tiles");
            if(query.exec(s)) {
                if(query.next()) {
                    //-- Total number of tiles in imported database
                    tileCount  = query.value(0).toULongLong();
                }
            }
            if(tileCount) {
                //-- Iterate Tile Sets
                s = QString("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC");
                if(query.exec(s)) {
                    while(query.next()) {
                        QString name            = query.value("name").toString();
                        quint64 setID           = query.value("setID").toULongLong();
                        QString mapType         = query.value("typeStr").toString();
                        double  topleftLat      = query.value("topleftLat").toDouble();
                        double  topleftLon      = query.value("topleftLon").toDouble();
                        double  bottomRightLat  = query.value("bottomRightLat").toDouble();
                        double  bottomRightLon  = query.value("bottomRightLon").toDouble();
                        int     minZoom         = query.value("minZoom").toInt();
                        int     maxZoom         = query.value("maxZoom").toInt();
                        int     type            = query.value("type").toInt();
                        quint32 numTiles        = query.value("numTiles").toUInt();
                        int     defaultSet      = query.value("defaultSet").toInt();
                        quint64 insertSetID     = _getDefaultTileSet();
                        //-- If not default set, create new one
                        if(!defaultSet) {
                            //-- Check if we have this tile set already
                            if(_findTileSetID(name, insertSetID)) {
                                int testCount = 0;
                                //-- Set with this name already exists. Make name unique.
                                while (true) {
                                    auto testName = QString::asprintf("%s %02d", name.toLatin1().data(), ++testCount);
                                    if(!_findTileSetID(testName, insertSetID) || testCount > 99) {
                                        name = testName;
                                        break;
                                    }
                                }
                            }
                            //-- Create new set
                            QSqlQuery cQuery(*_db);
                            cQuery.prepare("INSERT INTO TileSets("
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
                            if(!cQuery.exec()) {
                                task->setError("Error adding imported tile set to database");
                                break;
                            } else {
                                //-- Get just created (auto-incremented) setID
                                insertSetID = cQuery.lastInsertId().toULongLong();
                            }
                        }
                        //-- Find set tiles
                        QSqlQuery cQuery(*_db);
                        QSqlQuery subQuery(*dbImport);
                        QString sb = QString("SELECT * FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID WHERE B.setID = %1 GROUP BY A.tileID HAVING COUNT(A.tileID) = 1)").arg(setID);
                        if(subQuery.exec(sb)) {
                            quint64 tilesFound = 0;
                            quint64 tilesSaved = 0;
                            _db->transaction();
                            while(subQuery.next()) {
                                tilesFound++;
                                QString hash    = subQuery.value("hash").toString();
                                QString format  = subQuery.value("format").toString();
                                QByteArray img  = subQuery.value("tile").toByteArray();
                                int type        = subQuery.value("type").toInt();
                                //-- Save tile
                                cQuery.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)");
                                cQuery.addBindValue(hash);
                                cQuery.addBindValue(format);
                                cQuery.addBindValue(img);
                                cQuery.addBindValue(img.size());
                                cQuery.addBindValue(type);
                                cQuery.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
                                if(cQuery.exec()) {
                                    tilesSaved++;
                                    quint64 importTileID = cQuery.lastInsertId().toULongLong();
                                    QString s = QString("INSERT INTO SetTiles(tileID, setID) VALUES(%1, %2)").arg(importTileID).arg(insertSetID);
                                    cQuery.prepare(s);
                                    cQuery.exec();
                                    currentCount++;
                                    if(tileCount) {
                                        int progress = (int)((double)currentCount / (double)tileCount * 100.0);
                                        //-- Avoid calling this if (int) progress hasn't changed.
                                        if(lastProgress != progress) {
                                            lastProgress = progress;
                                            task->setProgress(progress);
                                        }
                                    }
                                }
                            }
                            _db->commit();
                            if(tilesSaved) {
                                //-- Update tile count (if any added)
                                s = QString("SELECT COUNT(size) FROM Tiles A INNER JOIN SetTiles B on A.tileID = B.tileID WHERE B.setID = %1").arg(insertSetID);
                                if(cQuery.exec(s)) {
                                    if(cQuery.next()) {
                                        quint64 count  = cQuery.value(0).toULongLong();
                                        s = QString("UPDATE TileSets SET numTiles = %1 WHERE setID = %2").arg(count).arg(insertSetID);
                                        cQuery.exec(s);
                                    }
                                }
                            }
                            qint64 uniqueTiles = tilesFound - tilesSaved;
                            if((quint64)uniqueTiles < tileCount) {
                                tileCount -= uniqueTiles;
                            } else {
                                tileCount = 0;
                            }
                            //-- If there was nothing new in this set, remove it.
                            if(!tilesSaved && !defaultSet) {
                                qCDebug(QGCTileCacheWorkerLog) << "No unique tiles in" << name << "Removing it.";
                                _deleteTileSet(insertSetID);
                            }
                        }
                    }
                } else {
                    task->setError("No tile set in database");
                }
            }
            delete dbImport;
            QSqlDatabase::removeDatabase(kExportSession);
            if(!tileCount) {
                task->setError("No unique tiles in imported database");
            }
        } else {
            task->setError("Error opening import database");
        }
    }
    task->setImportCompleted();
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_exportSets(QGCMapTask* mtask)
{
    if(!_testTask(mtask)) {
        return;
    }
    QGCExportTileTask* task = static_cast<QGCExportTileTask*>(mtask);
    //-- Delete target if it exists
    QFile file(task->path());
    file.remove();
    //-- Create exported database
    QScopedPointer<QSqlDatabase> dbExport(new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", kExportSession)));
    dbExport->setDatabaseName(task->path());
    dbExport->setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    if (dbExport->open()) {
        if(_createDB(*dbExport, false)) {
            //-- Prepare progress report
            quint64 tileCount = 0;
            quint64 currentCount = 0;
            for(int i = 0; i < task->sets().count(); i++) {
                QGCCachedTileSet* set = task->sets()[i];
                //-- Default set has no unique tiles
                if(set->defaultSet()) {
                    tileCount += set->totalTileCount();
                } else {
                    tileCount += set->uniqueTileCount();
                }
            }
            if(!tileCount) {
                tileCount = 1;
            }
            //-- Iterate sets to save
            for(int i = 0; i < task->sets().count(); i++) {
                QGCCachedTileSet* set = task->sets()[i];
                //-- Create Tile Exported Set
                QSqlQuery exportQuery(*dbExport);
                exportQuery.prepare("INSERT INTO TileSets("
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
                if(!exportQuery.exec()) {
                    task->setError("Error adding tile set to exported database");
                    break;
                } else {
                    //-- Get just created (auto-incremented) setID
                    quint64 exportSetID = exportQuery.lastInsertId().toULongLong();
                    //-- Find set tiles
                    QString s = QString("SELECT * FROM SetTiles WHERE setID = %1").arg(set->id());
                    QSqlQuery query(*_db);
                    if(query.exec(s)) {
                        dbExport->transaction();
                        while(query.next()) {
                            quint64 tileID = query.value("tileID").toULongLong();
                            //-- Get tile
                            QString s = QString("SELECT * FROM Tiles WHERE tileID = \"%1\"").arg(tileID);
                            QSqlQuery subQuery(*_db);
                            if(subQuery.exec(s)) {
                                if(subQuery.next()) {
                                    QString hash    = subQuery.value("hash").toString();
                                    QString format  = subQuery.value("format").toString();
                                    QByteArray img  = subQuery.value("tile").toByteArray();
                                    int type        = subQuery.value("type").toInt();
                                    //-- Save tile
                                    exportQuery.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)");
                                    exportQuery.addBindValue(hash);
                                    exportQuery.addBindValue(format);
                                    exportQuery.addBindValue(img);
                                    exportQuery.addBindValue(img.size());
                                    exportQuery.addBindValue(type);
                                    exportQuery.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
                                    if(exportQuery.exec()) {
                                        quint64 exportTileID = exportQuery.lastInsertId().toULongLong();
                                        QString s = QString("INSERT INTO SetTiles(tileID, setID) VALUES(%1, %2)").arg(exportTileID).arg(exportSetID);
                                        exportQuery.prepare(s);
                                        exportQuery.exec();
                                        currentCount++;
                                        task->setProgress((int)((double)currentCount / (double)tileCount * 100.0));
                                    }
                                }
                            }
                        }
                    }
                    dbExport->commit();
                }
            }
        } else {
            task->setError("Error creating export database");
        }
    } else {
        qCritical() << "Map Cache SQL error (create export database):" << dbExport->lastError();
        task->setError("Error opening export database");
    }
    dbExport.reset();
    QSqlDatabase::removeDatabase(kExportSession);
    task->setExportCompleted();
}

//-----------------------------------------------------------------------------
bool QGCCacheWorker::_testTask(QGCMapTask* mtask)
{
    if(!_valid) {
        mtask->setError("No Cache Database");
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
bool
QGCCacheWorker::_init()
{
    _failed = false;
    if(!_databasePath.isEmpty()) {
        qCDebug(QGCTileCacheWorkerLog) << "Mapping cache directory:" << _databasePath;
        //-- Initialize Database
        if (_connectDB()) {
            _valid = _createDB(*_db);
            if(!_valid) {
                _failed = true;
            }
        } else {
            qCritical() << "Map Cache SQL error (init() open db):" << _db->lastError();
            _failed = true;
        }
        _disconnectDB();
    } else {
        qCritical() << "Could not find suitable cache directory.";
        _failed = true;
    }
    return !_failed;
}

//-----------------------------------------------------------------------------
bool
QGCCacheWorker::_connectDB()
{
    _db.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", kSession)));
    _db->setDatabaseName(_databasePath);
    _db->setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    _valid = _db->open();
    return _valid;
}

//-----------------------------------------------------------------------------
bool
QGCCacheWorker::_createDB(QSqlDatabase& db, bool createDefault)
{
    bool res = false;
    QSqlQuery query(db);
    if(!query.exec(
        "CREATE TABLE IF NOT EXISTS Tiles ("
        "tileID INTEGER PRIMARY KEY NOT NULL, "
        "hash TEXT NOT NULL UNIQUE, "
        "format TEXT NOT NULL, "
        "tile BLOB NULL, "
        "size INTEGER, "
        "type INTEGER, "
        "date INTEGER DEFAULT 0)"))
    {
        qWarning() << "Map Cache SQL error (create Tiles db):" << query.lastError().text();
    } else {
        query.exec("CREATE INDEX IF NOT EXISTS hash ON Tiles ( hash, size, type ) ");
             
        if(!query.exec(
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
            "date INTEGER DEFAULT 0)"))
        {
            qWarning() << "Map Cache SQL error (create TileSets db):" << query.lastError().text();
        } else {
            if(!query.exec(
                "CREATE TABLE IF NOT EXISTS SetTiles ("
                "setID INTEGER, "
                "tileID INTEGER)"))
            {
                qWarning() << "Map Cache SQL error (create SetTiles db):" << query.lastError().text();
            } else {
                if(!query.exec(
                    "CREATE TABLE IF NOT EXISTS TilesDownload ("
                    "setID INTEGER, "
                    "hash TEXT NOT NULL UNIQUE, "
                    "type INTEGER, "
                    "x INTEGER, "
                    "y INTEGER, "
                    "z INTEGER, "
                    "state INTEGER DEFAULT 0)"))
                {
                    qWarning() << "Map Cache SQL error (create TilesDownload db):" << query.lastError().text();
                } else {
                    //-- Database it ready for use
                    res = true;
                }
            }
        }
    }
    //-- Create default tile set
    if(res && createDefault) {
        QString s = QString("SELECT name FROM TileSets WHERE name = \"%1\"").arg("Default Tile Set");
        if(query.exec(s)) {
            if(!query.next()) {
                query.prepare("INSERT INTO TileSets(name, defaultSet, date) VALUES(?, ?, ?)");
                query.addBindValue("Default Tile Set");
                query.addBindValue(1);
                query.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
                if(!query.exec()) {
                    qWarning() << "Map Cache SQL error (Creating default tile set):" << db.lastError();
                    res = false;
                }
            }
        } else {
            qWarning() << "Map Cache SQL error (Looking for default tile set):" << db.lastError();
        }
    }
    if(!res) {
        QFile file(_databasePath);
        file.remove();
    }
    return res;
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_disconnectDB()
{
    if (_db) {
        _db.reset();
        QSqlDatabase::removeDatabase(kSession);
    }
}
