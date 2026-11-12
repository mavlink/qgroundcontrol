#include "QGCMapEngineManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtCore/QStorageInfo>
#include <QtCore/QTemporaryDir>
#include <QtQml/QQmlEngine>

#include "ElevationMapProvider.h"
#include "FlightMapSettings.h"
#include "FlyViewSettings.h"
#include "PlanViewSettings.h"
#include "QGCCachedTileSet.h"
#include "QGCCompression.h"
#include "QGCCompressionJob.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"

using namespace Qt::StringLiterals;

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

    (void) connect(getQGCMapEngine(), &QGCMapEngine::updateTotals, this, &QGCMapEngineManager::_updateTotals,
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
    _topleftLat = lat0;
    _topleftLon = lon0;
    _bottomRightLat = lat1;
    _bottomRightLon = lon1;
    _minZoom = minZoom;
    _maxZoom = maxZoom;

    _imageSet.clear();
    _elevationSet.clear();
    _terrainSet.clear();

    for (int z = minZoom; z <= maxZoom; z++) {
        const QGCTileSet set = UrlFactory::getTileCount(z, lon0, lat0, lon1, lat1, mapName);
        _imageSet += set;
    }

    if (_fetchElevation) {
        const QString elevationProviderName =
            SettingsManager::instance()->flightMapSettings()->elevationMapProvider()->rawValue().toString();
        const QGCTileSet set = UrlFactory::getTileCount(1, lon0, lat0, lon1, lat1, elevationProviderName);
        _elevationSet += set;

        const SettingsManager* const settings = SettingsManager::instance();
        if (settings->flyViewSettings()->useGeoMapEngine()->rawValue().toBool() ||
            settings->planViewSettings()->useGeoMapEngine()->rawValue().toBool()) {
            // GeoMap fetches the Terrarium tile of every imagery tile, at most as fine as Terrarium serves, and the
            // anchor zoom tiles around every tile finer than those
            _terrainMaxZoom = qMin(maxZoom, TerrariumElevationProvider::kMaxZoom);
            _terrainMinZoom = qMin(minZoom, qMin(_terrainMaxZoom, TerrariumElevationProvider::kAnchorZoom));
            const QString terrainProviderName = QString::fromLatin1(TerrariumElevationProvider::kProviderKey);
            for (int z = _terrainMinZoom; z <= _terrainMaxZoom; z++) {
                _terrainSet += UrlFactory::getTileCount(z, lon0, lat0, lon1, lat1, terrainProviderName);
            }
        }
    }

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

    QGCFetchTileSetTask* task = new QGCFetchTileSetTask();
    (void) connect(task, &QGCFetchTileSetTask::tileSetFetched, this, &QGCMapEngineManager::_tileSetFetched);
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }
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
    if (_imageSet.tileSize > 0) {
        _createTileSet(name, mapType, _minZoom, _maxZoom, _imageSet);
    } else {
        qCWarning(QGCMapEngineManagerLog) << "No Tiles to save";
    }

    const int mapid = UrlFactory::getMapIdFromProviderType(mapType);
    if (_fetchElevation && !UrlFactory::isElevation(mapid)) {
        const QString elevationProviderName =
            SettingsManager::instance()->flightMapSettings()->elevationMapProvider()->rawValue().toString();
        _createTileSet(name + QStringLiteral(" Elevation"), elevationProviderName, 1, 1, _elevationSet);
        if (_terrainSet.tileCount > 0) {
            _createTileSet(name + QStringLiteral(" Terrain"),
                           QString::fromLatin1(TerrariumElevationProvider::kProviderKey), _terrainMinZoom,
                           _terrainMaxZoom, _terrainSet);
        }
    } else {
        qCWarning(QGCMapEngineManagerLog) << "No Tiles to save";
    }
}

void QGCMapEngineManager::_createTileSet(const QString& name, const QString& mapType, int minZoom, int maxZoom,
                                         const QGCTileSet& tiles)
{
    QGCCachedTileSet* const set = new QGCCachedTileSet(name);
    set->setMapTypeStr(mapType);
    set->setTopleftLat(_topleftLat);
    set->setTopleftLon(_topleftLon);
    set->setBottomRightLat(_bottomRightLat);
    set->setBottomRightLon(_bottomRightLon);
    set->setMinZoom(minZoom);
    set->setMaxZoom(maxZoom);
    set->setTotalTileSize(tiles.tileSize);
    set->setTotalTileCount(static_cast<quint32>(tiles.tileCount));
    set->setType(mapType);

    QGCCreateTileSetTask* const task = new QGCCreateTileSetTask(set);
    (void) connect(task, &QGCCreateTileSetTask::tileSetSaved, this, &QGCMapEngineManager::_tileSetSaved);
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }
}

void QGCMapEngineManager::_tileSetSaved(QGCCachedTileSet* set)
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

QString QGCMapEngineManager::mapAttribution(const QString& mapType)
{
    for (const SharedMapProvider& provider : UrlFactory::getProviders()) {
        if (provider->getMapName() == mapType) {
            return provider->attribution();
        }
    }

    return QString();
}

QString QGCMapEngineManager::terrainAttribution()
{
    return mapAttribution(QString::fromLatin1(TerrariumElevationProvider::kProviderKey));
}

QStringList QGCMapEngineManager::mapTypeList(const QString& provider)
{
    // A map is named "<provider> <type>"
    const QString providerPrefix = provider + QLatin1Char(' ');
    const QStringList maps = mapList();
    QStringList types;
    for (const QString& map : maps) {
        if (map.startsWith(providerPrefix)) {
            types.append(map.sliced(providerPrefix.size()));
        }
    }

    return types;
}

