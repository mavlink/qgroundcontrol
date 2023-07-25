/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Gus Grubba <gus@auterion.com>

#if !defined(__mobile__)
//-- TODO: #include "QGCQFileDialog.h"
#endif

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
QGCMapEngineManager::QGCMapEngineManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _topleftLat(0.0)
    , _topleftLon(0.0)
    , _bottomRightLat(0.0)
    , _bottomRightLon(0.0)
    , _minZoom(0)
    , _maxZoom(0)
    , _setID(UINT64_MAX)
    , _freeDiskSpace(0)
    , _diskSpace(0)
    , _fetchElevation(true)
    , _actionProgress(0)
    , _importAction(ActionNone)
    , _importReplace(false)
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
    _topleftLat     = lat0;
    _topleftLon     = lon0;
    _bottomRightLat = lat1;
    _bottomRightLon = lon1;
    _minZoom        = minZoom;
    _maxZoom        = maxZoom;

    _imageSet.clear();
    _elevationSet.clear();

    for(int z = minZoom; z <= maxZoom; z++) {
        QGCTileSet set = QGCMapEngine::getTileCount(z, lon0, lat0, lon1, lat1, mapName);
        _imageSet += set;
    }
    if (_fetchElevation) {
        QGCTileSet set = QGCMapEngine::getTileCount(1, lon0, lat0, lon1, lat1, "Airmap Elevation");
        _elevationSet += set;
    }

    emit tileCountChanged();
    emit tileSizeChanged();

    qCDebug(QGCMapEngineManagerLog) << "updateForCurrentView" << lat0 << lon0 << lat1 << lon1 << minZoom << maxZoom;
}

//-----------------------------------------------------------------------------
QString
QGCMapEngineManager::tileCountStr() const
{
    return QGCMapEngine::numberToString(_imageSet.tileCount + _elevationSet.tileCount);
}

