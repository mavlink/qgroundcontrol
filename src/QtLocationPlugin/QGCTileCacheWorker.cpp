/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCTileCacheWorker.h"
#include "QGCCachedTileSet.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGCLoggingCategory.h"
#include "QGCTileCacheDatabase.h"

#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <memory>

QGC_LOGGING_CATEGORY(QGCTileCacheWorkerLog, "test.qtlocationplugin.qgctilecacheworker")

QGCCacheWorker::QGCCacheWorker(QObject *parent)
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

    if (this->isRunning()) {
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

void QGCCacheWorker::run()
{
    if (!_valid && !_failed) {
        if (!_init()) {
            qCWarning(QGCTileCacheWorkerLog) << Q_FUNC_INFO << "Failed To Init Database";
            return;
        }
    }

    if (_valid) {
        if (_connectDB()) {
            (void) TilesTableModel::deleteBingNoTileImageTiles(_db);
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
    if (task->type() == QGCMapTask::taskInit) {
        return;
    }

    if (!_valid) {
        task->setError(tr("No Cache Database"));
        return;
    }

    switch (task->type()) {
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

bool QGCCacheWorker::_findTileSetID(const QString &name, quint64 &setID)
{
    return TileSetsTableModel::getTileSetID(_db, setID, name);
}

quint64 QGCCacheWorker::_getDefaultTileSet()
{
    quint64 result;
    (void) TileSetsTableModel::getDefaultTileSet(_db, result);
    return result;
}

void QGCCacheWorker::_saveTile(QGCMapTask *mtask)
{
    QGCSaveTileTask *const task = static_cast<QGCSaveTileTask*>(mtask);

    quint64 tileID;
    if (!TilesTableModel::insertTile(_db, tileID, *task->tile())) {
        return;
    }

    const quint64 setID = (task->tile()->tileSet() == UINT64_MAX) ? _getDefaultTileSet() : task->tile()->tileSet();
    (void) SetTilesTableModel::insertSetTiles(_db, setID, tileID);
}

void QGCCacheWorker::_getTile(QGCMapTask *mtask)
{
    QGCFetchTileTask *const task = static_cast<QGCFetchTileTask*>(mtask);

    QGCCacheTile *tile = nullptr;
    if (!TilesTableModel::getTile(_db, tile, task->hash())) {
        task->setError(tr("Tile not in cache database"));
        return;
    }

    task->setTileFetched(tile);
}

void QGCCacheWorker::_getTileSets(QGCMapTask *mtask)
{
    QGCFetchTileSetTask *const task = static_cast<QGCFetchTileSetTask*>(mtask);

    QList<QGCCachedTileSet*> tileSets;
    if (!TileSetsTableModel::getTileSets(_db, tileSets)) {
        task->setError(tr("No tile set in database"));
        return;
    }

    for (QGCCachedTileSet *tileSet: tileSets) {
        _updateSetTotals(tileSet);
        tileSet->moveToThread(QCoreApplication::instance()->thread());
        task->tileSetFetched(tileSet);
    }
}

void QGCCacheWorker::_updateSetTotals(QGCCachedTileSet *set)
{
    if (set->defaultSet()) {
        _updateTotals();
        set->setSavedTileCount(_totalCount);
        set->setSavedTileSize(_totalSize);
        set->setTotalTileCount(_defaultCount);
        set->setTotalTileSize(_defaultSize);
        return;
    }

    (void) TilesTableModel::updateSetTotals(_db, *set);
}

void QGCCacheWorker::_updateTotals()
{
    (void) TilesTableModel::updateTotals(_db, _totalCount, _totalSize, _defaultCount, _defaultSize, _getDefaultTileSet());
    emit updateTotals(_totalCount, _totalSize, _defaultCount, _defaultSize);

    if (!_updateTimer.isValid()) {
        _updateTimer.start();
    } else {
        (void) _updateTimer.restart();
    }
}

quint64 QGCCacheWorker::_findTile(const QString &hash)
{
    quint64 tileID;
    (void) TilesTableModel::getTileID(_db, tileID, hash);
    return tileID;
}

void QGCCacheWorker::_createTileSet(QGCMapTask *mtask)
{
    QGCCreateTileSetTask *const task = static_cast<QGCCreateTileSetTask*>(mtask);

    QGCCachedTileSet *const tileSet = task->tileSet();

    quint64 setID;
    if (!TileSetsTableModel::insertTileSet(_db, *tileSet, setID)) {
        mtask->setError(tr("Error saving tile set"));
        return;
    }

    tileSet->setId(setID);

    if (!TilesDownloadTableModel::insertTilesDownload(_db, tileSet)) {
        return;
    }

    _updateSetTotals(task->tileSet());
    task->setTileSetSaved();
}

void QGCCacheWorker::_getTileDownloadList(QGCMapTask *mtask)
{
    QGCGetTileDownloadListTask *const task = static_cast<QGCGetTileDownloadListTask*>(mtask);

    QQueue<QGCTile*> tiles;
    (void) TilesDownloadTableModel::getTileDownloadList(_db, tiles, task->setID(), task->count());
    task->setTileListFetched(tiles);
}

void QGCCacheWorker::_updateTileDownloadState(QGCMapTask *mtask)
{
    QGCUpdateTileDownloadStateTask *const task = static_cast<QGCUpdateTileDownloadStateTask*>(mtask);

    (void) TilesDownloadTableModel::updateTilesDownloadSet(_db, task->state(), task->setID(), task->hash());
}

void QGCCacheWorker::_pruneCache(QGCMapTask *mtask)
{
    QGCPruneCacheTask *const task = static_cast<QGCPruneCacheTask*>(mtask);

    if (TilesTableModel::prune(_db, _getDefaultTileSet(), task->amount())) {
        task->setPruned();
    }
}

void QGCCacheWorker::_deleteTileSet(QGCMapTask *mtask)
{
    QGCDeleteTileSetTask *const task = static_cast<QGCDeleteTileSetTask*>(mtask);

    _deleteTileSet(task->setID());
    task->setTileSetDeleted();
}

void QGCCacheWorker::_deleteTileSet(quint64 id)
{
    (void) TilesTableModel::deleteTileSet(_db, id);
    (void) TilesDownloadTableModel::deleteTileSet(_db, id);
    (void) TileSetsTableModel::deleteTileSet(_db, id);
    (void) SetTilesTableModel::deleteTileSet(_db, id);

    _updateTotals();
}

void QGCCacheWorker::_renameTileSet(QGCMapTask *mtask)
{
    QGCRenameTileSetTask *const task = static_cast<QGCRenameTileSetTask*>(mtask);

    if (!TileSetsTableModel::setName(_db, task->setID(), task->newName())) {
        task->setError(tr("Error renaming tile set"));
    }
}

void QGCCacheWorker::_resetCacheDatabase(QGCMapTask *mtask)
{
    QGCResetTask *const task = static_cast<QGCResetTask*>(mtask);

    (void) TilesTableModel::drop(_db);
    (void) TileSetsTableModel::drop(_db);
    (void) SetTilesTableModel::drop(_db);
    (void) TilesDownloadTableModel::drop(_db);

    _valid = _createDB(_db);
    task->setResetCompleted();
}

void QGCCacheWorker::_importSets(QGCMapTask *mtask)
{
    QGCImportTileTask *const task = static_cast<QGCImportTileTask*>(mtask);
    if (task->replace()) {
        _disconnectDB();

        QFile file(_databasePath);
        (void) file.remove();
        (void) QFile::copy(task->path(), _databasePath);

        task->setProgress(25);
        (void) _init();
        if (_valid) {
            task->setProgress(50);
            (void) _connectDB();
        }
        task->setProgress(100);
    } else {
        std::shared_ptr<QSqlDatabase> dbImport = std::make_shared<QSqlDatabase>(QSqlDatabase::addDatabase("QSQLITE", kExportSession));

        dbImport->setDatabaseName(task->path());
        dbImport->setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");

        if (dbImport->open()) {
            QSqlQuery query(*dbImport);

            quint64 currentCount = 0;
            int lastProgress = -1;

            quint64 tileCount = 0;
            (void) TilesTableModel::getTileCount(dbImport, tileCount);

            if (tileCount) {
                if (query.exec("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC")) {
                    while (query.next()) {
                        QString name = query.value("name").toString();
                        const quint64 setID = query.value("setID").toULongLong();
                        const QString mapType = query.value("typeStr").toString();
                        const double topleftLat = query.value("topleftLat").toDouble();
                        const double topleftLon = query.value("topleftLon").toDouble();
                        const double bottomRightLat = query.value("bottomRightLat").toDouble();
                        const double bottomRightLon = query.value("bottomRightLon").toDouble();
                        const int minZoom = query.value("minZoom").toInt();
                        const int maxZoom = query.value("maxZoom").toInt();
                        const int type = query.value("type").toInt();
                        const quint32 numTiles = query.value("numTiles").toUInt();
                        const int defaultSet = query.value("defaultSet").toInt();
                        quint64 insertSetID = _getDefaultTileSet();

                        if (!defaultSet) {
                            if (_findTileSetID(name, insertSetID)) {
                                int testCount = 0;
                                while (true) {
                                    const QString testName = QString::asprintf("%s %02d", name.toLatin1().data(), ++testCount);
                                    if (!_findTileSetID(testName, insertSetID) || (testCount > 99)) {
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
                                task->setError(tr("Error adding imported tile set to database"));
                                break;
                            } else {
                                insertSetID = cQuery.lastInsertId().toULongLong();
                            }
                        }

                        QSqlQuery cQuery(*_db);
                        QSqlQuery subQuery(*dbImport);
                        const QString sb = QStringLiteral("SELECT * FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID WHERE B.setID = %1 GROUP BY A.tileID HAVING COUNT(A.tileID) = 1)").arg(setID);
                        if (subQuery.exec(sb)) {
                            quint64 tilesFound = 0;
                            quint64 tilesSaved = 0;
                            (void) _db->transaction();
                            while (subQuery.next()) {
                                tilesFound++;
                                const QString hash = subQuery.value("hash").toString();
                                const QString format = subQuery.value("format").toString();
                                const QByteArray img = subQuery.value("tile").toByteArray();
                                const QString type = subQuery.value("type").toString();
                                // TODO: const int type = subQuery.value("type").toInt();

                                quint64 importTileID = 0;
                                if (TilesTableModel::insertTile(_db, importTileID, hash, format, img, type)) {
                                    tilesSaved++;
                                    (void) SetTilesTableModel::insertSetTiles(_db, insertSetID, importTileID);
                                    currentCount++;
                                    if (tileCount != 0) {
                                        const int progress = static_cast<int>((static_cast<double>(currentCount) / static_cast<double>(tileCount)) * 100.0);
                                        if (lastProgress != progress) {
                                            lastProgress = progress;
                                            task->setProgress(progress);
                                        }
                                    }
                                }
                            }
                            (void) _db->commit();

                            if (tilesSaved != 0) {
                                quint64 count = 0;
                                if (TilesTableModel::getSetTotal(_db, count, insertSetID)) {
                                    (void) TileSetsTableModel::setNumTiles(_db, insertSetID, count);
                                }
                            }

                            qint64 uniqueTiles = tilesFound - tilesSaved;
                            if (static_cast<quint64>(uniqueTiles) < tileCount) {
                                tileCount -= uniqueTiles;
                            } else {
                                tileCount = 0;
                            }

                            if ((tilesSaved == 0) && (defaultSet == 0)) {
                                qCDebug(QGCTileCacheWorkerLog) << "No unique tiles in" << name << "Removing it.";
                                _deleteTileSet(insertSetID);
                            }
                        }
                    }
                } else {
                    task->setError(tr("No tile set in database"));
                }
            }

            dbImport.reset();
            QSqlDatabase::removeDatabase(kExportSession);

            if (tileCount == 0) {
                task->setError(tr("No unique tiles in imported database"));
            }
        } else {
            task->setError(tr("Error opening import database"));
        }
    }

    task->setImportCompleted();
}

void QGCCacheWorker::_exportSets(QGCMapTask *mtask)
{
    QGCExportTileTask *const task = static_cast<QGCExportTileTask*>(mtask);

    QFile file(task->path());
    (void) file.remove();

    std::shared_ptr<QSqlDatabase> dbExport = std::make_shared<QSqlDatabase>(QSqlDatabase::addDatabase("QSQLITE", kExportSession));
    // TODO: QSqlDatabase::cloneDatabase?
    dbExport->setDatabaseName(task->path());
    dbExport->setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");

    if (dbExport->open()) {
        if (_createDB(dbExport, false)) {
            quint64 tileCount = 0;
            for (const QGCCachedTileSet *set: task->sets()) {
                if (set->defaultSet()) {
                    tileCount += set->totalTileCount();
                } else {
                    tileCount += set->uniqueTileCount();
                }
            }

            if (!tileCount) {
                tileCount = 1;
            }

            for (const QGCCachedTileSet *set: task->sets()) {
                quint64 exportSetID = 0;
                if (!TileSetsTableModel::insertTileSet(_db, *set, exportSetID)) {
                    task->setError(tr("Error adding tile set to exported database"));
                    break;
                }

                QString s = QStringLiteral("SELECT * FROM SetTiles WHERE setID = %1").arg(set->id());
                QSqlQuery query(*_db);
                if (!query.exec(s)) {
                    continue;
                }

                quint64 currentCount = 0;
                (void) dbExport->transaction();
                while (query.next()) {
                    const quint64 tileID = query.value("tileID").toULongLong();
                    s = QStringLiteral("SELECT * FROM Tiles WHERE tileID = \"%1\"").arg(tileID);
                    QSqlQuery subQuery(*_db);
                    if (!subQuery.exec(s) || !subQuery.next()) {
                        continue;
                    }

                    const QString hash = subQuery.value("hash").toString();
                    const QString format = subQuery.value("format").toString();
                    const QByteArray img = subQuery.value("tile").toByteArray();
                    const QString type = subQuery.value("type").toString();
                    // TODO: const int type = subQuery.value("type").toInt();
                    quint64 exportTileID = 0;
                    if (!TilesTableModel::insertTile(dbExport, exportTileID, hash, format, img, type)) {
                        continue;
                    }

                    if (!SetTilesTableModel::insertSetTiles(dbExport, exportSetID, exportTileID)) {
                        continue;
                    }

                    currentCount++;
                    const double progress = (static_cast<double>(currentCount) / static_cast<double>(tileCount)) * 100.0;
                    task->setProgress(static_cast<int>(progress));
                }
                (void) dbExport->commit();
            }
        } else {
            task->setError(tr("Error creating export database"));
        }
    } else {
        qCCritical(QGCTileCacheWorkerLog) << "Map Cache SQL error (create export database):" << dbExport->lastError();
        task->setError(tr("Error opening export database"));
    }

    dbExport.reset();
    QSqlDatabase::removeDatabase(kExportSession);
    task->setExportCompleted();
}

bool QGCCacheWorker::_init()
{
    bool result = false;

    qCDebug(QGCTileCacheWorkerLog) << "Mapping cache directory:" << _databasePath;
    if (_databasePath.isEmpty()) {
        qCCritical(QGCTileCacheWorkerLog) << "Could not find suitable cache directory.";
    } else if (!_connectDB()) {
        qCCritical(QGCTileCacheWorkerLog) << "Failed to initialize database connection";
    } else if (!_createDB(_db)) {
        qCCritical(QGCTileCacheWorkerLog) << "Failed to create database";
        _valid = false;
    } else {
        result = true;
    }

    if (!result) {
        _disconnectDB();
    }

    _failed = !result;

    return result;
}

bool QGCCacheWorker::_createDB(std::shared_ptr<QSqlDatabase> db, bool createDefault)
{
    bool result = false;

    if (TilesTableModel::create(db) && TileSetsTableModel::create(db) && SetTilesTableModel::create(db) && TilesDownloadTableModel::create(db)) {
        if (createDefault) {
            if (TileSetsTableModel::createDefaultTileSet(db)) {
                result = true;
            }
        } else {
            result = true;
        }
    }

    if (!result) {
        QFile file(_databasePath);
        (void) file.remove();
    }

    return result;
}

bool QGCCacheWorker::_connectDB()
{
    _db.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", kSession)));
    _db->setDatabaseName(_databasePath);
    _db->setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");

    _valid = _db->open();
    return _valid;
}

void QGCCacheWorker::_disconnectDB()
{
    if (_db) {
        _db.reset();
        QSqlDatabase::removeDatabase(kSession);
    }
}
