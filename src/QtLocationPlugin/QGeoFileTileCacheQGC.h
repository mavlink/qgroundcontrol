#pragma once

#include <QtLocation/private/qgeofiletilecache_p.h>

/// QtLocation's cache of the tiles of its maps, in memory only: the tile cache of QGCMapEngine stores the tiles
class QGeoFileTileCacheQGC : public QGeoFileTileCache
{
    Q_OBJECT

public:
    explicit QGeoFileTileCacheQGC(QObject* parent = nullptr);
    ~QGeoFileTileCacheQGC();

private:
    static QString _directory();

    /// The maxCacheMemorySize setting in bytes
    static int _memoryLimit();
};
