#include "QGCMapEngineManager.h"

#include <algorithm>
#include <atomic>
#include <memory>

#include <QtCore/QApplicationStatic>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtCore/QStorageInfo>
#include <QtQml/QQmlEngine>

#include "AppSettings.h"
#include "Compression.h"
#include "CompressionJob.h"
#include "ElevationMapProvider.h"
#include "FlightMapSettings.h"
#include "QGCCachedTileSet.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGCWorkDirectory.h"
#include "QGeoFileTileCacheQGC.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"

using namespace Qt::StringLiterals;

QGC_LOGGING_CATEGORY(QGCMapEngineManagerLog, "QtLocationPlugin.QGCMapEngineManager")

Q_APPLICATION_STATIC(QGCMapEngineManager, _mapEngineManager);

QGCMapEngineManager *QGCMapEngineManager::instance()
{
    return _mapEngineManager();
}

QGCMapEngineManager::QGCMapEngineManager(QObject *parent)
    : QObject(parent)
    , _tileSets(new QmlObjectListModel(this))
{
    qCDebug(QGCMapEngineManagerLog) << this;

    (void) qmlRegisterUncreatableType<QGCMapEngineManager>("QGroundControl.QGCMapEngineManager", 1, 0, "QGCMapEngineManager", "Reference only");

    (void) connect(getQGCMapEngine(), &QGCMapEngine::updateTotals, this, &QGCMapEngineManager::_updateTotals, Qt::UniqueConnection);
}

QGCMapEngineManager::~QGCMapEngineManager()
{
    // An unpacking that still runs is stopped first, so that nothing is written into the directory once it is
    // removed. The directory goes also if the unpacking had completed: the job leaves what it extracted
    delete _extractionJob.data();
    _removeExtractionOutput();

    _tileSets->clear();

    qCDebug(QGCMapEngineManagerLog) << this;
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

    qCDebug(QGCMapEngineManagerLog) << lat0 << lon0 << lat1 << lon1 << minZoom << maxZoom;
}

QString QGCMapEngineManager::tileCountStr() const
{
    return QGC::numberToString(_imageSet.tileCount + _elevationSet.tileCount);
}

QString QGCMapEngineManager::tileSizeStr() const
{
    return QGC::bigSizeToString(_imageSet.tileSize + _elevationSet.tileSize);
}

void QGCMapEngineManager::loadTileSets()
{
    if (_tileSets->count() > 0) {
        _tileSets->clear();
        emit tileSetsChanged();
    }

    QGCFetchTileSetTask *task = new QGCFetchTileSetTask();
    (void) connect(task, &QGCFetchTileSetTask::tileSetFetched, this, &QGCMapEngineManager::_tileSetFetched);
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }
}

