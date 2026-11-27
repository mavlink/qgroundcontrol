#include "MapEngineManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QHash>

#include "CacheTasks.h"
#include "CachedTileSet.h"
#include "ElevationMapProvider.h"
#include "MapProviderRegistry.h"
#include "MapTilesSettings.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "TileCache.h"
#include "TileSetTasks.h"

QGC_LOGGING_CATEGORY(MapEngineManagerLog, "MapTiles.MapEngineManager")

Q_APPLICATION_STATIC(MapEngineManager, _mapEngineManager);

namespace {

/// Whether \a listed and \a fetched tell of one set. Their id alone does not: a cache that replaced the one of the
/// listed set gives the ids again, and the UI reads what a set covers once.
bool describesSameTileSet(const TileSetRecord& listed, const TileSetRecord& fetched)
{
    return (listed.setID == fetched.setID) && (listed.defaultSet == fetched.defaultSet) &&
           (listed.mapId == fetched.mapId) && (listed.mapName == fetched.mapName) &&
           (listed.minZoom == fetched.minZoom) && (listed.maxZoom == fetched.maxZoom) &&
           (listed.topleftLat == fetched.topleftLat) && (listed.topleftLon == fetched.topleftLon) &&
           (listed.bottomRightLat == fetched.bottomRightLat) && (listed.bottomRightLon == fetched.bottomRightLon);
}

}  // namespace

MapEngineManager* MapEngineManager::instance()
{
    return _mapEngineManager();
}

MapEngineManager::MapEngineManager(QObject* parent)
    : QObject(parent)
    , _tileSets(new QmlObjectListModel(this))
{
    qCDebug(MapEngineManagerLog) << this;

    (void) connect(TileCache::instance(), &TileCache::updateTotals, this, &MapEngineManager::_updateTotals,
                   Qt::UniqueConnection);
}

MapEngineManager::~MapEngineManager()
{
    qCDebug(MapEngineManagerLog) << this;
}

void MapEngineManager::updateForCurrentView(double lon0, double lat0, double lon1, double lat1, int minZoom,
                                            int maxZoom, const QString& mapName)
{
    _plan = TileSetPlan::forView(lon0, lat0, lon1, lat1, minZoom, maxZoom, mapName, _fetchElevation);

    emit tileCountChanged();
    emit tileSizeChanged();

    qCDebug(MapEngineManagerLog) << lat0 << lon0 << lat1 << lon1 << minZoom << maxZoom;
}

QString MapEngineManager::tileCountStr() const
{
    return QGC::numberToString(tileCount());
}

QString MapEngineManager::tileSizeStr() const
{
    return QGC::bigSizeToString(tileSize());
}

void MapEngineManager::loadTileSets()
{
    CacheTask* const task = TileSetTasks::fetchTileSets(
        this, [this](const QList<CachedTileSet*>& tileSets) { _tileSetsFetched(tileSets); });
    _reportErrorOf(task);
    (void) TileCache::instance()->addTask(task);
}

void MapEngineManager::_tileSetsFetched(const QList<CachedTileSet*>& fetched)
{
    QHash<quint64, CachedTileSet*> listedSets;
    for (QObject* const object : std::as_const(*_tileSets->objectList())) {
        CachedTileSet* const listed = qobject_cast<CachedTileSet*>(object);
        if (listed) {
            (void) listedSets.insert(listed->id(), listed);
        }
    }

    // In the order of the cache, which has the default set first
    QObjectList tileSets;
    for (CachedTileSet* const tileSet : fetched) {
        CachedTileSet* const listed = listedSets.value(tileSet->id());
        if (listed && describesSameTileSet(listed->record(), tileSet->record())) {
            listed->setName(tileSet->name());
            // A download counts the tiles it fetches, which the cache has only once it stored them
            if (!listed->downloading()) {
                listed->setTotals(tileSet->totals());
            }
            tileSets.append(listed);
            delete tileSet;
        } else {
            _adoptTileSet(tileSet);
            tileSets.append(tileSet);
        }
    }

    if (tileSets == *_tileSets->objectList()) {
        return;
    }

    const QObjectList replaced = _tileSets->swapObjectList(tileSets);
    for (QObject* const object : replaced) {
        if (!tileSets.contains(object)) {
            delete object;
        }
    }
    emit tileSetsChanged();
}

void MapEngineManager::_adoptTileSet(CachedTileSet* tileSet)
{
    tileSet->setParent(this);
    (void) connect(tileSet, &CachedTileSet::selectedChanged, this, &MapEngineManager::selectedCountChanged);
    (void) connect(tileSet, &CachedTileSet::cacheTaskQueued, this, &MapEngineManager::_reportErrorOf);
}

void MapEngineManager::_removeTileSets()
{
    if (_tileSets->count() == 0) {
        return;
    }

    qDeleteAll(_tileSets->swapObjectList({}));
    emit tileSetsChanged();
}

