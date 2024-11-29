/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "QGeoFileTileCacheQGC.h"
#include "ElevationMapProvider.h"
#include "QmlObjectListModel.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "FlightMapSettings.h"
#include "QGCLoggingCategory.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtCore/QStorageInfo>
#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(QGCMapEngineManagerLog, "qgc.qtlocation.qmlcontrol.qgcmapenginemanagerlog")

Q_APPLICATION_STATIC(QGCMapEngineManager, _mapEngineManager);

QGCMapEngineManager *QGCMapEngineManager::instance()
{
    return _mapEngineManager();
}

QGCMapEngineManager::QGCMapEngineManager(QObject *parent)
    : QObject(parent)
    , _tileSets(new QmlObjectListModel(this))
{
    (void) qmlRegisterUncreatableType<QGCMapEngineManager>("QGroundControl.QGCMapEngineManager", 1, 0, "QGCMapEngineManager", "Reference only");

    (void) connect(getQGCMapEngine(), &QGCMapEngine::updateTotals, this, &QGCMapEngineManager::_updateTotals);

    // qCDebug(QGCMapEngineManagerLog) << Q_FUNC_INFO << this;
}

QGCMapEngineManager::~QGCMapEngineManager()
{
    _tileSets->clear();

    // qCDebug(QGCMapEngineManagerLog) << Q_FUNC_INFO << this;
}

void QGCMapEngineManager::updateForCurrentView(double lon0, double lat0, double lon1, double lat1, int minZoom, int maxZoom, const QString &mapName)
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
        const QString elevationProviderName = SettingsManager::instance()->flightMapSettings()->elevationMapProvider()->rawValue().toString();
        const QGCTileSet set = UrlFactory::getTileCount(1, lon0, lat0, lon1, lat1, elevationProviderName);
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
    if (_tileSets->count() > 0) {
        _tileSets->clear();
        emit tileSetsChanged();
    }

    QGCFetchTileSetTask* const task = new QGCFetchTileSetTask(nullptr);
    (void) connect(task, &QGCFetchTileSetTask::tileSetFetched, this, &QGCMapEngineManager::_tileSetFetched);
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    (void) getQGCMapEngine()->addTask(task);
}

void QGCMapEngineManager::_tileSetFetched(QGCCachedTileSet *tileSet)
{
    if (tileSet->type() == QStringLiteral("Invalid")) {
        tileSet->setMapTypeStr(QStringLiteral("Various"));
    }

    tileSet->setManager(this);
    (void) _tileSets->append(tileSet);
    emit tileSetsChanged();
}

