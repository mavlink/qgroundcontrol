/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Gus Grubba <mavlink@grubba.com>

#include "QGCMapEngineManager.h"
#include "QGCApplication.h"
#include "QGCMapTileSet.h"
#include "QGCMapUrlEngine.h"

#include <QSettings>
#include <QStorageInfo>
#include <stdio.h>

QGC_LOGGING_CATEGORY(QGCMapEngineManagerLog, "QGCMapEngineManagerLog")

static const char* kQmlOfflineMapKeyName = "QGCOfflineMap";

//-----------------------------------------------------------------------------
QGCMapEngineManager::QGCMapEngineManager(QGCApplication* app)
    : QGCTool(app)
    , _topleftLat(0.0)
    , _topleftLon(0.0)
    , _bottomRightLat(0.0)
    , _bottomRightLon(0.0)
    , _minZoom(0)
    , _maxZoom(0)
    , _setID(UINT64_MAX)
    , _freeDiskSpace(0)
    , _diskSpace(0)
{

}

//-----------------------------------------------------------------------------
QGCMapEngineManager::~QGCMapEngineManager()
{
    _tileSets.clear();
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);
   QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
   qmlRegisterUncreatableType<QGCMapEngineManager>("QGroundControl.QGCMapEngineManager", 1, 0, "QGCMapEngineManager", "Reference only");
   connect(getQGCMapEngine(), &QGCMapEngine::updateTotals, this, &QGCMapEngineManager::_updateTotals);
   _updateDiskFreeSpace();
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::updateForCurrentView(double lon0, double lat0, double lon1, double lat1, int minZoom, int maxZoom, const QString& mapName)
{
    UrlFactory::MapType mapType = QGCMapEngine::getTypeFromName(mapName);

    _topleftLat     = lat0;
    _topleftLon     = lon0;
    _bottomRightLat = lat1;
    _bottomRightLon = lon1;
    _minZoom        = minZoom;
    _maxZoom        = maxZoom;
    _totalSet.clear();
    for(int z = minZoom; z <= maxZoom; z++) {
        QGCTileSet set = QGCMapEngine::getTileCount(z, lon0, lat0, lon1, lat1, mapType);
        _totalSet += set;
    }
    //-- Beyond 100,000,000 tiles is just nuts
    if(_totalSet.tileCount > 100 * 1000 * 1000) {
        _crazySize = true;
        emit crazySizeChanged();
    } else {
        _crazySize = false;
        emit crazySizeChanged();
        emit tileX0Changed();
        emit tileX1Changed();
        emit tileY0Changed();
        emit tileY1Changed();
        emit tileCountChanged();
        emit tileSizeChanged();
    }
}

//-----------------------------------------------------------------------------
QString
QGCMapEngineManager::tileCountStr()
{
    return QGCMapEngine::numberToString(_totalSet.tileCount);
}