void QGCMapEngineManager::_tileSetFetched(QGCCachedTileSet *tileSet)
{
    if (tileSet->type() == QStringLiteral("Invalid")) {
        tileSet->setMapTypeStr(QStringLiteral("Various"));
    }

    tileSet->setManager(this);
    _tileSets->append(tileSet);
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

        QGCCreateTileSetTask *task = new QGCCreateTileSetTask(set);
        (void) connect(task, &QGCCreateTileSetTask::tileSetSaved, this, &QGCMapEngineManager::_tileSetSaved);
        (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        if (!getQGCMapEngine()->addTask(task)) {
            task->deleteLater();
        }
    } else {
        qCWarning(QGCMapEngineManagerLog) << "No Tiles to save";
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

        QGCCreateTileSetTask *task = new QGCCreateTileSetTask(set);
        (void) connect(task, &QGCCreateTileSetTask::tileSetSaved, this, &QGCMapEngineManager::_tileSetSaved);
        (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        if (!getQGCMapEngine()->addTask(task)) {
            task->deleteLater();
        }
    } else {
        qCWarning(QGCMapEngineManagerLog) << "No Tiles to save";
    }
}

void QGCMapEngineManager::_tileSetSaved(QGCCachedTileSet *set)
{
    qCDebug(QGCMapEngineManagerLog) << "New tile set saved (" << set->name() << "). Starting download...";

    _tileSets->append(set);
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

    static const QRegularExpression providerType = QRegularExpression(uR"(^([^\ ]*) (.*)$)"_s);
    (void) mapStringList.replaceInStrings(providerType, "\\2");
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

        QGCResetTask *task = new QGCResetTask();
        (void) connect(task, &QGCResetTask::resetCompleted, this, &QGCMapEngineManager::_resetCompleted);
        (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        if (!getQGCMapEngine()->addTask(task)) {
            task->deleteLater();
        }
    } else {
        tileSet->setDeleting(true);

        QGCDeleteTileSetTask *task = new QGCDeleteTileSetTask(tileSet->id());
        (void) connect(task, &QGCDeleteTileSetTask::tileSetDeleted, this, &QGCMapEngineManager::_tileSetDeleted);
        (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
        if (!getQGCMapEngine()->addTask(task)) {
            task->deleteLater();
        }
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

    QGCRenameTileSetTask *task = new QGCRenameTileSetTask(tileSet->id(), name);
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }
}

void QGCMapEngineManager::_tileSetDeleted(quint64 setID)
{
    for (qsizetype i = 0; i < _tileSets->count(); i++ ) {
        QGCCachedTileSet *set = qobject_cast<QGCCachedTileSet*>(_tileSets->get(i));
        if (set && (set->id() == setID)) {
            (void) _tileSets->removeAt(i);
            delete set;
            emit tileSetsChanged();
            break;
        }
    }
}

void QGCMapEngineManager::taskError(QGCMapTask::TaskType type, const QString &error)
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

    // A failed import or export has ended: the task reports nothing more
    if ((type == QGCMapTask::TaskType::taskImport) || (type == QGCMapTask::TaskType::taskExport)) {
        setImportAction(ImportAction::ActionDone);
    }
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

namespace {

/// Largest size the tile set of an archive may unpack to, as a multiple of the size of the archive. Map tiles are
/// PNG or JPEG images, which hardly compress, so a real tile set is little larger than its archive. A
/// decompression bomb (deflate reaches about 1000:1) stops at this limit
constexpr qint64 kMaxTileSetExpansion = 20;

/// Least that limit is, for a small archive, which may hold a database of mostly empty pages
constexpr qint64 kMinTileSetLimitBytes = 64LL * 1024 * 1024;

/// Start of the name of a directory that an archive is unpacked into
constexpr QLatin1StringView kUnpackDirectoryPrefix("qgc_tiles_");

/// Whether a file has the name of a tile set file: one exportSets() wrote, or a database
bool isTileSetFileName(const QString& fileName)
{
    static const QStringList suffixes{QLatin1Char('.') + QLatin1String(AppSettings::tilesetFileExtension),
                                      QStringLiteral(".db"), QStringLiteral(".sqlite")};
    return std::any_of(suffixes.cbegin(), suffixes.cend(),
                       [&fileName](const QString& suffix) { return fileName.endsWith(suffix, Qt::CaseInsensitive); });
}

}  // namespace

bool QGCMapEngineManager::importSets(const QString &path)
{
    if (_importAction == ImportAction::ActionImporting) {
        setErrorMessage(tr("Import already in progress"));
        return false;
    }

    setImportAction(ImportAction::ActionNone);

    if (path.isEmpty()) {
        return false;
    }

    // A tile set file that was packed into an archive is unpacked first
    if (Compression::isArchiveFile(path)) {
        return importArchive(path);
    }

    setImportAction(ImportAction::ActionImporting);

    QGCImportTileTask *task = new QGCImportTileTask(path, _importReplace);
    (void) connect(task, &QGCImportTileTask::actionCompleted, this, &QGCMapEngineManager::_actionCompleted);
    (void) connect(task, &QGCImportTileTask::actionProgress, this, &QGCMapEngineManager::_actionProgressHandler);
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
        return false;
    }

    return true;
}

bool QGCMapEngineManager::exportSets(const QString &path)
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
            rec.type = UrlFactory::getQtMapIdFromProviderType(set->type());
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

    QGCExportTileTask *task = new QGCExportTileTask(records, path);
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
    for (const QString &elevationProviderName : elevationStringList) {
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

QStringList QGCMapEngineManager::importArchivePatterns()
{
    return Compression::archiveNamePatterns();
}

bool QGCMapEngineManager::lacksRoomToUnpack(qint64 bytesAvailable, qint64 archiveSize)
{
    return (bytesAvailable >= 0) && (bytesAvailable < archiveSize);
}

bool QGCMapEngineManager::importArchive(const QString &archivePath)
{
    if (archivePath.isEmpty()) {
        setErrorMessage(tr("No archive path specified"));
        return false;
    }

    if (!QFile::exists(archivePath)) {
        setErrorMessage(tr("Archive file not found: %1").arg(archivePath));
        return false;
    }

    if (!Compression::isArchiveFile(archivePath)) {
        setErrorMessage(tr("Not a supported archive format: %1").arg(archivePath));
        return false;
    }

    if (_importAction == ImportAction::ActionImporting) {
        setErrorMessage(tr("Import already in progress"));
        return false;
    }

    // The tile set is unpacked beside the tile database, on its disk: a replace import then moves it into place
    // instead of copying it, and the free space that counts is that of this disk. The temporary directory of the
    // system may be in memory, or on a small disk
    QGeoFileTileCacheQGC::ensureInitialized();
    const QString databasePath = QGeoFileTileCacheQGC::getDatabaseFilePath();
    if (databasePath.isEmpty()) {
        setErrorMessage(tr("No tile cache directory to unpack the archive into"));
        return false;
    }
    const QString cacheDirectory = QFileInfo(databasePath).absolutePath();

    // What an import that crashed or was killed left behind goes first, also so that its space counts as free. One
    // that another QGC runs stays
    (void) QGCWorkDirectory::removeStale(cacheDirectory, kUnpackDirectoryPrefix);

    // Map tiles are images that do not compress further, so the tile set is about as large as the archive
    const qint64 archiveSize = QFileInfo(archivePath).size();
    const QStorageInfo storage(cacheDirectory);
    // A valid volume that is not ready, or whose free space the system cannot tell, reports -1
    const qint64 bytesAvailable = (storage.isValid() && storage.isReady()) ? storage.bytesAvailable() : -1;
    if (lacksRoomToUnpack(bytesAvailable, archiveSize)) {
        setErrorMessage(tr("Not enough disk space to unpack the archive: %1 needed, %2 free")
                            .arg(QGC::bigSizeToString(static_cast<quint64>(archiveSize)),
                                 QGC::bigSizeToString(static_cast<quint64>(bytesAvailable))));
        return false;
    }

    // A directory of this import alone: a name that could be told beforehand would let another user of the
    // directory put files where the tile set file is looked for. The archive is unpacked into a new directory in it,
    // beside the lock file that keeps it while the import runs
    auto extractionDirectory = std::make_unique<QGCWorkDirectory>(cacheDirectory, kUnpackDirectoryPrefix);
    if (!extractionDirectory->isValid()) {
        setErrorMessage(tr("Could not create temporary directory: %1").arg(extractionDirectory->errorString()));
        return false;
    }
    _extractionDirectory = std::move(extractionDirectory);

    setImportAction(ImportAction::ActionImporting);
    setActionProgress(0);

    // Only the first tile set file is unpacked, whatever else the archive holds. A later one is skipped but counted,
    // and fails the import once the extraction has ended: which one is meant cannot be told. The filter runs on the
    // worker, and the count is read in the callback, which comes after the worker has ended
    const auto tileSetEntries = std::make_shared<std::atomic_int>(0);
    _extractionJob = CompressionJob::startExtraction(
        this, archivePath, QDir(_extractionDirectory->path()).filePath(QStringLiteral("unpacked")),
        {.maxBytes = std::max(archiveSize * kMaxTileSetExpansion, kMinTileSetLimitBytes),
         .filter =
             [tileSetEntries](const Compression::ArchiveEntry& entry) {
                 return !entry.isDirectory && isTileSetFileName(entry.name) && (tileSetEntries->fetch_add(1) == 0);
             }},
        [this, tileSetEntries](const Compression::Outcome& outcome) {
            _handleExtractionFinished(outcome, tileSetEntries->load());
        });
    (void) connect(_extractionJob, &CompressionJob::progressChanged, this,
                   &QGCMapEngineManager::_handleExtractionProgress);
    return true;
}

void QGCMapEngineManager::_removeExtractionOutput()
{
    _extractionDirectory.reset();
}

void QGCMapEngineManager::_handleExtractionProgress(qreal progress)
{
    setActionProgress(static_cast<int>(progress * 50.0));
}

void QGCMapEngineManager::_handleExtractionFinished(const Compression::Outcome& outcome, int tileSetEntries)
{
    if (!outcome) {
        setErrorMessage(outcome.description());
        setImportAction(ImportAction::ActionDone);
        _removeExtractionOutput();
        return;
    }

    // An import that replaces the tiles must not take the wrong one
    if (tileSetEntries > 1) {
        setErrorMessage(tr("The archive holds more than one tile set file"));
        setImportAction(ImportAction::ActionDone);
        _removeExtractionOutput();
        return;
    }

    QStringList tileSetFiles;
    QDirIterator it(_extractionDirectory->path(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString filePath = it.next();
        if (isTileSetFileName(filePath)) {
            tileSetFiles.append(filePath);
        }
    }

    if (tileSetFiles.isEmpty()) {
        // A tile set file whose name the extraction does not write is skipped, and leaves a hint in the log only
        setErrorMessage((outcome.skippedEntries > 0)
                            ? tr("No tile database found in archive (entries skipped for their names or types: %1)")
                                  .arg(outcome.skippedEntries)
                            : tr("No tile database found in archive"));
        setImportAction(ImportAction::ActionDone);
        _removeExtractionOutput();
        return;
    }
    const QString dbPath = tileSetFiles.constFirst();

    qCDebug(QGCMapEngineManagerLog) << "Found tile database:" << dbPath;

    // The unpacked file is the import's own, so a replace import may move it into place
    QGCImportTileTask* task = new QGCImportTileTask(dbPath, _importReplace, true);
    (void) connect(task, &QGCImportTileTask::actionCompleted, this, [this]() {
        _actionCompleted();
        _removeExtractionOutput();
    });
    (void) connect(task, &QGCImportTileTask::actionProgress, this, [this](int percentage) {
        setActionProgress(50 + (percentage / 2));
    });
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::taskError);
    // What was unpacked goes also if the import fails
    (void) connect(task, &QGCMapTask::error, this, &QGCMapEngineManager::_removeExtractionOutput);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
        setErrorMessage(tr("Failed to start import task"));
        setImportAction(ImportAction::ActionDone);
        _removeExtractionOutput();
    }
}
