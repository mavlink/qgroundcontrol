#include "QGCTileCacheWorker.h"

#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaObject>
#include <QtCore/QSettings>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include "QGCApplication.h"
#include "QGCCachedTileSet.h"
#include "QGCLoggingCategory.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"

QGC_LOGGING_CATEGORY(QGCTileCacheWorkerLog, "QtLocationPlugin.QGCTileCacheWorker")

QGCCacheWorker::QGCCacheWorker(QObject *parent)
    : QThread(parent)
{
    qCDebug(QGCTileCacheWorkerLog) << this;
}

QGCCacheWorker::~QGCCacheWorker()
{
    qCDebug(QGCTileCacheWorkerLog) << this;
}

void QGCCacheWorker::stop()
{
    _stopping = true;

    QMutexLocker lock(&_taskQueueMutex);
    qDeleteAll(_taskQueue);
    _taskQueue.clear();

    if (isRunning()) {
        _waitc.wakeAll();
    }
}

bool QGCCacheWorker::enqueueTask(QGCMapTask *task)
{
    QMutexLocker lock(&_taskQueueMutex);

    if (_stopping) {
        lock.unlock();
        task->setError(tr("Worker Stopping"));
        task->deleteLater();
        return false;
    }

    if (!_valid && (task->type() != QGCMapTask::TaskType::taskInit)) {
        lock.unlock();
        task->setError(tr("Database Not Initialized"));
        task->deleteLater();
        return false;
    }

    _taskQueue.enqueue(task);
    lock.unlock();

    if (isRunning()) {
        _waitc.wakeAll();
    } else {
        start(QThread::NormalPriority);
    }

    return true;
}

void QGCCacheWorker::run()
{
    if (!_valid && !_failed) {
        if (!_init()) {
            qCWarning(QGCTileCacheWorkerLog) << "Failed To Init Database";
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
            if (count > kTaskQueueThreshold) {
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
            (void) _waitc.wait(lock.mutex(), kWorkerWaitTimeoutMs);
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
    case QGCMapTask::TaskType::taskInit:
        break;
    case QGCMapTask::TaskType::taskCacheTile:
        _saveTile(task);
        break;
    case QGCMapTask::TaskType::taskFetchTile:
        _getTile(task);
        break;
    case QGCMapTask::TaskType::taskFetchTileSets:
        _getTileSets(task);
        break;
    case QGCMapTask::TaskType::taskCreateTileSet:
        _createTileSet(task);
        break;
    case QGCMapTask::TaskType::taskGetTileDownloadList:
        _getTileDownloadList(task);
        break;
    case QGCMapTask::TaskType::taskUpdateTileDownloadState:
        _updateTileDownloadState(task);
        break;
    case QGCMapTask::TaskType::taskDeleteTileSet:
        _deleteTileSet(task);
        break;
    case QGCMapTask::TaskType::taskRenameTileSet:
        _renameTileSet(task);
        break;
    case QGCMapTask::TaskType::taskPruneCache:
        _pruneCache(task);
        break;
    case QGCMapTask::TaskType::taskReset:
        _resetCacheDatabase(task);
        break;
    case QGCMapTask::TaskType::taskExport:
        _exportSets(task);
        break;
    case QGCMapTask::TaskType::taskImport:
        _importSets(task);
        break;
    case QGCMapTask::TaskType::taskSetCacheEnabled:
        _setCacheEnabled(task);
        break;
    case QGCMapTask::TaskType::taskSetDefaultCacheEnabled:
        _setDefaultCacheEnabled(task);
        break;
    default:
        qCWarning(QGCTileCacheWorkerLog) << "given unhandled task type" << task->type();
        break;
    }
}

void QGCCacheWorker::_deleteBingNoTileTiles()
{
    static const QString alreadyDoneKey = QStringLiteral("_deleteBingNoTileTilesDone");

    QSettings settings;
    if (settings.value(alreadyDoneKey, false).toBool()) {
        return;
    }
    settings.setValue(alreadyDoneKey, true);

    // Previously we would store these empty tile graphics in the cache. This prevented the ability to zoom beyong the level
    // of available tiles. So we need to remove only of these still hanging around to make higher zoom levels work.
    QFile file(QStringLiteral(":/res/BingNoTileBytes.dat"));
    if (!file.open(QFile::ReadOnly)) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed to Open File" << file.fileName() << ":" << file.errorString();
        return;
    }

    const QByteArray noTileBytes = file.readAll();
    file.close();

    QSqlQuery query(_getDB());
    QList<quint64> idsToDelete;
    (void) query.prepare("SELECT tileID, tile, hash FROM Tiles WHERE LENGTH(tile) = ?");
    query.addBindValue(noTileBytes.length());
    if (!query.exec()) {
        qCWarning(QGCTileCacheWorkerLog) << "query failed";
        return;
    }

    while (query.next()) {
        if (query.value(1).toByteArray() == noTileBytes) {
            idsToDelete.append(query.value(0).toULongLong());
            qCDebug(QGCTileCacheWorkerLog) << "HASH:" << query.value(2).toString();
        }
    }

    (void) _batchDeleteTiles(query, idsToDelete, "Batch delete of Bing no-tile tiles failed:");
}

bool QGCCacheWorker::_findTileSetID(const QString &name, quint64 &setID)
{
    QSqlQuery query(_getDB());
    (void) query.prepare("SELECT setID FROM TileSets WHERE name = ?");
    query.addBindValue(name);
    if (query.exec() && query.next()) {
        setID = query.value(0).toULongLong();
        return true;
    }

    return false;
}

quint64 QGCCacheWorker::_getDefaultTileSet()
{
    if (_defaultSet != UINT64_MAX) {
        return _defaultSet;
    }

    QSqlQuery query(_getDB());
    (void) query.prepare("SELECT setID FROM TileSets WHERE defaultSet = ?");
    query.addBindValue(kSqlTrue);
    if (query.exec() && query.next()) {
        _defaultSet = query.value(0).toULongLong();
        return _defaultSet;
    }

    return UrlFactory::defaultTileSetId();
}

void QGCCacheWorker::_saveTile(QGCMapTask *mtask)
{
    if (!_valid) {
        qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (saveTile() open db):" << _getDB().lastError();
        return;
    }

    QGCSaveTileTask *task = static_cast<QGCSaveTileTask*>(mtask);

    // Convert type string to integer mapId for database storage
    const int mapId = UrlFactory::getQtMapIdFromProviderType(task->tile()->type);
    if (mapId < 0) {
        qCWarning(QGCTileCacheWorkerLog) << "Invalid mapId for tile type:" << task->tile()->type;
        return;
    }

    const quint64 defaultSetID = _getDefaultTileSet();
    const quint64 setID = (task->tile()->tileSet == UINT64_MAX) ? defaultSetID : task->tile()->tileSet;

    if (!_cacheEnabled.load()) {
        return;
    }

    if ((setID == defaultSetID) && !_defaultCachingEnabled.load()) {
        return;
    }

    // Use a transaction to ensure atomicity
    if (!_getDB().transaction()) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed to start transaction for saveTile";
        return;
    }

    QSqlQuery query(_getDB());
    (void) query.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)");
    query.addBindValue(task->tile()->hash);
    query.addBindValue(task->tile()->format);
    query.addBindValue(task->tile()->img);
    query.addBindValue(task->tile()->img.size());
    query.addBindValue(mapId);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());

    quint64 tileID = 0;
    if (query.exec()) {
        tileID = query.lastInsertId().toULongLong();
        query.finish();
        qCDebug(QGCTileCacheWorkerLog) << "Saved new tile HASH:" << task->tile()->hash;
    } else {
        // Tile already exists (hash UNIQUE constraint), fetch its ID
        query.finish();
        (void) query.prepare("SELECT tileID FROM Tiles WHERE hash = ?");
        query.addBindValue(task->tile()->hash);
        if (query.exec() && query.next()) {
            tileID = query.value(0).toULongLong();
            query.finish();
            qCDebug(QGCTileCacheWorkerLog) << "Tile already exists HASH:" << task->tile()->hash;
        } else {
            qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (find existing tile):" << query.lastError().text();
            query.finish();
            _getDB().rollback();
            return;
        }
    }

    // Associate tile with the set
    (void) query.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)");
    query.addBindValue(tileID);
    query.addBindValue(setID);
    if (!query.exec()) {
        qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (add tile into SetTiles):" << query.lastError().text();
        query.finish();
        _getDB().rollback();
        return;
    }
    query.finish();

    // Commit the transaction
    if (!_getDB().commit()) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed to commit saveTile transaction:" << _getDB().lastError();
        _getDB().rollback();
    }
}