void QGCMapEngineManager::startDownload(const QString &name, const QString &mapType)
{
    if (_imageSet.tileSize > 0) {
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
        (void) connect(task, &QGCCreateTileSetTask::tileSetSaved, this, &QGCMapEngineManager::_tileSetSaved);
        (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        (void) getQGCMapEngine()->addTask(task);
    } else {
        qCWarning(QGCMapEngineManagerLog) << Q_FUNC_INFO << "No Tiles to save";
    }

    const int mapid = UrlFactory::getQtMapIdFromProviderType(mapType);
    if (_fetchElevation && !UrlFactory::isElevation(mapid)) {
        QGCCachedTileSet* const set = new QGCCachedTileSet(name + QStringLiteral(" Elevation"));
        const QString elevationProviderName = SettingsManager::instance()->flightMapSettings()->elevationMapProvider()->rawValue().toString();
        set->setMapTypeStr(elevationProviderName);
        set->setTopleftLat(_topleftLat);
        set->setTopleftLon(_topleftLon);
        set->setBottomRightLat(_bottomRightLat);
        set->setBottomRightLon(_bottomRightLon);
        set->setMinZoom(1);
        set->setMaxZoom(1);
        set->setTotalTileSize(_elevationSet.tileSize);
        set->setTotalTileCount(static_cast<quint32>(_elevationSet.tileCount));
        set->setType(elevationProviderName);

        QGCCreateTileSetTask* const task = new QGCCreateTileSetTask(set);
        (void) connect(task, &QGCCreateTileSetTask::tileSetSaved, this, &QGCMapEngineManager::_tileSetSaved);
        (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        (void) getQGCMapEngine()->addTask(task);
    } else {
        qCWarning(QGCMapEngineManagerLog) << Q_FUNC_INFO << "No Tiles to save";
    }
}

void QGCMapEngineManager::_tileSetSaved(QGCCachedTileSet *set)
{
    qCDebug(QGCMapEngineManagerLog) << "New tile set saved (" << set->name() << "). Starting download...";

    (void) _tileSets->append(set);
    emit tileSetsChanged();
    set->createDownloadTask();
}

void QGCMapEngineManager::saveSetting(const QString &key, const QString &value)
{
    QSettings settings;
    settings.beginGroup(kQmlOfflineMapKeyName);
    settings.setValue(key, value);
}

QString QGCMapEngineManager::loadSetting(const QString &key, const QString &defaultValue)
{
    QSettings settings;
    settings.beginGroup(kQmlOfflineMapKeyName);
    return settings.value(key, defaultValue).toString();
}

QStringList QGCMapEngineManager::mapTypeList(const QString &provider)
{
    QStringList mapStringList = mapList();
    mapStringList = mapStringList.filter(QRegularExpression(provider));

    static const QRegularExpression providerType = QRegularExpression(QStringLiteral("^([^\\ ]*) (.*)$"));
    (void) mapStringList.replaceInStrings(providerType,"\\2");
    (void) mapStringList.removeDuplicates();

    return mapStringList;
}

void QGCMapEngineManager::deleteTileSet(QGCCachedTileSet *tileSet)
{
    qCDebug(QGCMapEngineManagerLog) << "Deleting tile set" << tileSet->name();

    if (tileSet->defaultSet()) {
        for (qsizetype i = 0; i < _tileSets->count(); i++ ) {
            QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
            if (set) {
                set->setDeleting(true);
            }
        }

        QGCResetTask* const task = new QGCResetTask(nullptr);
        (void) connect(task, &QGCResetTask::resetCompleted, this, &QGCMapEngineManager::_resetCompleted);
        (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        (void) getQGCMapEngine()->addTask(task);
    } else {
        tileSet->setDeleting(true);

        QGCDeleteTileSetTask* const task = new QGCDeleteTileSetTask(tileSet->id());
        (void) connect(task, &QGCDeleteTileSetTask::tileSetDeleted, this, &QGCMapEngineManager::_tileSetDeleted);
        (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        (void) getQGCMapEngine()->addTask(task);
    }
}

void QGCMapEngineManager::renameTileSet(QGCCachedTileSet *tileSet, const QString &newName)
{
    int idx = 1;
    QString name = newName;
    while (findName(name)) {
        name = QString("%1 (%2)").arg(newName).arg(idx++);
    }

    qCDebug(QGCMapEngineManagerLog) << "Renaming tile set" << tileSet->name() << "to" << name;
    tileSet->setName(name);
    emit tileSet->nameChanged();

    QGCRenameTileSetTask* const task = new QGCRenameTileSetTask(tileSet->id(), name);
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    (void) getQGCMapEngine()->addTask(task);
}

void QGCMapEngineManager::_tileSetDeleted(quint64 setID)
{
    for (qsizetype i = 0; i < _tileSets->count(); i++ ) {
        QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
        if (set && (set->id() == setID)) {
            _tileSets->removeAt(i);
            delete set;
            emit tileSetsChanged();
            return;
        }
    }
}

void QGCMapEngineManager::taskError(QGCMapTask::TaskType type, const QString &error)
{
    QString task;
    switch (type) {
    case QGCMapTask::taskFetchTileSets:
        task = QStringLiteral("Fetch Tile Set");
        break;
    case QGCMapTask::taskCreateTileSet:
        task = QStringLiteral("Create Tile Set");
        break;
    case QGCMapTask::taskGetTileDownloadList:
        task = QStringLiteral("Get Tile Download List");
        break;
    case QGCMapTask::taskUpdateTileDownloadState:
        task = QStringLiteral("Update Tile Download Status");
        break;
    case QGCMapTask::taskDeleteTileSet:
        task = QStringLiteral("Delete Tile Set");
        break;
    case QGCMapTask::taskReset:
        task = QStringLiteral("Reset Tile Sets");
        break;
    case QGCMapTask::taskExport:
        task = QStringLiteral("Export Tile Sets");
        break;
    default:
        task = QStringLiteral("Database Error");
        break;
    }

    QString serror = "Error in task: " + task;
    serror += "\nError description:\n";
    serror += error;

    setErrorMessage(serror);

    qCWarning(QGCMapEngineManagerLog) << serror;
}

void QGCMapEngineManager::_updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize)
{
    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
        if (set && set->defaultSet()) {
            set->setSavedTileSize(totalsize);
            set->setSavedTileCount(totaltiles);
            set->setTotalTileCount(defaulttiles);
            set->setTotalTileSize(defaultsize);
            return;
        }
    }
}

bool QGCMapEngineManager::findName(const QString &name) const
{
    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        const QGCCachedTileSet* const set = qobject_cast<const QGCCachedTileSet*>(_tileSets->get(i));
        if (set && (set->name() == name)) {
            return true;
        }
    }

    return false;
}

void QGCMapEngineManager::selectAll()
{
    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
        if (set) {
            set->setSelected(true);
        }
    }
}