void MapEngineManager::startDownload(const QString& name, const QString& mapType)
{
    if (tileCount() == 0) {
        qCWarning(MapEngineManagerLog) << "No Tiles to save";
        return;
    }

    if (_plan.map.tiles.tileCount > 0) {
        _createTileSet(name, mapType, _plan.map);
    }
    if (_plan.elevation.tiles.tileCount > 0) {
        _createTileSet(name + QStringLiteral(" Elevation"), MapTilesSettings::elevationMapProvider(), _plan.elevation);
    }
    if (_plan.terrain.tiles.tileCount > 0) {
        _createTileSet(name + QStringLiteral(" Terrain"), QString::fromLatin1(TerrariumElevationProvider::kProviderKey),
                       _plan.terrain);
    }
}

void MapEngineManager::_createTileSet(const QString& name, const QString& mapType, const TileSetPlan::Set& planned)
{
    CachedTileSet* const set = new CachedTileSet(TileSetRecord{
        .name = name,
        .mapName = mapType,
        .topleftLat = _plan.topleftLat,
        .topleftLon = _plan.topleftLon,
        .bottomRightLat = _plan.bottomRightLat,
        .bottomRightLon = _plan.bottomRightLon,
        .minZoom = planned.minZoom,
        .maxZoom = planned.maxZoom,
        .mapId = MapProviderRegistry::getMapIdFromProviderType(mapType),
        .numTiles = static_cast<quint32>(planned.tiles.tileCount),
    });
    TileSetTotals totals = set->totals();
    totals.totalTileSize = planned.tiles.tileSize;
    set->setTotals(totals);

    CacheTask* const task =
        TileSetTasks::createTileSet(set, this, [this](CachedTileSet* saved) { _tileSetSaved(saved); });
    _reportErrorOf(task);
    (void) TileCache::instance()->addTask(task);
}

void MapEngineManager::_tileSetSaved(CachedTileSet* set)
{
    qCDebug(MapEngineManagerLog) << "New tile set saved (" << set->name() << "). Starting download...";

    _adoptTileSet(set);
    _tileSets->append(set);
    emit tileSetsChanged();
    set->createDownloadTask();
}

void MapEngineManager::deleteTileSet(CachedTileSet* tileSet)
{
    qCDebug(MapEngineManagerLog) << "Deleting tile set" << tileSet->name();

    if (tileSet->defaultSet()) {
        for (QObject* const object : std::as_const(*_tileSets->objectList())) {
            if (CachedTileSet* const set = qobject_cast<CachedTileSet*>(object)) {
                set->cancelDownloadTask();
            }
        }
        _setDeleting(std::nullopt, true);

        CacheTask* const task = CacheTasks::resetCache();
        (void) connect(task, &CacheTask::finished, this, [this, name = task->name()](const QString& error) {
            if (error.isEmpty()) {
                // The cache has none of the listed sets left, and gives their ids again
                _removeTileSets();
                loadTileSets();
            } else {
                _taskFailed(name, error);
                _setDeleting(std::nullopt, false);
            }
        });
        (void) TileCache::instance()->addTask(task);
    } else {
        tileSet->cancelDownloadTask();
        tileSet->setDeleting(true);

        const quint64 setID = tileSet->id();
        CacheTask* const task = TileSetTasks::deleteTileSet(setID);
        (void) connect(task, &CacheTask::finished, this, [this, setID, name = task->name()](const QString& error) {
            if (error.isEmpty()) {
                _tileSetDeleted(setID);
            } else {
                _taskFailed(name, error);
                _setDeleting(setID, false);
            }
        });
        (void) TileCache::instance()->addTask(task);
    }
}

void MapEngineManager::clearCachedTiles()
{
    qCDebug(MapEngineManagerLog) << "Deleting the cached tiles";

    // The tiles are of the default set, which shows that it is busy
    std::optional<quint64> defaultSetID;
    for (QObject* const object : std::as_const(*_tileSets->objectList())) {
        const CachedTileSet* const set = qobject_cast<const CachedTileSet*>(object);
        if (set && set->defaultSet()) {
            defaultSetID = set->id();
        }
    }
    if (defaultSetID) {
        _setDeleting(defaultSetID, true);
    }

    CacheTask* const task = TileSetTasks::deleteCachedTiles();
    (void) connect(task, &CacheTask::finished, this, [this, defaultSetID, name = task->name()](const QString& error) {
        if (!error.isEmpty()) {
            _taskFailed(name, error);
        }
        if (defaultSetID) {
            _setDeleting(defaultSetID, false);
        }
    });
    if (!TileCache::instance()->addTask(task) && defaultSetID) {
        // A task that the cache dropped has no end
        _setDeleting(defaultSetID, false);
    }
}