void QGCCacheWorker::_getTile(QGCMapTask* mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCFetchTileTask *task = static_cast<QGCFetchTileTask*>(mtask);
    QSqlQuery query(_getDB());
    (void) query.prepare("SELECT tile, format, type FROM Tiles WHERE hash = ?");
    query.addBindValue(task->hash());
    if (query.exec() && query.next()) {
        const QByteArray &arrray = query.value(0).toByteArray();
        const QString &format = query.value(1).toString();
        const QString type = UrlFactory::getProviderTypeFromQtMapId(query.value(2).toInt());
        qCDebug(QGCTileCacheWorkerLog) << "(Found in DB) HASH:" << task->hash();
        QGCCacheTile *tile = new QGCCacheTile(task->hash(), arrray, format, type);
        task->setTileFetched(tile);
        return;
    }

    qCDebug(QGCTileCacheWorkerLog) << "(NOT in DB) HASH:" << task->hash();
    task->setError("Tile not in cache database");
}

void QGCCacheWorker::_getTileSets(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCFetchTileSetTask *task = static_cast<QGCFetchTileSetTask*>(mtask);
    QSqlQuery query(_getDB());
    const QString s = QStringLiteral("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC");
    qCDebug(QGCTileCacheWorkerLog) << s;
    if (!query.exec(s)) {
        task->setError("No tile set in database");
        return;
    }

    while (query.next()) {
        const QString name = query.value("name").toString();
        QGCCachedTileSet *set = new QGCCachedTileSet(name);
        set->setId(query.value("setID").toULongLong());
        set->setMapTypeStr(query.value("typeStr").toString());
        set->setTopleftLat(query.value("topleftLat").toDouble());
        set->setTopleftLon(query.value("topleftLon").toDouble());
        set->setBottomRightLat(query.value("bottomRightLat").toDouble());
        set->setBottomRightLon(query.value("bottomRightLon").toDouble());
        set->setMinZoom(query.value("minZoom").toInt());
        set->setMaxZoom(query.value("maxZoom").toInt());

        const int typeId = query.value("type").toInt();
        const QString typeStr = UrlFactory::getProviderTypeFromQtMapId(typeId);
        if (typeStr.isEmpty() && typeId != -1) {
            qCWarning(QGCTileCacheWorkerLog) << "Unknown map type ID:" << typeId << "for tile set:" << name;
        }
        set->setType(typeStr);

        set->setTotalTileCount(query.value("numTiles").toUInt());
        set->setDefaultSet(query.value("defaultSet").toInt() != 0);
        set->setCreationDate(QDateTime::fromSecsSinceEpoch(query.value("date").toUInt()));
        _updateSetTotals(set);
        // Object created here must be moved to app thread to be used there
        (void) set->moveToThread(QCoreApplication::instance()->thread());
        task->setTileSetFetched(set);
        if (!set->defaultSet()) {
            _emitDownloadStatus(set->id());
        }
    }
}

void QGCCacheWorker::_updateSetTotals(QGCCachedTileSet *set)
{
    if (set->defaultSet()) {
        // Query ALL tiles in default set (same pattern as regular sets for consistency)
        QSqlQuery query(_getDB());
        (void) query.prepare("SELECT COUNT(size), SUM(size) FROM Tiles A INNER JOIN SetTiles B on A.tileID = B.tileID WHERE B.setID = ?");
        query.addBindValue(set->id());
        if (!query.exec() || !query.next()) {
            return;
        }

        quint32 allCount = query.value(0).toUInt();
        quint64 allSize = query.value(1).toULongLong();

        // For default set, saved = total (no expected tile count like offline sets)
        set->setSavedTileCount(allCount);
        set->setSavedTileSize(allSize);
        set->setTotalTileCount(allCount);
        set->setTotalTileSize(allSize);

        // Also calculate unique tiles (tiles NOT shared with offline sets)
        _updateTotals();
        set->setUniqueTileCount(_defaultCount);
        set->setUniqueTileSize(_defaultSize);
        return;
    }

    QSqlQuery subquery(_getDB());
    (void) subquery.prepare("SELECT COUNT(size), SUM(size) FROM Tiles A INNER JOIN SetTiles B on A.tileID = B.tileID WHERE B.setID = ?");
    subquery.addBindValue(set->id());
    qCDebug(QGCTileCacheWorkerLog) << "Querying totals for set" << set->id();
    if (!subquery.exec() || !subquery.next()) {
        return;
    }

    set->setSavedTileCount(subquery.value(0).toUInt());
    set->setSavedTileSize(subquery.value(1).toULongLong());
    qCDebug(QGCTileCacheWorkerLog) << "Set" << set->id() << "Totals:" << set->savedTileCount() << " " << set->savedTileSize() << "Expected: " << set->totalTileCount() << " " << set->totalTilesSize();
    // Update (estimated) size
    quint64 avg = UrlFactory::averageSizeForType(set->type());
    if (set->totalTileCount() <= set->savedTileCount()) {
        // We're done so the saved size is the total size
        set->setTotalTileSize(set->savedTileSize());
    } else {
        // Otherwise we need to estimate it.
        if ((set->savedTileCount() > 10) && set->savedTileSize()) {
            avg = set->savedTileSize() / set->savedTileCount();
        }
        set->setTotalTileSize(avg * set->totalTileCount());
    }

    quint32 ucount = 0;
    quint64 usize = 0;
    (void) subquery.prepare("SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A join SetTiles B on A.tileID = B.tileID WHERE B.setID = ? GROUP by A.tileID HAVING COUNT(A.tileID) = 1)");
    subquery.addBindValue(set->id());
    if (subquery.exec() && subquery.next()) {
        ucount = subquery.value(0).toUInt();
        usize = subquery.value(1).toULongLong();
    }

    // If we haven't downloaded it all, estimate size of unique tiles
    quint32 expectedUcount = set->totalTileCount() - set->savedTileCount();
    if (ucount == 0) {
        usize = expectedUcount * avg;
    } else {
        expectedUcount = ucount;
    }
    set->setUniqueTileCount(expectedUcount);
    set->setUniqueTileSize(usize);
}

void QGCCacheWorker::_updateTotals()
{
    QSqlQuery query(_getDB());
    QString s = QStringLiteral("SELECT COUNT(size), SUM(size) FROM Tiles");
    qCDebug(QGCTileCacheWorkerLog) << s;
    if (query.exec(s) && query.next()) {
        _totalCount = query.value(0).toUInt();
        _totalSize  = query.value(1).toULongLong();
    }
    query.finish();

    const quint64 defaultSetID = _getDefaultTileSet();
    (void) query.prepare("SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A join SetTiles B on A.tileID = B.tileID WHERE B.setID = ? GROUP by A.tileID HAVING COUNT(A.tileID) = 1)");
    query.addBindValue(defaultSetID);
    qCDebug(QGCTileCacheWorkerLog) << "Querying default set totals";
    if (query.exec() && query.next()) {
        _defaultCount = query.value(0).toUInt();
        _defaultSize = query.value(1).toULongLong();
        query.finish();

        (void) query.prepare("UPDATE TileSets SET numTiles = ? WHERE setID = ?");
        query.addBindValue(_defaultCount);
        query.addBindValue(defaultSetID);
        (void) query.exec();
        query.finish();
    }

    emit updateTotals(_totalCount, _totalSize, _defaultCount, _defaultSize);

    QMutexLocker lock(&_taskQueueMutex);
    if (!_updateTimer.isValid()) {
        _updateTimer.start();
    } else {
        (void) _updateTimer.restart();
    }
}

