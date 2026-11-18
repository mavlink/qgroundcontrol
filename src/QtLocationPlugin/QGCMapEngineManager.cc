#include "QGCMapEngineManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QTemporaryDir>
#include <QtQml/QQmlEngine>

#include "ElevationMapProvider.h"
#include "FlightMapSettings.h"
#include "QGCCachedTileSet.h"
#include "QGCCompression.h"
#include "QGCCompressionJob.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "UrlFactory.h"

QGC_LOGGING_CATEGORY(QGCMapEngineManagerLog, "MapTiles.QGCMapEngineManager")

Q_APPLICATION_STATIC(QGCMapEngineManager, _mapEngineManager);

QGCMapEngineManager* QGCMapEngineManager::instance()
{
    return _mapEngineManager();
}

QGCMapEngineManager::QGCMapEngineManager(QObject* parent)
    : QObject(parent)
    , _tileSets(new QmlObjectListModel(this))
{
    qCDebug(QGCMapEngineManagerLog) << this;

    (void) qmlRegisterUncreatableType<QGCMapEngineManager>("QGroundControl.QGCMapEngineManager", 1, 0,
                                                           "QGCMapEngineManager", "Reference only");

    (void) connect(QGCMapEngine::instance(), &QGCMapEngine::updateTotals, this, &QGCMapEngineManager::_updateTotals,
                   Qt::UniqueConnection);
}

QGCMapEngineManager::~QGCMapEngineManager()
{
    _tileSets->clear();

    qCDebug(QGCMapEngineManagerLog) << this;
}

void QGCMapEngineManager::updateForCurrentView(double lon0, double lat0, double lon1, double lat1, int minZoom,
                                               int maxZoom, const QString& mapName)
{
    _plan = QGCTileSetPlan::forView(lon0, lat0, lon1, lat1, minZoom, maxZoom, mapName, _fetchElevation);

    emit tileCountChanged();
    emit tileSizeChanged();

    qCDebug(QGCMapEngineManagerLog) << lat0 << lon0 << lat1 << lon1 << minZoom << maxZoom;
}

QString QGCMapEngineManager::tileCountStr() const
{
    return QGC::numberToString(tileCount());
}

QString QGCMapEngineManager::tileSizeStr() const
{
    return QGC::bigSizeToString(tileSize());
}

void QGCMapEngineManager::loadTileSets()
{
    if (_tileSets->count() > 0) {
        _tileSets->clear();
        emit tileSetsChanged();
    }

    QGCMapTask* const task = QGCMapTasks::fetchTileSets(this, [this](const QList<QGCCachedTileSet*>& tileSets) {
        for (QGCCachedTileSet* const tileSet : tileSets) {
            _tileSetFetched(tileSet);
        }
    });
    reportErrorOf(task);
    (void) QGCMapEngine::instance()->addTask(task);
}

void QGCMapEngineManager::_tileSetFetched(QGCCachedTileSet* tileSet)
{
    if (tileSet->type() == QStringLiteral("Invalid")) {
        tileSet->setMapTypeStr(QStringLiteral("Various"));
    }

    tileSet->setManager(this);
    _tileSets->append(tileSet);
    emit tileSetsChanged();
}

void QGCMapEngineManager::startDownload(const QString& name, const QString& mapType)
{
    if (tileCount() == 0) {
        qCWarning(QGCMapEngineManagerLog) << "No Tiles to save";
        return;
    }

    if (_plan.map.tiles.tileCount > 0) {
        _createTileSet(name, mapType, _plan.map);
    }
    if (_plan.elevation.tiles.tileCount > 0) {
        const QString elevationProviderName =
            SettingsManager::instance()->flightMapSettings()->elevationMapProvider()->rawValue().toString();
        _createTileSet(name + QStringLiteral(" Elevation"), elevationProviderName, _plan.elevation);
    }
    if (_plan.terrain.tiles.tileCount > 0) {
        _createTileSet(name + QStringLiteral(" Terrain"), QString::fromLatin1(TerrariumElevationProvider::kProviderKey),
                       _plan.terrain);
    }
}

void QGCMapEngineManager::_createTileSet(const QString& name, const QString& mapType,
                                         const QGCTileSetPlan::Set& planned)
{
    QGCCachedTileSet* const set = new QGCCachedTileSet(name);
    set->setMapTypeStr(mapType);
    set->setTopleftLat(_plan.topleftLat);
    set->setTopleftLon(_plan.topleftLon);
    set->setBottomRightLat(_plan.bottomRightLat);
    set->setBottomRightLon(_plan.bottomRightLon);
    set->setMinZoom(planned.minZoom);
    set->setMaxZoom(planned.maxZoom);
    set->setTotalTileSize(planned.tiles.tileSize);
    set->setTotalTileCount(static_cast<quint32>(planned.tiles.tileCount));
    set->setType(mapType);
    set->setManager(this);

    QGCMapTask* const task =
        QGCMapTasks::createTileSet(set, this, [this](QGCCachedTileSet* saved) { _tileSetSaved(saved); });
    reportErrorOf(task);
    (void) QGCMapEngine::instance()->addTask(task);
}

