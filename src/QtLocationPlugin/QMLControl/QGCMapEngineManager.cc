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

#include "QGCMapEngineManager.h"
#include "QGCCachedTileSet.h"
#include "QGCMapUrlEngine.h"
#include "QGCMapEngine.h"
#include "ElevationMapProvider.h"
#include "QmlObjectListModel.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QSettings>
#include <QtCore/QRegularExpression>
#include <QtCore/QStorageInfo>
#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(QGCMapEngineManagerLog, "QGCMapEngineManagerLog")

static const QString kQmlOfflineMapKeyName = QStringLiteral("QGCOfflineMap");
static const QString kElevationMapType = QString(CopernicusElevationProvider::kProviderKey);

QGCMapEngineManager::QGCMapEngineManager(QObject *parent)
    : QObject(parent)
    , _tileSets(new QmlObjectListModel(this))
{
    qCDebug(QGCMapEngineManagerLog) << Q_FUNC_INFO << this;

    (void) connect(getQGCMapEngine(), &QGCMapEngine::updateTotals, this, &QGCMapEngineManager::_updateTotals);

    _updateDiskFreeSpace();
}

QGCMapEngineManager::~QGCMapEngineManager()
{
    _tileSets->clear();

    qCDebug(QGCMapEngineManagerLog) << Q_FUNC_INFO << this;
}

void QGCMapEngineManager::updateForCurrentView(double lon0, double lat0, double lon1, double lat1, int minZoom, int maxZoom, const QString& mapName)
{
    _topleftLat = lat0;
    _topleftLon = lon0;
    _bottomRightLat = lat1;
    _bottomRightLon = lon1;
    _minZoom = minZoom;
    _maxZoom = maxZoom;

    _imageSet.clear();
    _elevationSet.clear();

    for (int z = minZoom; z <= maxZoom; z++) {
        const QGCTileSet set = UrlFactory::getTileCount(z, lon0, lat0, lon1, lat1, mapName);
        _imageSet += set;
    }

    if (_fetchElevation) {
        const QGCTileSet set = UrlFactory::getTileCount(1, lon0, lat0, lon1, lat1, kElevationMapType);
        _elevationSet += set;
    }

    emit tileCountChanged();
    emit tileSizeChanged();

    qCDebug(QGCMapEngineManagerLog) << Q_FUNC_INFO << lat0 << lon0 << lat1 << lon1 << minZoom << maxZoom;
}

QString QGCMapEngineManager::tileCountStr() const
{
    return qgcApp()->numberToString(_imageSet.tileCount + _elevationSet.tileCount);
}

QString QGCMapEngineManager::tileSizeStr() const
{
    return qgcApp()->bigSizeToString(_imageSet.tileSize + _elevationSet.tileSize);
}

void QGCMapEngineManager::loadTileSets()
{
    if (_tileSets->count()) {
        _tileSets->clear();
        emit tileSetsChanged();
    }

    QGCFetchTileSetTask* const task = new QGCFetchTileSetTask();
    connect(task, &QGCFetchTileSetTask::tileSetFetched, this, &QGCMapEngineManager::_tileSetFetched);
    connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    getQGCMapEngine()->addTask(task);
}

void QGCMapEngineManager::_tileSetFetched(QGCCachedTileSet* tileSet)
{
    if (tileSet->type() == "Invalid") {
        tileSet->setMapTypeStr("Various");
    }
    _tileSets->append(tileSet);
    tileSet->setManager(this);
    emit tileSetsChanged();
}

void QGCMapEngineManager::startDownload(const QString& name, const QString& mapType)
{
    if (_imageSet.tileSize) {
        QGCCachedTileSet* const set = new QGCCachedTileSet(name);
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

        QGCCreateTileSetTask* const task = new QGCCreateTileSetTask(set);
        connect(task, &QGCCreateTileSetTask::tileSetSaved, this, &QGCMapEngineManager::_tileSetSaved);
        connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        getQGCMapEngine()->addTask(task);
    } else {
        qCWarning(QGCMapEngineManagerLog) << Q_FUNC_INFO << "No Tiles to save";
    }

    if ((mapType != kElevationMapType) && _fetchElevation) {
        QGCCachedTileSet* set = new QGCCachedTileSet(name + " Elevation");
        set->setMapTypeStr(kElevationMapType);
        set->setTopleftLat(_topleftLat);
        set->setTopleftLon(_topleftLon);
        set->setBottomRightLat(_bottomRightLat);
        set->setBottomRightLon(_bottomRightLon);
        set->setMinZoom(1);
        set->setMaxZoom(1);
        set->setTotalTileSize(_elevationSet.tileSize);
        set->setTotalTileCount(static_cast<quint32>(_elevationSet.tileCount));
        set->setType(kElevationMapType);

        QGCCreateTileSetTask* const task = new QGCCreateTileSetTask(set);
        connect(task, &QGCCreateTileSetTask::tileSetSaved, this, &QGCMapEngineManager::_tileSetSaved);
        connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        getQGCMapEngine()->addTask(task);
    } else {
        qCWarning(QGCMapEngineManagerLog) << Q_FUNC_INFO << "No Tiles to save";
    }
}

