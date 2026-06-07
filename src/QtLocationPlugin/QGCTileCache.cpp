#include "QGCTileCache.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <atomic>
#include <mutex>

#include "AppMessages.h"
#include "AppSettings.h"
#include "MapsSettings.h"
#include "QGCCacheTile.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileCacheDatabase.h"
#include "QGCTileCacheWorker.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(QGCTileCacheLog, "QtLocationPlugin.QGCTileCache")

namespace {
QString s_databaseFilePath;
QString s_cachePath;
std::atomic<bool> s_cacheWasReset = false;

bool wipeDirectory(const QString& dirPath)
{
    bool result = true;

    const QDir dir(dirPath);
    if (dir.exists(dirPath)) {
        s_cacheWasReset = true;

        const QFileInfoList fileList = dir.entryInfoList(
            QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
        for (const QFileInfo& info : fileList) {
            if (info.isDir()) {
                result = wipeDirectory(info.absoluteFilePath());
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

void wipeOldCaches()
{
    // Versioned caches are named "QGCMapCache<NN>" (e.g. QGCMapCache55). The live
    // cache is the bare "QGCMapCache" directory; remove only the numbered variants.
    static const QRegularExpression versionSuffix(QStringLiteral("^QGCMapCache\\d+$"));

    QStringList searchLocations;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    searchLocations << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    searchLocations << QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                    << QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
#endif

    for (const QString& location : searchLocations) {
        if (location.isEmpty()) {
            continue;
        }
        const QDir baseDir(location);
        const QFileInfoList entries =
            baseDir.entryInfoList({QStringLiteral("QGCMapCache*")}, QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo& entry : entries) {
            if (versionSuffix.match(entry.fileName()).hasMatch()) {
                wipeDirectory(entry.absoluteFilePath());
            }
        }
    }
}

void initCache()
{
    wipeOldCaches();

    // QString cacheDir = QAbstractGeoTileCache::baseCacheDirectory()
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    cacheDir += QStringLiteral("/QGCMapCache");
#else
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    cacheDir += QStringLiteral("/QGCMapCache");
#endif
    if (!QGCFileHelper::ensureDirectoryExists(cacheDir)) {
        qCWarning(QGCTileCacheLog) << "Could not create mapping disk cache directory:" << cacheDir;

        cacheDir = QDir::homePath() + QStringLiteral("/.qgcmapscache/");
        if (!QGCFileHelper::ensureDirectoryExists(cacheDir)) {
            qCWarning(QGCTileCacheLog) << "Could not create mapping disk cache directory:" << cacheDir;
            cacheDir.clear();
        }
    }

    s_cachePath = cacheDir;
    if (!s_cachePath.isEmpty()) {
        s_databaseFilePath = QString(s_cachePath + QStringLiteral("/qgcMapCache.db"));

        qCDebug(QGCTileCacheLog) << "Map Cache in:" << s_databaseFilePath;
    } else {
        qCCritical(QGCTileCacheLog) << "Could not find suitable map cache directory.";
    }

    if (s_cacheWasReset) {
        QGC::showAppMessage(QCoreApplication::translate("QGCTileCache",
                                                        "The Offline Map Cache database has been upgraded. "
                                                        "Your old map cache sets have been reset."));
    }
}
}  // namespace

void QGCTileCache::ensureInitialized()
{
    static std::once_flag cacheInit;
    std::call_once(cacheInit, []() { initCache(); });
}

quint32 QGCTileCache::getMaxDiskCacheSetting()
{
    return SettingsManager::instance()->mapsSettings()->maxCacheDiskSize()->rawValue().toUInt();
}

void QGCTileCache::cacheTile(const QString& type, int x, int y, int z, const QByteArray& image, const QString& format,
                             qulonglong set, const QByteArray& etag, const QByteArray& lastModified, qint64 expiresAt,
                             bool mustRevalidate)
{
    const QString hash = UrlFactory::getTileHash(type, x, y, z);
    cacheTile(type, hash, image, format, set, etag, lastModified, expiresAt, mustRevalidate);
}

void QGCTileCache::cacheTile(const QString& type, const QString& hash, const QByteArray& image, const QString& format,
                             qulonglong set, const QByteArray& etag, const QByteArray& lastModified, qint64 expiresAt,
                             bool mustRevalidate)
{
    AppSettings* appSettings = SettingsManager::instance()->appSettings();
    if (!appSettings->disableAllPersistence()->rawValue().toBool()) {
        QGCCacheTile* tile = new QGCCacheTile(hash, image, format, type, set);
        tile->etag = etag;
        tile->lastModified = lastModified;
        tile->expiresAt = expiresAt;
        tile->mustRevalidate = mustRevalidate;
        QGCSaveTileTask* task = new QGCSaveTileTask(tile);
        if (!getQGCMapEngine()->addTask(task)) {
            task->deleteLater();
        }
    }
}

void QGCTileCache::refreshTileValidators(const QString& type, const QString& hash, const QByteArray& etag,
                                         const QByteArray& lastModified, qint64 expiresAt)
{
    Q_UNUSED(type);
    AppSettings* appSettings = SettingsManager::instance()->appSettings();
    if (appSettings->disableAllPersistence()->rawValue().toBool()) {
        return;
    }
    QGCCommandTask* task = new QGCCommandTask(
        QGCMapTask::TaskType::taskRefreshTileValidators, [=](QGCCacheWorker& worker, QGCMapTask& self) {
            if (!worker.validateDatabase(&self)) {
                return false;
            }
            if (!worker.database()->refreshTileValidators(hash, etag, lastModified, expiresAt)) {
                self.setError("Error refreshing tile validators");
                return false;
            }
            return true;
        });
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }
}

QGCFetchTileTask* QGCTileCache::createFetchTileTask(const QString& type, int x, int y, int z)
{
    const QString hash = UrlFactory::getTileHash(type, x, y, z);
    QGCFetchTileTask* task = new QGCFetchTileTask(hash);
    return task;
}

QString QGCTileCache::getDatabaseFilePath()
{
    return s_databaseFilePath;
}

QString QGCTileCache::getCachePath()
{
    return s_cachePath;
}
