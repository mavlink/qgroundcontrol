#pragma once

#include <atomic>

#include <QtCore/QLoggingCategory>
#include <QtLocation/private/qgeofiletilecache_p.h>

Q_DECLARE_LOGGING_CATEGORY(QGeoFileTileCacheQGCLog)

class QGCFetchTileTask;

class QGeoFileTileCacheQGC : public QGeoFileTileCache
{
    Q_OBJECT

public:
    explicit QGeoFileTileCacheQGC(const QVariantMap &parameters, QObject *parent = nullptr);
    ~QGeoFileTileCacheQGC();

    static quint32 getMaxDiskCacheSetting();
    static void cacheTile(const QString &type, int x, int y, int z, const QByteArray &image, const QString &format, qulonglong set = UINT64_MAX);
    static void cacheTile(const QString &type, const QString &hash, const QByteArray &image, const QString &format, qulonglong set = UINT64_MAX);
    static QGCFetchTileTask *createFetchTileTask(const QString &type, int x, int y, int z);
    static QString getDatabaseFilePath() { return _databaseFilePath; }
    static QString getCachePath() { return _cachePath; }

private:
    // QString tileSpecToFilename(const QGeoTileSpec &spec, const QString &format, const QString &directory) const final;
    // QGeoTileSpec filenameToTileSpec(const QString &filename) const final;

    static void _initCache();
    static bool _wipeDirectory(const QString &dirPath);
    static void _wipeOldCaches();

    static QString _getCachePath(const QVariantMap &parameters);
    static uint32_t _getMemLimit(const QVariantMap &Parameters);

    static uint32_t _getDefaultMaxMemLimit() { return (3 * qPow(1024, 2)); }
    static uint32_t _getDefaultMaxDiskCache() { return 0; } // (50 * pow(1024, 2));
    static uint32_t _getDefaultExtraTexture() { return (6 * qPow(1024, 2)); }
    static uint32_t _getDefaultMinTexture() { return 0; }

    static quint32 _getMaxMemCacheSetting();

    // Initialized once via std::call_once in constructor before worker thread starts
    static QString _databaseFilePath;
    static QString _cachePath;
    static std::atomic<bool> _cacheWasReset;
};