void QGCCacheWorker::_emitDownloadStatus(quint64 setID)
{
    if (!_valid || (setID == 0)) {
        return;
    }

    QSqlQuery query(_getDB());
    (void) query.prepare("SELECT state, COUNT(*) FROM TilesDownload WHERE setID = ? GROUP BY state");
    query.addBindValue(setID);
    quint32 pending = 0;
    quint32 downloading = 0;
    quint32 errors = 0;
    if (query.exec()) {
        while (query.next()) {
            const int state = query.value(0).toInt();
            const quint32 count = query.value(1).toUInt();
            if (state == QGCTile::StatePending || state == QGCTile::StatePaused) {
                pending += count;
            } else if (state == QGCTile::StateDownloading) {
                downloading += count;
            } else if (state == QGCTile::StateError) {
                errors += count;
            }
        }
    }

    emit downloadStatusUpdated(setID, pending, downloading, errors);
}

void QGCCacheWorker::_setTaskError(QGCMapTask *task, const QString &baseMessage, const QSqlError &error) const
{
    if (!task) {
        return;
    }

    const QString details = error.text();
    if (details.isEmpty()) {
        task->setError(baseMessage);
    } else {
        task->setError(QStringLiteral("%1 (%2)").arg(baseMessage, details));
    }
}

quint64 QGCCacheWorker::_findTile(const QString &hash)
{
    quint64 tileID = 0;

    QSqlQuery query(_getDB());
    (void) query.prepare("SELECT tileID FROM Tiles WHERE hash = ?");
    query.addBindValue(hash);
    if (query.exec() && query.next()) {
        tileID = query.value(0).toULongLong();
    }

    return tileID;
}

void QGCCacheWorker::_createTileSet(QGCMapTask *mtask)
{
    if (!_valid) {
        mtask->setError(tr("Error saving tile set"));
        return;
    }

    QGCCreateTileSetTask *task = static_cast<QGCCreateTileSetTask*>(mtask);

    // Start transaction for entire operation (TileSet creation + download list)
    if (!_getDB().transaction()) {
        const QSqlError error = _getDB().lastError();
        qCWarning(QGCTileCacheWorkerLog) << "Failed to start transaction" << error.text();
        _setTaskError(mtask, tr("Error saving tile set"), error);
        return;
    }

    // Create Tile Set
    QSqlQuery query(_getDB());
    (void) query.prepare("INSERT INTO TileSets("
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
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!query.exec()) {
        const QString errorText = query.lastError().text();
        qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (add tileSet into TileSets):" << errorText;
        query.finish();
        _getDB().rollback();

        QString userError = tr("Error saving tile set");
        if (errorText.contains(QStringLiteral("UNIQUE constraint failed: TileSets.name"), Qt::CaseInsensitive)) {
            userError = tr("A tile set named '%1' already exists. Please choose a different name.")
                .arg(task->tileSet()->name());
        }

        mtask->setError(userError);
        return;
    }

    // Get just created (auto-incremented) setID
    const quint64 setID = query.lastInsertId().toULongLong();
    query.finish();
    task->tileSet()->setId(setID);

    // Prepare queries outside the loops for better performance
    QSqlQuery downloadQuery(_getDB());
    QSqlQuery setTileQuery(_getDB());
    if (!downloadQuery.prepare("INSERT OR IGNORE INTO TilesDownload(setID, hash, type, x, y, z, state) VALUES(?, ?, ?, ?, ?, ?, ?)")) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed to prepare download query:" << downloadQuery.lastError().text();
        downloadQuery.finish();
        _getDB().rollback();
        _setTaskError(mtask, tr("Error creating tile set download list"), downloadQuery.lastError());
        return;
    }
    if (!setTileQuery.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed to prepare set tile query:" << setTileQuery.lastError().text();
        downloadQuery.finish();
        setTileQuery.finish();
        _getDB().rollback();
        _setTaskError(mtask, tr("Error creating tile set download list"), setTileQuery.lastError());
        return;
    }

    bool success = true;
    QSqlError downloadListError;
    const QString type = task->tileSet()->type();
    const int mapId = UrlFactory::getQtMapIdFromProviderType(type);
    if (mapId < 0) {
        qCWarning(QGCTileCacheWorkerLog) << "Invalid mapId for tile set type:" << type;
        downloadQuery.finish();
        setTileQuery.finish();
        _getDB().rollback();
        _setTaskError(mtask, tr("Invalid map type: %1").arg(type), QSqlError());
        return;
    }
    quint32 initialPending = 0;

    for (int z = task->tileSet()->minZoom(); z <= task->tileSet()->maxZoom(); z++) {
        const QGCTileSet set = UrlFactory::getTileCount(z,
            task->tileSet()->topleftLon(), task->tileSet()->topleftLat(),
            task->tileSet()->bottomRightLon(), task->tileSet()->bottomRightLat(), type);

        for (int x = set.tileX0; x <= set.tileX1; x++) {
            for (int y = set.tileY0; y <= set.tileY1; y++) {
                // See if tile is already downloaded
                const QString hash = UrlFactory::getTileHash(type, x, y, z);
                const quint64 tileID = _findTile(hash);
                if (tileID == 0) {
                    // Set to download
                    downloadQuery.addBindValue(setID);
                    downloadQuery.addBindValue(hash);
                    downloadQuery.addBindValue(mapId);
                    downloadQuery.addBindValue(x);
                    downloadQuery.addBindValue(y);
                    downloadQuery.addBindValue(z);
                    downloadQuery.addBindValue(static_cast<int>(QGCTile::StatePending));
                    if (!downloadQuery.exec()) {
                        qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (add tile into TilesDownload):" << downloadQuery.lastError().text();
                        downloadListError = downloadQuery.lastError();
                        success = false;
                        break;
                    }
                    downloadQuery.finish();
                    initialPending++;
                } else {
                    // Tile already in the database. No need to download.
                    setTileQuery.addBindValue(tileID);
                    setTileQuery.addBindValue(setID);
                    if (!setTileQuery.exec()) {
                        qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (add tile into SetTiles):" << setTileQuery.lastError().text();
                        downloadListError = setTileQuery.lastError();
                    }
                    setTileQuery.finish();
                    qCDebug(QGCTileCacheWorkerLog) << "Already Cached HASH:" << hash;
                }
            }
            if (!success) {
                break;
            }
        }
        if (!success) {
            break;
        }
    }

    if (success) {
        if (!_getDB().commit()) {
            qCWarning(QGCTileCacheWorkerLog) << "Commit failed:" << _getDB().lastError();
            downloadQuery.finish();
            setTileQuery.finish();
            _getDB().rollback();
            downloadListError = _getDB().lastError();
            success = false;
        }
    } else {
        downloadQuery.finish();
        setTileQuery.finish();
        _getDB().rollback();
    }

    if (!success) {
        _setTaskError(mtask, tr("Error creating tile set download list"), downloadListError);
        return;
    }

    _updateSetTotals(task->tileSet());
    task->tileSet()->setDownloadStats(initialPending, 0, 0);
    _emitDownloadStatus(setID);
    task->setTileSetSaved();
}

