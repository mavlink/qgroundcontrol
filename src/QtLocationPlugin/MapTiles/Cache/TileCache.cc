#include "TileCache.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QtMath>

#include "CacheTasks.h"
#include "CacheTile.h"
#include "CacheWorker.h"
#include "MapProviderRegistry.h"
#include "MapTilesApp.h"
#include "MapTilesSettings.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(TileCacheLog, "MapTiles.TileCache")

Q_APPLICATION_STATIC(TileCache, _tileCache);

namespace {

/// Removes the cache of the versions up to 5.0, which had it in a folder of its own and in a form that cannot be
/// read any more. Returns true when there was one.
bool wipeOldCache()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    const QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    const QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
#endif
    QDir oldCache(cacheLocation + QStringLiteral("/QGCMapCache300"));
    if (!oldCache.exists()) {
        return false;
    }
    (void) oldCache.removeRecursively();
    return true;
}

QString createCacheDirectory()
{
    const bool cacheWasReset = wipeOldCache();

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#endif
    cacheDir += QStringLiteral("/QGCMapCache");
    if (!QGCFileHelper::ensureDirectoryExists(cacheDir)) {
        qCWarning(TileCacheLog) << "Could not create mapping disk cache directory:" << cacheDir;

        cacheDir = QDir::homePath() + QStringLiteral("/.qgcmapscache/");
        if (!QGCFileHelper::ensureDirectoryExists(cacheDir)) {
            qCWarning(TileCacheLog) << "Could not create mapping disk cache directory:" << cacheDir;
            cacheDir.clear();
        }
    }

    if (cacheDir.isEmpty()) {
        qCCritical(TileCacheLog) << "Could not find suitable map cache directory.";
    } else {
        qCDebug(TileCacheLog) << "Map Cache in:" << cacheDir;
    }

    if (cacheWasReset) {
        // The context is the class this message moved from, so its existing translations still apply
        MapTilesApp::showMessage(QCoreApplication::translate("QGeoFileTileCacheQGC",
                                                             "The Offline Map Cache database has been upgraded. "
                                                             "Your old map cache sets have been reset."));
    }

    return cacheDir;
}

}  // namespace

TileCache::TileCache(QObject* parent)
    : QObject(parent)
{
    qCDebug(TileCacheLog) << this;
}

TileCache::~TileCache()
{
    shutdown();

    qCDebug(TileCacheLog) << this;
}

void TileCache::shutdown()
{
    if (_worker) {
        _worker->stop();
        (void) _worker->wait();
        delete _worker;
        _worker = nullptr;
    }
    // The worker deleted a prune that was still queued without ending it
    _pruning = false;
    _unprunedSize = 0;
    _initialized = false;
}

void TileCache::close()
{
    _closed = true;
    shutdown();
}

void TileCache::closeInstance()
{
    // The application static is destroyed only after QCoreApplication, too late for the worker's
    // database teardown; don't create the cache just to close it.
    if (_tileCache.exists()) {
        _tileCache()->close();
    }
}

TileCache* TileCache::instance()
{
    return _tileCache();
}

void TileCache::init(const QString& databasePath)
{
    if (_initialized || _closed) {
        return;
    }

    _initialized = true;

    _worker = new CacheWorker(this);
    _worker->setDatabaseFile(databasePath);
    _worker->setCacheVariants(MapProviderRegistry::cacheVariants());
    (void) connect(_worker, &CacheWorker::updateTotals, this, &TileCache::_updateTotals);

    // A worker that cannot open the database ends its first task with an error. The cache stays initialized, without
    // a worker that runs: the maps ask their servers for every tile, and are not to open the cache again for each.
    // The worker is the context: a cache that is shut down, which deletes it, has nothing left to tell of it.
    CacheTask* const task = CacheTasks::init();
    (void) connect(task, &CacheTask::finished, _worker, [](const QString& error) {
        if (!error.isEmpty()) {
            qCWarning(TileCacheLog) << "The tile cache could not be opened:" << error;
            MapTilesApp::showMessage(
                tr("The map tile cache could not be opened. Maps are not cached, and offline maps are not available."));
        }
    });
    (void) addTask(task);
}

void TileCache::ensureInitialized()
{
    if (_initialized || _closed) {
        return;
    }

    init(databaseFilePath());
}

QString TileCache::cachePath()
{
    static const QString path = createCacheDirectory();
    return path;
}

QString TileCache::databaseFilePath()
{
    const QString path = cachePath();
    return path.isEmpty() ? QString() : (path + QStringLiteral("/qgcMapCache.db"));
}

void TileCache::cacheTile(int mapId, const QString& hash, const QByteArray& image, const QString& format,
                          quint64 tileSet)
{
    if (MapTilesSettings::persistenceDisabled()) {
        return;
    }

    (void) addTask(CacheTasks::saveTile(CacheTile(hash, image, format, mapId, tileSet)));
}

bool TileCache::addTask(CacheTask* task)
{
    if (!_worker) {
        task->deleteLater();
        return false;
    }

    return _worker->enqueueTask(task);
}

void TileCache::_updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize)
{
    emit updateTotals(totaltiles, totalsize, defaulttiles, defaultsize);

    const quint32 maxDiskCacheMB = MapTilesSettings::maxCacheDiskSizeMB();
    const quint64 maxSize = static_cast<quint64>(maxDiskCacheMB) * qPow(1024, 2);
    if (!_pruning && (defaultsize > maxSize) && (defaultsize != _unprunedSize)) {
        _pruning = true;

        const quint64 amountToPrune = defaultsize - maxSize;
        CacheTask* const task = CacheTasks::pruneCache(amountToPrune);
        // Whatever the outcome: a prune that failed must not keep the next one from being tried. That one waits for
        // the cache to change, as the totals that follow the failure would start it again at once, without end.
        (void) connect(task, &CacheTask::finished, this, [this, defaultsize](const QString& error) {
            _pruning = false;
            _unprunedSize = error.isEmpty() ? 0 : defaultsize;
        });
        if (!addTask(task)) {
            _pruning = false;
        }
    }
}
