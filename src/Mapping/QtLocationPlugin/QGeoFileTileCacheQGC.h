#pragma once

#include <QtLocation/private/qgeofiletilecache_p.h>

class QGeoFileTileCacheQGC : public QGeoFileTileCache
{
    Q_OBJECT

public:
    explicit QGeoFileTileCacheQGC(QObject* parent = nullptr);
    ~QGeoFileTileCacheQGC();

private:
    static QString _getCachePath();
    static uint32_t _getMemLimit();

    static uint32_t _getDefaultMaxMemLimit() { return (3 * qPow(1024, 2)); }

    // 0 keeps QtLocation from storing tiles on disk: the tile cache of QGCMapEngine stores them
    static uint32_t _getDefaultMaxDiskCache() { return 0; }

    static uint32_t _getDefaultExtraTexture() { return (6 * qPow(1024, 2)); }

    static uint32_t _getDefaultMinTexture() { return 0; }

    static quint32 _getMaxMemCacheSetting();
};