void QGCMapEngineManager::_tileSetSaved(QGCCachedTileSet* set)
{
    qCDebug(QGCMapEngineManagerLog) << "New tile set saved (" << set->name() << "). Starting download...";

    _tileSets->append(set);
    emit tileSetsChanged();
    set->createDownloadTask();
}

void QGCMapEngineManager::deleteTileSet(QGCCachedTileSet* tileSet)
{
    qCDebug(QGCMapEngineManagerLog) << "Deleting tile set" << tileSet->name();

    if (tileSet->defaultSet()) {
        _setDeleting(std::nullopt, true);

        QGCMapTask* const task = QGCMapTasks::resetCache();
        (void) connect(task, &QGCMapTask::finished, this, [this, name = task->name()](const QString& error) {
            if (error.isEmpty()) {
                loadTileSets();
            } else {
                _taskFailed(name, error);
                _setDeleting(std::nullopt, false);
            }
        });
        (void) QGCMapEngine::instance()->addTask(task);
    } else {
        tileSet->setDeleting(true);

        const quint64 setID = tileSet->id();
        QGCMapTask* const task = QGCMapTasks::deleteTileSet(setID);
        (void) connect(task, &QGCMapTask::finished, this, [this, setID, name = task->name()](const QString& error) {
            if (error.isEmpty()) {
                _tileSetDeleted(setID);
            } else {
                _taskFailed(name, error);
                _setDeleting(setID, false);
            }
        });
        (void) QGCMapEngine::instance()->addTask(task);
    }
}

void QGCMapEngineManager::_setDeleting(std::optional<quint64> setID, bool deleting)
{
    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
        if (set && (!setID || (set->id() == *setID))) {
            set->setDeleting(deleting);
        }
    }
}

void QGCMapEngineManager::renameTileSet(QGCCachedTileSet* tileSet, const QString& newName)
{
    int idx = 1;
    QString name = newName;
    while (findName(name)) {
        name = QString("%1 (%2)").arg(newName).arg(idx++);
    }

    qCDebug(QGCMapEngineManagerLog) << "Renaming tile set" << tileSet->name() << "to" << name;
    tileSet->setName(name);
    emit tileSet->nameChanged();

    QGCMapTask* const task = QGCMapTasks::renameTileSet(tileSet->id(), name);
    reportErrorOf(task);
    (void) QGCMapEngine::instance()->addTask(task);
}

void QGCMapEngineManager::_tileSetDeleted(quint64 setID)
{
    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        QGCCachedTileSet* set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
        if (set && (set->id() == setID)) {
            (void) _tileSets->removeAt(i);
            delete set;
            emit tileSetsChanged();
            break;
        }
    }
}

void QGCMapEngineManager::reportErrorOf(QGCMapTask* task)
{
    (void) connect(task, &QGCMapTask::finished, this, [this, name = task->name()](const QString& error) {
        if (!error.isEmpty()) {
            _taskFailed(name, error);
        }
    });
}

void QGCMapEngineManager::_taskFailed(const QString& taskName, const QString& error)
{
    QString serror = QStringLiteral("Error in task: ") + taskName;
    serror += QStringLiteral("\nError description:\n");
    serror += error;

    setErrorMessage(serror);

    qCWarning(QGCMapEngineManagerLog) << serror;
}

void QGCMapEngineManager::_updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles,
                                        quint64 defaultsize)
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

bool QGCMapEngineManager::findName(const QString& name) const
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

bool QGCMapEngineManager::importSets(const QString& path)
{
    setImportAction(ImportAction::ActionNone);

    if (path.isEmpty()) {
        return false;
    }

    setImportAction(ImportAction::ActionImporting);

    return _startAction(QGCMapTasks::importSets(path, _importReplace));
}

bool QGCMapEngineManager::exportSets(const QString& path)
{
    setImportAction(ImportAction::ActionNone);

    if (path.isEmpty()) {
        return false;
    }

    QList<TileSetRecord> records;

    for (qsizetype i = 0; i < _tileSets->count(); i++) {
        QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
        if (set && set->selected()) {
            TileSetRecord rec;
            rec.setID = set->id();
            rec.name = set->name();
            rec.mapTypeStr = set->mapTypeStr();
            rec.topleftLat = set->topleftLat();
            rec.topleftLon = set->topleftLon();
            rec.bottomRightLat = set->bottomRightLat();
            rec.bottomRightLon = set->bottomRightLon();
            rec.minZoom = set->minZoom();
            rec.maxZoom = set->maxZoom();
            rec.type = UrlFactory::getMapIdFromProviderType(set->type());
            rec.numTiles = set->totalTileCount();
            rec.defaultSet = set->defaultSet();
            rec.date = set->creationDate().toSecsSinceEpoch();
            records.append(rec);
        }
    }

    if (records.isEmpty()) {
        return false;
    }

    setImportAction(ImportAction::ActionExporting);

    return _startAction(QGCMapTasks::exportSets(records, path));
}

