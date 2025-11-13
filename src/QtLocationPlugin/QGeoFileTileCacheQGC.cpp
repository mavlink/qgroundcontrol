#include "QGeoFileTileCacheQGC.h"

#include <QtCore/QDir>
#include <QtCore/QLoggingCategory>
#include <QtCore/QStandardPaths>

#include "AppSettings.h"
#include "MapsSettings.h"
#include "QGCApplication.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(QGeoFileTileCacheQGCLog, "QtLocationPlugin.QGeoFileTileCacheQGC")

QString QGeoFileTileCacheQGC::_databaseFilePath;
QString QGeoFileTileCacheQGC::_cachePath;
bool QGeoFileTileCacheQGC::_cacheWasReset = false;

QGeoFileTileCacheQGC::QGeoFileTileCacheQGC(const QVariantMap &parameters, QObject *parent)
    : QGeoFileTileCache(QString(), parent)
{
    qCDebug(QGeoFileTileCacheQGCLog) << this;

    static std::once_flag cacheInit;
    std::call_once(cacheInit, []() {
        _initCache();
    });

    directory_ = _getCachePath(parameters);

    setCostStrategyDisk(QGeoFileTileCache::ByteSize);
    setMaxDiskUsage(_getDefaultMaxDiskCache());
    setCostStrategyMemory(QGeoFileTileCache::ByteSize);
    setMaxMemoryUsage(_getMemLimit(parameters));
    setCostStrategyTexture(QGeoFileTileCache::ByteSize);
    setMinTextureUsage(_getDefaultMinTexture());
    setExtraTextureUsage(_getDefaultExtraTexture() - minTextureUsage());
}

QGeoFileTileCacheQGC::~QGeoFileTileCacheQGC()
{
    if (QGeoFileTileCacheQGCLog().isDebugEnabled()) {
        printStats();
    }

    qCDebug(QGeoFileTileCacheQGCLog) << this;
}

void QGeoFileTileCacheQGC::init()
{
    if (directory_.isEmpty()) {
        qCWarning(QGeoFileTileCacheQGCLog) << "Cache directory is empty";
        return;
    }

    const bool directoryCreated = QDir::root().mkpath(directory_);
    if (!directoryCreated) {
        qCWarning(QGeoFileTileCacheQGCLog) << "Failed to create cache directory:" << directory_;
    }

    qCDebug(QGeoFileTileCacheQGCLog) << "Tile cache directory:" << directory_;
}

void QGeoFileTileCacheQGC::printStats()
{
    qCDebug(QGeoFileTileCacheQGCLog) << "======== QGeoFileTileCacheQGC Statistics ========";
    qCDebug(QGeoFileTileCacheQGCLog) << "Tile Directory:" << directory_;
    qCDebug(QGeoFileTileCacheQGCLog) << "Database Path:" << _databaseFilePath;
    qCDebug(QGeoFileTileCacheQGCLog) << "Cache Path:" << _cachePath;
    qCDebug(QGeoFileTileCacheQGCLog) << "Cache Was Reset:" << _cacheWasReset;

    qCDebug(QGeoFileTileCacheQGCLog) << "--- Memory Cache ---";
    qCDebug(QGeoFileTileCacheQGCLog) << "Max Memory:" << maxMemoryUsage() << "bytes";
    qCDebug(QGeoFileTileCacheQGCLog) << "Current Memory:" << memoryUsage() << "bytes";
    memoryCache_.printStats();

    qCDebug(QGeoFileTileCacheQGCLog) << "--- Texture Cache ---";
    qCDebug(QGeoFileTileCacheQGCLog) << "Max Texture:" << maxTextureUsage() << "bytes";
    qCDebug(QGeoFileTileCacheQGCLog) << "Min Texture:" << minTextureUsage() << "bytes";
    qCDebug(QGeoFileTileCacheQGCLog) << "Current Texture:" << textureUsage() << "bytes";
    textureCache_.printStats();

    qCDebug(QGeoFileTileCacheQGCLog) << "--- Disk Cache (Database-backed, not file-based) ---";
    qCDebug(QGeoFileTileCacheQGCLog) << "File-based disk cache is disabled (using SQLite database)";
    qCDebug(QGeoFileTileCacheQGCLog) << "=================================================";
}

