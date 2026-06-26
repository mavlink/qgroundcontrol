#pragma once

#include <QtLocation/private/qabstractgeotilecache_p.h>

// SQLite-backed tile cache. The flat-file QGeoFileTileCache is deliberately NOT
// used as a base: the SQLite tile store (QGCTileCacheDatabase via the cache
// worker) is the sole authoritative cache and owns expiry, ETag/Last-Modified
// revalidation and LRU. This subclasses QAbstractGeoTileCache directly so none
// of QGeoFileTileCache's memory/disk/texture QCache3Q caches or its startup
// loadTiles() disk scan are allocated. get() returns a null texture so every
// visible tile is routed through the fetcher -> reply -> SQLite path; insert()
// is a no-op so the engine never writes a duplicate flat-file copy.
//
// The cache facade (paths, tile-save tasks, validator refresh) lives in the
// free QGCTileCache namespace; this class holds only the QtLocation vtable.
class QGeoFileTileCacheQGC : public QAbstractGeoTileCache
{
    Q_OBJECT

public:
    explicit QGeoFileTileCacheQGC(const QVariantMap& parameters, QObject* parent = nullptr);
    ~QGeoFileTileCacheQGC();

    QSharedPointer<QGeoTileTexture> get(const QGeoTileSpec& spec) override;
    void insert(const QGeoTileSpec& spec, const QByteArray& bytes, const QString& format,
                QAbstractGeoTileCache::CacheAreas areas = QAbstractGeoTileCache::AllCaches) override;

    void init() override {}

    // No in-process caches to size, so the texture/cost-strategy knobs are inert.
    void setMinTextureUsage(int) override {}

    void setExtraTextureUsage(int) override {}

    int maxTextureUsage() const override { return 0; }

    int minTextureUsage() const override { return 0; }

    int textureUsage() const override { return 0; }

    void clearAll() override {}

    void setCostStrategyDisk(CostStrategy) override {}

    CostStrategy costStrategyDisk() const override { return ByteSize; }

    void setCostStrategyMemory(CostStrategy) override {}

    CostStrategy costStrategyMemory() const override { return ByteSize; }

    void setCostStrategyTexture(CostStrategy) override {}

    CostStrategy costStrategyTexture() const override { return ByteSize; }

protected:
    void printStats() override {}
};
