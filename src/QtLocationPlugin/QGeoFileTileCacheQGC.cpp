#include "QGeoFileTileCacheQGC.h"
#include "QGCMapEngine.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "SettingsManager.h"
#include "MapsSettings.h"
#include "QGCMapUrlEngine.h"
#include "QGCMapEngineData.h"
#include <QGCLoggingCategory.h>

#include <QtCore/QStandardPaths>
#include <QtCore/QLoggingCategory>
#include <QtCore/QDir>

QGC_LOGGING_CATEGORY(QGeoFileTileCacheQGCLog, "qgc.qtlocationplugin.qgeofiletilecacheqgc")

QGeoFileTileCacheQGC::QGeoFileTileCacheQGC(const QVariantMap &parameters, QObject *parent)
    : QGeoFileTileCache(_getCachePath(parameters), parent)
{
    // qCDebug(QGeoFileTileCacheQGCLog) << Q_FUNC_INFO << this;

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
    // printStats();

    // qCDebug(QGeoFileTileCacheQGCLog) << Q_FUNC_INFO << this;
}

QString QGeoFileTileCacheQGC::_getCachePath(const QVariantMap &parameters)
{
    QString cacheDir;
    if (parameters.contains(QStringLiteral("mapping.cache.directory"))) {
        cacheDir = parameters.value(QStringLiteral("mapping.cache.directory")).toString();
    } else {
        cacheDir = getQGCMapEngine()->getCachePath() + QLatin1String("/providers");
        if (!QFileInfo::exists(cacheDir)) {
            if (!QDir::root().mkpath(cacheDir)) {
                qCWarning(QGeoFileTileCacheQGCLog) << "Could not create mapping disk cache directory:" << cacheDir;
                cacheDir = QDir::homePath() + QStringLiteral("/.qgcmapscache/");
            }
        }
    }

    if (!QFileInfo::exists(cacheDir)) {
        if (!QDir::root().mkpath(cacheDir)) {
            qCWarning(QGeoFileTileCacheQGCLog) << "Could not create mapping disk cache directory:" << cacheDir;
            cacheDir.clear();
        }
    }

    return cacheDir;
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
        memLimit = _getMaxMemCacheSetting() * pow(1024, 2);
    }
    if (memLimit == 0) {
        memLimit = _getDefaultMaxMemLimit();
    }
    // 1MB Minimum Memory Cache Required
    if (memLimit < pow(1024, 2)) {
        memLimit = pow(1024, 2);
    }
    // MaxMemoryUsage is 32bit Integer, Round down to 1GB
    if (memLimit > pow(1024, 3)) {
        memLimit = pow(1024, 3);
    }

    return memLimit;
}

quint32 QGeoFileTileCacheQGC::_getMaxMemCacheSetting()
{
    return qgcApp()->toolbox()->settingsManager()->mapsSettings()->maxCacheMemorySize()->rawValue().toUInt();
}

quint32 QGeoFileTileCacheQGC::getMaxDiskCacheSetting()
{
    return qgcApp()->toolbox()->settingsManager()->mapsSettings()->maxCacheDiskSize()->rawValue().toUInt();
}

void QGeoFileTileCacheQGC::cacheTile(const QString &type, int x, int y, int z, const QByteArray &image, const QString &format, qulonglong set)
{
    const QString hash = UrlFactory::getTileHash(type, x, y, z);
    cacheTile(type, hash, image, format, set);
}

void QGeoFileTileCacheQGC::cacheTile(const QString &type, const QString &hash, const QByteArray &image, const QString &format, qulonglong set)
{
    AppSettings* const appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
    if (!appSettings->disableAllPersistence()->rawValue().toBool()) {
        QGCCacheTile* const tile = new QGCCacheTile(hash, image, format, type, set);
        QGCSaveTileTask* const task = new QGCSaveTileTask(tile);
        (void) getQGCMapEngine()->addTask(task);
    }
}

QGCFetchTileTask* QGeoFileTileCacheQGC::createFetchTileTask(const QString &type, int x, int y, int z)
{
    const QString hash = UrlFactory::getTileHash(type, x, y, z);
    QGCFetchTileTask* const task = new QGCFetchTileTask(hash);
    return task;
}