void QGCMapEngineManager::selectNone()
{
    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
        if (set) {
            set->setSelected(false);
        }
    }
}

int QGCMapEngineManager::selectedCount() const
{
    int count = 0;

    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        const QGCCachedTileSet* const set = qobject_cast<const QGCCachedTileSet*>(_tileSets->get(i));
        if (set && set->selected()) {
            count++;
        }
    }

    return count;
}

bool QGCMapEngineManager::importSets(const QString &path)
{
    setImportAction(ActionNone);

    if (path.isEmpty()) {
        return false;
    }

    setImportAction(ActionImporting);

    QGCImportTileTask* const task = new QGCImportTileTask(path, _importReplace);
    (void) connect(task, &QGCImportTileTask::actionCompleted, this, &QGCMapEngineManager::_actionCompleted);
    (void) connect(task, &QGCImportTileTask::actionProgress, this, &QGCMapEngineManager::_actionProgressHandler);
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    (void) getQGCMapEngine()->addTask(task);

    return true;
}

bool QGCMapEngineManager::exportSets(const QString &path)
{
    setImportAction(ActionNone);

    if (path.isEmpty()) {
        return false;
    }

    QVector<QGCCachedTileSet*> sets;

    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
        if (set->selected()) {
            (void) sets.append(set);
        }
    }

    if (sets.isEmpty()) {
        return false;
    }

    setImportAction(ActionExporting);

    QGCExportTileTask* const task = new QGCExportTileTask(sets, path);
    (void) connect(task, &QGCExportTileTask::actionCompleted, this, &QGCMapEngineManager::_actionCompleted);
    (void) connect(task, &QGCExportTileTask::actionProgress, this, &QGCMapEngineManager::_actionProgressHandler);
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    (void) getQGCMapEngine()->addTask(task);

    return true;
}

void QGCMapEngineManager::_actionCompleted()
{
    const ImportAction oldState = _importAction;
    setImportAction(ActionDone);

    if (oldState == ActionImporting) {
        loadTileSets();
    }
}

QString QGCMapEngineManager::getUniqueName() const
{
    int count = 1;
    while (true) {
        const QString name = QStringLiteral("Tile Set ") + QString::asprintf("%03d", count++);
        if (!findName(name)) {
            return name;
        }
    }

    return QStringLiteral("");
}

QStringList QGCMapEngineManager::mapList()
{
    return UrlFactory::getProviderTypes();
}

QStringList QGCMapEngineManager::mapProviderList()
{
    QStringList mapStringList = mapList();
    const QStringList elevationStringList = elevationProviderList();
    for (const QString &elevationProviderName : elevationStringList) {
        (void) mapStringList.removeAll(elevationProviderName);
    }

    static const QRegularExpression providerType = QRegularExpression(QStringLiteral("^([^\\ ]*) (.*)$"));
    (void) mapStringList.replaceInStrings(providerType,"\\1");
    (void) mapStringList.removeDuplicates();

    return mapStringList;
}

QStringList QGCMapEngineManager::elevationProviderList()
{
    return UrlFactory::getElevationProviderTypes();
}
