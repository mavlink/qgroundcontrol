#include "QGeoFileTileCacheQGC.h"

#include <QtCore/QDir>
#include <QtCore/QLoggingCategory>

#include "MapsSettings.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(QGeoFileTileCacheQGCLog, "QtLocationPlugin.QGeoFileTileCacheQGC")

QGeoFileTileCacheQGC::QGeoFileTileCacheQGC(QObject* parent)
    : QGeoFileTileCache(baseCacheDirectory(), parent)
{
    qCDebug(QGeoFileTileCacheQGCLog) << this;

    setCostStrategyDisk(QGeoFileTileCache::ByteSize);
    setMaxDiskUsage(_getDefaultMaxDiskCache());
    setCostStrategyMemory(QGeoFileTileCache::ByteSize);
    setMaxMemoryUsage(_getMemLimit());
    setCostStrategyTexture(QGeoFileTileCache::ByteSize);
    setMinTextureUsage(_getDefaultMinTexture());
    setExtraTextureUsage(_getDefaultExtraTexture() - minTextureUsage());

    directory_ = _getCachePath();
}

QGeoFileTileCacheQGC::~QGeoFileTileCacheQGC()
{
    if (QGeoFileTileCacheQGCLog().isDebugEnabled()) {
        printStats();
    }

    qCDebug(QGeoFileTileCacheQGCLog) << this;
}

uint32_t QGeoFileTileCacheQGC::_getMemLimit()
{
    // Value saved in MB
    uint32_t memLimit = _getMaxMemCacheSetting() * qPow(1024, 2);
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

QString QGeoFileTileCacheQGC::_getCachePath()
{
    QString cacheDir = QGCMapEngine::cachePath() + QLatin1String("/providers");
    if (!QGCFileHelper::ensureDirectoryExists(cacheDir)) {
        qCWarning(QGeoFileTileCacheQGCLog) << "Could not create mapping disk cache directory:" << cacheDir;
        cacheDir = QDir::homePath() + QStringLiteral("/.qgcmapscache/");
    }

    if (!QGCFileHelper::ensureDirectoryExists(cacheDir)) {
        qCWarning(QGeoFileTileCacheQGCLog) << "Could not create mapping disk cache directory:" << cacheDir;
        cacheDir.clear();
    }

    return cacheDir;
}