//-----------------------------------------------------------------------------
QString
QGCMapEngineManager::tileSizeStr() const
{
    return QGCMapEngine::bigSizeToString(_imageSet.tileSize + _elevationSet.tileSize);
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
    if(tileSet->type() == "Invalid") {
        tileSet->setMapTypeStr("Various");
    }
    _tileSets.append(tileSet);
    tileSet->setManager(this);
    emit tileSetsChanged();
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::startDownload(const QString& name, const QString& mapType)
{
    if(_imageSet.tileSize) {
        QGCCachedTileSet* set = new QGCCachedTileSet(name);
        set->setMapTypeStr(mapType);
        set->setTopleftLat(_topleftLat);
        set->setTopleftLon(_topleftLon);
        set->setBottomRightLat(_bottomRightLat);
        set->setBottomRightLon(_bottomRightLon);
        set->setMinZoom(_minZoom);
        set->setMaxZoom(_maxZoom);
        set->setTotalTileSize(_imageSet.tileSize);
        set->setTotalTileCount(static_cast<quint32>(_imageSet.tileCount));
        set->setType(mapType);
        QGCCreateTileSetTask* task = new QGCCreateTileSetTask(set);
        //-- Create Tile Set (it will also create a list of tiles to download)
        connect(task, &QGCCreateTileSetTask::tileSetSaved, this, &QGCMapEngineManager::_tileSetSaved);
        connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        getQGCMapEngine()->addTask(task);
    } else {
        qWarning() <<  "QGCMapEngineManager::startDownload() No Tiles to save";
    }
    if (mapType != "Airmap Elevation" && _fetchElevation) {
        QGCCachedTileSet* set = new QGCCachedTileSet(name + " Elevation");
        set->setMapTypeStr("Airmap Elevation");
        set->setTopleftLat(_topleftLat);
        set->setTopleftLon(_topleftLon);
        set->setBottomRightLat(_bottomRightLat);
        set->setBottomRightLon(_bottomRightLon);
        set->setMinZoom(1);
        set->setMaxZoom(1);
        set->setTotalTileSize(_elevationSet.tileSize);
        set->setTotalTileCount(static_cast<quint32>(_elevationSet.tileCount));
        set->setType("Airmap Elevation");
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
QStringList
QGCMapEngineManager::mapProviderList()
{
    // Extract Provider name from MapName ( format : "Provider Type")
    QStringList mapList = getQGCMapEngine()->getMapNameList();
    mapList.replaceInStrings(QRegExp("^([^\\ ]*) (.*)$"),"\\1");
    mapList.removeDuplicates();
    return mapList;
}

//-----------------------------------------------------------------------------
QStringList
QGCMapEngineManager::mapTypeList(QString provider)
{
    // Extract type name from MapName ( format : "Provider Type")
    QStringList mapList = getQGCMapEngine()->getMapNameList();
    mapList = mapList.filter(QRegularExpression(provider));
    mapList.replaceInStrings(QRegExp("^([^\\ ]*) (.*)$"),"\\2");
    mapList.removeDuplicates();
    return mapList;
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
            if(set) {
                set->setDeleting(true);
            }
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
QGCMapEngineManager::renameTileSet(QGCCachedTileSet* tileSet, QString newName)
{
    //-- Name must be unique
    int idx = 1;
    QString name = newName;
    while(findName(name)) {
        name = QString("%1 (%2)").arg(newName).arg(idx++);
    }
    qCDebug(QGCMapEngineManagerLog) << "Renaming tile set " << tileSet->name() << "to" << name;
    tileSet->setName(name);
    QGCRenameTileSetTask* task = new QGCRenameTileSetTask(tileSet->setID(), name);
    connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    getQGCMapEngine()->addTask(task);
    emit tileSet->nameChanged();
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
    QGCCachedTileSet* setToDelete = nullptr;
    int i = 0;
    for(i = 0; i < _tileSets.count(); i++ ) {
        QGCCachedTileSet* set = qobject_cast<QGCCachedTileSet*>(_tileSets.get(i));
        if (set && set->setID() == setID) {
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
    case QGCMapTask::taskExport:
        task = "Export Tile Sets";
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
        if (set && set->defaultSet()) {
            set->setSavedTileSize(totalsize);
            set->setSavedTileCount(totaltiles);
            set->setTotalTileCount(defaulttiles);
            set->setTotalTileSize(defaultsize);
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
        if (set && set->name() == name) {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::selectAll() {
    for(int i = 0; i < _tileSets.count(); i++ ) {
        QGCCachedTileSet* set = qobject_cast<QGCCachedTileSet*>(_tileSets.get(i));
        if(set) {
            set->setSelected(true);
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::selectNone() {
    for(int i = 0; i < _tileSets.count(); i++ ) {
        QGCCachedTileSet* set = qobject_cast<QGCCachedTileSet*>(_tileSets.get(i));
        if(set) {
            set->setSelected(false);
        }
    }
}

//-----------------------------------------------------------------------------
int
QGCMapEngineManager::selectedCount() {
    int count = 0;
    for(int i = 0; i < _tileSets.count(); i++ ) {
        QGCCachedTileSet* set = qobject_cast<QGCCachedTileSet*>(_tileSets.get(i));
        if(set && set->selected()) {
            count++;
        }
    }
    return count;
}

//-----------------------------------------------------------------------------
bool
QGCMapEngineManager::importSets(QString path) {
    _importAction = ActionNone;
    emit importActionChanged();
    QString dir = path;
    if(dir.isEmpty()) {
#if defined(__mobile__)
        //-- TODO: This has to be something fixed
        dir = QDir(QDir::homePath()).filePath(QString("export_%1.db").arg(QDateTime::currentDateTime().toTime_t()));
#else
        dir = QString(); //-- TODO: QGCQFileDialog::getOpenFileName(
        //    nullptr,
        //    "Import Tile Set",
        //    QDir::homePath(),
        //    "Tile Sets (*.qgctiledb)");
#endif
    }
    if(!dir.isEmpty()) {
        _importAction = ActionImporting;
        emit importActionChanged();
        QGCImportTileTask* task = new QGCImportTileTask(dir, _importReplace);
        connect(task, &QGCImportTileTask::actionCompleted, this, &QGCMapEngineManager::_actionCompleted);
        connect(task, &QGCImportTileTask::actionProgress, this, &QGCMapEngineManager::_actionProgressHandler);
        connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        getQGCMapEngine()->addTask(task);
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCMapEngineManager::exportSets(QString path) {
    _importAction = ActionNone;
    emit importActionChanged();
    QString dir = path;
    if(dir.isEmpty()) {
#if defined(__mobile__)
        dir = QDir(QDir::homePath()).filePath(QString("export_%1.db").arg(QDateTime::currentDateTime().toTime_t()));
#else
        dir = QString(); //-- TODO: QGCQFileDialog::getSaveFileName(
        //    MainWindow::instance(),
        //    "Export Tile Set",
        //    QDir::homePath(),
        //    "Tile Sets (*.qgctiledb)",
        //    "qgctiledb",
        //    true);
#endif
    }
    if(!dir.isEmpty()) {
        QVector<QGCCachedTileSet*> sets;
        for(int i = 0; i < _tileSets.count(); i++ ) {
            QGCCachedTileSet* set = qobject_cast<QGCCachedTileSet*>(_tileSets.get(i));
            if(set->selected()) {
                sets.append(set);
            }
        }
        if(sets.count()) {
            _importAction = ActionExporting;
            emit importActionChanged();
            QGCExportTileTask* task = new QGCExportTileTask(sets, dir);
            connect(task, &QGCExportTileTask::actionCompleted, this, &QGCMapEngineManager::_actionCompleted);
            connect(task, &QGCExportTileTask::actionProgress, this, &QGCMapEngineManager::_actionProgressHandler);
            connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
            getQGCMapEngine()->addTask(task);
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::_actionProgressHandler(int percentage)
{
    _actionProgress = percentage;
    emit actionProgressChanged();
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::_actionCompleted()
{
    ImportAction oldState = _importAction;
    _importAction = ActionDone;
    emit importActionChanged();
    //-- If we just imported, reload it all
    if(oldState == ActionImporting) {
        loadTileSets();
    }
}

//-----------------------------------------------------------------------------
void
QGCMapEngineManager::resetAction()
{
    _importAction = ActionNone;
    emit importActionChanged();
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
        name += QString::asprintf("%03d", count++);
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
        quint32 total = static_cast<quint32>(info.bytesTotal() / 1024);
        quint32 free  = static_cast<quint32>(info.bytesFree()  / 1024);
        qCDebug(QGCMapEngineManagerLog) << info.rootPath() << "has" << free << "Mbytes available.";
        if(_freeDiskSpace != free) {
            _freeDiskSpace = free;
            _diskSpace = total;
            emit freeDiskSpaceChanged();
        }
    }
}
