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

/// Forces generator lookups to miss while it lives, and none once it is gone
class ForcedMisses
{
public:
    /// A count that no test uses up, for a test in which every lookup must miss. An exact count is used up by
    /// any lookup of the process, also by one that an earlier test left queued in the cache worker.
    static constexpr int kEveryLookup = 10000;

    explicit ForcedMisses(int count) { setForcedMissCount(count); }

    ~ForcedMisses() { setForcedMissCount(0); }

    ForcedMisses(const ForcedMisses&) = delete;
    ForcedMisses& operator=(const ForcedMisses&) = delete;

    /// The lookups that miss from here on: 0 ends the misses
    void setCount(int count) { setForcedMissCount(count); }
};

/// The terrarium-encoded PNG the generator would serve for a slippy tile (for
/// mock network replies in network-fallback tests).
QByteArray syntheticTerrariumTileData(int x, int y, int zoom);

/// The type of a map provider that is no elevation provider, for which the generator serves its placeholder image:
/// the first such type of UrlFactory::getProviderTypes(), or the one \a index places after it among them. Empty
/// when there are not that many.
QString mapProviderType(int index = 0);

/// Empties the tile cache of the process and waits for that. The cache outlives a test class, which can run more
/// than once in a process (--unittest-stress) and after other classes (--unittest): a class whose tests fetch tiles
/// that the cache must lack calls this from initTestCase().
[[nodiscard]] bool resetTileCache();

/// Waits until the cache worker of the process has run the tasks queued so far, and the main thread has handled
/// what they reported: the worker runs its tasks in order. Looks up no tile, so it uses up no forced miss.
[[nodiscard]] bool waitForCacheTasks();
}  // namespace UnitTestTileGenerator