void QGCMapEngineManager::deleteTileSet(QGCCachedTileSet* tileSet)
{
    qCDebug(QGCMapEngineManagerLog) << "Deleting tile set" << tileSet->name();

    if (tileSet->defaultSet()) {
        for (qsizetype i = 0; i < _tileSets->count(); i++) {
            QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
            if (set) {
                set->setDeleting(true);
            }
        }

        QGCResetTask* task = new QGCResetTask();
        (void) connect(task, &QGCResetTask::resetCompleted, this, &QGCMapEngineManager::_resetCompleted);
        (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        if (!getQGCMapEngine()->addTask(task)) {
            task->deleteLater();
        }
    } else {
        tileSet->setDeleting(true);

        QGCDeleteTileSetTask* task = new QGCDeleteTileSetTask(tileSet->id());
        (void) connect(task, &QGCDeleteTileSetTask::tileSetDeleted, this, &QGCMapEngineManager::_tileSetDeleted);
        (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        if (!getQGCMapEngine()->addTask(task)) {
            task->deleteLater();
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

    QGCRenameTileSetTask* task = new QGCRenameTileSetTask(tileSet->id(), name);
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }
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

void QGCMapEngineManager::taskError(QGCMapTask::TaskType type, const QString& error)
{
    QString task;
    switch (type) {
        case QGCMapTask::TaskType::taskFetchTileSets:
            task = QStringLiteral("Fetch Tile Set");
            break;
        case QGCMapTask::TaskType::taskCreateTileSet:
            task = QStringLiteral("Create Tile Set");
            break;
        case QGCMapTask::TaskType::taskGetTileDownloadList:
            task = QStringLiteral("Get Tile Download List");
            break;
        case QGCMapTask::TaskType::taskUpdateTileDownloadState:
            task = QStringLiteral("Update Tile Download Status");
            break;
        case QGCMapTask::TaskType::taskDeleteTileSet:
            task = QStringLiteral("Delete Tile Set");
            break;
        case QGCMapTask::TaskType::taskReset:
            task = QStringLiteral("Reset Tile Sets");
            break;
        case QGCMapTask::TaskType::taskExport:
            task = QStringLiteral("Export Tile Sets");
            break;
        default:
            task = QStringLiteral("Database Error");
            break;
    }

    QString serror = QStringLiteral("Error in task: ") + task;
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

bool QGCMapEngineManager::importSets(const QString& path)
{
    setImportAction(ImportAction::ActionNone);

    if (path.isEmpty()) {
        return false;
    }

    setImportAction(ImportAction::ActionImporting);

    QGCImportTileTask* task = new QGCImportTileTask(path, _importReplace);
    (void) connect(task, &QGCImportTileTask::actionCompleted, this, &QGCMapEngineManager::_actionCompleted);
    (void) connect(task, &QGCImportTileTask::actionProgress, this, &QGCMapEngineManager::_actionProgressHandler);
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
        return false;
    }

    return true;
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

    QGCExportTileTask* task = new QGCExportTileTask(records, path);
    (void) connect(task, &QGCExportTileTask::actionCompleted, this, &QGCMapEngineManager::_actionCompleted);
    (void) connect(task, &QGCExportTileTask::actionProgress, this, &QGCMapEngineManager::_actionProgressHandler);
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
        return false;
    }

    return true;
}

void QGCMapEngineManager::_actionCompleted()
{
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

QStringList QGCMapEngineManager::mapList()
{
    return UrlFactory::getProviderTypes();
}

QStringList QGCMapEngineManager::mapProviderList()
{
    QStringList mapStringList = mapList();
    // Subtract ALL elevation providers (not just user-selectable ones) so
    // hidden elevation providers don't leak into the imagery provider list
    const QStringList elevationStringList = UrlFactory::getElevationProviderTypes();
    for (const QString& elevationProviderName : elevationStringList) {
        (void) mapStringList.removeAll(elevationProviderName);
    }

    static const QRegularExpression providerType = QRegularExpression(uR"(^([^\ ]*) (.*)$)"_s);
    (void) mapStringList.replaceInStrings(providerType, "\\1");
    (void) mapStringList.removeDuplicates();

    return mapStringList;
}

QStringList QGCMapEngineManager::elevationProviderList()
{
    // User-facing list only: hidden providers (e.g. Terrarium, GeoMap-only) are
    // still registered for cache/fetch but must not be selectable as the app's
    // elevation provider
    QStringList types;
    for (const SharedMapProvider& provider : UrlFactory::getProviders()) {
        if (provider->isElevationProvider() && provider->isUserSelectable()) {
            types.append(provider->getMapName());
        }
    }
    return types;
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

    QGCImportTileTask* task = new QGCImportTileTask(dbPath, _importReplace);
    (void) connect(task, &QGCImportTileTask::actionCompleted, this, [this]() {
        _actionCompleted();
        QDir(_extractionOutputDir).removeRecursively();
        _extractionOutputDir.clear();
    });
    (void) connect(task, &QGCImportTileTask::actionProgress, this,
                   [this](int percentage) { setActionProgress(50 + (percentage / 2)); });
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
        setErrorMessage(tr("Failed to start import task"));
        setImportAction(ImportAction::ActionDone);
        QDir(_extractionOutputDir).removeRecursively();
        _extractionOutputDir.clear();
    }
}