void QGCCacheWorker::_getTileDownloadList(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QQueue<QGCTile*> tiles;
    QGCGetTileDownloadListTask *task = static_cast<QGCGetTileDownloadListTask*>(mtask);
    QSqlQuery query(_getDB());
    (void) query.prepare("SELECT hash, type, x, y, z FROM TilesDownload WHERE setID = ? AND state = ? LIMIT ?");
    query.addBindValue(task->setID());
    query.addBindValue(static_cast<int>(QGCTile::StatePending));
    query.addBindValue(task->count());
    if (query.exec()) {
        while (query.next()) {
            QGCTile *tile = new QGCTile;
            tile->tileSet = task->setID();
            tile->hash = query.value("hash").toString();
            tile->type = UrlFactory::getProviderTypeFromQtMapId(query.value("type").toInt());
            tile->x = query.value("x").toInt();
            tile->y = query.value("y").toInt();
            tile->z = query.value("z").toInt();
            tiles.enqueue(tile);
        }
        query.finish();

        if (!tiles.isEmpty()) {
            if (!_getDB().transaction()) {
                qCWarning(QGCTileCacheWorkerLog) << "Failed to start transaction for tile download list update";
            } else {
                // Prepare query once outside the loop
                if (!query.prepare("UPDATE TilesDownload SET state = ? WHERE setID = ? and hash = ?")) {
                    qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL prepare error:" << query.lastError().text();
                    query.finish();
                    _getDB().rollback();
                } else {
                    bool success = true;
                    for (const QGCTile *tile : tiles) {
                        query.addBindValue(static_cast<int>(QGCTile::StateDownloading));
                        query.addBindValue(task->setID());
                        query.addBindValue(tile->hash);
                        if (!query.exec()) {
                            qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (set TilesDownload state):" << query.lastError().text();
                            success = false;
                            break;
                        }
                        query.finish();
                    }

                    if (success) {
                        if (!_getDB().commit()) {
                            qCWarning(QGCTileCacheWorkerLog) << "Commit failed:" << _getDB().lastError();
                            query.finish();
                            _getDB().rollback();
                        }
                    } else {
                        query.finish();
                        _getDB().rollback();
                    }
                }
            }
        }
    }
    task->setTileListFetched(tiles);
    _emitDownloadStatus(task->setID());
}

void QGCCacheWorker::_updateTileDownloadState(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCUpdateTileDownloadStateTask *task = static_cast<QGCUpdateTileDownloadStateTask*>(mtask);
    QSqlQuery query(_getDB());
    if (task->state() == QGCTile::StateComplete) {
        (void) query.prepare("DELETE FROM TilesDownload WHERE setID = ? AND hash = ?");
        query.addBindValue(task->setID());
        query.addBindValue(task->hash());
    } else if (task->hash() == "*") {
        if (task->hasFromStateFilter()) {
            (void) query.prepare("UPDATE TilesDownload SET state = ? WHERE setID = ? AND state = ?");
            query.addBindValue(static_cast<int>(task->state()));
            query.addBindValue(task->setID());
            query.addBindValue(static_cast<int>(task->fromState()));
        } else {
            (void) query.prepare("UPDATE TilesDownload SET state = ? WHERE setID = ?");
            query.addBindValue(static_cast<int>(task->state()));
            query.addBindValue(task->setID());
        }
    } else {
        (void) query.prepare("UPDATE TilesDownload SET state = ? WHERE setID = ? AND hash = ?");
        query.addBindValue(static_cast<int>(task->state()));
        query.addBindValue(task->setID());
        query.addBindValue(task->hash());
    }

    if (!query.exec()) {
        qCWarning(QGCTileCacheWorkerLog) << "Error:" << query.lastError().text();
    }

    _emitDownloadStatus(task->setID());
}

void QGCCacheWorker::_pruneCache(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCPruneCacheTask *task = static_cast<QGCPruneCacheTask*>(mtask);
    QSqlQuery query(_getDB());
    (void) query.prepare("SELECT tileID, size, hash FROM Tiles WHERE tileID IN (SELECT A.tileID FROM SetTiles A join SetTiles B on A.tileID = B.tileID WHERE B.setID = ? GROUP by A.tileID HAVING COUNT(A.tileID) = 1) ORDER BY DATE ASC LIMIT ?");
    query.addBindValue(_getDefaultTileSet());
    query.addBindValue(kPruneBatchSize);
    if (!query.exec()) {
        return;
    }

    QList<quint64> tlist;
    qint64 amount = static_cast<qint64>(task->amount());
    while (query.next() && (amount >= 0)) {
        tlist << query.value(0).toULongLong();
        amount -= query.value(1).toULongLong();
        qCDebug(QGCTileCacheWorkerLog) << "HASH:" << query.value(2).toString();
    }

    (void) _batchDeleteTiles(query, tlist, "Batch delete failed:");
    task->setPruned();
}

void QGCCacheWorker::_deleteTileSet(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCDeleteTileSetTask *task = static_cast<QGCDeleteTileSetTask*>(mtask);
    _deleteTileSet(task->setID());
    task->setTileSetDeleted();
}

void QGCCacheWorker::_deleteTileSet(qulonglong id)
{
    // Validate that we're not deleting the default set
    const quint64 defaultSetID = _getDefaultTileSet();
    if (id == defaultSetID) {
        qCWarning(QGCTileCacheWorkerLog) << "Cannot delete default tile set";
        return;
    }

    // Use a transaction to ensure atomicity
    if (!_getDB().transaction()) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed to start transaction for deleteTileSet";
        return;
    }

    QSqlQuery query(_getDB());
    bool success = true;

    // Step 1: Find tiles that are ONLY in this set (will become orphaned after deletion)
    QList<quint64> tilesToDelete;
    (void) query.prepare("SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID WHERE B.setID = ? GROUP BY A.tileID HAVING COUNT(A.tileID) = 1");
    query.addBindValue(id);
    if (query.exec()) {
        while (query.next()) {
            tilesToDelete.append(query.value(0).toULongLong());
        }
        query.finish();
    } else {
        qCWarning(QGCTileCacheWorkerLog) << "Error finding tiles to delete:" << query.lastError().text();
        query.finish();
        success = false;
    }

    // Step 2: Delete the tile set (CASCADE will delete from SetTiles and TilesDownload)
    if (success) {
        (void) query.prepare("DELETE FROM TileSets WHERE setID = ?");
        query.addBindValue(id);
        if (!query.exec()) {
            qCWarning(QGCTileCacheWorkerLog) << "Error deleting from TileSets:" << query.lastError().text();
            query.finish();
            success = false;
        } else {
            query.finish();
        }
    }

    // Step 3: Delete orphaned tiles (tiles that were only in this set)
    if (success && !tilesToDelete.isEmpty()) {
        if (!_batchDeleteTiles(query, tilesToDelete, "Error deleting orphaned tiles:")) {
            success = false;
        }
    }

    // Commit or rollback
    if (success) {
        if (!_getDB().commit()) {
            qCWarning(QGCTileCacheWorkerLog) << "Failed to commit deleteTileSet transaction:" << _getDB().lastError();
            _getDB().rollback();
            success = false;
        }
    } else {
        _getDB().rollback();
    }

    if (success) {
        _updateTotals();
    }
}

void QGCCacheWorker::_renameTileSet(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCRenameTileSetTask *task = static_cast<QGCRenameTileSetTask*>(mtask);
    QSqlQuery query(_getDB());
    (void) query.prepare("UPDATE TileSets SET name = ? WHERE setID = ?");
    query.addBindValue(task->newName());
    query.addBindValue(task->setID());
    if (!query.exec()) {
        const QString error = query.lastError().text();
        qCWarning(QGCTileCacheWorkerLog) << "Error renaming tile set:" << error;
        QString userMessage = tr("Error renaming tile set");
        if (error.contains(QStringLiteral("UNIQUE constraint failed"), Qt::CaseInsensitive)) {
            userMessage = tr("A tile set named '%1' already exists. Please choose a different name.")
                .arg(task->newName());
        }
        task->setError(userMessage);
    }
}

