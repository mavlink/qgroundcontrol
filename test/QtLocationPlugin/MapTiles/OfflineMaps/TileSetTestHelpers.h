#pragma once

#include <memory>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtTest/QTest>

#include "CachedTileSet.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "TileCache.h"
#include "TileRegion.h"
#include "TileSetTasks.h"
#include "UnitTest.h"

// For the tests of the offline tile sets and of their downloads

// The region of a download test lies within one tile at this zoom
inline constexpr int kTileSetTestMinZoom = 14;
inline constexpr double kTileSetTestTopLat = 47.3905;
inline constexpr double kTileSetTestBottomLat = 47.3895;
inline constexpr double kTileSetTestHalfLonSpan = 0.0005;
// With this zoom the region has more tiles than a set requests at a time, and fewer than the database gives a set
// to download at a time
inline constexpr int kTileSetTestManyTilesZoom = 19;
inline constexpr int kTileSetTestRequestsAtOnce = 6;
inline constexpr quint32 kTileSetTestDownloadListSize = 256;

/// A set of \a providerType that is saved in the tile cache database and has downloaded nothing. Its region lies
/// around \a longitude, and its zooms are kTileSetTestMinZoom to \a maxZoom. Null when it could not be saved.
inline std::unique_ptr<CachedTileSet> tileSetTestSavedSet(const QString& name, const QString& providerType,
                                                          double longitude, int maxZoom)
{
    const double leftLon = longitude - kTileSetTestHalfLonSpan;
    const double rightLon = longitude + kTileSetTestHalfLonSpan;
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromProviderType(providerType);
    quint32 tileCount = 0;
    for (int zoom = kTileSetTestMinZoom; zoom <= maxZoom; zoom++) {
        const TileRegion tiles =
            provider->getTileCount(zoom, leftLon, kTileSetTestTopLat, rightLon, kTileSetTestBottomLat);
        tileCount += tiles.tileCount;
    }

    TileCache* const cache = TileCache::instance();
    cache->ensureInitialized();

    CachedTileSet* const newSet = new CachedTileSet(TileSetRecord{.name = name,
                                                                  .mapName = providerType,
                                                                  .topleftLat = kTileSetTestTopLat,
                                                                  .topleftLon = leftLon,
                                                                  .bottomRightLat = kTileSetTestBottomLat,
                                                                  .bottomRightLon = rightLon,
                                                                  .minZoom = kTileSetTestMinZoom,
                                                                  .maxZoom = maxZoom,
                                                                  .mapId = provider->mapId(),
                                                                  .numTiles = tileCount});

    bool saved = false;
    // Declared after what its signal writes, so that a late signal is dropped
    const QObject savedContext;
    // The task owns the set until it is saved
    if (!cache->addTask(TileSetTasks::createTileSet(newSet, &savedContext, [&saved](auto) { saved = true; }))) {
        return nullptr;
    }
    if (!QTest::qWaitFor([&saved] { return saved; }, TestTimeout::mediumMs())) {
        return nullptr;
    }
    return std::unique_ptr<CachedTileSet>(newSet);
}
