/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGeoFileTileCacheQGC.h"
#include "QGCMapEngine.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "MapsSettings.h"
#include "QGCMapUrlEngine.h"
#include "QGCMapTasks.h"
#include <QGCLoggingCategory.h>

#include <QtCore/QStandardPaths>
#include <QtCore/QLoggingCategory>
#include <QtCore/QDir>

QGC_LOGGING_CATEGORY(QGeoFileTileCacheQGCLog, "qgc.qtlocationplugin.qgeofiletilecacheqgc")

QString QGeoFileTileCacheQGC::_databaseFilePath;
QString QGeoFileTileCacheQGC::_cachePath;
bool QGeoFileTileCacheQGC::_cacheWasReset = false;

QGeoFileTileCacheQGC::QGeoFileTileCacheQGC(const QVariantMap &parameters, QObject *parent)
    : QGeoFileTileCache(baseCacheDirectory(), parent)
{
    // qCDebug(QGeoFileTileCacheQGCLog) << Q_FUNC_INFO << this;

    setCostStrategyDisk(QGeoFileTileCache::ByteSize);
    setMaxDiskUsage(_getDefaultMaxDiskCache());
    setCostStrategyMemory(QGeoFileTileCache::ByteSize);
    setMaxMemoryUsage(_getMemLimit(parameters));
    setCostStrategyTexture(QGeoFileTileCache::ByteSize);
    setMinTextureUsage(_getDefaultMinTexture());
    setExtraTextureUsage(_getDefaultExtraTexture() - minTextureUsage());

    static std::once_flag cacheInit;
    std::call_once(cacheInit, [this]() {
        _initCache();
    });

    directory_ = _getCachePath(parameters);
}

QGeoFileTileCacheQGC::~QGeoFileTileCacheQGC()
{
#ifdef QT_DEBUG
    // printStats();
#endif

    // qCDebug(QGeoFileTileCacheQGCLog) << Q_FUNC_INFO << this;
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
    return SettingsManager::instance()->mapsSettings()->maxCacheMemorySize()->rawValue().toUInt();
}

quint32 QGeoFileTileCacheQGC::getMaxDiskCacheSetting()
{
    return SettingsManager::instance()->mapsSettings()->maxCacheDiskSize()->rawValue().toUInt();
}

void QGeoFileTileCacheQGC::cacheTile(const QString &type, int x, int y, int z, const QByteArray &image, const QString &format, qulonglong set)
{
    const QString hash = UrlFactory::getTileHash(type, x, y, z);
    cacheTile(type, hash, image, format, set);
}

void QGeoFileTileCacheQGC::cacheTile(const QString &type, const QString &hash, const QByteArray &image, const QString &format, qulonglong set)
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
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

QString QGeoFileTileCacheQGC::_getCachePath(const QVariantMap &parameters)
{
    QString cacheDir;
    if (parameters.contains(QStringLiteral("mapping.cache.directory"))) {
        cacheDir = parameters.value(QStringLiteral("mapping.cache.directory")).toString();
    } else {
        cacheDir = _cachePath + QLatin1String("/providers");
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
    const QStringList oldCaches = {"/QGCMapCache55", "/QGCMapCache100"};
    for (const QString &cache : oldCaches) {
        QString oldCacheDir;
        #if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
            oldCacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        #else
            oldCacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
        #endif
        oldCacheDir += cache;
        _wipeDirectory(oldCacheDir);
    }
}

void QGeoFileTileCacheQGC::_initCache()
{
    _wipeOldCaches();

    // QString cacheDir = QAbstractGeoTileCache::baseCacheDirectory()
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
#endif
    cacheDir += QStringLiteral("/QGCMapCache") + QString(kCachePathVersion);
    if (!QDir::root().mkpath(cacheDir)) {
        qCWarning(QGeoFileTileCacheQGCLog) << "Could not create mapping disk cache directory:" << cacheDir;

        cacheDir = QDir::homePath() + QStringLiteral("/.qgcmapscache/");
        if (!QDir::root().mkpath(cacheDir)) {
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