void QGCCacheWorker::_resetCacheDatabase(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCResetTask *task = static_cast<QGCResetTask*>(mtask);
    QSqlQuery query(_getDB());

    // Temporarily disable foreign key constraints to allow table drops
    (void) query.exec("PRAGMA foreign_keys = OFF");

    // Drop tables in any order now that foreign keys are disabled
    (void) query.exec("DROP TABLE IF EXISTS TilesDownload");
    (void) query.exec("DROP TABLE IF EXISTS SetTiles");
    (void) query.exec("DROP TABLE IF EXISTS TileSets");
    (void) query.exec("DROP TABLE IF EXISTS Tiles");

    // Recreate database (this will re-enable foreign keys in _createDB)
    QSqlDatabase db = _getDB();
    _valid = _createDB(db);

    // Clear cached default set ID
    _defaultSet = UINT64_MAX;

    task->setResetCompleted();
}

void QGCCacheWorker::_notifyImportFailure(QGCImportTileTask *task, const QString &message)
{
    task->markErrorHandled();
    task->setError(message);
    task->setImportCompleted();
    if (qgcApp()) {
        QMetaObject::invokeMethod(qgcApp(), [message]() {
            qgcApp()->showAppMessage(message);
        }, Qt::QueuedConnection);
    }
}

void QGCCacheWorker::_importSets(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCImportTileTask *task = static_cast<QGCImportTileTask*>(mtask);

    if (!_canAccessDatabase()) {
        task->setError(tr("Map cache database is unavailable."));
        task->setImportCompleted();
        return;
    }

    QString versionError;
    const VersionReadStatus versionStatus = _checkDatabaseVersion(task->path(), &versionError);
    if (versionStatus != VersionReadStatus::Success) {
        if (versionError.isEmpty()) {
            versionError = tr("Imported cache is incompatible with this QGroundControl version.");
        }
        _notifyImportFailure(task, versionError);
        return;
    }

    if (task->replace()) {
        (void) _importReplace(task);
    } else {
        (void) _importAppend(task);
    }

    task->setImportCompleted();
}

bool QGCCacheWorker::_importReplace(QGCImportTileTask *task)
{
    _disconnectDB();
    _defaultSet = UINT64_MAX;
    (void) QFile::remove(_databasePath);

    if (!QFile::copy(task->path(), _databasePath)) {
        _notifyImportFailure(task, tr("Failed to copy imported cache database."));
        return false;
    }

    task->setProgress(25);
    _init();
    if (_valid) {
        task->setProgress(50);
        _connectDB();
    }
    task->setProgress(100);

    return true;
}

bool QGCCacheWorker::_importAppend(QGCImportTileTask *task)
{
    bool importSuccess = true;
    auto recordError = [&importSuccess, task](const QString &message) {
        task->setError(message);
        importSuccess = false;
    };

    QScopedPointer<QSqlDatabase> dbImport(new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", kExportSession)));
    dbImport->setDatabaseName(task->path());
    dbImport->setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    if (dbImport->open()) {
        QSqlQuery query(*dbImport);
        quint64 tileCount = 0;
        int lastProgress = -1;
        QString s = QStringLiteral("SELECT COUNT(tileID) FROM Tiles");
        if (query.exec(s) && query.next()) {
            tileCount  = query.value(0).toULongLong();
        }

        if (tileCount > 0) {
            s = QStringLiteral("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC");
            if (query.exec(s)) {
                quint64 currentCount = 0;
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
                    if (defaultSet == 0) {
                        if (!_generateUniqueTileSetName(name)) {
                            recordError(QStringLiteral("Too many tile sets with similar names"));
                            break;
                        }
                        QSqlQuery cQuery(_getDB());
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
                        cQuery.addBindValue(QDateTime::currentSecsSinceEpoch());
                        if (!cQuery.exec()) {
                            recordError(QStringLiteral("Error adding imported tile set to database"));
                            break;
                        } else {
                            insertSetID = cQuery.lastInsertId().toULongLong();
                        }
                    }

                    QSqlQuery cQuery(_getDB());
                    QSqlQuery subQuery(*dbImport);
                    (void) subQuery.prepare("SELECT * FROM Tiles WHERE tileID IN (SELECT tileID FROM SetTiles WHERE setID = ?)");
                    subQuery.addBindValue(setID);
                    if (subQuery.exec()) {
                        quint64 tilesFound = 0;
                        quint64 tilesSaved = 0;

                        if (!_getDB().transaction()) {
                            qCWarning(QGCTileCacheWorkerLog) << "Failed to start transaction for tile import";
                            recordError(QStringLiteral("Error importing tiles"));
                            break;
                        }

                        QSqlQuery findTileQuery(_getDB());
                        QSqlQuery insertTileQuery(_getDB());
                        QSqlQuery insertSetTileQuery(_getDB());

                        if (!findTileQuery.prepare("SELECT tileID FROM Tiles WHERE hash = ?")) {
                            qCWarning(QGCTileCacheWorkerLog) << "Import find tile prepare error:" << findTileQuery.lastError().text();
                            findTileQuery.finish();
                            insertTileQuery.finish();
                            insertSetTileQuery.finish();
                            _getDB().rollback();
                            recordError(QStringLiteral("Error importing tiles"));
                            break;
                        }
                        if (!insertTileQuery.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)")) {
                            qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL prepare error:" << insertTileQuery.lastError().text();
                            findTileQuery.finish();
                            insertTileQuery.finish();
                            insertSetTileQuery.finish();
                            _getDB().rollback();
                            recordError(QStringLiteral("Error importing tiles"));
                            break;
                        }
                        if (!insertSetTileQuery.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
                            qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL prepare error:" << insertSetTileQuery.lastError().text();
                            findTileQuery.finish();
                            insertTileQuery.finish();
                            insertSetTileQuery.finish();
                            _getDB().rollback();
                            recordError(QStringLiteral("Error importing tiles"));
                            break;
                        }

                        bool success = true;
                        quint64 setTilesLinked = 0;
                        while (subQuery.next()) {
                            tilesFound++;
                            const QString hash = subQuery.value("hash").toString();
                            const QString format = subQuery.value("format").toString();
                            const QByteArray img = subQuery.value("tile").toByteArray();
                            const int type = subQuery.value("type").toInt();

                            quint64 importTileID = 0;
                            findTileQuery.addBindValue(hash);
                            if (findTileQuery.exec() && findTileQuery.next()) {
                                importTileID = findTileQuery.value(0).toULongLong();
                                findTileQuery.finish();
                                qCDebug(QGCTileCacheWorkerLog) << "Tile already exists, reusing HASH:" << hash;
                            } else {
                                findTileQuery.finish();

                                insertTileQuery.addBindValue(hash);
                                insertTileQuery.addBindValue(format);
                                insertTileQuery.addBindValue(img);
                                insertTileQuery.addBindValue(img.size());
                                insertTileQuery.addBindValue(type);
                                insertTileQuery.addBindValue(QDateTime::currentSecsSinceEpoch());

                                if (!insertTileQuery.exec()) {
                                    qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (import tile):" << insertTileQuery.lastError().text();
                                    success = false;
                                    break;
                                }

                                importTileID = insertTileQuery.lastInsertId().toULongLong();
                                insertTileQuery.finish();
                                tilesSaved++;
                            }

                            if (importTileID == 0) {
                                qCWarning(QGCTileCacheWorkerLog) << "Import failed: got tileID=0 for hash:" << hash;
                                success = false;
                                break;
                            }

                            insertSetTileQuery.addBindValue(importTileID);
                            insertSetTileQuery.addBindValue(insertSetID);

                            if (!insertSetTileQuery.exec()) {
                                qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (import SetTiles):" << insertSetTileQuery.lastError().text()
                                                                 << "tileID:" << importTileID << "setID:" << insertSetID;
                                success = false;
                                break;
                            }
                            insertSetTileQuery.finish();
                            setTilesLinked++;

                            currentCount++;
                            if (tileCount > 0) {
                                int progress = static_cast<int>((static_cast<double>(currentCount) / static_cast<double>(tileCount)) * 100.0);
                                if (progress > 100) {
                                    progress = 100;
                                }
                                if (lastProgress != progress) {
                                    lastProgress = progress;
                                    task->setProgress(progress);
                                }
                            }
                        }

                        if (success) {
                            if (!_getDB().commit()) {
                                qCWarning(QGCTileCacheWorkerLog) << "Commit failed:" << _getDB().lastError();
                                findTileQuery.finish();
                                insertTileQuery.finish();
                                insertSetTileQuery.finish();
                                _getDB().rollback();
                                success = false;
                            }
                        } else {
                            findTileQuery.finish();
                            insertTileQuery.finish();
                            insertSetTileQuery.finish();
                            _getDB().rollback();
                        }

                        if (!success) {
                            recordError(QStringLiteral("Error importing tiles"));
                            break;
                        }

                        if (setTilesLinked > 0) {
                            (void) cQuery.prepare("SELECT COUNT(size) FROM Tiles A INNER JOIN SetTiles B on A.tileID = B.tileID WHERE B.setID = ?");
                            cQuery.addBindValue(insertSetID);
                            if (cQuery.exec() && cQuery.next()) {
                                const quint64 count = cQuery.value(0).toULongLong();
                                (void) cQuery.prepare("UPDATE TileSets SET numTiles = ? WHERE setID = ?");
                                cQuery.addBindValue(count);
                                cQuery.addBindValue(insertSetID);
                                (void) cQuery.exec();
                            }
                        }

                        if (tilesSaved > 0) {
                            if (tilesSaved < tileCount) {
                                tileCount -= tilesSaved;
                            } else {
                                tileCount = 0;
                            }
                        }

                        if ((setTilesLinked == 0) && (defaultSet == 0)) {
                            qCDebug(QGCTileCacheWorkerLog) << "No tiles linked for set" << name << "- removing it.";
                            _deleteTileSet(insertSetID);
                        }
                    }

                    if (!importSuccess) {
                        break;
                    }
                }
            } else {
                recordError(QStringLiteral("No tile set in database"));
            }
        }
        // Note: tileCount == 0 is acceptable; imported tiles may already exist
    } else {
        recordError(QStringLiteral("Error opening import database"));
    }

    dbImport.reset();
    QSqlDatabase::removeDatabase(kExportSession);
    return importSuccess;
}

