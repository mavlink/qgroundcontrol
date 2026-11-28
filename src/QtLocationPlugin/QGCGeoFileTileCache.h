#pragma once

#include <QtCore/QHash>
#include <QtLocation/private/qgeofiletilecache_p.h>
#include <QtLocation/private/qgeotilespec_p.h>

/// QtLocation's cache of the tiles of its maps, in memory only: TileCache stores the tiles
class QGCGeoFileTileCache : public QGeoFileTileCache
{
    Q_OBJECT

public:
    explicit QGCGeoFileTileCache(QObject* parent = nullptr);
    ~QGCGeoFileTileCache();

    void clearAll() final;
    void handleError(const QGeoTileSpec& spec, const QString& errorString) final;

    /// The texture of a tile that was shown from the tile above is made again from the picture of that tile once it
    /// was evicted: the tile has no bytes to decode again
    QSharedPointer<QGeoTileTexture> get(const QGeoTileSpec& spec) final;

    /// The tile \a spec, which its provider lacks, shows the part of the tile \a above that shows the same ground,
    /// scaled up to a whole tile. \a abovePicture is the picture of the tile above, and \a aboveData, unless it is
    /// empty, its bytes as fetched: the memory cache keeps them for as long as it keeps tiles, for the tile to be made
    /// again from them.
    void insertFromAbove(const QGeoTileSpec& spec, const QGeoTileSpec& above, const QImage& abovePicture,
                         const QByteArray& aboveData, const QString& aboveFormat);

protected:
    void init() final;

private:
    static QString _directory();

    /// The maxCacheMemorySize setting in bytes
    static int _memoryLimit();

    QHash<QGeoTileSpec, QGeoTileSpec> _tilesAbove;  ///< The tile above that each tile shown from above shows a part of
};
