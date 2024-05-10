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
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkDiskCache>

QGC_LOGGING_CATEGORY(QGeoFileTileCacheQGCLog, "qgc.qtlocationplugin.qgeofiletilecacheqgc")

QGeoFileTileCacheQGC::QGeoFileTileCacheQGC(QNetworkAccessManager* networkManager, const QVariantMap &parameters, QObject *parent)
    : QGeoFileTileCache(_getCachePath(parameters), parent)
    , m_diskCache(new QNetworkDiskCache(this))
{
    if (getQGCMapEngine()->wasCacheReset()) {
        qgcApp()->showAppMessage(tr("The Offline Map Cache database has been upgraded. "
                    "Your old map cache sets have been reset."));
    }

    setCostStrategyDisk(QGeoFileTileCache::ByteSize);
    setMaxDiskUsage(_getDefaultMaxDiskCache());
    setCostStrategyMemory(QGeoFileTileCache::ByteSize);
    setMaxMemoryUsage(_getMemLimit(parameters));
    setCostStrategyTexture(QGeoFileTileCache::ByteSize);
    setMinTextureUsage(_getDefaultMinTexture());
    setExtraTextureUsage(_getDefaultExtraTexture() - minTextureUsage());

    if (networkManager) {
        m_diskCache->setCacheDirectory(directory());
        m_diskCache->setMaximumCacheSize(static_cast<qint64>(_getDefaultMaxDiskCache()));
        networkManager->setCache(m_diskCache);
    }

    qCDebug(QGeoFileTileCacheQGCLog) << Q_FUNC_INFO << this;
}

QGeoFileTileCacheQGC::~QGeoFileTileCacheQGC()
{
    printStats();
    qCDebug(QGeoFileTileCacheQGCLog) << Q_FUNC_INFO << this;
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
                qCWarning(QGeoFileTileCacheQGCLog) << "Could not create mapping disk cache directory: " << cacheDir;
                cacheDir = QDir::homePath() + QStringLiteral("/.qgcmapscache/");
            }
        }
    }
    if (!QFileInfo::exists(cacheDir)) {
        if (!QDir::root().mkpath(cacheDir)) {
            qCWarning(QGeoFileTileCacheQGCLog) << "Could not create mapping disk cache directory: " << cacheDir;
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
    if (!memLimit) {
        memLimit = _getMaxMemCacheSetting() * pow(1024, 2);
    }
    if (!memLimit) {
        memLimit = _getDefaultMaxMemLimit();
    }
    if (memLimit < pow(1024, 2)) {
        memLimit = pow(1024, 2);
    }
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

void QGeoFileTileCacheQGC::cacheTile(const QString& type, int x, int y, int z, const QByteArray& image, const QString &format, qulonglong set)
{
    const QString hash = UrlFactory::getTileHash(type, x, y, z);
    cacheTile(type, hash, image, format, set);
}

void QGeoFileTileCacheQGC::cacheTile(const QString& type, const QString& hash, const QByteArray& image, const QString& format, qulonglong set)
{
    AppSettings* const appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
    if (!appSettings->disableAllPersistence()->rawValue().toBool()) {
        QGCCacheTile* const tile = new QGCCacheTile(hash, image, format, type, set);
        QGCSaveTileTask* const task = new QGCSaveTileTask(tile);
        (void) getQGCMapEngine()->addTask(task);
    }
}

QGCFetchTileTask* QGeoFileTileCacheQGC::createFetchTileTask(const QString& type, int x, int y, int z)
{
    const QString hash = UrlFactory::getTileHash(type, x, y, z);
    QGCFetchTileTask* const task = new QGCFetchTileTask(hash);
    return task;
}