void QGCCacheWorker::_exportSets(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCExportTileTask *task = static_cast<QGCExportTileTask*>(mtask);

    if (!_canAccessDatabase()) {
        task->setError(tr("Map cache database is unavailable."));
        task->setExportCompleted();
        return;
    }

    // Delete target if it exists
    (void) QFile::remove(task->path());
    // Create exported database
    QScopedPointer<QSqlDatabase> dbExport(new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", kExportSession)));
    dbExport->setDatabaseName(task->path());
    dbExport->setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    if (dbExport->open()) {
        if (_createDB(*dbExport, false)) {
            // Prepare progress report
            quint64 tileCount = 0;
            quint64 currentCount = 0;
            for (int i = 0; i < task->sets().count(); i++) {
                const QGCCachedTileSet *set = task->sets().at(i);
                // Default set has no unique tiles
                if (set->defaultSet()) {
                    tileCount += set->totalTileCount();
                } else {
                    tileCount += set->uniqueTileCount();
                }
            }

            if (tileCount == 0) {
                tileCount = 1;
            }

            // Iterate sets to save
            for (int i = 0; i < task->sets().count(); i++) {
                const QGCCachedTileSet *set = task->sets().at(i);

                // Determine the map type ID for export
                const QString setType = set->type();
                int qtMapId;
                if (setType.isEmpty() || set->defaultSet()) {
                    // Default tile set or tile sets with no type use the default map ID
                    qtMapId = UrlFactory::defaultSetMapId();
                } else {
                    qtMapId = UrlFactory::getQtMapIdFromProviderType(setType);
                }

                // Create Tile Exported Set
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
                exportQuery.addBindValue(qtMapId);
                exportQuery.addBindValue(set->totalTileCount());
                exportQuery.addBindValue(set->defaultSet());
                exportQuery.addBindValue(QDateTime::currentSecsSinceEpoch());
                if (!exportQuery.exec()) {
                    qCWarning(QGCTileCacheWorkerLog) << "Error adding tile set to exported database:" << exportQuery.lastError().text();
                    _setTaskError(task, tr("Error adding tile set to exported database"), exportQuery.lastError());
                    break;
                }

                const quint64 exportSetID = exportQuery.lastInsertId().toULongLong();
                if (exportSetID == 0) {
                    qCWarning(QGCTileCacheWorkerLog) << "Failed to get setID for exported tile set:" << set->name();
                    task->setError(tr("Error creating exported tile set"));
                    break;
                }
                QSqlQuery query(_getDB());
                (void) query.prepare("SELECT * FROM SetTiles WHERE setID = ?");
                query.addBindValue(set->id());
                if (!query.exec()) {
                    continue;
                }

                if (!dbExport->transaction()) {
                    qCWarning(QGCTileCacheWorkerLog) << "Failed to start transaction for export";
                    _setTaskError(task, tr("Error exporting tiles"), dbExport->lastError());
                    break;
                }

                // Prepare queries once outside the loop for better performance
                QSqlQuery fetchTileQuery(_getDB());
                QSqlQuery insertTileQuery(*dbExport);
                QSqlQuery insertSetTileQuery(*dbExport);

                if (!fetchTileQuery.prepare("SELECT * FROM Tiles WHERE tileID = ?")) {
                    qCWarning(QGCTileCacheWorkerLog) << "Export prepare error:" << fetchTileQuery.lastError().text();
                    dbExport->rollback();
                    _setTaskError(task, tr("Error exporting tiles"), fetchTileQuery.lastError());
                    break;
                }
                if (!insertTileQuery.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)")) {
                    qCWarning(QGCTileCacheWorkerLog) << "Export insert prepare error:" << insertTileQuery.lastError().text();
                    dbExport->rollback();
                    _setTaskError(task, tr("Error exporting tiles"), insertTileQuery.lastError());
                    break;
                }
                if (!insertSetTileQuery.prepare("INSERT INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
                    qCWarning(QGCTileCacheWorkerLog) << "Export SetTiles prepare error:" << insertSetTileQuery.lastError().text();
                    dbExport->rollback();
                    _setTaskError(task, tr("Error exporting tiles"), insertSetTileQuery.lastError());
                    break;
                }

                // Prepare query to find existing tiles by hash
                QSqlQuery findTileQuery(*dbExport);
                if (!findTileQuery.prepare("SELECT tileID FROM Tiles WHERE hash = ?")) {
                    qCWarning(QGCTileCacheWorkerLog) << "Export find tile prepare error:" << findTileQuery.lastError().text();
                    dbExport->rollback();
                    _setTaskError(task, tr("Error exporting tiles"), findTileQuery.lastError());
                    break;
                }

                bool success = true;
                while (query.next()) {
                    const quint64 tileID = query.value("tileID").toULongLong();

                    fetchTileQuery.addBindValue(tileID);
                    if (!fetchTileQuery.exec() || !fetchTileQuery.next()) {
                        qCWarning(QGCTileCacheWorkerLog) << "Export tile fetch error:" << fetchTileQuery.lastError().text();
                        success = false;
                        break;
                    }

                    const QString hash = fetchTileQuery.value("hash").toString();
                    const QString format = fetchTileQuery.value("format").toString();
                    const QByteArray img = fetchTileQuery.value("tile").toByteArray();
                    const int type = fetchTileQuery.value("type").toInt();
                    fetchTileQuery.finish();

                    // First check if tile already exists in export DB (by hash)
                    quint64 exportTileID = 0;
                    findTileQuery.addBindValue(hash);
                    if (findTileQuery.exec() && findTileQuery.next()) {
                        // Tile already exists, use its ID
                        exportTileID = findTileQuery.value(0).toULongLong();
                        findTileQuery.finish();
                    } else {
                        // Tile doesn't exist, insert it
                        findTileQuery.finish();

                        insertTileQuery.addBindValue(hash);
                        insertTileQuery.addBindValue(format);
                        insertTileQuery.addBindValue(img);
                        insertTileQuery.addBindValue(img.size());
                        insertTileQuery.addBindValue(type);
                        insertTileQuery.addBindValue(QDateTime::currentSecsSinceEpoch());
                        if (!insertTileQuery.exec()) {
                            qCWarning(QGCTileCacheWorkerLog) << "Export tile insert error:" << insertTileQuery.lastError().text();
                            success = false;
                            break;
                        }

                        exportTileID = insertTileQuery.lastInsertId().toULongLong();
                        insertTileQuery.finish();
                    }

                    if (exportTileID == 0) {
                        qCWarning(QGCTileCacheWorkerLog) << "Export failed: got tileID=0 for hash:" << hash;
                        success = false;
                        break;
                    }

                    insertSetTileQuery.addBindValue(exportTileID);
                    insertSetTileQuery.addBindValue(exportSetID);
                    if (!insertSetTileQuery.exec()) {
                        qCWarning(QGCTileCacheWorkerLog) << "Export SetTiles insert error:" << insertSetTileQuery.lastError().text()
                                                         << "tileID:" << exportTileID << "setID:" << exportSetID;
                        success = false;
                        break;
                    }
                    insertSetTileQuery.finish();

                    currentCount++;
                    task->setProgress(static_cast<int>((static_cast<double>(currentCount) / static_cast<double>(tileCount)) * 100.0));
                }

                if (success) {
                    if (!dbExport->commit()) {
                        qCWarning(QGCTileCacheWorkerLog) << "Export commit failed:" << dbExport->lastError();
                        dbExport->rollback();
                        success = false;
                    }
                } else {
                    dbExport->rollback();
                }

                if (!success) {
                    _setTaskError(task, tr("Error exporting tiles"), dbExport->lastError());
                    break;
                }
            }
        } else {
            _setTaskError(task, tr("Error creating export database"), dbExport->lastError());
        }
    } else {
        qCCritical(QGCTileCacheWorkerLog) << "Map Cache SQL error (create export database):" << dbExport->lastError();
        _setTaskError(task, tr("Error opening export database"), dbExport->lastError());
    }
    dbExport.reset();
    QSqlDatabase::removeDatabase(kExportSession);
    task->setExportCompleted();
}

void QGCCacheWorker::_setCacheEnabled(QGCMapTask *mtask)
{
    QGCSetCacheEnabledTask *task = static_cast<QGCSetCacheEnabledTask*>(mtask);
    _cacheEnabled = task->enabled();
}

void QGCCacheWorker::_setDefaultCacheEnabled(QGCMapTask *mtask)
{
    QGCSetDefaultCacheEnabledTask *task = static_cast<QGCSetDefaultCacheEnabledTask*>(mtask);
    _defaultCachingEnabled = task->enabled();
}

bool QGCCacheWorker::_batchDeleteTiles(QSqlQuery& query, const QList<quint64>& tileIds, const QString& errorContext)
{
    if (tileIds.isEmpty()) {
        return true;
    }

    const int maxVariables = kSqliteDefaultVariableLimit;
    if (maxVariables <= 0) {
        qCWarning(QGCTileCacheWorkerLog) << "Invalid SQLite variable limit; delete operation aborted";
        return false;
    }

    const qsizetype totalTiles = tileIds.size();
    qsizetype offset = 0;
    while (offset < totalTiles) {
        const qsizetype remaining = totalTiles - offset;
        const int chunkSize = remaining > maxVariables ? maxVariables : static_cast<int>(remaining);

        QStringList placeholders;
        placeholders.reserve(chunkSize);
        for (int i = 0; i < chunkSize; i++) {
            placeholders.append(QStringLiteral("?"));
        }

        const QString statement =
            QStringLiteral("DELETE FROM Tiles WHERE tileID IN (%1)").arg(placeholders.join(QStringLiteral(", ")));
        query.finish();
        if (!query.prepare(statement)) {
            qCWarning(QGCTileCacheWorkerLog) << errorContext << "prepare failed:" << query.lastError().text();
            return false;
        }

        for (int i = 0; i < chunkSize; i++) {
            query.addBindValue(tileIds.at(offset + i));
        }

        if (!query.exec()) {
            qCWarning(QGCTileCacheWorkerLog) << errorContext << query.lastError().text();
            return false;
        }

        offset += chunkSize;
    }

    return true;
}

bool QGCCacheWorker::_generateUniqueTileSetName(QString& name)
{
    quint64 unusedSetID;
    if (!_findTileSetID(name, unusedSetID)) {
        return true;
    }

    int testCount = 0;
    while (testCount < kMaxNameGenerationAttempts) {
        const QString testName = QString::asprintf("%s %03d", name.toLatin1().constData(), ++testCount);
        if (!_findTileSetID(testName, unusedSetID)) {
            name = testName;
            return true;
        }
    }

    qCWarning(QGCTileCacheWorkerLog) << "Could not generate unique name for tile set:" << name;
    return false;
}

bool QGCCacheWorker::_testTask(QGCMapTask *mtask)
{
    if (!_valid) {
        mtask->setError("No Cache Database");
        return false;
    }

    return true;
}

bool QGCCacheWorker::_canAccessDatabase() const
{
    if (QCoreApplication::instance()) {
        return true;
    }

    static bool warningShown = false;
    if (!warningShown) {
        warningShown = true;
        qCWarning(QGCTileCacheWorkerLog) << "Skipping map cache database access after application shutdown.";
    }
    return false;
}

QGCCacheWorker::VersionReadStatus QGCCacheWorker::_readDatabaseVersion(const QString &path, const QString &connectionName, int &version) const
{
    if (!_canAccessDatabase()) {
        return VersionReadStatus::NoAccess;
    }

    VersionReadStatus status = VersionReadStatus::OpenFailed;

    {
        QSqlDatabase versionDb = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        versionDb.setDatabaseName(path);
        versionDb.setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
        if (versionDb.open()) {
            QSqlQuery query(versionDb);
            if (query.exec("PRAGMA user_version") && query.next()) {
                version = query.value(0).toInt();
                status = VersionReadStatus::Success;
            } else {
                status = VersionReadStatus::QueryFailed;
            }
            versionDb.close();
        } else {
            status = VersionReadStatus::OpenFailed;
        }
    }

    QSqlDatabase::removeDatabase(connectionName);
    return status;
}

bool QGCCacheWorker::_init()
{
    _failed = false;
    {
        QSettings settings;
        settings.beginGroup(QStringLiteral("Maps"));
        const bool disableDefault = settings.value(QStringLiteral("disableDefaultCache"), false).toBool();
        _defaultCachingEnabled.store(!disableDefault);
        settings.endGroup();
    }
    if (!_databasePath.isEmpty()) {
        qCDebug(QGCTileCacheWorkerLog) << "Mapping cache directory:" << _databasePath;
        if (!_verifyDatabaseVersion()) {
            qCCritical(QGCTileCacheWorkerLog) << "Failed to verify cache database";
            _failed = true;
            return false;
        }
        // Initialize Database
        if (_connectDB()) {
            QSqlDatabase db = _getDB();
            _valid = _createDB(db);
            if (!_valid) {
                _failed = true;
            }
        } else {
            qCCritical(QGCTileCacheWorkerLog) << "Map Cache SQL error (open db):" << _getDB().lastError();
            _failed = true;
        }
        _disconnectDB();
    } else {
        qCCritical(QGCTileCacheWorkerLog) << "Could not find suitable cache directory.";
        _failed = true;
    }

    return !_failed;
}

bool QGCCacheWorker::_verifyDatabaseVersion() const
{
    if (_databasePath.isEmpty()) {
        return false;
    }

    if (!_canAccessDatabase()) {
        return false;
    }

    QFileInfo dbInfo(_databasePath);
    if (!dbInfo.exists()) {
        return true;
    }

    int version = 0;
    const VersionReadStatus status = _readDatabaseVersion(
        _databasePath,
        QStringLiteral("QGeoTileWorkerVersionCheck"),
        version);

    if (status == VersionReadStatus::NoAccess) {
        return false;
    }

    if (status == VersionReadStatus::OpenFailed) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed to open cache database for version check. Removing it.";
        (void) QFile::remove(_databasePath);
        return true;
    }

    if (status == VersionReadStatus::QueryFailed) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed to read cache schema version. Removing database.";
        (void) QFile::remove(_databasePath);
        return true;
    }

    if (version >= kCurrentSchemaVersion) {
        return true;
    }

    qCWarning(QGCTileCacheWorkerLog) << "Outdated map cache detected (schema version" << version
                                     << "). Clearing cache database.";
    if (!QFile::remove(_databasePath)) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed to remove cache database:" << _databasePath;
    }

    return true;
}

