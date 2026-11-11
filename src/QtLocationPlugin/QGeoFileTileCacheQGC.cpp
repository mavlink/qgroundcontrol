#include "QGeoFileTileCacheQGC.h"

#include <QtCore/QDir>
#include <QtCore/QLoggingCategory>

#include "MapsSettings.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(QGeoFileTileCacheQGCLog, "QtLocationPlugin.QGeoFileTileCacheQGC")

QGeoFileTileCacheQGC::QGeoFileTileCacheQGC(const QVariantMap &parameters, QObject *parent)
    : QGeoFileTileCache(baseCacheDirectory(), parent)
{
    qCDebug(QGeoFileTileCacheQGCLog) << this;

    setCostStrategyDisk(QGeoFileTileCache::ByteSize);
    setMaxDiskUsage(_getDefaultMaxDiskCache());
    setCostStrategyMemory(QGeoFileTileCache::ByteSize);
    setMaxMemoryUsage(_getMemLimit(parameters));
    setCostStrategyTexture(QGeoFileTileCache::ByteSize);
    setMinTextureUsage(_getDefaultMinTexture());
    setExtraTextureUsage(_getDefaultExtraTexture() - minTextureUsage());

    directory_ = _getCachePath(parameters);
}

QGeoFileTileCacheQGC::~QGeoFileTileCacheQGC()
{
    if (QGeoFileTileCacheQGCLog().isDebugEnabled()) {
        printStats();
    }

    qCDebug(QGeoFileTileCacheQGCLog) << this;
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

QString QGeoFileTileCacheQGC::_getCachePath(const QVariantMap &parameters)
{
    QString cacheDir;
    if (parameters.contains(QStringLiteral("mapping.cache.directory"))) {
        cacheDir = parameters.value(QStringLiteral("mapping.cache.directory")).toString();
    } else {
        cacheDir = QGCMapEngine::cachePath() + QLatin1String("/providers");
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
