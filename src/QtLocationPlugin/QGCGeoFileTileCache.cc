#include "QGCGeoFileTileCache.h"

#include <QtCore/QDir>
#include <QtCore/QtMinMax>
#include <QtGui/QPainter>

#include "MapTilesSettings.h"
#include "QGCLoggingCategory.h"
#include "TileCache.h"

QGC_LOGGING_CATEGORY(QGCGeoFileTileCacheLog, "QtLocationPlugin.QGCGeoFileTileCache")

namespace {

/// The part of \a abovePicture, the picture of the tile \a levels zoom levels above \a spec, that shows the ground of
/// \a spec, as a picture of the size of \a abovePicture
QImage fileTileCachePartOfTileAbove(const QImage& abovePicture, const QGeoTileSpec& spec, int levels)
{
    const int tilesPerEdge = 1 << levels;
    const QSizeF partSize(static_cast<qreal>(abovePicture.width()) / tilesPerEdge,
                          static_cast<qreal>(abovePicture.height()) / tilesPerEdge);
    const QRectF part(QPointF((spec.x() & (tilesPerEdge - 1)) * partSize.width(),
                              (spec.y() & (tilesPerEdge - 1)) * partSize.height()),
                      partSize);

    // Scaled with the pixels around the part: the tiles of neighbouring parts then meet without a seam
    const QRect source = part.toAlignedRect().adjusted(-1, -1, 1, 1).intersected(abovePicture.rect());
    const qreal scaleX = abovePicture.width() / part.width();
    const qreal scaleY = abovePicture.height() / part.height();
    const QRectF target((source.x() - part.x()) * scaleX, (source.y() - part.y()) * scaleY, source.width() * scaleX,
                        source.height() * scaleY);

    QImage tile(abovePicture.size(),
                abovePicture.hasAlphaChannel() ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32);
    tile.fill(Qt::transparent);
    QPainter painter(&tile);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(target, abovePicture.copy(source));
    painter.end();
    return tile;
}

}  // namespace

QGCGeoFileTileCache::QGCGeoFileTileCache(QObject* parent)
    : QGeoFileTileCache(_directory(), parent)
{
    qCDebug(QGCGeoFileTileCacheLog) << this;

    setMaxDiskUsage(0);
    setMaxMemoryUsage(_memoryLimit());
}

QGCGeoFileTileCache::~QGCGeoFileTileCache()
{
    if (QGCGeoFileTileCacheLog().isDebugEnabled()) {
        printStats();
    }

    qCDebug(QGCGeoFileTileCacheLog) << this;
}

void QGCGeoFileTileCache::init()
{
    // Not QGeoFileTileCache::init(), which is for a cache on disk: it creates the directory of the cache and lists
    // it, and deletes the tiles of old QtLocation versions from the cache directory that applications share
    constexpr int kExtraTextureBytes = 6 * 1024 * 1024;  // QtLocation's own limit
    setExtraTextureUsage(kExtraTextureBytes);
}

void QGCGeoFileTileCache::clearAll()
{
    // Not QGeoFileTileCache::clearAll(), which lists its directory for tiles to delete
    textureCache_.clear();
    memoryCache_.clear();
    _tilesAbove.clear();
}

void QGCGeoFileTileCache::handleError(const QGeoTileSpec& spec, const QString& errorString)
{
    qCWarning(QGCGeoFileTileCacheLog) << "tile" << spec.x() << spec.y() << "of zoom level" << spec.zoom() << "of map"
                                      << spec.mapId() << ':' << errorString;
}

QSharedPointer<QGeoTileTexture> QGCGeoFileTileCache::get(const QGeoTileSpec& spec)
{
    if (QSharedPointer<QGeoTileTexture> texture = QGeoFileTileCache::get(spec)) {
        return texture;
    }
    const auto above = _tilesAbove.constFind(spec);
    if (above == _tilesAbove.cend()) {
        return nullptr;
    }
    // Decoded again from the memory cache when its texture was evicted too
    const QSharedPointer<QGeoTileTexture> aboveTexture = QGeoFileTileCache::get(above.value());
    if (!aboveTexture || aboveTexture->image.isNull()) {
        return nullptr;
    }
    return addToTextureCache(spec,
                             fileTileCachePartOfTileAbove(aboveTexture->image, spec, spec.zoom() - above->zoom()));
}

void QGCGeoFileTileCache::insertFromAbove(const QGeoTileSpec& spec, const QGeoTileSpec& above,
                                          const QImage& abovePicture, const QByteArray& aboveData,
                                          const QString& aboveFormat)
{
    // Bounds the memory of a long flight over places without tiles. A tile that is forgotten is asked for again.
    constexpr qsizetype kMaxTiles = 16384;
    if (_tilesAbove.size() >= kMaxTiles) {
        _tilesAbove.clear();
    }
    // Scaled up when a map looks the tile up, as QtLocation decodes the bytes of a tile only then
    (void) _tilesAbove.insert(spec, above);

    if (!aboveData.isEmpty() && !memoryCache_.object(above)) {
        addToMemoryCache(above, aboveData, aboveFormat);
    }
    // The tiles under it are scaled up without decoding it again
    if (!abovePicture.isNull() && !textureCache_.object(above)) {
        (void) addToTextureCache(above, abovePicture);
    }
}

QString QGCGeoFileTileCache::_directory()
{
    // Never created, as no tile fits a disk usage of 0. QGeoFileTileCache::clearMapId() deletes tiles from its
    // directory, which is the working directory when it is given none.
    const QString cachePath = TileCache::cachePath();
    return (cachePath.isEmpty() ? QDir::tempPath() + QLatin1String("/QGCMapCache") : cachePath) +
           QLatin1String("/providers");
}

int QGCGeoFileTileCache::_memoryLimit()
{
    constexpr quint32 kBytesPerMegabyte = 1024 * 1024;
    constexpr quint32 kDefaultMegabytes = 3;  // QtLocation's own limit
    constexpr quint32 kMaxMegabytes = 1024;   // QtLocation takes the limit as an int

    const quint32 megabytes = MapTilesSettings::maxCacheMemorySizeMB();
    return static_cast<int>(qMin((megabytes == 0) ? kDefaultMegabytes : megabytes, kMaxMegabytes) * kBytesPerMegabyte);
}