void QGeoFileTileCacheQGC::handleError(const QGeoTileSpec &spec, const QString &errorString)
{
    qCWarning(QGeoFileTileCacheQGCLog) << "Tile load error - Map:" << spec.mapId()
                                       << "X:" << spec.x() << "Y:" << spec.y() << "Zoom:" << spec.zoom()
                                       << "Error:" << errorString;
}

bool QGeoFileTileCacheQGC::isTileBogus(const QByteArray &bytes) const
{
    if (bytes.size() == 7 && bytes == QByteArrayLiteral("NoRetry")) {
        return true;
    }

    if (bytes.isEmpty()) {
        return true;
    }

    return false;
}

void QGeoFileTileCacheQGC::insert(const QGeoTileSpec &spec, const QByteArray &bytes, const QString &format, QAbstractGeoTileCache::CacheAreas areas)
{
    if (bytes.isEmpty()) {
        return;
    }

    if (areas & QAbstractGeoTileCache::MemoryCache) {
        addToMemoryCache(spec, bytes, format);
    }
}

QSharedPointer<QGeoTileTexture> QGeoFileTileCacheQGC::get(const QGeoTileSpec &spec)
{
    return getFromMemory(spec);
}

void QGeoFileTileCacheQGC::clearAll()
{
    textureCache_.clear();
    memoryCache_.clear();
}

uint32_t QGeoFileTileCacheQGC::_getMemLimit(const QVariantMap &parameters)
{
    uint32_t memLimit = 0;
    if (parameters.contains(QStringLiteral("mapping.cache.memory.size"))) {
        bool ok = false;
        memLimit = parameters.value(QStringLiteral("mapping.cache.memory.size")).toString().toUInt(&ok);
        if (!ok) {
            memLimit = 0;
        }
    }

    if (memLimit == 0) {
        // Value saved in MB
        memLimit = _getMaxMemCacheSetting() * qPow(1024, 2);
    }
    if (memLimit == 0) {
        memLimit = _getDefaultMaxMemLimit();
    }

    // 1MB Minimum Memory Cache Required
    // MaxMemoryUsage is 32bit Integer, Round down to 1GB
    memLimit = qBound(static_cast<uint32_t>(qPow(1024, 2)), memLimit, static_cast<uint32_t>(qPow(1024, 3)));
    return memLimit;
}

quint32 QGeoFileTileCacheQGC::_getMaxMemCacheSetting()
{
    return SettingsManager::instance()->mapsSettings()->maxCacheMemorySize()->rawValue().toUInt();
}

quint32 QGeoFileTileCacheQGC::getMaxDiskCacheSetting()
{
    return SettingsManager::instance()->mapsSettings()->maxCacheDiskSize()->rawValue().toUInt();
}

void QGeoFileTileCacheQGC::cacheTile(const QString &type, int x, int y, int z, const QByteArray &image, const QString &format, qulonglong set)
{
    if (type.isEmpty()) {
        qCWarning(QGeoFileTileCacheQGCLog) << "Cannot cache tile with empty type";
        return;
    }

    if (image.isEmpty()) {
        qCWarning(QGeoFileTileCacheQGCLog) << "Cannot cache empty tile image";
        return;
    }

    if (format.isEmpty()) {
        qCWarning(QGeoFileTileCacheQGCLog) << "Cannot cache tile with empty format";
        return;
    }

    const QString hash = UrlFactory::getTileHash(type, x, y, z);
    cacheTile(type, hash, image, format, set);
}

void QGeoFileTileCacheQGC::cacheTile(const QString &type, const QString &hash, const QByteArray &image, const QString &format, qulonglong set)
{
    if (type.isEmpty() || hash.isEmpty() || image.isEmpty() || format.isEmpty()) {
        qCWarning(QGeoFileTileCacheQGCLog) << "Cannot cache tile with empty parameters";
        return;
    }

    AppSettings *appSettings = SettingsManager::instance()->appSettings();
    if (!appSettings->disableAllPersistence()->rawValue().toBool()) {
        QGCCacheTile *tile = new QGCCacheTile(hash, image, format, type, set);
        QGCSaveTileTask *task = new QGCSaveTileTask(tile);
        if (!getQGCMapEngine()->addTask(task)) {
            task->deleteLater();
        }
    }
}

