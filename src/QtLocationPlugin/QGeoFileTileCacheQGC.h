#pragma once

#include <QtLocation/private/qgeofiletilecache_p.h>

class QGeoFileTileCacheQGC : public QGeoFileTileCache
{
    Q_OBJECT

public:
    explicit QGeoFileTileCacheQGC(const QVariantMap &parameters, QObject *parent = nullptr);
    ~QGeoFileTileCacheQGC();

private:
    // QString tileSpecToFilename(const QGeoTileSpec &spec, const QString &format, const QString &directory) const final;
    // QGeoTileSpec filenameToTileSpec(const QString &filename) const final;

    static QString _getCachePath(const QVariantMap &parameters);
    static uint32_t _getMemLimit(const QVariantMap &Parameters);

    static uint32_t _getDefaultMaxMemLimit() { return (3 * qPow(1024, 2)); }
    static uint32_t _getDefaultMaxDiskCache() { return 0; } // (50 * pow(1024, 2));
    static uint32_t _getDefaultExtraTexture() { return (6 * qPow(1024, 2)); }
    static uint32_t _getDefaultMinTexture() { return 0; }

    static quint32 _getMaxMemCacheSetting();
};
