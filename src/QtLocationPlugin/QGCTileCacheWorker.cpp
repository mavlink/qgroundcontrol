/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Map Tile Cache Worker Thread
 *
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "QGCMapEngine.h"
#include "QGCMapTileSet.h"

#include <QVariant>
#include <QtSql/QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QApplication>
#include <QFile>

#include "time.h"

const char* kDefaultSet = "Default Tile Set";
const QString kSession  = QLatin1String("QGeoTileWorkerSession");

QGC_LOGGING_CATEGORY(QGCTileCacheLog, "QGCTileCacheLog")

//-- Update intervals

#define LONG_TIMEOUT        5
#define SHORT_TIMEOUT       2

//-----------------------------------------------------------------------------
QGCCacheWorker::QGCCacheWorker()
    : _db(NULL)
    , _valid(false)
    , _failed(false)
    , _defaultSet(UINT64_MAX)
    , _totalSize(0)
    , _totalCount(0)
    , _defaultSize(0)
    , _defaultCount(0)
    , _lastUpdate(0)
    , _updateTimeout(SHORT_TIMEOUT)
{

}

//-----------------------------------------------------------------------------
QGCCacheWorker::~QGCCacheWorker()
{

}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::setDatabaseFile(const QString& path)
{
    _databasePath = path;
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::quit()
{
    _mutex.lock();
    while(_taskQueue.count()) {
        QGCMapTask* task = _taskQueue.dequeue();
        delete task;
    }
    _mutex.unlock();
    if(this->isRunning()) {
        _waitc.wakeAll();
    }
}

//-----------------------------------------------------------------------------
bool
QGCCacheWorker::enqueueTask(QGCMapTask* task)
{
    //-- If not initialized, the only allowed task is Init
    if(!_valid && task->type() != QGCMapTask::taskInit) {
        task->setError("Database Not Initialized");
        task->deleteLater();
        return false;
    }
    if(!_taskQueue.contains(task))
    {
        _mutex.lock();
        _taskQueue.enqueue(task);
        _mutex.unlock();
        if(this->isRunning()) {
            _waitc.wakeAll();
        } else {
            this->start(QThread::NormalPriority);
        }
        return true;
    }
    //-- Should never happen
    return false;
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::run()
{
    if(!_valid && !_failed) {
        _init();
    }
    if(_valid) {
        _db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", kSession));
        _db->setDatabaseName(_databasePath);
        _db->setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
        _valid = _db->open();
    }
    while(true) {
        QGCMapTask* task;
        if(_taskQueue.count()) {
            _mutex.lock();
            task = _taskQueue.dequeue();
            _mutex.unlock();
            switch(task->type()) {
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
                case QGCMapTask::taskPruneCache:
                    _pruneCache(task);
                    break;
                case QGCMapTask::taskReset:
                    _resetCacheDatabase(task);
                    break;
            }
            task->deleteLater();
            //-- Check for update timeout
            size_t count = _taskQueue.count();
            if(count > 100) {
                _updateTimeout = LONG_TIMEOUT;
            } else if(count < 25) {
                _updateTimeout = SHORT_TIMEOUT;
            }
            if(!count || (time(0) - _lastUpdate > _updateTimeout)) {
                _updateTotals();
            }
        } else {
            //-- Wait a bit before shutting things down
            _waitmutex.lock();
            int timeout = 5000;
            if(!_waitc.wait(&_waitmutex, timeout))
            {
                _waitmutex.unlock();
                _mutex.lock();
                //-- If nothing to do, close db and leave thread
                if(!_taskQueue.count()) {
                    _mutex.unlock();
                    break;
                }
                _mutex.unlock();
            }
            _waitmutex.unlock();
        }
    }
    if(_db) {
        delete _db;
        _db = NULL;
        QSqlDatabase::removeDatabase(kSession);
    }
}
//-----------------------------------------------------------------------------
bool
QGCCacheWorker::_findTileSetID(const QString name, quint64& setID)
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
        query.addBindValue(QDateTime::currentDateTime().toTime_t());
        if(query.exec()) {
            quint64 tileID = query.lastInsertId().toULongLong();
            quint64 setID = task->tile()->set() == UINT64_MAX ? _getDefaultTileSet() : task->tile()->set();
            QString s = QString("INSERT INTO SetTiles(tileID, setID) VALUES(%1, %2)").arg(tileID).arg(setID);
            query.prepare(s);
            if(!query.exec()) {
                qWarning() << "Map Cache SQL error (add tile into SetTiles):" << query.lastError().text();
            }
            qCDebug(QGCTileCacheLog) << "_saveTile() HASH:" << task->tile()->hash();
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
    if(!_valid) {
        mtask->setError("No Cache Database");
        return;
    }
    bool found = false;
    QGCFetchTileTask* task = static_cast<QGCFetchTileTask*>(mtask);
    QSqlQuery query(*_db);
    QString s = QString("SELECT tile, format, type FROM Tiles WHERE hash = \"%1\"").arg(task->hash());
    if(query.exec(s)) {
        if(query.next()) {
            QByteArray ar   = query.value(0).toByteArray();
            QString format  = query.value(1).toString();
            UrlFactory::MapType type = (UrlFactory::MapType)query.value(2).toInt();
            qCDebug(QGCTileCacheLog) << "_getTile() (Found in DB) HASH:" << task->hash();
            QGCCacheTile* tile = new QGCCacheTile(task->hash(), ar, format, type);
            task->setTileFetched(tile);
            found = true;
        }
    }
    if(!found) {
        qCDebug(QGCTileCacheLog) << "_getTile() (NOT in DB) HASH:" << task->hash();
        task->setError("Tile not in cache database");
    }
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_getTileSets(QGCMapTask* mtask)
{
    if(!_valid) {
        mtask->setError("No Cache Database");
        return;
    }
    QGCFetchTileSetTask* task = static_cast<QGCFetchTileSetTask*>(mtask);
    QSqlQuery query(*_db);
    QString s = QString("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC");
    if(query.exec(s)) {
        while(query.next()) {
            QString name = query.value("name").toString();
            QString desc = query.value("description").toString();
            QGCCachedTileSet* set = new QGCCachedTileSet(name, desc);
            set->setId(query.value("setID").toULongLong());
            set->setMapTypeStr(query.value("typeStr").toString());
            set->setTopleftLat(query.value("topleftLat").toDouble());
            set->setTopleftLon(query.value("topleftLon").toDouble());
            set->setBottomRightLat(query.value("bottomRightLat").toDouble());
            set->setBottomRightLon(query.value("bottomRightLon").toDouble());
            set->setMinZoom(query.value("minZoom").toInt());
            set->setMaxZoom(query.value("maxZoom").toInt());
            set->setType((UrlFactory::MapType)query.value("type").toInt());
            set->setNumTiles(query.value("numTiles").toUInt());
            set->setTilesSize(query.value("tilesSize").toULongLong());
            set->setDefaultSet(query.value("defaultSet").toInt() != 0);
            set->setCreationDate(QDateTime::fromTime_t(query.value("date").toUInt()));
            //-- Load thumbnail (if not default set)
            if(!set->defaultSet()) {
                int w = query.value("thumbW").toInt();
                int h = query.value("thumbH").toInt();
                if(w && h) {
                    QByteArray ba = query.value("thumbNail").toByteArray();
                    set->setThumbNail(QImage((uchar*)(void*)ba.data(), w, h, QImage::Format_RGB32));
                }
            }
            _updateSetTotals(set);
            //-- Object created here must be moved to app thread to be used there
            set->moveToThread(QApplication::instance()->thread());
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
        set->setSavedTiles(_totalCount);
        set->setSavedSize(_totalSize);
        set->setNumTiles(_defaultCount);
        set->setTilesSize(_defaultSize);
        return;
    }
    QSqlQuery subquery(*_db);
    QString sq = QString("SELECT COUNT(size), SUM(size) FROM Tiles A INNER JOIN SetTiles B on A.tileID = B.tileID WHERE B.setID = %1").arg(set->id());
    if(subquery.exec(sq)) {
        if(subquery.next()) {
            set->setSavedTiles(subquery.value(0).toUInt());
            set->setSavedSize(subquery.value(1).toULongLong());
            //-- Update estimated size
            if(set->savedTiles() > 10 && set->savedSize()) {
                quint32 avg = set->savedSize() / set->savedTiles();
                set->setTilesSize(avg * set->numTiles());
            }
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
    if(query.exec(s)) {
        if(query.next()) {
            _totalCount = query.value(0).toUInt();
            _totalSize  = query.value(1).toULongLong();
        }
    }
    s = QString("SELECT COUNT(size), SUM(size) FROM Tiles A INNER JOIN SetTiles B on A.tileID = B.tileID WHERE B.setID = %1").arg(_getDefaultTileSet());
    if(query.exec(s)) {
        if(query.next()) {
            _defaultCount = query.value(0).toUInt();
            _defaultSize  = query.value(1).toULongLong();
        }
    }
    emit updateTotals(_totalCount, _totalSize, _defaultCount, _defaultSize);
    _lastUpdate = time(0);
}

//-----------------------------------------------------------------------------
bool
QGCCacheWorker::_findTile(const QString hash)
{
    QSqlQuery query(*_db);
    QString s = QString("SELECT type FROM Tiles WHERE hash = \"%1\"").arg(hash);
    if(query.exec(s)) {
        if(query.next()) {
            return true;
        }
    }
    return false;
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
            "name, description, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, maxZoom, type, numTiles, tilesSize, thumbNail, thumbW, thumbH, date"
            ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        query.addBindValue(task->tileSet()->name());
        query.addBindValue(task->tileSet()->description());
        query.addBindValue(task->tileSet()->mapTypeStr());
        query.addBindValue(task->tileSet()->topleftLat());
        query.addBindValue(task->tileSet()->topleftLon());
        query.addBindValue(task->tileSet()->bottomRightLat());
        query.addBindValue(task->tileSet()->bottomRightLon());
        query.addBindValue(task->tileSet()->minZoom());
        query.addBindValue(task->tileSet()->maxZoom());
        query.addBindValue(task->tileSet()->type());
        query.addBindValue(task->tileSet()->numTiles());
        query.addBindValue(task->tileSet()->tilesSize());
        if(task->tileSet()->thumbNail().isNull()) {
            query.addBindValue(QByteArray(1,'\0'));
            query.addBindValue(0);
            query.addBindValue(0);
        } else {
            query.addBindValue(QByteArray((const char *)(void*)task->tileSet()->thumbNail().convertToFormat(QImage::Format_RGB32).bits(), task->tileSet()->thumbNail().byteCount()));
            query.addBindValue(task->tileSet()->thumbNail().width());
            query.addBindValue(task->tileSet()->thumbNail().height());
        }
        query.addBindValue(QDateTime::currentDateTime().toTime_t());
        if(!query.exec()) {
            qWarning() << "Map Cache SQL error (add tileSet into TileSets):" << query.lastError().text();
        } else {
            //-- Get just creted (auto-incremented) setID
            quint64 setID = query.lastInsertId().toULongLong();
            task->tileSet()->setId(setID);
            //-- Prepare Download List
            for(int z = task->tileSet()->minZoom(); z <= task->tileSet()->maxZoom(); z++) {
                QGCTileSet set = QGCMapEngine::getTileCount(z,
                    task->tileSet()->topleftLon(), task->tileSet()->topleftLat(),
                    task->tileSet()->bottomRightLon(), task->tileSet()->bottomRightLat(), task->tileSet()->type());
                UrlFactory::MapType type = task->tileSet()->type();
                for(int x = set.tileX0; x <= set.tileX1; x++) {
                    for(int y = set.tileY0; y <= set.tileY1; y++) {
                        //-- See if tile is already downloaded
                        QString hash = QGCMapEngine::getTileHash(type, x, y, z);
                        if(!_findTile(hash)) {
                            //-- Set to download
                            query.prepare("INSERT OR IGNORE INTO TilesDownload(setID, hash, type, x, y, z, state) VALUES(?, ?, ?, ?, ? ,? ,?)");
                            query.addBindValue(setID);
                            query.addBindValue(hash);
                            query.addBindValue(type);
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
                        }
                    }
                }
            }
            //-- Now update how many tiles we actually have to download
            quint64 actual_size = actual_count * UrlFactory::averageSizeForType(task->tileSet()->type());
            QString s = QString("UPDATE TileSets SET numTiles = %1, tilesSize = %2 WHERE setID = %3").arg(actual_count).arg(actual_size).arg(task->tileSet()->setID());
            if(!query.exec(s)) {
                qWarning() << "Map Cache SQL error (set TilesDownload state):" << query.lastError().text();
            }
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
    if(!_valid) {
        mtask->setError("No Cache Database");
        return;
    }
    QList<QGCTile*> tiles;
    QGCGetTileDownloadListTask* task = static_cast<QGCGetTileDownloadListTask*>(mtask);
    QSqlQuery query(*_db);
    QString s = QString("SELECT hash, type, x, y, z FROM TilesDownload WHERE setID = %1 AND state = 0 LIMIT %2").arg(task->setID()).arg(task->count());
    if(query.exec(s)) {
        while(query.next()) {
            QGCTile* tile = new QGCTile;
            tile->setHash(query.value("hash").toString());
            tile->setType((UrlFactory::MapType)query.value("type").toInt());
            tile->setX(query.value("x").toInt());
            tile->setY(query.value("y").toInt());
            tile->setZ(query.value("z").toInt());
            tiles.append(tile);
        }
        for(int i = 0; i < tiles.size(); i++) {
            s = QString("UPDATE TilesDownload SET state = %1 WHERE setID = %2 and hash = \"%3\"").arg((int)QGCTile::StateDownloading).arg(task->setID()).arg(tiles[i]->hash());
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
    if(!_valid) {
        mtask->setError("No Cache Database");
        return;
    }
    QGCUpdateTileDownloadStateTask* task = static_cast<QGCUpdateTileDownloadStateTask*>(mtask);
    QSqlQuery query(*_db);
    QString s;
    if(task->state() == QGCTile::StateComplete) {
        s = QString("DELETE FROM TilesDownload WHERE setID = %1 AND hash = \"%2\"").arg(task->setID()).arg(task->hash());
    } else {
        if(task->hash() == "*") {
            s = QString("UPDATE TilesDownload SET state = %1 WHERE setID = %2").arg((int)task->state()).arg(task->setID());
        } else {
            s = QString("UPDATE TilesDownload SET state = %1 WHERE setID = %2 AND hash = \"%3\"").arg((int)task->state()).arg(task->setID()).arg(task->hash());
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
    if(!_valid) {
        mtask->setError("No Cache Database");
        return;
    }
    QGCPruneCacheTask* task = static_cast<QGCPruneCacheTask*>(mtask);
    QSqlQuery query(*_db);
    QString s;
    s = QString("SELECT tileID, size, hash FROM Tiles WHERE tileID IN (SELECT tileID FROM SetTiles WHERE setID = %1) ORDER BY DATE ASC LIMIT 128").arg(_getDefaultTileSet());
    qint64 amount = (qint64)task->amount();
    QList<quint64> tlist;
    if(query.exec(s)) {
        while(query.next() && amount >= 0) {
            tlist << query.value(0).toULongLong();
            amount -= query.value(1).toULongLong();
            qCDebug(QGCTileCacheLog) << "_pruneCache() HASH:" << query.value(2).toString();
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
    if(!_valid) {
        mtask->setError("No Cache Database");
        return;
    }
    QGCDeleteTileSetTask* task = static_cast<QGCDeleteTileSetTask*>(mtask);
    QSqlQuery query(*_db);
    QString s;
    s = QString("DELETE FROM Tiles WHERE tileID IN (SELECT tileID FROM SetTiles WHERE setID = %1)").arg(task->setID());
    query.exec(s);
    s = QString("DELETE FROM TilesDownload WHERE setID = %1").arg(task->setID());
    query.exec(s);
    s = QString("DELETE FROM TileSets WHERE setID = %1").arg(task->setID());
    query.exec(s);
    s = QString("DELETE FROM SetTiles WHERE setID = %1").arg(task->setID());
    query.exec(s);
    _updateTotals();
    task->setTileSetDeleted();
}
//-----------------------------------------------------------------------------
void
QGCCacheWorker::_resetCacheDatabase(QGCMapTask* mtask)
{
    if(!_valid) {
        mtask->setError("No Cache Database");
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
    _createDB();
    task->setResetCompleted();
}

//-----------------------------------------------------------------------------
bool
QGCCacheWorker::_init()
{
    _failed = false;
    if(!_databasePath.isEmpty()) {
        qCDebug(QGCTileCacheLog) << "Mapping cache directory:" << _databasePath;
        //-- Initialize Database
        _db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", kSession));
        _db->setDatabaseName(_databasePath);
        _db->setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
        if (_db->open()) {
            _createDB();
        } else {
            qCritical() << "Map Cache SQL error (init() open db):" << _db->lastError();
            _failed = true;
        }
        delete _db;
        _db = NULL;
        QSqlDatabase::removeDatabase(kSession);
    } else {
        qCritical() << "Could not find suitable cache directory.";
        _failed = true;
    }
    return _failed;
}

//-----------------------------------------------------------------------------
void
QGCCacheWorker::_createDB()
{
    QSqlQuery query(*_db);
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
        if(!query.exec(
            "CREATE TABLE IF NOT EXISTS TileSets ("
            "setID INTEGER PRIMARY KEY NOT NULL, "
            "name TEXT NOT NULL UNIQUE, "
            "description TEXT NOT NULL, "
            "typeStr TEXT, "
            "topleftLat REAL DEFAULT 0.0, "
            "topleftLon REAL DEFAULT 0.0, "
            "bottomRightLat REAL DEFAULT 0.0, "
            "bottomRightLon REAL DEFAULT 0.0, "
            "minZoom INTEGER DEFAULT 3, "
            "maxZoom INTEGER DEFAULT 3, "
            "type INTEGER DEFAULT -1, "
            "numTiles INTEGER DEFAULT 0, "
            "tilesSize INTEGER DEFAULT 0, "
            "defaultSet INTEGER DEFAULT 0, "
            "thumbNail BLOB NULL, "
            "thumbW INTEGER DEFAULT 0, "
            "thumbH INTEGER DEFAULT 0, "
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
                    _valid = true;
                }
            }
        }
    }
    //-- Create default tile set
    if(_valid) {
        QString s = QString("SELECT name FROM TileSets WHERE name = \"%1\"").arg(kDefaultSet);
        if(query.exec(s)) {
            if(!query.next()) {
                query.prepare("INSERT INTO TileSets(name, description, defaultSet, date) VALUES(?, ?, ?, ?)");
                query.addBindValue(kDefaultSet);
                query.addBindValue("System wide tile cache");
                query.addBindValue(1);
                query.addBindValue(QDateTime::currentDateTime().toTime_t());
                if(!query.exec()) {
                    qWarning() << "Map Cache SQL error (Creating default tile set):" << _db->lastError();
                    _valid = false;
                }
            }
        } else {
            qWarning() << "Map Cache SQL error (Looking for default tile set):" << _db->lastError();
        }
    }
    if(!_valid) {
        QFile file(_databasePath);
        file.remove();
    }
    _failed = !_valid;
}