void QGCMapEngineManager::_tileSetSaved(QGCCachedTileSet *set)
{
    qCDebug(QGCMapEngineManagerLog) << "New tile set saved (" << set->name() << "). Starting download...";

    _tileSets->append(set);
    emit tileSetsChanged();
    set->createDownloadTask();
}

void QGCMapEngineManager::saveSetting(const QString& key, const QString& value)
{
    QSettings settings;
    settings.beginGroup(kQmlOfflineMapKeyName);
    settings.setValue(key, value);
}

QString QGCMapEngineManager::loadSetting(const QString& key, const QString& defaultValue)
{
    QSettings settings;
    settings.beginGroup(kQmlOfflineMapKeyName);
    return settings.value(key, defaultValue).toString();
}

QStringList QGCMapEngineManager::mapList()
{
    return UrlFactory::getProviderTypes();
}

QStringList QGCMapEngineManager::mapProviderList()
{
    QStringList mapStringList = mapList();
    mapStringList.removeAll(CopernicusElevationProvider::kProviderKey);

    static const QRegularExpression providerType = QRegularExpression("^([^\\ ]*) (.*)$");
    mapStringList.replaceInStrings(providerType,"\\1");
    mapStringList.removeDuplicates();

    return mapStringList;
}

QStringList QGCMapEngineManager::mapTypeList(const QString& provider)
{
    QStringList mapStringList = mapList();
    mapStringList = mapStringList.filter(QRegularExpression(provider));

    static const QRegularExpression providerType = QRegularExpression("^([^\\ ]*) (.*)$");
    mapStringList.replaceInStrings(providerType,"\\2");
    mapStringList.removeDuplicates();

    return mapStringList;
}

void QGCMapEngineManager::deleteTileSet(QGCCachedTileSet* tileSet)
{
    qCDebug(QGCMapEngineManagerLog) << "Deleting tile set " << tileSet->name();

    if (tileSet->defaultSet()) {
        for (size_t i = 0; i < _tileSets->count(); i++ ) {
            QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
            if (set) {
                set->setDeleting(true);
            }
        }

        QGCResetTask* const task = new QGCResetTask();
        connect(task, &QGCResetTask::resetCompleted, this, &QGCMapEngineManager::_resetCompleted);
        connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        getQGCMapEngine()->addTask(task);
    } else {
        tileSet->setDeleting(true);

        QGCDeleteTileSetTask* const task = new QGCDeleteTileSetTask(tileSet->setID());
        connect(task, &QGCDeleteTileSetTask::tileSetDeleted, this, &QGCMapEngineManager::_tileSetDeleted);
        connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        getQGCMapEngine()->addTask(task);
    }
}

void QGCMapEngineManager::renameTileSet(QGCCachedTileSet* tileSet, const QString& newName)
{
    int idx = 1;
    QString name = newName;
    while (findName(name)) {
        name = QString("%1 (%2)").arg(newName).arg(idx++);
    }
    qCDebug(QGCMapEngineManagerLog) << "Renaming tile set " << tileSet->name() << "to" << name;
    tileSet->setName(name);
    emit tileSet->nameChanged();

    QGCRenameTileSetTask* const task = new QGCRenameTileSetTask(tileSet->setID(), name);
    connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    getQGCMapEngine()->addTask(task);
}

void QGCMapEngineManager::_resetCompleted()
{
    loadTileSets();
}

void QGCMapEngineManager::_tileSetDeleted(quint64 setID)
{
    QGCCachedTileSet* setToDelete = nullptr;

    size_t i;
    for (i = 0; i < _tileSets->count(); i++ ) {
        QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
        if (set && (set->setID() == setID)) {
            setToDelete = set;
            break;
        }
    }

    if (setToDelete) {
        _tileSets->removeAt(i);
        delete setToDelete;
        emit tileSetsChanged();
    }
}