QGCFetchTileTask* QGeoFileTileCacheQGC::createFetchTileTask(const QString &type, int x, int y, int z)
{
    if (type.isEmpty()) {
        qCWarning(QGeoFileTileCacheQGCLog) << "Cannot create fetch task with empty type";
        return nullptr;
    }

    const QString hash = UrlFactory::getTileHash(type, x, y, z);
    if (hash.isEmpty()) {
        qCWarning(QGeoFileTileCacheQGCLog) << "Cannot create fetch task with empty hash";
        return nullptr;
    }

    return new QGCFetchTileTask(hash);
}

QString QGeoFileTileCacheQGC::_getCachePath(const QVariantMap &parameters)
{
    QString cacheDir;
    if (parameters.contains(QStringLiteral("mapping.cache.directory"))) {
        cacheDir = parameters.value(QStringLiteral("mapping.cache.directory")).toString();
    } else {
        cacheDir = _cachePath + QLatin1String("/providers");
        if (!QGCFileHelper::ensureDirectoryExists(cacheDir)) {
            qCWarning(QGeoFileTileCacheQGCLog) << "Could not create mapping disk cache directory:" << cacheDir;
            cacheDir = QDir::homePath() + QStringLiteral("/.qgcmapscache/");
        }
    }

    if (!QGCFileHelper::ensureDirectoryExists(cacheDir)) {
        qCWarning(QGeoFileTileCacheQGCLog) << "Could not create mapping disk cache directory:" << cacheDir;
        cacheDir.clear();
    }

    return cacheDir;
}

bool QGeoFileTileCacheQGC::_wipeDirectory(const QString &dirPath)
{
    bool result = true;

    const QDir dir(dirPath);
    if (dir.exists(dirPath)) {
        _cacheWasReset = true;

        const QFileInfoList fileList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
        for (const QFileInfo &info : fileList) {
            if (info.isDir()) {
                result = _wipeDirectory(info.absoluteFilePath());
            } else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirPath);
    }

    return result;
}

void QGeoFileTileCacheQGC::_wipeOldCaches()
{
    QStringList searchPaths;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    searchPaths.append(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
#else
    searchPaths.append(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation));
    searchPaths.append(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
#endif

    for (const QString &searchPath : searchPaths) {
        const QDir dir(searchPath);
        if (!dir.exists()) {
            continue;
        }

        const QStringList cacheDirs = dir.entryList(QStringList() << QStringLiteral("QGCMapCache*"), QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &cacheDirName : cacheDirs) {
            if (cacheDirName == QStringLiteral("QGCMapCache")) {
                continue;
            }

            const QString fullPath = dir.absoluteFilePath(cacheDirName);
            qCDebug(QGeoFileTileCacheQGCLog) << "Removing legacy cache directory" << fullPath;
            _wipeDirectory(fullPath);
        }
    }
}

void QGeoFileTileCacheQGC::_initCache()
{
    _wipeOldCaches();

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#endif
    cacheDir += QStringLiteral("/QGCMapCache") + QString(kCachePathVersion);
    if (!QGCFileHelper::ensureDirectoryExists(cacheDir)) {
        qCWarning(QGeoFileTileCacheQGCLog) << "Could not create mapping disk cache directory:" << cacheDir;

        cacheDir = QDir::homePath() + QStringLiteral("/.qgcmapscache/");
        if (!QGCFileHelper::ensureDirectoryExists(cacheDir)) {
            qCWarning(QGeoFileTileCacheQGCLog) << "Could not create mapping disk cache directory:" << cacheDir;
            cacheDir.clear();
        }
    }

    _cachePath = cacheDir;
    if (!_cachePath.isEmpty()) {
        _databaseFilePath = QString(_cachePath + QStringLiteral("/qgcMapCache.db"));

        qCDebug(QGeoFileTileCacheQGCLog) << "Map Cache in:" << _databaseFilePath;
    } else {
        qCCritical(QGeoFileTileCacheQGCLog) << "Could not find suitable map cache directory.";
    }

    if (_cacheWasReset) {
        qgcApp()->showAppMessage(tr(
            "The Offline Map Cache database has been upgraded. "
            "Your old map cache sets have been reset."));
    }
}