QGCCacheWorker::VersionReadStatus QGCCacheWorker::_checkDatabaseVersion(const QString &path, QString *errorMessage) const
{
    QFileInfo dbInfo(path);
    if (!dbInfo.exists()) {
        if (errorMessage) {
            *errorMessage = tr("Imported cache database was not found.");
        }
        return VersionReadStatus::OpenFailed;
    }

    int version = 0;
    const VersionReadStatus status = _readDatabaseVersion(
        path,
        QStringLiteral("QGeoTileImportVersionCheck"),
        version);

    if (status == VersionReadStatus::Success) {
        if (version != kCurrentSchemaVersion) {
            if (errorMessage) {
                *errorMessage = tr("Imported cache version (%1) is not compatible (expected %2).")
                    .arg(version)
                    .arg(kCurrentSchemaVersion);
            }
            return VersionReadStatus::VersionMismatch;
        }
        return VersionReadStatus::Success;
    }

    if (errorMessage) {
        switch (status) {
        case VersionReadStatus::OpenFailed:
            *errorMessage = tr("Unable to open imported cache database.");
            break;
        case VersionReadStatus::QueryFailed:
            *errorMessage = tr("Imported cache database is invalid.");
            break;
        case VersionReadStatus::NoAccess:
            *errorMessage = tr("Map cache database is unavailable.");
            break;
        default:
            break;
        }
    }

    return status;
}