void QGCMapEngineManager::taskError(QGCMapTask::TaskType type, const QString& error)
{
    qCWarning(QGCMapEngineManagerLog) << Q_FUNC_INFO;

    QString task;
    switch (type) {
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

    setErrorMessage(serror);
}

void QGCMapEngineManager::_updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize)
{
    for (size_t i = 0; i < _tileSets->count(); i++) {
        QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
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

bool QGCMapEngineManager::findName(const QString &name) const
{
    for (size_t i = 0; i < _tileSets->count(); i++) {
        const QGCCachedTileSet* const set = qobject_cast<const QGCCachedTileSet*>(_tileSets->getConst(i));
        if (set && (set->name() == name)) {
            return true;
        }
    }
    return false;
}

void QGCMapEngineManager::selectAll()
{
    for (size_t i = 0; i < _tileSets->count(); i++) {
        QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
        if (set) {
            set->setSelected(true);
        }
    }
}

void QGCMapEngineManager::selectNone()
{
    for (size_t i = 0; i < _tileSets->count(); i++) {
        QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
        if (set) {
            set->setSelected(false);
        }
    }
}

int QGCMapEngineManager::selectedCount() const
{
    int count = 0;

    for (size_t i = 0; i < _tileSets->count(); i++) {
        const QGCCachedTileSet* const set = qobject_cast<const QGCCachedTileSet*>(_tileSets->getConst(i));
        if (set && set->selected()) {
            count++;
        }
    }

    return count;
}

bool QGCMapEngineManager::importSets(const QString &path) {
    _importAction = ActionNone;
    emit importActionChanged();

    QString dir = path;
    if (dir.isEmpty()) {
#if defined(__mobile__)
        //-- TODO: This has to be something fixed
        dir = QDir(QDir::homePath()).filePath(QString("export_%1.db").arg(QDateTime::currentDateTime().toSecsSinceEpoch()));
#else
        dir = QString(); //-- TODO: QGCQFileDialog::getOpenFileName(
        //    nullptr,
        //    "Import Tile Set",
        //    QDir::homePath(),
        //    "Tile Sets (*.qgctiledb)");
#endif
    }

    if (dir.isEmpty()) {
        return false;
    }

    _importAction = ActionImporting;
    emit importActionChanged();

    QGCImportTileTask* const task = new QGCImportTileTask(dir, _importReplace);
    connect(task, &QGCImportTileTask::actionCompleted, this, &QGCMapEngineManager::_actionCompleted);
    connect(task, &QGCImportTileTask::actionProgress, this, &QGCMapEngineManager::_actionProgressHandler);
    connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    getQGCMapEngine()->addTask(task);

    return true;
}

bool QGCMapEngineManager::exportSets(const QString &path)
{
    _importAction = ActionNone;
    emit importActionChanged();

    QString dir = path;
    if (dir.isEmpty()) {
#if defined(__mobile__)
        dir = QDir(QDir::homePath()).filePath(QString("export_%1.db").arg(QDateTime::currentDateTime().toSecsSinceEpoch()));
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

    if (!dir.isEmpty()) {
        QVector<QGCCachedTileSet*> sets;

        for (size_t i = 0; i < _tileSets->count(); i++) {
            QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
            if (set->selected()) {
                sets.append(set);
            }
        }

        if (sets.count()) {
            _importAction = ActionExporting;
            emit importActionChanged();

            QGCExportTileTask* const task = new QGCExportTileTask(sets, dir);
            connect(task, &QGCExportTileTask::actionCompleted, this, &QGCMapEngineManager::_actionCompleted);
            connect(task, &QGCExportTileTask::actionProgress, this, &QGCMapEngineManager::_actionProgressHandler);
            connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
            getQGCMapEngine()->addTask(task);

            return true;
        }
    }

    return false;
}

void QGCMapEngineManager::_actionProgressHandler(int percentage)
{
    _actionProgress = percentage;
    emit actionProgressChanged();
}

void QGCMapEngineManager::_actionCompleted()
{
    const ImportAction oldState = _importAction;
    _importAction = ActionDone;
    emit importActionChanged();

    if (oldState == ActionImporting) {
        loadTileSets();
    }
}

void QGCMapEngineManager::resetAction()
{
    _importAction = ActionNone;
    emit importActionChanged();
}

QString QGCMapEngineManager::getUniqueName() const
{
    int count = 1;
    while (true) {
        const QString name = "Tile Set " + QString::asprintf("%03d", count++);
        if(!findName(name)) {
            return name;
        }
    }

    return QString("");
}

void QGCMapEngineManager::_updateDiskFreeSpace()
{
    const QString path = getQGCMapEngine()->getCachePath();
    if (path.isEmpty()) {
        return;
    }

    const QStorageInfo info(path);
    const quint32 total = static_cast<quint32>(info.bytesTotal() / 1024);
    const quint32 free = static_cast<quint32>(info.bytesFree() / 1024);
    if (_freeDiskSpace != free) {
        _freeDiskSpace = free;
        _diskSpace = total;
        emit freeDiskSpaceChanged();
    }
    qCDebug(QGCMapEngineManagerLog) << info.rootPath() << "has" << free << "Mbytes available.";
}
