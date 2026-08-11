#pragma once

#include <QtCore/QByteArray>

class QString;
struct QGCCacheTile;

/// Serves synthetic tiles to the tile cache layer while running unit tests.
///
/// Installed on QGCCacheWorker at test-run startup, the generator is consulted on
/// every tile cache miss so production code never falls back to real network fetches:
///   - Map providers get a placeholder image tile.
///   - Elevation providers get a terrain tile synthesized from the UnitTestTerrainData
///     regions (0 height outside them): Copernicus via the production serializer,
///     Terrarium as an RGB-encoded PNG.
///   - Hashes that resolve to no provider return nullptr, preserving the miss-error path.
namespace UnitTestTileGenerator {
/// Installs the generator hook on QGCCacheWorker.
void install();

/// Initializes the global map engine (tile cache worker) with a fresh temp database
/// so terrain queries work without a QML map ever being created. Full-app test runs
/// only: the worker thread requires orderly QApplication teardown.
void initMapEngine();

/// Stops the map engine worker thread. Must be called before QApplication teardown:
/// the worker's database teardown cannot run after the app is gone.
void shutdownMapEngine();

/// Generates a synthetic tile for the given tile hash. Returns nullptr if the hash
/// does not resolve to a known provider. Caller takes ownership.
QGCCacheTile* generateTile(const QString& hash);

/// Forces the next \a count generator lookups to miss (return nullptr) so the cache
/// task errors and production network fallback paths run. Reset to 0 after use.
void setForcedMissCount(int count);

/// The terrarium-encoded PNG the generator would serve for a slippy tile (for
/// mock network replies in network-fallback tests).
QByteArray syntheticTerrariumTileData(int x, int y, int zoom);
}  // namespace UnitTestTileGenerator