//-----------------------------------------------------------------------------
QString
QGCMapEngineManager::tileSizeStr()
{
    return QGCMapEngine::bigSizeToString(_totalSet.tileSize);
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::loadTileSets()
{
    if(_tileSets.count()) {
        _tileSets.clear();
        emit tileSetsChanged();
    }
    QGCFetchTileSetTask* task = new QGCFetchTileSetTask();
    connect(task, &QGCFetchTileSetTask::tileSetFetched, this, &QGCMapEngineManager::_tileSetFetched);
    connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    getQGCMapEngine()->addTask(task);
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::_tileSetFetched(QGCCachedTileSet* tileSet)
{
    //-- A blank (default) type means it uses various types and not just one
    if(tileSet->type() == UrlFactory::Invalid) {
        tileSet->setMapTypeStr("Various");
    }
    _tileSets.append(tileSet);
    tileSet->setManager(this);
    emit tileSetsChanged();
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::startDownload(const QString& name, const QString& description, const QString& mapType, const QImage& image)
{
    if(_totalSet.tileSize) {
        QGCCachedTileSet* set = new QGCCachedTileSet(name, description);
        set->setMapTypeStr(mapType);
        set->setTopleftLat(_topleftLat);
        set->setTopleftLon(_topleftLon);
        set->setBottomRightLat(_bottomRightLat);
        set->setBottomRightLon(_bottomRightLon);
        set->setMinZoom(_minZoom);
        set->setMaxZoom(_maxZoom);
        set->setTilesSize(_totalSet.tileSize);
        set->setNumTiles(_totalSet.tileCount);
        set->setType(QGCMapEngine::getTypeFromName(mapType));
        if(!image.isNull())
            set->setThumbNail(image);
        QGCCreateTileSetTask* task = new QGCCreateTileSetTask(set);
        //-- Create Tile Set (it will also create a list of tiles to download)
        connect(task, &QGCCreateTileSetTask::tileSetSaved, this, &QGCMapEngineManager::_tileSetSaved);
        connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        getQGCMapEngine()->addTask(task);
    } else {
        qWarning() <<  "QGCMapEngineManager::startDownload() No Tiles to save";
    }
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::_tileSetSaved(QGCCachedTileSet *set)
{
    qCDebug(QGCMapEngineManagerLog) << "New tile set saved (" << set->name() << "). Starting download...";
    _tileSets.append(set);
    emit tileSetsChanged();
    //-- Start downloading tiles
    set->createDownloadTask();
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::saveSetting (const QString& key, const QString& value)
{
    QSettings settings;
    settings.beginGroup(kQmlOfflineMapKeyName);
    settings.setValue(key, value);
}

//-----------------------------------------------------------------------------
QString
QGCMapEngineManager::loadSetting (const QString& key, const QString& defaultValue)
{
    QSettings settings;
    settings.beginGroup(kQmlOfflineMapKeyName);
    return settings.value(key, defaultValue).toString();
}

//-----------------------------------------------------------------------------
QStringList
QGCMapEngineManager::mapList()
{
    return getQGCMapEngine()->getMapNameList();
}

//-----------------------------------------------------------------------------
QString
QGCMapEngineManager::mapboxToken()
{
    return getQGCMapEngine()->getMapBoxToken();
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::setMapboxToken(QString token)
{
    getQGCMapEngine()->setMapBoxToken(token);
}

//-----------------------------------------------------------------------------
quint32
QGCMapEngineManager::maxMemCache()
{
    return getQGCMapEngine()->getMaxMemCache();
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::setMaxMemCache(quint32 size)
{
    getQGCMapEngine()->setMaxMemCache(size);
}

//-----------------------------------------------------------------------------
quint32
QGCMapEngineManager::maxDiskCache()
{
    return getQGCMapEngine()->getMaxDiskCache();
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::setMaxDiskCache(quint32 size)
{
    getQGCMapEngine()->setMaxDiskCache(size);
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::deleteTileSet(QGCCachedTileSet* tileSet)
{
    qCDebug(QGCMapEngineManagerLog) << "Deleting tile set " << tileSet->name();
    //-- If deleting default set, delete it all
    if(tileSet->defaultSet()) {
        for(int i = 0; i < _tileSets.count(); i++ ) {
            QGCCachedTileSet* set = qobject_cast<QGCCachedTileSet*>(_tileSets.get(i));
            Q_ASSERT(set);
            set->setDeleting(true);
        }
        QGCResetTask* task = new QGCResetTask();
        connect(task, &QGCResetTask::resetCompleted, this, &QGCMapEngineManager::_resetCompleted);
        connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        getQGCMapEngine()->addTask(task);
    } else {
        tileSet->setDeleting(true);
        QGCDeleteTileSetTask* task = new QGCDeleteTileSetTask(tileSet->setID());
        connect(task, &QGCDeleteTileSetTask::tileSetDeleted, this, &QGCMapEngineManager::_tileSetDeleted);
        connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        getQGCMapEngine()->addTask(task);
    }
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::_resetCompleted()
{
    //-- Reload sets
    loadTileSets();
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::_tileSetDeleted(quint64 setID)
{
    //-- Tile Set successfully deleted
    QGCCachedTileSet* setToDelete = NULL;
    int i = 0;
    for(i = 0; i < _tileSets.count(); i++ ) {
        QGCCachedTileSet* set = qobject_cast<QGCCachedTileSet*>(_tileSets.get(i));
        Q_ASSERT(set);
        if (set->setID() == setID) {
            setToDelete = set;
            break;
        }
    }
    if(setToDelete) {
        _tileSets.removeAt(i);
        delete setToDelete;
    }
    emit tileSetsChanged();
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::taskError(QGCMapTask::TaskType type, QString error)
{
    QString task;
    switch(type) {
    case QGCMapTask::taskFetchTileSets:
        task = "Fetch Tile Set";
        break;
    case QGCMapTask::taskCreateTileSet:
        task = "Create Tile Set";
        break;
    case QGCMapTask::taskGetTileDownloadList:
        task = "Get Tile Download List";
        break;
    case QGCMapTask::taskUpdateTileDownloadState:
        task = "Update Tile Download Status";
        break;
    case QGCMapTask::taskDeleteTileSet:
        task = "Delete Tile Set";
        break;
    case QGCMapTask::taskReset:
        task = "Reset Tile Sets";
        break;
    default:
        task = "Database Error";
        break;
    }
    QString serror = "Error in task: " + task;
    serror += "\nError description:\n";
    serror += error;
    qWarning() << "QGCMapEngineManager::_taskError()";
    setErrorMessage(serror);
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::_updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize)
{
    for(int i = 0; i < _tileSets.count(); i++ ) {
        QGCCachedTileSet* set = qobject_cast<QGCCachedTileSet*>(_tileSets.get(i));
        Q_ASSERT(set);
        if (set->defaultSet()) {
            set->setSavedSize(totalsize);
            set->setSavedTiles(totaltiles);
            set->setNumTiles(defaulttiles);
            set->setTilesSize(defaultsize);
            return;
        }
    }
    _updateDiskFreeSpace();
}

//-----------------------------------------------------------------------------
bool
QGCMapEngineManager::findName(const QString& name)
{
    for(int i = 0; i < _tileSets.count(); i++ ) {
        QGCCachedTileSet* set = qobject_cast<QGCCachedTileSet*>(_tileSets.get(i));
        Q_ASSERT(set);
        if (set->name() == name) {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
QString
QGCMapEngineManager::getUniqueName()
{
    QString test = "Tile Set ";
    QString name;
    int count = 1;
    while (true) {
        name = test;
        name += QString().sprintf("%03d", count++);
        if(!findName(name))
            return name;
    }
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::_updateDiskFreeSpace()
{
    QString path = getQGCMapEngine()->getCachePath();
    if(!path.isEmpty()) {
        QStorageInfo info(path);
        quint32 total = (quint32)(info.bytesTotal() / 1024);
        quint32 free  = (quint32)(info.bytesFree()  / 1024);
        qCDebug(QGCMapEngineManagerLog) << info.rootPath() << "has" << free << "Mbytes available.";
        if(_freeDiskSpace != free) {
            _freeDiskSpace = free;
            _diskSpace = total;
            emit freeDiskSpaceChanged();
        }
    }
}