bool QGCMapEngineManager::_startAction(QGCMapTask* task)
{
    const ImportAction action = _importAction;
    (void) connect(task, &QGCMapTask::progress, this, &QGCMapEngineManager::setActionProgress);
    (void) connect(task, &QGCMapTask::finished, this,
                   [this, name = task->name()](const QString& error) { _actionFinished(name, error); });
    if (QGCMapEngine::instance()->addTask(task)) {
        return true;
    }

    // A task that the engine dropped has no outcome
    if (_importAction == action) {
        setImportAction(ImportAction::ActionNone);
    }
    return false;
}

void QGCMapEngineManager::_actionFinished(const QString& taskName, const QString& error)
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

QString QGCMapEngineManager::getUniqueName() const
{
    int count = 1;
    while (true) {
        const QString name = QStringLiteral("Tile Set ") + QString::asprintf("%03d", count++);
        if (!findName(name)) {
            return name;
        }
    }
}

bool QGCMapEngineManager::importArchive(const QString& archivePath)
{
    if (archivePath.isEmpty()) {
        setErrorMessage(tr("No archive path specified"));
        return false;
    }

    if (!QFile::exists(archivePath)) {
        setErrorMessage(tr("Archive file not found: %1").arg(archivePath));
        return false;
    }

    if (!QGCCompression::isArchiveFile(archivePath)) {
        setErrorMessage(tr("Not a supported archive format: %1").arg(archivePath));
        return false;
    }

    if (_importAction == ImportAction::ActionImporting) {
        setErrorMessage(tr("Import already in progress"));
        return false;
    }

    const QString tempPath =
        QDir::temp().filePath(QStringLiteral("qgc_tiles_") + QString::number(QDateTime::currentMSecsSinceEpoch()));
    if (!QDir().mkpath(tempPath)) {
        setErrorMessage(tr("Could not create temporary directory"));
        return false;
    }

    _extractionOutputDir = tempPath;

    if (_extractionJob == nullptr) {
        _extractionJob = new QGCCompressionJob(this);
        connect(_extractionJob, &QGCCompressionJob::progressChanged, this,
                &QGCMapEngineManager::_handleExtractionProgress);
        connect(_extractionJob, &QGCCompressionJob::finished, this, &QGCMapEngineManager::_handleExtractionFinished);
    }

    setImportAction(ImportAction::ActionImporting);
    setActionProgress(0);

    _extractionJob->extractArchive(archivePath, tempPath);
    return true;
}

void QGCMapEngineManager::_handleExtractionProgress(qreal progress)
{
    setActionProgress(static_cast<int>(progress * 50.0));
}

void QGCMapEngineManager::_handleExtractionFinished(bool success)
{
    if (!success) {
        const QString error = _extractionJob != nullptr ? _extractionJob->errorString() : tr("Extraction failed");
        setErrorMessage(error);
        setImportAction(ImportAction::ActionDone);
        QDir(_extractionOutputDir).removeRecursively();
        _extractionOutputDir.clear();
        return;
    }

    QString dbPath;
    QDirIterator it(_extractionOutputDir, {QStringLiteral("*.db"), QStringLiteral("*.sqlite")}, QDir::Files,
                    QDirIterator::Subdirectories);
    if (it.hasNext()) {
        dbPath = it.next();
    }

    if (dbPath.isEmpty()) {
        setErrorMessage(tr("No tile database found in archive"));
        setImportAction(ImportAction::ActionDone);
        QDir(_extractionOutputDir).removeRecursively();
        _extractionOutputDir.clear();
        return;
    }

    qCDebug(QGCMapEngineManagerLog) << "Found tile database:" << dbPath;

    QGCMapTask* const task = QGCMapTasks::importSets(dbPath, _importReplace);
    (void) connect(task, &QGCMapTask::progress, this,
                   [this](int percentage) { setActionProgress(50 + (percentage / 2)); });
    (void) connect(task, &QGCMapTask::finished, this, [this, name = task->name()](const QString& error) {
        _actionFinished(name, error);
        QDir(_extractionOutputDir).removeRecursively();
        _extractionOutputDir.clear();
    });
    if (!QGCMapEngine::instance()->addTask(task) && (_importAction == ImportAction::ActionImporting)) {
        // Dropped without an outcome: the engine has no cache worker
        setErrorMessage(tr("Failed to start import task"));
        setImportAction(ImportAction::ActionDone);
        QDir(_extractionOutputDir).removeRecursively();
        _extractionOutputDir.clear();
    }
}