void MapEngineManager::_setDeleting(std::optional<quint64> setID, bool deleting)
{
    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        CachedTileSet* const set = qobject_cast<CachedTileSet*>(_tileSets->get(i));
        if (set && (!setID || (set->id() == *setID))) {
            set->setDeleting(deleting);
        }
    }
}

void MapEngineManager::renameTileSet(CachedTileSet* tileSet, const QString& newName)
{
    int idx = 1;
    QString name = newName;
    while (findName(name)) {
        name = QString("%1 (%2)").arg(newName).arg(idx++);
    }

    qCDebug(MapEngineManagerLog) << "Renaming tile set" << tileSet->name() << "to" << name;
    tileSet->setName(name);

    CacheTask* const task = TileSetTasks::renameTileSet(tileSet->id(), name);
    _reportErrorOf(task);
    (void) TileCache::instance()->addTask(task);
}

void MapEngineManager::_tileSetDeleted(quint64 setID)
{
    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        CachedTileSet* set = qobject_cast<CachedTileSet*>(_tileSets->get(i));
        if (set && (set->id() == setID)) {
            (void) _tileSets->removeAt(i);
            delete set;
            emit tileSetsChanged();
            break;
        }
    }
}

void MapEngineManager::_reportErrorOf(CacheTask* task)
{
    (void) connect(task, &CacheTask::finished, this, [this, name = task->name()](const QString& error) {
        if (!error.isEmpty()) {
            _taskFailed(name, error);
        }
    });
}

void MapEngineManager::_taskFailed(const QString& taskName, const QString& error)
{
    const QString serror = tr("Error in task: %1\nError description:\n%2").arg(taskName, error);

    setErrorMessage(serror);

    qCWarning(MapEngineManagerLog) << serror;
}

void MapEngineManager::_updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize)
{
    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        CachedTileSet* const set = qobject_cast<CachedTileSet*>(_tileSets->get(i));
        if (set && set->defaultSet()) {
            TileSetTotals totals = set->totals();
            totals.savedTileSize = totalsize;
            totals.savedTileCount = totaltiles;
            totals.totalTileCount = defaulttiles;
            totals.totalTileSize = defaultsize;
            set->setTotals(totals);
            return;
        }
    }
}

bool MapEngineManager::findName(const QString& name) const
{
    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        const CachedTileSet* const set = qobject_cast<const CachedTileSet*>(_tileSets->get(i));
        if (set && (set->name() == name)) {
            return true;
        }
    }

    return false;
}

void MapEngineManager::selectAll()
{
    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        CachedTileSet* const set = qobject_cast<CachedTileSet*>(_tileSets->get(i));
        if (set) {
            set->setSelected(true);
        }
    }
}

int MapEngineManager::selectedCount() const
{
    int count = 0;

    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        const CachedTileSet* const set = qobject_cast<const CachedTileSet*>(_tileSets->get(i));
        if (set && set->selected()) {
            count++;
        }
    }

    return count;
}

bool MapEngineManager::importSets(const QString& path)
{
    setImportAction(ImportAction::ActionNone);

    if (path.isEmpty()) {
        return false;
    }

    setImportAction(ImportAction::ActionImporting);

    return _startAction(TileSetTasks::importSets(path, _importReplace));
}

bool MapEngineManager::exportSets(const QString& path)
{
    setImportAction(ImportAction::ActionNone);

    if (path.isEmpty()) {
        return false;
    }

    QList<TileSetRecord> records;

    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        CachedTileSet* const set = qobject_cast<CachedTileSet*>(_tileSets->get(i));
        if (set && set->selected()) {
            records.append(set->record());
        }
    }

    if (records.isEmpty()) {
        return false;
    }

    setImportAction(ImportAction::ActionExporting);

    return _startAction(TileSetTasks::exportSets(records, path));
}

bool MapEngineManager::_startAction(CacheTask* task)
{
    const ImportAction action = _importAction;
    (void) connect(task, &CacheTask::progress, this, &MapEngineManager::setActionProgress);
    (void) connect(task, &CacheTask::finished, this,
                   [this, name = task->name()](const QString& error) { _actionFinished(name, error); });
    if (TileCache::instance()->addTask(task)) {
        return true;
    }

    // A task that the cache dropped has no outcome
    if (_importAction == action) {
        setImportAction(ImportAction::ActionNone);
    }
    return false;
}

void MapEngineManager::_actionFinished(const QString& taskName, const QString& error)
{
    if (!error.isEmpty()) {
        _taskFailed(taskName, error);
    }

    // Also after an error: the action is over, and an import that failed can have added sets before it did
    const ImportAction oldState = _importAction;
    setImportAction(ImportAction::ActionDone);

    if (oldState == ImportAction::ActionImporting) {
        loadTileSets();
    }
}

QString MapEngineManager::getUniqueName() const
{
    int count = 1;
    while (true) {
        const QString name = QStringLiteral("Tile Set ") + QString::asprintf("%03d", count++);
        if (!findName(name)) {
            return name;
        }
    }
}