bool QGCCacheWorker::_connectDB()
{
    if (!_canAccessDatabase()) {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", kSession);
    db.setDatabaseName(_databasePath);
    db.setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    _valid = db.open();

    if (_valid) {
        // Enable foreign key constraints for this connection
        QSqlQuery query(db);
        if (!query.exec("PRAGMA foreign_keys = ON")) {
            qCWarning(QGCTileCacheWorkerLog) << "Failed to enable foreign keys:" << query.lastError().text();
            _valid = false;
        }
    }

    return _valid;
}

QSqlDatabase QGCCacheWorker::_getDB() const
{
    QSqlDatabase db = QSqlDatabase::database(kSession);
    if (!db.isValid()) {
        qCCritical(QGCTileCacheWorkerLog) << "Database connection invalid - connection not found for session:" << kSession;
    } else if (!db.isOpen()) {
        qCCritical(QGCTileCacheWorkerLog) << "Database connection closed - attempting to use closed database:" << kSession;
    }
    return db;
}

bool QGCCacheWorker::_createDB(QSqlDatabase &db, bool createDefault)
{
    bool res = false;
    QSqlQuery query(db);

    // Enable foreign key constraints
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed to enable foreign keys:" << query.lastError().text();
        return false;
    }

    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS Tiles ("
        "tileID INTEGER PRIMARY KEY NOT NULL, "
        "hash TEXT NOT NULL UNIQUE, "
        "format TEXT NOT NULL, "
        "tile BLOB NULL, "
        "size INTEGER, "
        "type INTEGER, "
        "date INTEGER DEFAULT 0)"))
    {
        qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (create Tiles db):" << query.lastError().text();
    } else {
        (void) query.exec("CREATE INDEX IF NOT EXISTS hash ON Tiles ( hash, size, type ) ");
        (void) query.exec("CREATE INDEX IF NOT EXISTS idx_tiles_date ON Tiles ( date )");
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
            "date INTEGER DEFAULT 0)"))
        {
            qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (create TileSets db):" << query.lastError().text();
        } else if (!query.exec(
            "CREATE TABLE IF NOT EXISTS SetTiles ("
            "setID INTEGER NOT NULL, "
            "tileID INTEGER NOT NULL, "
            "PRIMARY KEY (setID, tileID), "
            "FOREIGN KEY (setID) REFERENCES TileSets(setID) ON DELETE CASCADE, "
            "FOREIGN KEY (tileID) REFERENCES Tiles(tileID) ON DELETE CASCADE)")) {
            qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (create SetTiles db):" << query.lastError().text();
        } else {
            (void) query.exec("CREATE INDEX IF NOT EXISTS idx_settiles_setid ON SetTiles ( setID )");
            (void) query.exec("CREATE INDEX IF NOT EXISTS idx_settiles_tileid ON SetTiles ( tileID )");
            if (!query.exec(
            "CREATE TABLE IF NOT EXISTS TilesDownload ("
            "setID INTEGER NOT NULL, "
            "hash TEXT NOT NULL UNIQUE, "
            "type INTEGER, "
            "x INTEGER, "
            "y INTEGER, "
            "z INTEGER, "
            "state INTEGER DEFAULT 0, "
            "FOREIGN KEY (setID) REFERENCES TileSets(setID) ON DELETE CASCADE)")) {
            qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (create TilesDownload db):" << query.lastError().text();
            } else {
                (void) query.exec("CREATE INDEX IF NOT EXISTS idx_tilesdownload_setid_state ON TilesDownload ( setID, state )");

                // Clean up any duplicate entries in SetTiles from older database versions
                (void) query.exec(
                    "DELETE FROM SetTiles WHERE rowid NOT IN ("
                    "SELECT MIN(rowid) FROM SetTiles GROUP BY setID, tileID)");

                // Database it ready for use
                res = true;
            }
        }
    }

    // Create or update default tile set
    if (res && createDefault) {
        (void) query.prepare("SELECT setID, type FROM TileSets WHERE name = ?");
        query.addBindValue("Default Tile Set");
        if (query.exec()) {
            if (!query.next()) {
                // Create new default tile set
                (void) query.prepare("INSERT INTO TileSets(name, defaultSet, type, date) VALUES(?, ?, ?, ?)");
                query.addBindValue("Default Tile Set");
                query.addBindValue(kSqlTrue);
                query.addBindValue(UrlFactory::defaultSetMapId());
                query.addBindValue(QDateTime::currentSecsSinceEpoch());
                if (!query.exec()) {
                    qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (Creating default tile set):" << db.lastError();
                    res = false;
                }
            } else {
                // Update existing default tile set if it has no type set
                const int existingType = query.value("type").toInt();
                if (existingType != UrlFactory::defaultSetMapId()) {
                    const quint64 setID = query.value("setID").toULongLong();
                    (void) query.prepare("UPDATE TileSets SET type = ? WHERE setID = ?");
                    query.addBindValue(UrlFactory::defaultSetMapId());
                    query.addBindValue(setID);
                    if (!query.exec()) {
                        qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (Updating default tile set type):" << db.lastError();
                    }
                }
            }
        } else {
            qCWarning(QGCTileCacheWorkerLog) << "Map Cache SQL error (Looking for default tile set):" << db.lastError();
        }
    }

    if (!res) {
        (void) QFile::remove(_databasePath);
    } else {
        QSqlQuery pragma(db);
        const QString statement = QStringLiteral("PRAGMA user_version = %1").arg(kCurrentSchemaVersion);
        if (!pragma.exec(statement)) {
            qCWarning(QGCTileCacheWorkerLog) << "Failed to set schema version:" << pragma.lastError().text();
        }
    }

    return res;
}

void QGCCacheWorker::_disconnectDB()
{
    if (QSqlDatabase::contains(kSession)) {
        QSqlDatabase::removeDatabase(kSession);
    }
}
